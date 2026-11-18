#pragma once
#ifndef RECORDFORMAT_H
#define RECORDFORMAT_H

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

class RecordFormat
{
public:
    static std::string escapeText(const std::string& text)
    {
        std::string escaped;
        escaped.reserve(text.size());
        for (char c : text)
        {
            if (c == '\\')
                escaped += "\\\\";
            else if (c == ' ')
                escaped += "\\s";
            else if (c == '#')
                escaped += "\\h";
            else
                escaped += c;
        }
        return escaped.empty() ? "-" : escaped;
    }

    static std::string unescapeText(const std::string& text)
    {
        if (text == "-")
            return "";
        std::string result;
        result.reserve(text.size());
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '\\' && i + 1 < text.size())
            {
                char next = text[i + 1];
                if (next == 's') { result += ' '; ++i; continue; }
                if (next == 'h') { result += '#'; ++i; continue; }
                if (next == '\\') { result += '\\'; ++i; continue; }
            }
            result += text[i];
        }
        return result;
    }

    static std::string formatNumber(double value)
    {
        std::ostringstream oss;
        if (std::floor(value) == value)
            oss << static_cast<long long>(value);
        else
            oss << std::fixed << std::setprecision(2) << value;
        return oss.str();
    }

    static std::string joinFields(const std::vector<std::string>& fields)
    {
        std::ostringstream oss;
        for (std::size_t i = 0; i < fields.size(); ++i)
        {
            if (i > 0)
                oss << ' ';
            oss << fields[i];
        }
        oss << " #";
        return oss.str();
    }

    static bool splitLine(const std::string& line, std::vector<std::string>& fields)
    {
        fields.clear();
        std::istringstream iss(line);
        std::string token;
        bool foundTerminator = false;

        while (iss >> token)
        {
            if (token == "#")
            {
                foundTerminator = true;
                break;
            }
            fields.push_back(token);
        }

        // Reject trailing tokens after '#' or missing '#'
        std::string extra;
        if (!foundTerminator || (iss >> extra))
            return false;

        return !fields.empty();
    }

    static bool toInt(const std::string& text, int& out)
    {
        if (text.empty())
            return false;
        try
        {
            std::size_t pos = 0;
            int val = std::stoi(text, &pos);
            if (pos != text.size())
                return false;
            out = val;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static bool toDouble(const std::string& text, double& out)
    {
        if (text.empty())
            return false;
        try
        {
            std::size_t pos = 0;
            double val = std::stod(text, &pos);
            if (pos != text.size() || !std::isfinite(val))
                return false;
            out = val;
            return true;
        }
        catch (...)
        {
            return false;
        }
    }
};

#endif