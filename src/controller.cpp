#include "controller.hpp"
#include <MathBuffer.h>

HX711 loadcell;
SimpleKalmanFilter scaleKalmanFilter(0.2, 0.2, 0.05);

#define ABS(a) (((a) > 0) ? (a) : ((a) * -1))
#define COEFF_A 0.0000192663
#define COEFF_B 0.0046853639
#define COEFF_C 10

double scaleWeight = 0;
unsigned long lastSignificantWeightChangeAt = 0;
unsigned long lastSignificantPotiChangeAt = 0;
unsigned long lastTareAt = 0; // if 0, should tare load cell, else represent when it was last tared
bool scaleReady = false;
int scaleStatus = STATUS_EMPTY;
int currentGrinderMode = GRINDER_MODE_SCALE;
int grindingInProgress = 0;
double cupWeightEmpty = 0;
double totalTimerTime = 0;
unsigned long startedGrindingAt = 0;
unsigned long cupDetectedAt = 0;
unsigned long finishedGrindingAt = 0;
unsigned long stableSince = 0;
MathBuffer<double, 100> weightHistory;
int potiValue = 0;
int lastStablePotiValue = 0;
float coffeeDoseWeight = 14.0;
unsigned long grindmillis = 0;
int errorCode = 0;

void tareScale()
{
  Serial.println("Taring scale");
  loadcell.tare(TARE_MEASURES);
  lastTareAt = millis();
  Serial.println("tara done");
}

void updateScale()
{
  float lastEstimate;
  if (lastTareAt == 0)
  {
    tareScale();
  }
  if (loadcell.wait_ready_timeout(300))
  {
    lastEstimate = scaleKalmanFilter.updateEstimate(loadcell.get_units());
    scaleWeight = lastEstimate;
    weightHistory.push(scaleWeight);
    scaleReady = true;
  }
  else
  {
    Serial.println("HX711 not found.");
    scaleReady = false;
  }
}

void startGrinding()
{
  pinMode(GRINDER_START_PIN, OUTPUT);
  digitalWrite(GRINDER_START_PIN, GRINDER_ON);
}

void stopGrinding()
{
  // avoid draining too much current into pin by making it open/tri-state (high resistance?)
  pinMode(GRINDER_START_PIN, INPUT);
}

void updateGrinderMode()
{
  potiValue = analogRead(A0);
  currentGrinderMode = digitalRead(GRINDER_MODE_BUTTON_PIN);
}

void updateGrindingInProgress()
{
  grindingInProgress = digitalRead(GRINDER_SENSE_PIN);
}

unsigned long extrapolateEndMillis()
{
  unsigned long now = weightHistory.getHeadTimestamp();
  unsigned long offset = now - startedGrindingAt;
  unsigned long durationBegin = now - 4000;
  if (offset < 4000)
  { // we only use data from 4 seconds after grind start
    return 99999999;
  }

  double max = weightHistory.maxSince((int64_t)durationBegin);
  double min = weightHistory.minSince((int64_t)durationBegin);
  double diff = max - min;
  double duration = (double)(4000);
  double slope = (duration) / (diff);
  double missing = coffeeDoseWeight + cupWeightEmpty - max;
  Serial.printf("slope %3.1f ms/g missing %3.1f\n", slope, missing);
  double expectedGrindEnd = ((missing)*slope);
  Serial.printf("end in %3.1fms abs:%lu\n", expectedGrindEnd, now + (long)expectedGrindEnd);
  return (long)expectedGrindEnd;
}

int isCupDetected(void){

  return (ABS(weightHistory.minSince((int64_t)millis() - 200) - CUP_WEIGHT) < CUP_DETECTION_TOLERANCE &&
        ABS(weightHistory.maxSince((int64_t)millis() - 200) - CUP_WEIGHT) < CUP_DETECTION_TOLERANCE) ||
        (ABS(weightHistory.minSince((int64_t)millis() - 200) - CUP_WEIGHT2) < CUP_DETECTION_TOLERANCE &&
        ABS(weightHistory.maxSince((int64_t)millis() - 200) - CUP_WEIGHT2) < CUP_DETECTION_TOLERANCE);
}

