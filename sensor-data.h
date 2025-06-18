#pragma once

#include <stdlib.h>

#define QSTR     "%d.%02u"
#define Q2STR(q) ((q) / 100), abs((q) % 100)

typedef struct {
  int16_t temp;
  int16_t rh;
  int16_t light_visible;
  int16_t light_total;
} sensor_data_t;
