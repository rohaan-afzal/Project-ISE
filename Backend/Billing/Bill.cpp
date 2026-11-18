#include "Bill.h"
using namespace std;

BillLine::BillLine() : unitPrice(0.0), discount(0.0), tax(0.0), quantity(0){}

BillLine::BillLine(const Product& product, int quantity){
    productAcronym = product.acronym;
    unitPrice = product.unitPrice;
    discount = product.discount;
    tax = product.tax;
    this->quantity = quantity;
}

Bill::Bill() : totalWithoutTax(0.0), totalTax(0.0), totalDiscount(0.0), totalPayable(0.0){}