#include "PaymentService.h"
#include "PaymentProcessor.h"
#include "../Billing/BillCalc.h"
#include <stdexcept>

void PaymentService::processPayment(Order& order, double amountDue, double amountTendered,
                                    const std::string& mode, const std::string& merchant) const
{
    order.payment = PaymentProcessor::create(mode)->process(amountDue, amountTendered, merchant);
    order.status = "paid";
}

void PaymentService::updatePaidOrderPayment(Order& order, const Payment& previousPayment,
                                            double updatedOrderTotal,
                                            double amountTendered,
                                            const std::string& mode,
                                            const std::string& merchant) const
{
    const double difference = PricingService::roundMoney(updatedOrderTotal - previousPayment.amount);
    if (difference > 0.0) {
        PaymentProcessor::create(mode)->process(difference, amountTendered, merchant);
        order.payment = Payment(updatedOrderTotal, mode, merchant);
    }
    else if (difference < 0.0) {
        PaymentProcessor::create(previousPayment.mode)->processRefund(
            -difference, previousPayment.merchant);
        order.payment = Payment(updatedOrderTotal, previousPayment.mode, previousPayment.merchant);
    }
    else {
        order.payment = Payment(updatedOrderTotal, previousPayment.mode, previousPayment.merchant);
    }
    order.status = "paid";
}
