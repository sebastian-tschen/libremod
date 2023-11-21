#include <Arduino.h>
#include <ESP8266WiFi.h>

#include "display.hpp"
#include "controller.hpp"

WiFiClient espClient;

void setup() {
  pinMode(GRINDER_ACTIVE_PIN, OUTPUT);
  pinMode(GRINDER_SENSE_PIN, INPUT);
  digitalWrite(GRINDER_ACTIVE_PIN, GRINDER_OFF);
  pinMode(GRINDER_MODE_BUTTON_PIN, INPUT);

  Serial.begin(115200);
  while(!Serial){delay(100);}


  // Serial.println();
  // Serial.println("******************************************************");
  // Serial.print("Connecting to ");
  // Serial.println(ssid);

  // WiFi.begin(ssid, password);

  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());

  setupDisplay();
  setupScale();

}


void loop() {

  // drawadc();
  // detectTimeGrind();

  updateScale();
  updateDisplay(NULL);
  scaleStatusLoop();
}
