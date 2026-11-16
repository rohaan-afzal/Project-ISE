#pragma once

#include <opencv2/core.hpp>
#include <string>

// Replaceable boundary between barcode acquisition/decoding and product lookup.
class BarcodeReader {
public:
    virtual ~BarcodeReader() = default;
    virtual std::string read(const cv::Mat& image) const = 0;
};
