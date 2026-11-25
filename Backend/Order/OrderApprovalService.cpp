#include "OrderApprovalService.h"
#include "OrderWorkflowValidator.h"
#include <stdexcept>

std::string OrderApprovalService::requireManagerApproval(const Order& order,
                                                        bool explicitlyApproved,
                                                        const std::string& managerName)
{
    OrderWorkflowValidator::requireEditablePaidOrder(order);
    if (!explicitlyApproved)
        throw std::invalid_argument("A manager must explicitly approve the paid-order edit.");
    if (managerName.empty())
        throw std::invalid_argument("Manager approval is required to edit a paid order.");
    return managerName;
}
