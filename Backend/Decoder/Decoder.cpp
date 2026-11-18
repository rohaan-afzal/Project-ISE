#include "Decoder.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;

// Code 128 Constants matching the encoder specifications
const string Decoder::startPattern = "11010011100";
const string Decoder::stopPattern = "1100011101011";
const int Decoder::startValue = 105;
const int Decoder::checksumDivisor = 103;

// Full Code 128 Pattern Table (0 to 102)[cite: 4]
const string Decoder::patterns[103] = {
    "11011001100", "11001101100", "11001100110", "10010011000", "10010001100",
    "10001001100", "10011001000", "10011000100", "10001100100", "11001001000",
    "11001000100", "11000100100", "10110011100", "10011011100", "10011001110",
    "10111001100", "10011101100", "10011100110", "11001110010", "11001011100",
    "11001001110", "11011100100", "11001110100", "11101101110", "11101001100",
    "11100101100", "11100100110", "11101100100", "11100110100", "11100110010",
    "11011011000", "11011000110", "11000110110", "10100011000", "10001011000",
    "10001000110", "10110001000", "10001101000", "10001100010", "11010001000",
    "11000101000", "11000100010", "10110111000", "10110001110", "10001101110",
    "10111011000", "10111000110", "10001110110", "11101110110", "11010001110",
    "11000101110", "11011101000", "11011100010", "11011101110", "11101011000",
    "11101000110", "11100010110", "11101101000", "11101100010", "11100011010",
    "11101111010", "11001000010", "11110001010", "10100110000", "10100001100",
    "10010110000", "10010000110", "10000101100", "10000100110", "10110010000",
    "10110000100", "10011010000", "10011000010", "10000110100", "10000110010",
    "11000010010", "11001010000", "11110111010", "11000010100", "10001111010",
    "10100111100", "10010111100", "10010011110", "10111100100", "10011110100",
    "10011110010", "11110100100", "11110010100", "11110010010", "11011011110",
    "11011110110", "11110110110", "10101111000", "10100011110", "10001011110",
    "10111101000", "10111100010", "11110101000", "11110100010", "10111011110",
    "10111101110", "11101011110", "11110101110"
};

int Decoder::findPatternIndex(const string& patternChunk)
{
    for (int i = 0; i < 103; i++) {
        if (patterns[i] == patternChunk) {
            return i; // Returns the decoded index value (0-102)
        }
    }
    return -1; // Not found / invalid pattern
}

string Decoder::decodeBinary(const string& binaryBits)
{
    static const string startA = "11010000100";
    static const string startB = "11010010000";
    int startCode = -1;
    int activeSet = 0; // A, B, C
    const string startBits = binaryBits.substr(0, min<size_t>(11, binaryBits.size()));
    if (startBits == startA) { startCode = 103; activeSet = 0; }
    else if (startBits == startB) { startCode = 104; activeSet = 1; }
    else if (startBits == startPattern) { startCode = startValue; activeSet = 2; }
    else throw invalid_argument("Invalid Code 128 start pattern.");

    // Start + N data symbols + checksum + 13-bit stop pattern.
    if (binaryBits.size() < 46 || (binaryBits.size() - 35) % 11 != 0)
        throw invalid_argument("Incomplete Code 128 symbol.");
    if (binaryBits.find_first_not_of("01") != string::npos)
        throw invalid_argument("Code 128 data must contain only binary digits.");
    const size_t dataCount = (binaryBits.size() - 35) / 11;
    if (binaryBits.substr(binaryBits.size() - 13) != stopPattern)
        throw invalid_argument("Invalid Code 128 stop pattern.");

    vector<int> values;
    values.reserve(dataCount + 1);
    int weightedDataSum = 0;
    for (size_t i = 0; i <= dataCount; ++i) {
        const size_t offset = 11 + i * 11;
        const int value = findPatternIndex(binaryBits.substr(offset, 11));
        if (value < 0) throw runtime_error("Invalid Code 128 character pattern.");
        values.push_back(value);
        if (i < dataCount) weightedDataSum += value * static_cast<int>(i + 1);
    }
    int validatedStart = -1;
    const int starts[] = {startCode, 103, 104, 105};
    for (const int candidate : starts) {
        if ((candidate + weightedDataSum) % checksumDivisor == values.back()) {
            validatedStart = candidate;
            break;
        }
    }
    if (validatedStart < 0)
        throw runtime_error("Barcode checksum validation failed.");

    string decoded;
    activeSet = validatedStart == 103 ? 0 : (validatedStart == 104 ? 1 : 2);
    bool shiftNext = false;
    auto appendCharacter = [&](int code, int set) {
        if (set == 2) {
            if (code < 0 || code > 99) return;
            string pair = to_string(code);
            if (code < 10) pair.insert(pair.begin(), '0');
            decoded += pair;
        } else if (set == 0 && code <= 95) {
            decoded.push_back(static_cast<char>(code < 64 ? code + 32 : code - 64));
        } else if (set == 1 && code <= 95) {
            decoded.push_back(static_cast<char>(code + 32));
        }
    };

    for (size_t index = 0; index < dataCount; ++index) {
        const int code = values[index];
        if (activeSet == 2) {
            if (code <= 99) appendCharacter(code, activeSet);
            else if (code == 100) activeSet = 1;
            else if (code == 101) activeSet = 0;
            // Code 102 (FNC1) is a separator; product lookup uses its digits.
            continue;
        }
        if (shiftNext) {
            appendCharacter(code, activeSet == 0 ? 1 : 0);
            shiftNext = false;
        } else if (code <= 95) {
            appendCharacter(code, activeSet);
        } else if (code == 98) {
            shiftNext = true;
        } else if (code == 99) {
            activeSet = 2;
        } else if (code == 100) {
            activeSet = 1;
        } else if (code == 101) {
            activeSet = 0;
        }
    }
    if (decoded.empty()) throw runtime_error("Code 128 barcode contains no product data.");
    return decoded;
}

string Decoder::decodeRuns(const vector<ColorRun>& runs, const vector<int>& moduleCounts)
{
    // Reconstruct the binary bit stream from normalized module counts and color runs
    string reconstructedBinary = "";

    for (size_t i = 0; i < runs.size(); i++) {
        char bitChar = runs[i].isBlack ? '1' : '0';
        int modules = moduleCounts[i];

        // Append the bit character according to its module width
        reconstructedBinary.append(modules, bitChar);
    }

    // Pass the reconstructed binary string into the binary decoder
    return decodeBinary(reconstructedBinary);
}
