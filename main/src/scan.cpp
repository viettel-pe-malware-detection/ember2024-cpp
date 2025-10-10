#include "efeum/scanning/scan.h"
#include <filesystem>
#include <iostream>
#include <vector>

double scan(BoosterHandle booster, wchar_t const* peFilePath) {
    auto absPath = std::filesystem::absolute(peFilePath);
    std::cerr << "Scanning: " << absPath << '\n';

    if (!std::filesystem::exists(peFilePath)) {
        std::cerr << "File does not exist: " << peFilePath << std::endl;
        return -1;
    } else {
        std::cerr << "File exists and size = "
                << std::filesystem::file_size(peFilePath)
                << " bytes\n";
    }

    // mmap it
    std::error_code error;
    mio::mmap_source mmap;
    try {
        mmap = mio::make_mmap_source(absPath.string(), 0, mio::map_entire_file, error);
    } catch (std::system_error const& e) {
        std::cerr << "Error while mmap'ing (exception): " << e.what() << '\n';
        return -1;
    }

    if (error) {
        std::cerr << "Error while mmap'ing: " << error << '\n';
        return -1;
    }
    std::cerr << "mmap'ed successfully" << '\n';

    #define N_ROWS 1
    #define N_COLS ef.getDim()

    EMBER2024FeatureExtractor ef;
    feature_t const* inputVector = ef.run(reinterpret_cast<uint8_t const*>(mmap.data()), mmap.size());
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
        return 1;
    }

    return out_result[0];
}
