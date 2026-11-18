#include "CameraBarcodeReader.h"
#include "../Decoder/Decoder.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/objdetect/barcode.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>

namespace {
std::string decodeScanline(const cv::Mat& row) {
    std::vector<ColorRun> runs;
    if (row.empty())
        throw std::runtime_error("Empty scan line.");
    bool black = row.at<unsigned char>(0, 0) < 128;
    int length = 1;
    for (int x = 1; x < row.cols; ++x) {
        const bool nextBlack = row.at<unsigned char>(0, x) < 128;
        if (nextBlack == black)
            ++length;
        else {
            runs.push_back({black, length, 0});
            black = nextBlack;
            length = 1;
        }
    }
    runs.push_back({black, length, 0});
    // Locate the Code 128 start pattern, then sample module centers directly.
    // Direct sampling tolerates camera blur and screen moire better than rounding
    // every run independently into module counts.
    const std::array<std::array<int, 6>, 3> startPatterns{{
        {{2, 1, 1, 4, 1, 2}}, // Code 128 A
        {{2, 1, 1, 2, 1, 4}}, // Code 128 B
        {{2, 1, 1, 2, 3, 2}}  // Code 128 C
    }};
    if (runs.size() < 6)
        throw std::runtime_error("No barcode found in this scan line.");
    std::vector<int> runOffsets(runs.size(), 0);
    for (size_t i = 1; i < runs.size(); ++i)
        runOffsets[i] = runOffsets[i - 1] + runs[i - 1].pixelLength;
    for (size_t start = 0; start + 6 <= runs.size(); ++start) {
        const int runStartX = runOffsets[start];
        if (runs.size() - start < 25)
            continue;
        if (!runs[start].isBlack || runs[start + 1].isBlack)
            continue;
        for (const auto& startWidths : startPatterns) {
            double numerator = 0.0;
            double denominator = 0.0;
            for (size_t i = 0; i < startWidths.size(); ++i) {
                numerator += runs[start + i].pixelLength * startWidths[i];
                denominator += startWidths[i] * startWidths[i];
            }
            const double scale = numerator / denominator;
            bool startMatches = true;
            for (size_t i = 0; i < startWidths.size(); ++i)
                if (std::abs(runs[start + i].pixelLength - startWidths[i] * scale) > std::max(1.25, scale * 1.1))
                    startMatches = false;
            if (!startMatches || scale <= 0.0) continue;

            const int availableSymbols = static_cast<int>((runs.size() - start - 19) / 6);
            const int maxSymbols = std::min(20, availableSymbols);
            for (int symbols = 1; symbols <= maxSymbols; ++symbols) {
                const int barcodeModules = 35 + 11 * symbols;
                for (int scaleStep = -10; scaleStep <= 10; ++scaleStep) {
                    const double adjustedScale = scale * (1.0 + scaleStep * 0.01);
                    for (int shift = -2; shift <= 2; ++shift) {
                        std::string bits;
                        bits.reserve(static_cast<size_t>(barcodeModules));
                        bool inBounds = true;
                        for (int module = 0; module < barcodeModules; ++module) {
                            const int x = static_cast<int>(std::floor(runStartX + shift + (module + 0.5) * adjustedScale));
                            if (x < 0 || x >= row.cols)
                            {
                                inBounds = false;
                                break;
                            }
                            bits.push_back(row.at<unsigned char>(0, x) < 128 ? '1' : '0');
                        }
                        if (!inBounds)
                            continue;
                        try {
                            return Decoder::decodeBinary(bits);
                        }
                        catch (const std::exception&) { /* Adjust scale/alignment and symbol length. */ }
                    }
                }
            }
        }
    }
    throw std::runtime_error("No valid barcode detected. Check focus, lighting, and visibility.");
}

std::string tryRows(const cv::Mat& binary) {
    std::string lastError = "No barcode detected. Move the barcode into view and try again.";
    // A barcode may sit anywhere in the frame; scan many rows through its bar height.
    constexpr int rowSamples = 21;
    for (int i = 0; i < rowSamples; ++i) {
        const int y = (binary.rows - 1) * i / (rowSamples - 1);
        const int firstRow = std::max(0, y - 1);
        const int lastRow = std::min(binary.rows - 1, y + 1);
        cv::Mat stripe = binary.rowRange(firstRow, lastRow + 1);
        cv::Mat majority;
        cv::reduce(stripe, majority, 0, cv::REDUCE_AVG, CV_8U);
        cv::threshold(majority, majority, 127, 255, cv::THRESH_BINARY);
        try {
            return decodeScanline(majority);
        }
        catch (const std::exception& e) {
            lastError = e.what();
        }
    }
    throw std::runtime_error(lastError);
}

cv::Mat rotateForDeskew(const cv::Mat& source, double degrees) {
    const cv::Point2f center(source.cols / 2.0F, source.rows / 2.0F);
    const double radians = degrees * CV_PI / 180.0;
    const double alpha = std::cos(radians);
    const double beta = std::sin(radians);

    cv::Mat transform = (cv::Mat_<double>(2, 3) <<
                             alpha,  beta, (1.0 - alpha) * center.x - beta * center.y,
                         -beta,  alpha, beta * center.x + (1.0 - alpha) * center.y);

    const int width = static_cast<int>(std::ceil(source.cols * std::abs(alpha) + source.rows * std::abs(beta)));
    const int height = static_cast<int>(std::ceil(source.rows * std::abs(alpha) + source.cols * std::abs(beta)));
    transform.at<double>(0, 2) += width / 2.0 - center.x;
    transform.at<double>(1, 2) += height / 2.0 - center.y;

    cv::Mat rotated;
    cv::warpAffine(source, rotated, transform, cv::Size(width, height), cv::INTER_NEAREST,
                   cv::BORDER_CONSTANT, cv::Scalar(255));
    return rotated;
}
}

