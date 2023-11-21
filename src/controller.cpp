#include "controller.hpp"
#include <MathBuffer.h>


HX711 loadcell;
SimpleKalmanFilter scaleKalmanFilter(0.2, 0.2, 0.05);


#define ABS(a) (((a) > 0) ? (a) : ((a) * -1))

double scaleWeight = 0;
unsigned long grindStartedAt = 0;
unsigned long grindStoppedAt = 0;
unsigned long lastSignificantWeightChangeAt = 0;
unsigned long lastSignificantPotiChangeAt = 0;
unsigned long lastTareAt = 0; // if 0, should tare load cell, else represent when it was last tared
bool scaleReady = false;
int scaleStatus = STATUS_EMPTY;
double cupWeightEmpty = 0;
unsigned long startedGrindingAt = 0;
unsigned long finishedGrindingAt = 0;
MathBuffer<double, 100> weightHistory;
int potiValue = 0;
int lastStablePotiValue = 0;
float coffeeDoseWeight = 14.0;
unsigned long grindmillis = 0;

void tareScale() {
  Serial.println("Taring scale");
  loadcell.tare(TARE_MEASURES);
  lastTareAt = millis();
  Serial.println("tara done");
}

void detectTimeGrind(){



  if (digitalRead(GRINDER_SENSE_PIN)==1){
    grindStartedAt = millis();
    while (digitalRead(GRINDER_SENSE_PIN)==1){
      Serial.println("waiting for grind to finish");
    }
    grindmillis = millis() - grindStartedAt;
  }
  return;

}

void updateScale() {
  float lastEstimate;
  if (lastTareAt == 0) {
    tareScale();
  }
  if (loadcell.wait_ready_timeout(300)) {
    lastEstimate = scaleKalmanFilter.updateEstimate(loadcell.get_units());
    scaleWeight = lastEstimate;
    weightHistory.push(scaleWeight);
    scaleReady = true;
  } else {
    Serial.println("HX711 not found.");
    scaleReady = false;
  }
  potiValue = analogRead(A0);


}

unsigned long extrapolateEndMillis(){
  unsigned long now = millis();
  unsigned long offset = millis()-startedGrindingAt;
  unsigned long durationBegin = now-4000;
  if (offset< 4000){ // we only use data from 4 seconds after grind start
    return 99999999l;
  }

  double max = weightHistory.maxSince((int64_t)durationBegin);
  double min = weightHistory.minSince((int64_t)durationBegin);
  double diff = max-min;
  double duration = (double)(4000);
  double slope = (duration)/(diff);
  double missing = coffeeDoseWeight + cupWeightEmpty - max;
  Serial.printf("slope %3.1f ms/g missing %3.1f\n", slope,missing);
  double expectedGrindEnd = ((missing) * slope);
  Serial.printf("end in %3.1fms abs:%i\n", expectedGrindEnd,now+(long)expectedGrindEnd);
  return (long)expectedGrindEnd;
}

