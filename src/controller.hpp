#pragma once

#include <SimpleKalmanFilter.h>
#include "HX711.h"

#define STATUS_EMPTY 0
#define STATUS_GRINDING_IN_PROGRESS 1
#define STATUS_GRINDING_FINISHED 2
#define STATUS_GRINDING_FAILED 3
#define STATUS_TIMER_ON 4
#define MODE_TIMER 0
#define MODE_SCALE 1

#define CUP_WEIGHT 58.8
#define CUP_DETECTION_TOLERANCE 5 // 5 grams tolerance above or bellow cup weight to detect it

#define LOADCELL_DOUT_PIN 12
#define LOADCELL_SCK_PIN 13

#define LOADCELL_SCALE_FACTOR -711

#define TARE_MEASURES 20 // use the average of measure for taring
#define SIGNIFICANT_WEIGHT_CHANGE 5 // 5 grams changes are used to detect a significant change
#define SIGNIFICANT_POTI_CHANGE 2 // 5 grams changes are used to detect a significant change
#define MAX_GRINDING_TIME 40000 // 40 seconds diff
#define EXPECTED_DELAY 836 // 40 seconds diff
#define GRINDING_FAILED_WEIGHT_TO_RESET 500 // force on balance need to be measured to reset grinding

#define GRINDER_ACTIVE_PIN 16

#define GRINDER_ON 0
#define GRINDER_OFF 1

#define GRINDER_MODE_BUTTON_PIN 14

#define GRINDER_MODE_TIMER 0
#define GRINDER_MODE_OPEN 1

#define GRINDER_SENSE_PIN 15

#define TARE_MIN_INTERVAL 10 * 1000 // auto-tare at most once every 10 seconds

extern double scaleWeight;
extern unsigned long lastSignificantWeightChangeAt;
extern unsigned long lastSignificantPotiChangeAt;
extern unsigned long lastTareAt;
extern bool scaleReady;
extern int scaleStatus;
extern double cupWeightEmpty;
extern unsigned long startedGrindingAt;
extern unsigned long finishedGrindingAt;
extern int potiValue;
extern float coffeeDoseWeight;
extern unsigned long grindmillis;

void updateScale();
void scaleStatusLoop();
void setupScale();
void detectTimeGrind();