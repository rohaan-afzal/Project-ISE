#include "PaymentProcessor.h"
#include "Input-Validator.h"
#include <stdexcept>

std::unique_ptr<PaymentProcessor> PaymentProcessor::create(const std::string& mode)
{
    if (mode == "cash")
        return std::make_unique<CashPaymentProcessor>();
    if (mode == "card")
        return std::make_unique<CardPaymentProcessor>();
    if (mode == "wallet")
        return std::make_unique<WalletPaymentProcessor>();
    throw std::invalid_argument("Unsupported payment mode: " + mode);
}

Payment CashPaymentProcessor::process(double amountDue, double amountTendered, const std::string& merchant) const
{
    if (amountDue <= 0.0)
        throw std::invalid_argument("Order total must be greater than zero.");
    if (amountTendered < amountDue)
        throw std::invalid_argument("Cash tendered is less than total payable.");

    const std::string resolvedMerchant = merchant.empty() ? "Cash Counter" : merchant;
    if (!InputValidator::isValidPayment(amountDue, "cash", resolvedMerchant))
        throw std::invalid_argument("Invalid cash payment details.");

    return Payment(amountDue, "cash", resolvedMerchant);
}

Payment CardPaymentProcessor::process(double amountDue, double amountTendered, const std::string& merchant) const
{
    if (amountDue <= 0.0)
        throw std::invalid_argument("Order total must be greater than zero.");
    if (merchant.empty())
        throw std::invalid_argument("Card merchant/bank name is required (e.g., HBL, Meezan, Alfalah).");
    if (amountTendered > 0.0 && amountTendered < amountDue)
        throw std::invalid_argument("Card payment amount is less than total payable.");

    if (!InputValidator::isValidPayment(amountDue, "card", merchant))
        throw std::invalid_argument("Invalid card payment details.");

    return Payment(amountDue, "card", merchant);
}

Payment WalletPaymentProcessor::process(double amountDue, double amountTendered, const std::string& merchant) const
{
    if (amountDue <= 0.0)
        throw std::invalid_argument("Order total must be greater than zero.");
    if (merchant.empty())
        throw std::invalid_argument("Wallet provider/merchant is required (e.g., JazzCash, EasyPaisa, SadaPay).");
    if (amountTendered > 0.0 && amountTendered < amountDue)
        throw std::invalid_argument("Wallet payment amount is less than total payable.");

    if (!InputValidator::isValidPayment(amountDue, "wallet", merchant))
        throw std::invalid_argument("Invalid wallet payment details.");

    return Payment(amountDue, "wallet", merchant);
}