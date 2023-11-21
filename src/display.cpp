#include "display.hpp"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

#define SLEEP_AFTER_MS 10 * 1000 // sleep after 10 seconds

void centerPrintToScreen(char const *str, u8g2_uint_t y) {
  u8g2_uint_t width = u8g2.getStrWidth(str);
  u8g2.setCursor(128 / 2 - width / 2, y);
  u8g2.print(str);
}
void centerPrintToScreenX2(char const *str, u8g2_uint_t y) {
  u8g2_uint_t width = u8g2.getStrWidth(str);
  u8g2.drawUTF8X2(128 /2 - width, y, str);
}

void drawadc(){

    u8g2.clearBuffer();
    potiValue = analogRead(A0);
    if (millis() - lastSignificantWeightChangeAt > SLEEP_AFTER_MS) {
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      char s[8];
      char p[8];
      sprintf(s, "%i",grindmillis);
      centerPrintToScreenX2(s,25);
      sprintf(p, "%i",potiValue);
      centerPrintToScreenX2(p,50);
      u8g2.sendBuffer();
      return;
    }
}

void updateDisplay( void * parameter) {
  char buf[64];
  u8g2.clearBuffer();

  if ((scaleStatus == STATUS_EMPTY )||(scaleStatus == STATUS_GRINDING_FINISHED)){
    if (millis() - lastSignificantWeightChangeAt > SLEEP_AFTER_MS && millis() - lastSignificantPotiChangeAt > SLEEP_AFTER_MS) {
      u8g2.sendBuffer();
      delay(100);
      return;
    }
  }
  if (scaleStatus == STATUS_GRINDING_IN_PROGRESS) {
    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x13_tr);
    centerPrintToScreen("Grinding...", 0);


    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.setCursor(0, 32);
    snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight - cupWeightEmpty);
    u8g2.print(buf);

    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawGlyph(64, 32, 0x2794);

    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.setCursor(84, 32);
    snprintf(buf, sizeof(buf), "%3.1fg", coffeeDoseWeight);
    u8g2.print(buf);

    u8g2.setFontPosBottom();
    u8g2.setFont(u8g2_font_7x13_tr);
    snprintf(buf, sizeof(buf), "%3.1fs", (double)(millis() - startedGrindingAt) / 1000);
    centerPrintToScreen(buf, 64);
  } else if (scaleStatus == STATUS_EMPTY) {

    u8g2.setFont(u8g2_font_7x14B_tf);
    snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight);
    centerPrintToScreenX2(buf,25);
    snprintf(buf, sizeof(buf), "%3.1fg", coffeeDoseWeight);
    centerPrintToScreenX2(buf,50);

  } else if (scaleStatus == STATUS_TIMER_ON) {

    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x14B_tf);
    centerPrintToScreen("Tun off Timer Mode ", 0);
    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x13_tr);
    centerPrintToScreen("Press the balance", 32);
    centerPrintToScreen("to reset", 42);

  } else if (scaleStatus == STATUS_GRINDING_FAILED) {

    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x14B_tf);
    centerPrintToScreen("Grinding failed", 0);

    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x13_tr);
    centerPrintToScreen("Press the balance", 32);
    centerPrintToScreen("to reset", 42);
  } else if (scaleStatus == STATUS_GRINDING_FINISHED) {

    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x13_tr);
    u8g2.setCursor(0, 0);
    centerPrintToScreen("Grinding finished", 0);

    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.setCursor(0, 32);
    snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight - cupWeightEmpty);
    u8g2.print(buf);

    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawGlyph(64, 32, 0x2794);

    u8g2.setFontPosCenter();
    u8g2.setFont(u8g2_font_7x14B_tf);
    u8g2.setCursor(84, 32);
    snprintf(buf, sizeof(buf), "%3.1fg", coffeeDoseWeight);
    u8g2.print(buf);

    u8g2.setFontPosBottom();
    u8g2.setFont(u8g2_font_7x13_tr);
    u8g2.setCursor(64, 64);
    snprintf(buf, sizeof(buf), "%3.1fs", (double)(finishedGrindingAt - startedGrindingAt) / 1000);
    centerPrintToScreen(buf, 64);
  }
  u8g2.sendBuffer();


}

void setupDisplay() {
  Serial.println("init_display");

  u8g2.begin();

  u8g2.clearBuffer();					// clear the internal memory
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.drawUTF8X2(50, 40, "☕️");
  u8g2.sendBuffer();
  Serial.println("init_display done");
}
