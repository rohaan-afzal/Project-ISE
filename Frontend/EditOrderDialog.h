#pragma once
#ifndef EDITORDERDIALOG_H
#define EDITORDERDIALOG_H

#include <QDialog>
#include <vector>
#include "../Backend/Order/Order.h"

class OrderService;
class ProductService;
class QComboBox;
class QTableWidget;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QLabel;

class EditOrderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditOrderDialog(OrderService* orderService,
        ProductService* productService,
        QWidget* parent = nullptr);

private slots:
    void onOrderSelected(int index);
    void onAddItemClicked();
    void onUpdateQtyClicked();
    void onRemoveItemClicked();
    void onSaveEditedOrderClicked();

private:
    void loadEligibleOrders();
    void refreshItemsTable();

    OrderService* m_orderService;
    ProductService* m_productService;
    std::vector<OrderItem> m_workingItems;

    QComboBox* m_orderCombo = nullptr;
    QLabel* m_orderMetaLabel = nullptr;
    QTableWidget* m_itemsTable = nullptr;
    QLineEdit* m_barcodeInput = nullptr;
    QSpinBox* m_qtySpin = nullptr;
    QDoubleSpinBox* m_specialDiscSpin = nullptr;
    QLineEdit* m_pinInput = nullptr;
    QLabel* m_totalLabel = nullptr;
};

#endif