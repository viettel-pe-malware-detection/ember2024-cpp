#include "efeum/scanning/scan.h"
#include "efeum/mmapping.h"
#include <iostream>
#include <vector>

#include <LightGBM/c_api.h>
#include "efeum/ml/lgbm.h"
#include "efe/core.h"


template<typename T = feature_t>
constexpr int getLGBMInputDataType() {
    if constexpr (std::is_same_v<T, float>) {
        return C_API_DTYPE_FLOAT32;
    } else {
        static_assert( std::is_same_v<T, double> );
        return C_API_DTYPE_FLOAT64;
    }
}

double scan(BoosterHandle booster, wchar_t const* peFilePath) {
    mmap_source_t mmap = create_mmap_source(peFilePath, 0, 0);
    if (!mmap) {
        std::cerr << "Error while mmap'ing." << '\n';
        return -1;
    }
    std::cerr << "mmap'ed successfully" << '\n';

    #define N_ROWS 1
    #define N_COLS ef.getDim()

    EMBER2024FeatureExtractor ef;
    feature_t const* inputVector = ef.run(
        reinterpret_cast<uint8_t const*>(get_mmap_data(mmap)),
        get_mmap_size(mmap)
    );
    std::cerr << "feature extraction run successfully" << '\n';
    // std::cerr << "feature vector:\n";
    // for (size_t i = 0; i < N_COLS; ++i) {
    //     if (i % 11 == 0) std::cerr << '\n';
    //     std::cerr << inputVector[i] << " ";
    // }
    // std::cerr << '\n';

    std::vector<double> out_result(N_ROWS, 0.0);
    int64_t out_len;

    const char* params = ""; // "predict_disable_shape_check=true";
    int ret = LGBM_BoosterPredictForMat(
        booster,
        inputVector,             // pointer to input features
        getLGBMInputDataType(),     // data type
        N_ROWS,                       // number of rows
        N_COLS,                       // number of columns
        1,                       // is_row_major
        C_API_PREDICT_NORMAL,    // prediction type
        0,                       // start_iteration
        -1,                      // num_iteration (-1 = all)
        params,                 // parameter string
        &out_len,
        out_result.data()
    );

    if (ret != 0) {
        const char *err_msg = LGBM_GetLastError();
        std::cerr << "Prediction failed: " << err_msg << std::endl;
        out_result[0] = -1;
    }

    delete_mmap_source(mmap);

    return out_result[0];
}
