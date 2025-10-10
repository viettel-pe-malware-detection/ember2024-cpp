#include <LightGBM/c_api.h>
#include "efeum/ml/lgbm.h"
#include <iostream>

int LoadLGBMModel(char const* modelFile, BoosterHandle* pBooster, int* numFeatures) {
    int num_iterations;
    int ret = LGBM_BoosterCreateFromModelfile(modelFile, &num_iterations, pBooster);
    if (ret != 0) {
        std::cerr << "Failed to load model!" << std::endl;
        return ret;
    }

    if (numFeatures != NULL) {
        LGBM_BoosterGetNumFeature(*pBooster, numFeatures);
    }

    return ret;
}

void FreeLGBMModel(BoosterHandle booster) {
    LGBM_BoosterFree(booster);
}