void scaleStatusLoop() {
  double tenSecAvg;
  tenSecAvg = weightHistory.averageSince((int64_t)millis() - 10000);
  // Serial.printf("Avg: %f, currentWeight: %f\n", tenSecAvg, scaleWeight);

  if (ABS(tenSecAvg - scaleWeight) > SIGNIFICANT_WEIGHT_CHANGE) {
    // Serial.printf("Detected significant change: %f\n", ABS(avg - scaleWeight));
    lastSignificantWeightChangeAt = millis();
  }

  if (ABS(lastStablePotiValue - potiValue) > SIGNIFICANT_POTI_CHANGE) {
    lastStablePotiValue = potiValue;
    lastSignificantPotiChangeAt = millis();
    coffeeDoseWeight = (((float)lastStablePotiValue/1024.0)*15.0)+7.0;
  }

  if (scaleStatus == STATUS_EMPTY) {
    if (millis() - lastTareAt > TARE_MIN_INTERVAL && ABS(tenSecAvg) > 0.2 && tenSecAvg < 3 && scaleWeight < 3) {
      // tare if: not tared recently, more than 0.2 away from 0, less than 3 grams total (also works for negative weight)
      lastTareAt = 0;
    }

    if (ABS(weightHistory.minSince((int64_t)millis() - 1000) - CUP_WEIGHT) < CUP_DETECTION_TOLERANCE &&
      ABS(weightHistory.maxSince((int64_t)millis() - 1000) - CUP_WEIGHT) < CUP_DETECTION_TOLERANCE
    ) {

      if (digitalRead(GRINDER_MODE_BUTTON_PIN) == GRINDER_MODE_TIMER){
        Serial.println("Failed because Timer Mode is ON");
        scaleStatus = STATUS_TIMER_ON;
        return;
      }else{
        // using average over last 500ms as empty cup weight
        Serial.println("Starting grinding");
        cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
        scaleStatus = STATUS_GRINDING_IN_PROGRESS;
        startedGrindingAt = millis();
        digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_ON);
        return;
      }
    }


  } else if (scaleStatus == STATUS_GRINDING_IN_PROGRESS) {
    if (!scaleReady) {
      digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
      scaleStatus = STATUS_GRINDING_FAILED;
    }

    if (millis() - startedGrindingAt > MAX_GRINDING_TIME) {
      Serial.println("Failed because grinding took too long");
      digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
      scaleStatus = STATUS_GRINDING_FAILED;
      return;
    }

    if (
      millis() - startedGrindingAt > 4000 && // started grinding at least 4s ago
      scaleWeight - weightHistory.firstValueOlderThan(millis() - 4000) < 1 // less than a gram has been ground in the last 4 second
    ) {
      Serial.println("Failed because no change in weight was detected");
      digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
      scaleStatus = STATUS_GRINDING_FAILED;
      return;
    }

    if (weightHistory.minSince((int64_t)millis() - 200) < cupWeightEmpty - CUP_DETECTION_TOLERANCE) {
      Serial.printf("Failed because weight too low, min: %f, min value: %f\n", weightHistory.minSince((int64_t)millis() - 200), CUP_WEIGHT + CUP_DETECTION_TOLERANCE);
      digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
      scaleStatus = STATUS_GRINDING_FAILED;
      return;
    }
    double remailingMillis = extrapolateEndMillis();
    if ((remailingMillis - EXPECTED_DELAY) <200){
      delay(remailingMillis - EXPECTED_DELAY);
      finishedGrindingAt = millis();
      digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
      scaleStatus = STATUS_GRINDING_FINISHED;
      lastSignificantWeightChangeAt = finishedGrindingAt;
      Serial.printf("Finished grinding with sleep at %i\n",finishedGrindingAt);
      return;
    }

    if (weightHistory.maxSince((int64_t)millis() - 200) >= cupWeightEmpty + coffeeDoseWeight) {
      finishedGrindingAt = millis();
      digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
      scaleStatus = STATUS_GRINDING_FINISHED;
      Serial.println("Finished grinding");
      return;
    }
  } else if (scaleStatus == STATUS_GRINDING_FINISHED) {
    //calculate Delay
    if (scaleWeight < 5) {
      Serial.println("Going back to empty");
      scaleStatus = STATUS_EMPTY;
      return;
    }
  } else if (scaleStatus == STATUS_GRINDING_FAILED || scaleStatus == STATUS_TIMER_ON) {
    if (scaleWeight >= GRINDING_FAILED_WEIGHT_TO_RESET) {
      Serial.println("Going back to empty");
      scaleStatus = STATUS_EMPTY;
      return;
    }
  }

}

void setupScale() {

  Serial.println("init_loadcell");
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  Serial.println("init_set_scale_factor");
  loadcell.set_scale(LOADCELL_SCALE_FACTOR);
}
