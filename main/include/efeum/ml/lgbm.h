#pragma once

#ifndef BoosterHandle
typedef void* BoosterHandle;
#endif

int LoadLGBMModel(char const* modelFile, BoosterHandle* booster, int* numFeatures);

void FreeLGBMModel(BoosterHandle booster);
