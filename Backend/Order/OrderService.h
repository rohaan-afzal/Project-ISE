#pragma once
#ifndef ORDERSERVICE_H
#define ORDERSERVICE_H

#include <string>
#include <vector>
#include "Order.h"
#include "OrderRepository.h"
#include "ProductService.h"
#include "BillCalc3.h"

class OrderService
{
private:
    ProductService& productService;
    OrderRepository& orderRepository;
    OrderDetailRepository& detailRepository;
    BillService& billService;
    const SeasonalDiscountService* seasonService;
    Order currentOrder;

    static std::string currentDateString();
    static std::string currentTimeString();
    std::string generateNextOrderID() const;

public:
    OrderService(ProductService& prodService,
                 OrderRepository& orderRepo,
                 OrderDetailRepository& detailRepo,
                 BillService& billSvc,
                 const SeasonalDiscountService* seasonSvc = nullptr);

    void startNewOrder(const std::string& customerName = "Walk-in Customer",
                       const std::string& customerPhone = "-");
    void setCustomerDetails(const std::string& customerName, const std::string& customerPhone);

    void addProductByBarcode(const std::string& barcode, int quantity = 1);
    void updateItemQuantity(const std::string& barcode, int newQuantity);
    void removeItem(const std::string& barcode);
    void clearOrder();

    void holdCurrentOrder();
    std::vector<Order> getHeldOrders() const;
    void resumeOrder(const std::string& orderID);

    Bill checkoutAndGenerateBill(const std::string& paymentMode,
                                 double amountTendered,
                                 const std::string& merchant = "",
                                 double specialDiscountPercent = 0.0);

    const Order& getCurrentOrder() const { return currentOrder; }
    double getSubtotal() const;
    double getTotalTax() const;
    double getTotalDiscount() const;
    double getActiveSeasonalPercent() const;
    double getTotalPayable(double specialDiscountPercent = 0.0) const;


    bool verifyManagerPin(const std::string& pin) const;
    bool isOrderEditableWithin7Days(const Order& order, std::string& reasonOut) const;
    std::vector<Order> getEditablePaidOrders() const;
    Order getOrderWithDetails(const std::string& orderID) const;
    Bill saveEditedPaidOrder(const std::string& orderID,
                             const std::vector<OrderItem>& updatedItems,
                             const std::string& managerPin,
                             double specialDiscountPercent = 0.0);
};

#endif