std::string CameraBarcodeReader::read(const cv::Mat& image) const {
    if (image.empty())
        throw std::invalid_argument("The camera or image returned an empty frame.");
    cv::Mat gray;
    if (image.channels() == 1)
        gray = image.clone();
    else if (image.channels() == 3)
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    else if (image.channels() == 4)
        cv::cvtColor(image, gray, cv::COLOR_BGRA2GRAY);
    else
        throw std::invalid_argument("Unsupported camera image format.");

    // Run the detector on each right-angle orientation so vertical barcodes work too.
    cv::barcode::BarcodeDetector retailDetector;
    retailDetector.setDownsamplingThreshold(1600.0);
    std::string lastError = "No barcode detected. Move the barcode into view and try again.";
    const int rotations[] = {cv::ROTATE_90_CLOCKWISE, cv::ROTATE_180, cv::ROTATE_90_COUNTERCLOCKWISE};
    for (int orientation = 0; orientation < 4; ++orientation) {
        cv::Mat orientedGray;
        if (orientation == 0) orientedGray = gray;
        else cv::rotate(gray, orientedGray, rotations[orientation - 1]);

        std::vector<std::string> retailCodes;
        std::vector<std::string> retailTypes;
        if (retailDetector.detectAndDecodeWithType(orientedGray, retailCodes, retailTypes)) {
            for (const auto& code : retailCodes)
                if (!code.empty())
                    return code;
        }

        cv::Mat enlarged;
        if (orientedGray.cols < 1600)
            cv::resize(orientedGray, enlarged, cv::Size(), 2.0, 2.0, cv::INTER_LINEAR);
        else
            enlarged = orientedGray;

        cv::Mat otsu;
        cv::threshold(enlarged, otsu, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        cv::Mat adaptive;
        cv::adaptiveThreshold(enlarged, adaptive, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY, 31, 7);
        const cv::Mat thresholds[] = {otsu, adaptive};
        for (const auto& threshold : thresholds) {
            for (int inverted = 0; inverted < 2; ++inverted) {
                cv::Mat binary;
                if (inverted) cv::bitwise_not(threshold, binary);
                else binary = threshold;
                try { return tryRows(binary); }
                catch (const std::exception& e) { lastError = e.what(); }

                // Only deskew after the fast horizontal scan fails. Small angle
                // steps cover handheld tilt without multiplying normal-frame cost.
                for (const double angle : {-20.0, -10.0, 10.0, 20.0}) {
                    try {
                        return tryRows(rotateForDeskew(binary, angle));
                    }
                    catch (const std::exception& e) {
                        lastError = e.what();
                    }
                }
            }
        }
    }
    throw std::runtime_error(lastError);
}

std::string CameraBarcodeReader::readFromCamera(
    const std::function<void(const cv::Mat&)>& onFrame,
    const std::function<bool()>& shouldCancel) const {
    cv::VideoCapture camera(0, cv::CAP_DSHOW);
    if (!camera.isOpened())
        camera.open(0);
    if (!camera.isOpened())
        throw std::runtime_error("Could not open a camera. Check that it is connected and available.");

    // A small capture buffer prevents the preview from displaying stale frames
    // after a heavier decode pass. VGA is sufficient for typical product labels.
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    camera.set(cv::CAP_PROP_BUFFERSIZE, 1);

    cv::Mat latestFrame;
    std::mutex frameMutex;
    std::atomic_bool stopCapture{false};
    std::uint64_t capturedFrames = 0;
    std::thread captureThread([&] {
        cv::Mat captured;
        while (!stopCapture.load() && !shouldCancel()) {
            if (!camera.read(captured) || captured.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            {
                std::lock_guard<std::mutex> lock(frameMutex);
                latestFrame = captured.clone();
                ++capturedFrames;
            }
        }
    });

    std::string lastDecodeError = "No barcode detected yet. Hold it in view or cancel the scan.";
    std::uint64_t lastProcessedFrame = 0;
    while (!shouldCancel()) {
        cv::Mat frame;
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            if (capturedFrames != lastProcessedFrame) {
                frame = latestFrame;
                lastProcessedFrame = capturedFrames;
            }
        }
        if (frame.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        if (onFrame) onFrame(frame);
        try {
            const std::string barcode = read(frame);
            stopCapture = true;
            if (captureThread.joinable()) captureThread.join();
            camera.release();
            return barcode;
        } catch (const std::exception& e) {
            lastDecodeError = e.what();
        }
    }
    stopCapture = true;
    if (captureThread.joinable()) captureThread.join();
    camera.release();
    throw std::runtime_error("Scan cancelled. Last decoder status: " + lastDecodeError);
}