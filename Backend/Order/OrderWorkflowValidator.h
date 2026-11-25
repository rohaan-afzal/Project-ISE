#pragma once

#include "Order.h"

class OrderWorkflowValidator {
public:
    static bool canEditOnHoldOrder(const Order& order);
    static bool canEditPaidOrderWithinSevenDays(const Order& order);
    static void requireEditablePaidOrder(const Order& order);
};
