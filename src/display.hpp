#pragma once

#include "controller.hpp"
#include <SPI.h>
#include <U8g2lib.h>

void setupDisplay();
void updateDisplay( void * parameter);
void drawadc();
