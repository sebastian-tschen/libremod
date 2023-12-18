#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "display.hpp"
#include "controller.hpp"


WiFiClient espClient;

void setup() {
  pinMode(GRINDER_START_PIN, INPUT);
  pinMode(GRINDER_SENSE_PIN, INPUT);
  pinMode(GRINDER_MODE_BUTTON_PIN, INPUT);
  setupDisplay();

  Serial.begin(115200);
  while(!Serial){delay(100);}

  setupScale();

}


void loop() {
  updateScale();
  updateDisplay();
  scaleStatusLoop();
}
