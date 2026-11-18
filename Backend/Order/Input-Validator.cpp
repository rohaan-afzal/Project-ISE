#include "Input-Validator.h"
#include "Record-Format.h"
#include <cctype>
#include <vector>

bool InputValidator::isValidBarcode(const std::string& barcode)
{
    if (barcode.size() != 6 || barcode[0] == '0')
        return false;
    for (char c : barcode)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
            return false;
    }
    return true;
}

bool InputValidator::isValidID(const std::string& id)
{
    if (id.empty() || id.size() > 20)
        return false;
    for (char c : id)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_')
            return false;
    }
    return true;
}

bool InputValidator::isValidDate(const std::string& date)
{
    // Format: YYYY-MM-DD
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
        return false;
    for (int i = 0; i < 10; ++i)
    {
        if (i == 4 || i == 7) continue;
        if (!std::isdigit(static_cast<unsigned char>(date[i])))
            return false;
    }
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    return (month >= 1 && month <= 12 && day >= 1 && day <= 31);
}

bool InputValidator::isValidTime(const std::string& time)
{
    // Format: HH:MM or HH:MM:SS
    if (time.size() != 5 && time.size() != 8)
        return false;
    if (time[2] != ':' || (time.size() == 8 && time[5] != ':'))
        return false;
    for (std::size_t i = 0; i < time.size(); ++i)
    {
        if (i == 2 || i == 5) continue;
        if (!std::isdigit(static_cast<unsigned char>(time[i])))
            return false;
    }
    int hour = std::stoi(time.substr(0, 2));
    int minute = std::stoi(time.substr(3, 2));
    int second = (time.size() == 8) ? std::stoi(time.substr(6, 2)) : 0;
    return (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59);
}

bool InputValidator::isValidUnit(const std::string& unit)
{
    if (unit.empty() || unit.size() > 16)
        return false;
    for (char c : unit)
    {
        if (!std::isalpha(static_cast<unsigned char>(c)))
            return false;
    }
    return true;
}

bool InputValidator::isValidQuantity(int quantity, int availableStock)
{
    if (quantity <= 0)
        return false;
    if (availableStock >= 0 && quantity > availableStock)
        return false;
    return true;
}

bool InputValidator::isValidOrderStatus(const std::string& status)
{
    return status == "in-progress" ||
        status == "on-hold" ||
        status == "paid" ||
        status == "archived";
}

bool InputValidator::isValidPaymentMode(const std::string& mode)
{
    return mode == "cash" ||
        mode == "card" ||
        mode == "wallet";
}

bool InputValidator::isValidPayment(double amount, const std::string& mode, const std::string& merchant)
{
    if (amount < 0.0 || !isValidPaymentMode(mode))
        return false;
    if ((mode == "card" || mode == "wallet") && merchant.empty())
        return false;
    return true;
}

bool InputValidator::isValidHashRecord(const std::string& line, int expectedFieldCount)
{
    std::vector<std::string> fields;
    if (!RecordFormat::splitLine(line, fields))
        return false;
    return expectedFieldCount <= 0 || static_cast<int>(fields.size()) == expectedFieldCount;
}