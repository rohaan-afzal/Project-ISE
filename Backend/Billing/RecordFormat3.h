
#pragma once
#ifndef RECORDFORMAT_H
#define RECORDFORMAT_H

#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

class RecordFormat
{
public:

    // Escapes text so that it can safely be stored as one field.
    static std::string escapeText(const std::string& text)
    {
        if (text.empty())
            return "-";

        std::string escaped;

        for (char c : text)
        {
            if (c == '\\')
                escaped += "\\\\";
            else if (c == ' ')
                escaped += "\\s";
            else if (c == '#')
                escaped += "\\h";
            else if (c == '\t')
                escaped += "\\t";
            else if (c == '\n')
                escaped += "\\n";
            else if (c == '\r')
                escaped += "\\r";
            else if (c == '-' && text.size() == 1)
                escaped += "\\d";
            else
                escaped += c;
        }

        return escaped;
    }


    // Converts escaped text back to its original form.
    static std::string unescapeText(const std::string& text)
    {
        if (text == "-")
            return "";

        std::string result;

        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] == '\\' && i + 1 < text.size())
            {
                char next = text[i + 1];

                if (next == 's')
                {
                    result += ' ';
                    ++i;
                }
                else if (next == 'h')
                {
                    result += '#';
                    ++i;
                }
                else if (next == '\\')
                {
                    result += '\\';
                    ++i;
                }
                else if (next == 't')
                {
                    result += '\t';
                    ++i;
                }
                else if (next == 'n')
                {
                    result += '\n';
                    ++i;
                }
                else if (next == 'r')
                {
                    result += '\r';
                    ++i;
                }
                else if (next == 'd')
                {
                    result += '-';
                    ++i;
                }
                else
                {
                    result += text[i];
                }
            }
            else
            {
                result += text[i];
            }
        }

        return result;
    }


    // Formats monetary values to two decimal places.
    static std::string formatNumber(double value)
    {
        if (!std::isfinite(value))
            throw std::invalid_argument("Invalid numeric value");

        std::ostringstream oss;

        oss << std::fixed << std::setprecision(2) << value;

        return oss.str();
    }


    // Joins fields and adds the required # terminator.
    static std::string joinFields(const std::vector<std::string>& fields)
    {
        if (fields.empty())
            throw std::invalid_argument("Cannot format an empty record");

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


    // Splits a record and verifies its # terminator.
    static bool splitLine(
        const std::string& line,
        std::vector<std::string>& fields)
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

        // Reject missing terminator.
        if (!foundTerminator)
            return false;

        // Reject anything after the terminator.
        std::string extra;

        if (iss >> extra)
            return false;

        return !fields.empty();
    }


    // Converts a string into an integer.
    static bool toInt(const std::string& text, int& out)
    {
        if (text.empty())
            return false;

        try
        {
            std::size_t pos = 0;

            int value = std::stoi(text, &pos);

            if (pos != text.size())
                return false;

            out = value;

            return true;
        }
        catch (...)
        {
            return false;
        }
    }


    // Converts a string into a valid finite double.
    static bool toDouble(const std::string& text, double& out)
    {
        if (text.empty())
            return false;

        try
        {
            std::size_t pos = 0;

            double value = std::stod(text, &pos);

            if (pos != text.size() || !std::isfinite(value))
                return false;

            out = value;

            return true;
        }
        catch (...)
        {
            return false;
        }
    }
};

#endif
