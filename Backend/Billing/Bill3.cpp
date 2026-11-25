#include "Bill3.h"
using namespace std;

BillLine::BillLine() : unitPrice(0.0), quantity(0), discount(0.0), unitTax(0.0)
{}

BillLine::BillLine(const Product& product, int quantity)
{
    productAcronym = product.acronym;
    unitPrice = product.unitPrice;
    this->quantity = quantity;
    discount = product.discount * quantity;
    unitTax = product.tax;
}

Bill::Bill() : totalWithoutTax(0.0), totalTax(0.0), totalDiscount(0.0), totalPayable(0.0),
    amountReceived(0.0), balanceReturned(0.0), seasonalDiscount(0.0), specialDiscount(0.0)
{
}

BillRequest::BillRequest() : amountReceived(0.0), specialDiscountPercent(0.0)
{
}