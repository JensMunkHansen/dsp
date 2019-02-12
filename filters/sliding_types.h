#pragma once

#include <stdint.h>

typedef struct ImageInt32 {
  uint32_t nLines;
  uint32_t nSamples;
  float* pData;
} Image2D;

typedef struct SlidingFilterBank {
  uint32_t nFilters;
  uint32_t nCoefficients;
  float* pData;
} SlidingFilterBank;
