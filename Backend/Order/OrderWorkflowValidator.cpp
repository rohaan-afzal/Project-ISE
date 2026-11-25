#include "OrderWorkflowValidator.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {
bool withinSevenDays(const Order& order)
{
    std::tm paidAt{};
    std::istringstream dateTime(order.date + " " + order.time);
    dateTime >> std::get_time(&paidAt, "%Y-%m-%d %H:%M:%S");
    if (dateTime.fail()) return false;

    const std::time_t paidTimestamp = std::mktime(&paidAt);
    if (paidTimestamp == static_cast<std::time_t>(-1)) return false;
    const std::time_t now = std::time(nullptr);
    const double ageSeconds = std::difftime(now, paidTimestamp);
    return ageSeconds >= 0.0 && ageSeconds <= 7.0 * 24.0 * 60.0 * 60.0;
}
}

bool OrderWorkflowValidator::canEditOnHoldOrder(const Order& order)
{
    return order.status == "on-hold";
}

bool OrderWorkflowValidator::canEditPaidOrderWithinSevenDays(const Order& order)
{
    return order.status == "paid" && withinSevenDays(order);
}

void OrderWorkflowValidator::requireEditablePaidOrder(const Order& order)
{
    if (order.status != "paid")
        throw std::runtime_error("Only paid orders can use the paid-order edit workflow.");
    if (!withinSevenDays(order))
        throw std::runtime_error("Paid orders can only be edited within seven days of payment.");
}
