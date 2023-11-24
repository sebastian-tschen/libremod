#include "display.hpp"
#include "octopus.hpp"

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

#define SLEEP_AFTER_MS 10 * 1000 // sleep after 10 seconds
#define TOP_PROGRESS 16
#define BOTTOM_PROGRESS 48
#define BOTH_PROGRESS 32
#define NO_PROGRESS 0

void centerPrintToScreen(char const *str, u8g2_uint_t y)
{
  u8g2_uint_t width = u8g2.getStrWidth(str);
  u8g2.setCursor(128 / 2 - width / 2, y);
  u8g2.print(str);
}
void centerPrintToScreenX2(char const *str, u8g2_uint_t y)
{
  u8g2_uint_t width = u8g2.getStrWidth(str);
  u8g2.drawUTF8X2(128 / 2 - width, y, str);
}

void allignRightPrintToScreen(char const *str, u8g2_uint_t y)
{
  u8g2_uint_t width = u8g2.getStrWidth(str);
  u8g2.setCursor(100 - width, y);
  u8g2.print(str);
}

void displayTopBottomProgress(char const *top, char const *bottom, double progress, int progressType)
{

  u8g2.clearBuffer();

  if (progressType != 0)
  {
    int rad = (int)((progress) * 64.0);
    if (rad <0){
      rad =0;
    }
    u8g2.setDrawColor(1);
    u8g2.drawDisc(64, progressType, rad, U8G2_DRAW_ALL);
    if (progressType == TOP_PROGRESS)
    {
      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 32, 128, 32);
    }
    if (progressType == BOTTOM_PROGRESS)
    {
      u8g2.setDrawColor(0);
      u8g2.drawBox(0, 0, 128, 32);
    }
  }

  u8g2.setDrawColor(2);
  u8g2.setFontMode(1);
  u8g2.setFont(u8g2_font_luRS19_tf);
  allignRightPrintToScreen(top, 27);
  allignRightPrintToScreen(bottom, 58);

  u8g2.sendBuffer();
}

void updateDisplay()
{
  char buf[64];
  char buf2[64];
  u8g2.clearBuffer();

  if (scaleStatus == STATUS_SCALE_GRINDING_IN_PROGRESS)
  {
    double timeToDisplay = (double)((millis() - startedGrindingAt) / 1000);
    double weightToDisplay = scaleWeight - cupWeightEmpty;
    double progress =0.0;
    progress = weightToDisplay / coffeeDoseWeight;
    snprintf(buf, sizeof(buf), "%3.1fs", timeToDisplay);
    snprintf(buf2, sizeof(buf2), "%3.1fg", weightToDisplay);
    displayTopBottomProgress(buf, buf2, progress, BOTTOM_PROGRESS);
  }
  else if (scaleStatus == STATUS_TIMER_GRINDING_IN_PROGRESS)
  {
      double timeToDisplay = totalTimerTime - (double)(millis() - startedGrindingAt)/1000;
      double weightToDisplay = scaleWeight - cupWeightEmpty;
      double progress = (totalTimerTime-timeToDisplay)/totalTimerTime;
      snprintf(buf, sizeof(buf), "%3.1fs", timeToDisplay);
      snprintf(buf2, sizeof(buf2), "%3.1fg", weightToDisplay);
      displayTopBottomProgress(buf, buf2, progress, TOP_PROGRESS);
  }
  else if (scaleStatus == STATUS_MANUAL_GRINDING_IN_PROGRESS)
  {
      double timeToDisplay = (double)(millis() - startedGrindingAt)/1000;
      double weightToDisplay = scaleWeight - cupWeightEmpty;
      double progress = timeToDisplay*0.05;
      snprintf(buf, sizeof(buf), "%3.1fs", timeToDisplay);
      snprintf(buf2, sizeof(buf2), "%3.1fg", weightToDisplay);
      displayTopBottomProgress(buf, buf2, progress, TOP_PROGRESS);
  }

  else if (scaleStatus == STATUS_EMPTY)
  {
    if (millis() - lastSignificantWeightChangeAt > SLEEP_AFTER_MS && millis() - lastSignificantPotiChangeAt > SLEEP_AFTER_MS)
    {
      u8g2.sendBuffer();
      delay(100);
      return;
    }
    if (currentGrinderMode == GRINDER_MODE_TIMER){
      snprintf(buf, sizeof(buf), "%3.1fs", totalTimerTime);
      snprintf(buf2, sizeof(buf2), "%3.1fg", scaleWeight);
      displayTopBottomProgress(buf, buf2, 1, TOP_PROGRESS);
    }
    else if (currentGrinderMode == GRINDER_MODE_SCALE)
    {
      snprintf(buf, sizeof(buf), "%3.1fg", scaleWeight);
      snprintf(buf2, sizeof(buf2), "%3.1fg", coffeeDoseWeight);
      displayTopBottomProgress(buf, buf2, 1, BOTTOM_PROGRESS);
    }

  }
  else if (scaleStatus == STATUS_SCALE_GRINDING_FINISHED)
  {

    double timeToDisplay = (double)((finishedGrindingAt - startedGrindingAt) / 1000);
    double weightToDisplay = scaleWeight - cupWeightEmpty;
    double progress = weightToDisplay / coffeeDoseWeight;
    snprintf(buf, sizeof(buf), "%3.1fs", timeToDisplay);
    snprintf(buf2, sizeof(buf2), "%3.1fg", weightToDisplay);
    displayTopBottomProgress(buf, buf2, progress, BOTTOM_PROGRESS);
  }
  else if (scaleStatus == STATUS_TIMER_GRINDING_FINISHED)
  {
      double timeToDisplay = 0.0;
      double weightToDisplay = scaleWeight - cupWeightEmpty;
      double progress = 1.0;
      snprintf(buf, sizeof(buf), "%3.1fs", timeToDisplay);
      snprintf(buf2, sizeof(buf2), "%3.1fg", weightToDisplay);
      displayTopBottomProgress(buf, buf2, progress, TOP_PROGRESS);
  }
  else if (scaleStatus == STATUS_MANUAL_GRINDING_FINISHED)
  {
      double timeToDisplay = (double)(finishedGrindingAt - startedGrindingAt)/1000;
      double weightToDisplay = scaleWeight - cupWeightEmpty;
      double progress = timeToDisplay*0.05;
      snprintf(buf, sizeof(buf), "%3.1fs", timeToDisplay);
      snprintf(buf2, sizeof(buf2), "%3.1fg", weightToDisplay);
      displayTopBottomProgress(buf, buf2, progress, TOP_PROGRESS);
  }
  else if (scaleStatus == STATUS_GRINDING_FAILED)
  {

    u8g2.setFontPosTop();
    u8g2.setFont(u8g2_font_7x14B_tf);
    centerPrintToScreen("Grinding failed", 0);

    u8g2.setFont(u8g2_font_7x13_tr);
    centerPrintToScreen("Press the balance", 32);
    centerPrintToScreen("to reset", 42);
    u8g2.setFontPosBaseline();
  }
  u8g2.sendBuffer();
}

void setupDisplay()
{
  Serial.println("init_display");

  u8g2.begin();
  u8g2.clearBuffer(); // clear the internal memory
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.drawXBM(0,0,coffeeOctopus_width,coffeeOctopus_height,coffeeOctopus_bits);
  u8g2.sendBuffer();
  Serial.println("init_display done");
}
