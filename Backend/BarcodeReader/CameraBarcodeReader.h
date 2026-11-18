#pragma once

#include "BarcodeReader.h"
#include <functional>

class CameraBarcodeReader final : public BarcodeReader {
public:
    std::string read(const cv::Mat& image) const override;
    std::string readFromCamera(
        const std::function<void(const cv::Mat&)>& onFrame,
        const std::function<bool()>& shouldCancel) const;
};
