#pragma once
#ifndef INPUTVALIDATOR_H
#define INPUTVALIDATOR_H

#include <string>

class InputValidator
{
public:
    static bool isValidBarcode(const std::string& barcode);
    static bool isValidID(const std::string& id);
    static bool isValidDate(const std::string& date); // YYYY-MM-DD
    static bool isValidTime(const std::string& time); // HH:MM or HH:MM:SS
    static bool isValidUnit(const std::string& unit);
    static bool isValidQuantity(int quantity, int availableStock = -1);
    static bool isValidOrderStatus(const std::string& status);
    static bool isValidPaymentMode(const std::string& mode);
    static bool isValidPayment(double amount, const std::string& mode, const std::string& merchant);
    static bool isValidHashRecord(const std::string& line, int expectedFieldCount);
};

#endif