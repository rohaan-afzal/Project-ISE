#pragma once
#ifndef PAYMENT_H
#define PAYMENT_H

#include <string>

struct Payment
{
    double amount;
    std::string mode;     // "cash", "card", "wallet"
    std::string merchant; // e.g., "Cash Counter", "HBL POS", "JazzCash", "EasyPaisa"

    Payment() : amount(0.0), mode("cash"), merchant("-") {}
    Payment(double amt, const std::string& paymentMode, const std::string& merchantName = "-")
        : amount(amt), mode(paymentMode), merchant(merchantName.empty() ? "-" : merchantName) {}
};

#endif