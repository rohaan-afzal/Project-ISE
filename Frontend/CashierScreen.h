#pragma once
#ifndef CASHIERSCREEN_H
#define CASHIERSCREEN_H

#include <QWidget>
#include <QString>
#include <memory>
#include "../Backend/Product/ProductLookupController.h"

class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class CameraWidget;
class ProductService;
class OrderService;
class ProductDescriptionView;
class DescriptionButton;

class CashierScreen : public QWidget
{
    Q_OBJECT

public:
    explicit CashierScreen(ProductService* productService,
                           OrderService* orderService,
                           QWidget* parent = nullptr);

    void stopCamera();
    void refreshHeldOrders();

private slots:
    void onBarcodeScanned(const QString& barcode);
    void onManualAddClicked();
    void onDescriptionClicked();
    void onUpdateQtyClicked();
    void onRemoveItemClicked();
    void onClearOrderClicked();
    void onHoldOrderClicked();
    void onResumeOrderClicked();
    void onEditPaidOrderClicked();
    void onPaymentModeChanged(const QString& mode);
    void onCheckoutClicked();

private:
    ProductService* m_productService;
    OrderService* m_orderService;

    CameraWidget* m_cameraWidget;
    QLineEdit* m_barcodeInput;
    QSpinBox* m_addQtySpin;
    QPushButton* m_addBtn;
    DescriptionButton* m_descriptionBtn;

    QLineEdit* m_customerNameInput;
    QLineEdit* m_customerPhoneInput;
    QLabel* m_orderIdLabel;

    QTableWidget* m_cartTable;
    QPushButton* m_updateQtyBtn;
    QPushButton* m_removeItemBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_holdBtn;
    QComboBox* m_heldOrdersCombo;
    QPushButton* m_resumeBtn;
    QComboBox* m_paidOrdersCombo;
    QPushButton* m_editPaidBtn;

    QLabel* m_subtotalLabel;
    QLabel* m_taxLabel;
    QLabel* m_discountLabel;
    QLabel* m_payableLabel;

    QComboBox* m_paymentModeCombo;
    QComboBox* m_merchantCombo;
    QDoubleSpinBox* m_tenderedSpin;
    QPushButton* m_checkoutBtn;
    QLabel* m_statusLabel;
    ProductDescriptionView* m_descriptionView;
    std::unique_ptr<ProductLookupController> m_lookupController;

    void setupUi();
    void refreshOrderTable();
    void refreshPaidOrders();
    void setStatusMessage(const QString& text, bool isError = false);
    QString selectedCartBarcode() const;
};

#endif
