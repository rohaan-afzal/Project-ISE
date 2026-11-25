#pragma once

#include "Payment.h"
#include "Order.h"
#include <string>

class PaymentService {
public:
    void processPayment(Order& order, double amountDue, double amountTendered,
                           const std::string& mode, const std::string& merchant) const;
    void updatePaidOrderPayment(Order& order, const Payment& previousPayment,
                                   double updatedOrderTotal,
                                   double amountTendered, const std::string& mode,
                                   const std::string& merchant) const;
};
