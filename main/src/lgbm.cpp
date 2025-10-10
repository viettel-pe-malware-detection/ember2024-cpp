#include <LightGBM/c_api.h>
#include "efeum/ml/lgbm.h"
#include <iostream>

int LoadLGBMModel(char const* modelFile, BoosterHandle* booster, int* numFeatures) {
    int num_iterations;
    int ret = LGBM_BoosterCreateFromModelfile(modelFile, &num_iterations, booster);
    if (ret != 0) {
        std::cerr << "Failed to load model!" << std::endl;
        return ret;
    }

    if (numFeatures != NULL) {
        LGBM_BoosterGetNumFeature(booster, numFeatures);
    }

    return ret;
}

void FreeLGBMModel(BoosterHandle booster) {
    LGBM_BoosterFree(booster);
}