void scaleStatusLoop()
{
  double tenSecAvg = weightHistory.averageSince((int64_t)millis() - 10000);
  // Serial.printf("Avg: %f, currentWeight: %f\n", tenSecAvg, scaleWeight);

  if (ABS(tenSecAvg - scaleWeight) > SIGNIFICANT_WEIGHT_CHANGE)
  {
    // Serial.printf("Detected significant change: %f\n", ABS(avg - scaleWeight));
    lastSignificantWeightChangeAt = millis();
  }

  if (ABS(lastStablePotiValue - potiValue) > SIGNIFICANT_POTI_CHANGE)
  {
    lastStablePotiValue = potiValue;
    lastSignificantPotiChangeAt = millis();
    coffeeDoseWeight = lastStablePotiValue*lastStablePotiValue*COEFF_A + lastStablePotiValue*COEFF_B + COEFF_C;
    totalTimerTime = (potiValue * (9.530318) + 4229.09) / 1000;
  }

  if (scaleStatus == STATUS_EMPTY)
  {
    updateGrinderMode();
    updateGrindingInProgress();

    if (millis() - lastTareAt > TARE_MIN_INTERVAL && ABS(tenSecAvg) > 0.2 && tenSecAvg < 3 && scaleWeight < 3)
    {
      // tare if: not tared recently, more than 0.2 away from 0, less than 3 grams total (also works for negative weight)
      tareScale();
      lastTareAt = 0;
    }
    // check if someone pressed a button on the grinder an we are currently grinding
    if (grindingInProgress)
    {
      if (currentGrinderMode == GRINDER_MODE_TIMER)
      {
        startedGrindingAt = millis();
        cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
        scaleStatus = STATUS_TIMER_GRINDING_IN_PROGRESS;
        Serial.println("Externally started Timer Grind");
        return;
      }
      else
      {
        if (scaleWeight > MIN_CUP_WEIGHT)
        { // if we have currently something cup like on the scale, then start a regular-weight based grind
          Serial.println("Starting grinding due to button-press");
          cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
          scaleStatus = STATUS_SCALE_GRINDING_IN_PROGRESS;
          startedGrindingAt = millis();
          startGrinding();
          return;
        }
        else
        { // if there ist nothing substantial on the scale, start a manual grindinf process
          Serial.println("Starting manual grinding");
          scaleStatus = STATUS_MANUAL_GRINDING_IN_PROGRESS;
          cupWeightEmpty = 0;
          startedGrindingAt = millis();
          return;
        }
      }
    }
    // see if we can detect a cup
    if (isCupDetected())
    {
      scaleStatus=STATUS_SCALE_CUP_DETECTED;
      cupDetectedAt = millis();
      Serial.println("cup detected");
      return;
    }
  }
  else if (scaleStatus == STATUS_SCALE_GRINDING_IN_PROGRESS)
  {
    if (!scaleReady)
    {
      Serial.println("Failed because scale not ready");
      stopGrinding();
      scaleStatus = STATUS_GRINDING_FAILED;
      errorCode = SCALE_NOT_READY;
      return;
    }

    if (millis() - startedGrindingAt > MAX_GRINDING_TIME)
    {
      Serial.println("Failed because grinding took too long");
      stopGrinding();
      scaleStatus = STATUS_GRINDING_FAILED;
      errorCode = GRINDING_TOOK_TOO_LONG;
      return;
    }

    if (
        millis() - startedGrindingAt > 4000 &&                               // started grinding at least 4s ago
        scaleWeight - weightHistory.firstValueOlderThan(millis() - 4000) < 1 // less than a gram has been ground in the last 4 second
    )
    {
      Serial.println("Failed because no change in weight was detected");
      stopGrinding();
      scaleStatus = STATUS_GRINDING_FAILED;
      errorCode = NO_WEIGHT_CHANGE;
      return;
    }

    if (weightHistory.minSince((int64_t)millis() - 200) < cupWeightEmpty - CUP_DETECTION_TOLERANCE)
    {
      Serial.printf("Failed because weight too low, min: %f, min value: %f\n", weightHistory.minSince((int64_t)millis() - 200), CUP_WEIGHT + CUP_DETECTION_TOLERANCE);
      stopGrinding();
      scaleStatus = STATUS_GRINDING_FAILED;
      errorCode = WEIGHT_TOO_LOW;
      return;
    }
    double remailingMillis = extrapolateEndMillis();
    if ((remailingMillis - EXPECTED_DELAY) < 200)
    {
      delay(remailingMillis - EXPECTED_DELAY);
      finishedGrindingAt = millis();
      stopGrinding();
      scaleStatus = STATUS_SCALE_GRINDING_FINISHED;
      lastSignificantWeightChangeAt = finishedGrindingAt;
      Serial.printf("Finished grinding with sleep at %lu\n", finishedGrindingAt);
      return;
    }

    if (weightHistory.maxSince((int64_t)millis() - 200) >= cupWeightEmpty + coffeeDoseWeight)
    {
      finishedGrindingAt = millis();
      stopGrinding();
      scaleStatus = STATUS_SCALE_GRINDING_FINISHED;
      Serial.println("Finished grinding");
      return;
    }
  }
  else if (scaleStatus == STATUS_SCALE_CUP_DETECTED){
      if(!isCupDetected()){
        scaleStatus = STATUS_EMPTY;
        Serial.println("cup removed");
        return;
      }
      double refWeight = weightHistory.averageSince((int64_t)millis() - 500);
      stableSince=weightHistory.withinRangeSince(refWeight-0.06,refWeight+0.06);
      Serial.printf("stableSince %i", millis()-stableSince);

      if (stableSince>0 && millis()-stableSince > 1000){ // are we stable since at least 2 seconds?
        if (currentGrinderMode == GRINDER_MODE_TIMER)
        {
          cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
          scaleStatus = STATUS_TIMER_GRINDING_IN_PROGRESS;
          Serial.println("turning button on");
          startGrinding();
          Serial.println("turned button on");
          while (digitalRead(GRINDER_SENSE_PIN) == 0)
          {
            Serial.println(".");
            delay(10);
          }
          Serial.println("\n");
          startedGrindingAt = millis();
          stopGrinding();
          Serial.println("turned button off");
          return;
        }
        else
        {
          // using average over last 500ms as empty cup weight
          Serial.println("Starting grinding");
          cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
          scaleStatus = STATUS_SCALE_GRINDING_IN_PROGRESS;
          startedGrindingAt = millis();
          startGrinding();
          return;
        }
      }
  }
  else if (scaleStatus == STATUS_TIMER_GRINDING_IN_PROGRESS)
  {
    updateGrindingInProgress();
    if (grindingInProgress == 0)
    {
      scaleStatus = STATUS_TIMER_GRINDING_FINISHED;
      finishedGrindingAt = millis();
      Serial.print("Timer Grinding finished");
    }
    return;
  }
  else if (scaleStatus == STATUS_MANUAL_GRINDING_IN_PROGRESS)
  {

    updateGrindingInProgress();
    if (grindingInProgress == 0)
    {
      scaleStatus = STATUS_MANUAL_GRINDING_FINISHED;
      finishedGrindingAt = millis();
      Serial.print("Manual Grinding finished");
    }
    return;
  }
  else if (scaleStatus == STATUS_SCALE_GRINDING_FINISHED)
  {
    // calculate Delay
    if (scaleWeight < 5)
    {
      Serial.println("Going back to empty");
      scaleStatus = STATUS_EMPTY;
      return;
    }
  }
  else if (scaleStatus == STATUS_TIMER_GRINDING_FINISHED)
  {
    // calculate Delay
    if (cupWeightEmpty > MIN_CUP_WEIGHT)
    {
      if (scaleWeight < 5)
      {
        Serial.println("Going back to empty");
        scaleStatus = STATUS_EMPTY;
        return;
      }
    }
    else
    {
      if (millis() - finishedGrindingAt > 10000)
      {
        Serial.println("Going back to empty after 10s");
        scaleStatus = STATUS_EMPTY;
        return;
      }
    }
  }
  else if (scaleStatus == STATUS_MANUAL_GRINDING_FINISHED)
  {
    // calculate Delay
    if (millis() - finishedGrindingAt > 10000)
    {
      Serial.println("Going back to empty after 10s");
      scaleStatus = STATUS_EMPTY;
      return;
    }
  }
  else if (scaleStatus == STATUS_GRINDING_FAILED)
  {
    if (scaleWeight >= GRINDING_FAILED_WEIGHT_TO_RESET)
    {
      Serial.println("Going back to empty after failure");
      scaleStatus = STATUS_EMPTY;
      return;
    }
  }
}

  void setupScale(){

    Serial.println("init_loadcell");
    loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    Serial.println("init_set_scale_factor");
    loadcell.set_scale(LOADCELL_SCALE_FACTOR);
  }
