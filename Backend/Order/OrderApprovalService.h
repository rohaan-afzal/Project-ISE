#pragma once

#include <string>
#include "Order.h"

class OrderApprovalService {
public:
    // The manager's explicit approval is recorded with the edited order.
    static std::string requireManagerApproval(const Order& order, bool explicitlyApproved,
                                              const std::string& managerName);
};
