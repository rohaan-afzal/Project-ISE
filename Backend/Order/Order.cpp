#include "Order.h"

Order::Order()
    : status("in-progress")
{
}

std::vector<BillLine> Order::toBillLines() const
{
    std::vector<BillLine> lines;
    lines.reserve(items.size());
    for (const auto& item : items)
    {
        lines.emplace_back(item.product, item.quantity);
    }
    return lines;
}