#pragma once
#ifndef PAYMENTPROCESSOR_H
#define PAYMENTPROCESSOR_H

#include <memory>
#include <string>
#include "Payment.h"

class PaymentProcessor
{
public:
    virtual ~PaymentProcessor() = default;
    virtual Payment process(double amountDue, double amountTendered, const std::string& merchant) const = 0;
    virtual std::string modeName() const = 0;

    static std::unique_ptr<PaymentProcessor> create(const std::string& mode);
};

class CashPaymentProcessor : public PaymentProcessor
{
public:
    Payment process(double amountDue, double amountTendered, const std::string& merchant) const override;
    std::string modeName() const override { return "cash"; }
};

class CardPaymentProcessor : public PaymentProcessor
{
public:
    Payment process(double amountDue, double amountTendered, const std::string& merchant) const override;
    std::string modeName() const override { return "card"; }
};

class WalletPaymentProcessor : public PaymentProcessor
{
public:
    Payment process(double amountDue, double amountTendered, const std::string& merchant) const override;
    std::string modeName() const override { return "wallet"; }
};

#endif