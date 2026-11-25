#pragma once
#ifndef ORDERSERVICE_H
#define ORDERSERVICE_H

#include <string>
#include <vector>
#include "Order.h"
#include "OrderRepository.h"
#include "ProductService.h"
#include "BillCalc.h"
#include "InventoryService.h"
#include "PaymentService.h"

class OrderService
{
private:
    ProductService& productService;
    OrderRepository& orderRepository;
    OrderDetailRepository& detailRepository;
    BillService& billService;
    InventoryService inventoryService;
    PaymentService paymentService;
    Order currentOrder;
    bool paidOrderEditActive;
    Order originalPaidOrder;
    bool resumedHeldOrderActive;
    Order persistedHeldOrder;

    static std::string currentDateString();
    static std::string currentTimeString();
    std::string generateNextOrderID() const;

public:
    OrderService(ProductService& prodService,
        OrderRepository& orderRepo,
        OrderDetailRepository& detailRepo,
        BillService& billSvc);

    void startNewOrder(const std::string& customerName = "Walk-in Customer",
        const std::string& customerPhone = "-");
    void setCustomerDetails(const std::string& customerName, const std::string& customerPhone);

    void addProductByBarcode(const std::string& barcode, int quantity = 1);
    void updateItemQuantity(const std::string& barcode, int newQuantity);
    void removeItem(const std::string& barcode);
    void clearOrder();

    void holdCurrentOrder();
    std::vector<Order> getHeldOrders() const;
    std::vector<Order> getEditablePaidOrders() const;
    void resumeOrder(const std::string& orderID);
    void beginPaidOrderEdit(const std::string& orderID, const std::string& managerName,
                            bool managerApproved);
    bool isEditingPaidOrder() const { return paidOrderEditActive; }
    void archiveOrder(const std::string& orderID);

    Bill checkoutAndGenerateBill(const std::string& paymentMode,
        double amountTendered,
        const std::string& merchant = "");

    const Order& getCurrentOrder() const { return currentOrder; }
    double getSubtotal() const;
    double getTotalTax() const;
    double getTotalDiscount() const;
    double getTotalPayable() const;
};

#endif
