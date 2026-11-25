#include "CashierScreen.h"
#include "CameraWidget.h"
#include "BillDialog.h"
#include "../Backend/Product/ProductService.h"
#include "../Backend/Product/ProductDescriptionView.h"
#include "../Backend/Product/ProductLookupController.h"
#include "DescriptionButton.h"
#include "../Backend/Order/OrderService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QInputDialog>
#include <QFont>
#include <algorithm>
#include <stdexcept>

CashierScreen::CashierScreen(ProductService* productService,
    OrderService* orderService,
    QWidget* parent)
    : QWidget(parent),
    m_productService(productService),
    m_orderService(orderService)
{
    setupUi();
    refreshOrderTable();
    refreshHeldOrders();
    refreshPaidOrders();
}

void CashierScreen::setupUi()
{
    QHBoxLayout* rootLayout = new QHBoxLayout(this);

    // Left column: Scanner + Product Description
    QVBoxLayout* leftCol = new QVBoxLayout();

    QGroupBox* scanGroup = new QGroupBox("1. Scan or Enter Product Barcode", this);
    QVBoxLayout* scanLayout = new QVBoxLayout(scanGroup);

    m_cameraWidget = new CameraWidget(this);
    scanLayout->addWidget(m_cameraWidget);

    QHBoxLayout* manualRow = new QHBoxLayout();
    m_barcodeInput = new QLineEdit(this);
    m_barcodeInput->setPlaceholderText("6-digit barcode (e.g. 123456)");
    m_barcodeInput->setMaxLength(6);
    m_barcodeInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,5}"), m_barcodeInput));

    m_addQtySpin = new QSpinBox(this);
    m_addQtySpin->setRange(1, 999);
    m_addQtySpin->setValue(1);
    m_addQtySpin->setPrefix("Qty: ");

    m_addBtn = new QPushButton("Add Item", this);
    m_descriptionBtn = new DescriptionButton(this);

    manualRow->addWidget(m_barcodeInput, 1);
    manualRow->addWidget(m_addQtySpin);
    manualRow->addWidget(m_addBtn);
    manualRow->addWidget(m_descriptionBtn);
    scanLayout->addLayout(manualRow);

    leftCol->addWidget(scanGroup);

    QGroupBox* descGroup = new QGroupBox("Product Specifications", this);
    QVBoxLayout* descLayout = new QVBoxLayout(descGroup);
    m_descriptionView = new ProductDescriptionView(this);
    m_lookupController = std::make_unique<ProductLookupController>(m_productService, m_descriptionView);
    descLayout->addWidget(m_descriptionView);
    leftCol->addWidget(descGroup, 1);

    rootLayout->addLayout(leftCol, 4);

    // Right column: Customer, Cart Table, Held Orders, Payment & Checkout
    QVBoxLayout* rightCol = new QVBoxLayout();

    QGroupBox* customerGroup = new QGroupBox("2. Order & Customer Details", this);
    QHBoxLayout* customerRow = new QHBoxLayout(customerGroup);

    m_orderIdLabel = new QLabel("Order: -", this);
    QFont boldFont = m_orderIdLabel->font();
    boldFont.setBold(true);
    m_orderIdLabel->setFont(boldFont);

    m_customerNameInput = new QLineEdit(this);
    m_customerNameInput->setPlaceholderText("Customer (max 4 letters)");
    m_customerNameInput->setMaxLength(4);
    m_customerNameInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[A-Za-z]{0,4}"), m_customerNameInput));

    m_customerPhoneInput = new QLineEdit(this);
    m_customerPhoneInput->setPlaceholderText("Phone (max 11 digits)");
    m_customerPhoneInput->setMaxLength(11);
    m_customerPhoneInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[0-9]{0,11}"), m_customerPhoneInput));

    customerRow->addWidget(m_orderIdLabel);
    customerRow->addWidget(m_customerNameInput, 1);
    customerRow->addWidget(m_customerPhoneInput, 1);
    rightCol->addWidget(customerGroup);

    QGroupBox* cartGroup = new QGroupBox("3. Current Order Items", this);
    QVBoxLayout* cartLayout = new QVBoxLayout(cartGroup);

    m_cartTable = new QTableWidget(0, 7, this);
    m_cartTable->setHorizontalHeaderLabels(
        QStringList() << "Barcode" << "Product" << "Unit Price" << "Tax" << "Discount" << "Qty" << "Line Total");
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->verticalHeader()->setVisible(false);
    m_cartTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    cartLayout->addWidget(m_cartTable, 1);

    QHBoxLayout* cartActions = new QHBoxLayout();
    m_updateQtyBtn = new QPushButton("Set Qty to Spinbox", this);
    m_removeItemBtn = new QPushButton("Remove Selected", this);
    m_clearBtn = new QPushButton("Clear Order", this);
    m_holdBtn = new QPushButton("Hold Order", this);
    cartActions->addWidget(m_updateQtyBtn);
    cartActions->addWidget(m_removeItemBtn);
    cartActions->addWidget(m_clearBtn);
    cartActions->addWidget(m_holdBtn);
    cartLayout->addLayout(cartActions);

    QHBoxLayout* heldRow = new QHBoxLayout();
    heldRow->addWidget(new QLabel("Held Orders:", this));
    m_heldOrdersCombo = new QComboBox(this);
    m_resumeBtn = new QPushButton("Resume Held Order", this);
    heldRow->addWidget(m_heldOrdersCombo, 1);
    heldRow->addWidget(m_resumeBtn);
    cartLayout->addLayout(heldRow);

    QHBoxLayout* paidEditRow = new QHBoxLayout();
    paidEditRow->addWidget(new QLabel("Paid orders (7 days):", this));
    m_paidOrdersCombo = new QComboBox(this);
    m_editPaidBtn = new QPushButton("Edit with Manager Approval", this);
    paidEditRow->addWidget(m_paidOrdersCombo, 1);
    paidEditRow->addWidget(m_editPaidBtn);
    cartLayout->addLayout(paidEditRow);

    rightCol->addWidget(cartGroup, 2);

    QGroupBox* checkoutGroup = new QGroupBox("4. Totals & Payment", this);
    QHBoxLayout* checkoutSplit = new QHBoxLayout(checkoutGroup);

    QFormLayout* totalsForm = new QFormLayout();
    m_subtotalLabel = new QLabel("PKR 0.00", this);
    m_taxLabel = new QLabel("PKR 0.00", this);
    m_discountLabel = new QLabel("PKR 0.00", this);
    m_payableLabel = new QLabel("PKR 0.00", this);
    QFont payFont = m_payableLabel->font();
    payFont.setBold(true);
    payFont.setPointSize(payFont.pointSize() + 2);
    m_payableLabel->setFont(payFont);

    totalsForm->addRow("Subtotal:", m_subtotalLabel);
    totalsForm->addRow("Total Tax:", m_taxLabel);
    totalsForm->addRow("Total Discount:", m_discountLabel);
    totalsForm->addRow("Total Payable:", m_payableLabel);
    checkoutSplit->addLayout(totalsForm, 1);

    QFormLayout* paymentForm = new QFormLayout();
    m_paymentModeCombo = new QComboBox(this);
    m_paymentModeCombo->addItems(QStringList() << "cash" << "card" << "wallet");

    m_merchantCombo = new QComboBox(this);

    m_tenderedSpin = new QDoubleSpinBox(this);
    m_tenderedSpin->setRange(0.0, 10000000.0);
    m_tenderedSpin->setDecimals(2);
    m_tenderedSpin->setPrefix("PKR ");

    m_checkoutBtn = new QPushButton("Complete Payment && Generate Bill", this);

    paymentForm->addRow("Payment Mode:", m_paymentModeCombo);
    paymentForm->addRow("Merchant:", m_merchantCombo);
    paymentForm->addRow("Amount Tendered:", m_tenderedSpin);
    paymentForm->addRow("", m_checkoutBtn);
    checkoutSplit->addLayout(paymentForm, 1);

    rightCol->addWidget(checkoutGroup);

    m_statusLabel = new QLabel("Ready to record order.", this);
    m_statusLabel->setWordWrap(true);
    rightCol->addWidget(m_statusLabel);

    rootLayout->addLayout(rightCol, 6);

    connect(m_cameraWidget, &CameraWidget::barcodeScanned, this, &CashierScreen::onBarcodeScanned);
    connect(m_addBtn, &QPushButton::clicked, this, &CashierScreen::onManualAddClicked);
    connect(m_barcodeInput, &QLineEdit::returnPressed, this, &CashierScreen::onManualAddClicked);
    connect(m_descriptionBtn, &QPushButton::clicked, this, &CashierScreen::onDescriptionClicked);
    connect(m_updateQtyBtn, &QPushButton::clicked, this, &CashierScreen::onUpdateQtyClicked);
    connect(m_removeItemBtn, &QPushButton::clicked, this, &CashierScreen::onRemoveItemClicked);
    connect(m_clearBtn, &QPushButton::clicked, this, &CashierScreen::onClearOrderClicked);
    connect(m_holdBtn, &QPushButton::clicked, this, &CashierScreen::onHoldOrderClicked);
    connect(m_resumeBtn, &QPushButton::clicked, this, &CashierScreen::onResumeOrderClicked);
    connect(m_editPaidBtn, &QPushButton::clicked, this, &CashierScreen::onEditPaidOrderClicked);
    connect(m_paymentModeCombo, &QComboBox::currentTextChanged, this, &CashierScreen::onPaymentModeChanged);
    connect(m_checkoutBtn, &QPushButton::clicked, this, &CashierScreen::onCheckoutClicked);

    onPaymentModeChanged(m_paymentModeCombo->currentText());
}

void CashierScreen::stopCamera()
{
    if (m_cameraWidget)
        m_cameraWidget->stopScanning();
}

void CashierScreen::setStatusMessage(const QString& text, bool isError)
{
    m_statusLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
    m_statusLabel->setText(text);
}

QString CashierScreen::selectedCartBarcode() const
{
    const int row = m_cartTable->currentRow();
    if (row < 0 || !m_cartTable->item(row, 0))
        return QString();
    return m_cartTable->item(row, 0)->text();
}

void CashierScreen::onBarcodeScanned(const QString& barcode)
{
    m_barcodeInput->setText(barcode);
    m_lookupController->showProductDescription(barcode.toStdString());
    try
    {
        m_orderService->addProductByBarcode(barcode.toStdString(), 1);
        refreshOrderTable();
        setStatusMessage("Added scanned product: " + barcode);
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void CashierScreen::onManualAddClicked()
{
    const QString barcode = m_barcodeInput->text().trimmed();
    if (barcode.isEmpty())
    {
        setStatusMessage("Please enter or scan a 6-digit barcode.", true);
        return;
    }

    m_lookupController->showProductDescription(barcode.toStdString());
    try
    {
        m_orderService->addProductByBarcode(barcode.toStdString(), m_addQtySpin->value());
        refreshOrderTable();
        setStatusMessage("Added " + QString::number(m_addQtySpin->value()) + "x barcode " + barcode + ".");
        m_barcodeInput->clear();
        m_addQtySpin->setValue(1);
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void CashierScreen::onDescriptionClicked()
{
    QString barcode = m_barcodeInput->text().trimmed();
    if (barcode.isEmpty())
        barcode = selectedCartBarcode();

    if (barcode.isEmpty())
    {
        setStatusMessage("Enter a barcode or select an item in the cart first.", true);
        return;
    }

    m_lookupController->showProductDescription(barcode.toStdString());
    Product prod;
    if (m_productService->getProductByBarcode(barcode.toStdString(), prod))
        setStatusMessage("Showing description for " + QString::fromStdString(prod.name) + ".");
    else
        setStatusMessage("Product not found for barcode: " + barcode, true);
}

void CashierScreen::onUpdateQtyClicked()
{
    const QString barcode = selectedCartBarcode();
    if (barcode.isEmpty())
    {
        setStatusMessage("Select an item row in the order table first.", true);
        return;
    }

    try
    {
        m_orderService->updateItemQuantity(barcode.toStdString(), m_addQtySpin->value());
        refreshOrderTable();
        setStatusMessage("Updated quantity for " + barcode + " to " + QString::number(m_addQtySpin->value()) + ".");
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void CashierScreen::onRemoveItemClicked()
{
    const QString barcode = selectedCartBarcode();
    if (barcode.isEmpty())
    {
        setStatusMessage("Select an item row to remove.", true);
        return;
    }

    m_orderService->removeItem(barcode.toStdString());
    refreshOrderTable();
    setStatusMessage("Removed item " + barcode + " from order.");
}

void CashierScreen::onClearOrderClicked()
{
    m_orderService->clearOrder();
    m_checkoutBtn->setText("Complete Payment && Generate Bill");
    m_holdBtn->setEnabled(true);
    m_customerNameInput->clear();
    m_customerPhoneInput->clear();
    refreshOrderTable();
    setStatusMessage("Order cleared.");
}

void CashierScreen::onHoldOrderClicked()
{
    try
    {
        const QString custName = m_customerNameInput->text().trimmed().isEmpty()
            ? "Walk"
            : m_customerNameInput->text().trimmed();
        m_orderService->setCustomerDetails(
            custName.toStdString(),
            m_customerPhoneInput->text().trimmed().toStdString());

        const std::string heldId = m_orderService->getCurrentOrder().orderID;
        m_orderService->holdCurrentOrder();

        m_customerNameInput->clear();
        m_customerPhoneInput->clear();
        refreshOrderTable();
        refreshHeldOrders();
        refreshPaidOrders();
        setStatusMessage("Order " + QString::fromStdString(heldId) + " placed on hold.");
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void CashierScreen::onResumeOrderClicked()
{
    if (m_heldOrdersCombo->count() == 0)
    {
        setStatusMessage("No held orders available to resume.", true);
        return;
    }

    const QString orderId = m_heldOrdersCombo->currentData().toString();
    try
    {
        m_orderService->resumeOrder(orderId.toStdString());
        const Order& current = m_orderService->getCurrentOrder();
        m_customerNameInput->setText(QString::fromStdString(current.customerName).left(4));
        m_customerPhoneInput->setText(current.customerPhone == "-" ? "" : QString::fromStdString(current.customerPhone).left(11));
        refreshOrderTable();
        setStatusMessage("Resumed order " + orderId + ".");
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void CashierScreen::onEditPaidOrderClicked()
{
    if (m_paidOrdersCombo->count() == 0)
    {
        setStatusMessage("There are no paid orders eligible for editing in the last seven days.", true);
        return;
    }

    const QString orderID = m_paidOrdersCombo->currentData().toString();
    if (QMessageBox::question(this, "Manager approval required",
            "A manager must approve editing paid order " + orderID + ". Continue with approval?")
        != QMessageBox::Yes)
        return;

    bool accepted = false;
    const QString managerName = QInputDialog::getText(
        this, "Manager approval", "Manager name approving this paid-order edit:",
        QLineEdit::Normal, QString(), &accepted).trimmed();
    if (!accepted) return;
    if (managerName.isEmpty())
    {
        setStatusMessage("Manager approval requires a name.", true);
        return;
    }

    try
    {
        m_orderService->beginPaidOrderEdit(orderID.toStdString(), managerName.toStdString(), true);
        const Order& order = m_orderService->getCurrentOrder();
        m_customerNameInput->setText(QString::fromStdString(order.customerName).left(4));
        m_customerPhoneInput->setText(order.customerPhone == "-" ? "" :
            QString::fromStdString(order.customerPhone).left(11));
        m_checkoutBtn->setText("Save Paid Edit && Adjust Payment");
        m_holdBtn->setEnabled(false);
        refreshOrderTable();
        setStatusMessage("Editing paid order " + orderID + ". Manager approval by " + managerName + " recorded.");
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void CashierScreen::refreshHeldOrders()
{
    m_heldOrdersCombo->clear();
    const std::vector<Order> held = m_orderService->getHeldOrders();
    for (const auto& ord : held)
    {
        const QString id = QString::fromStdString(ord.orderID);
        const QString label = id + " - " + QString::fromStdString(ord.customerName) +
            " (" + QString::fromStdString(ord.time) + ")";
        m_heldOrdersCombo->addItem(label, id);
    }
}

void CashierScreen::refreshPaidOrders()
{
    m_paidOrdersCombo->clear();
    for (const auto& order : m_orderService->getEditablePaidOrders())
    {
        const QString id = QString::fromStdString(order.orderID);
        m_paidOrdersCombo->addItem(id + " - " + QString::fromStdString(order.customerName) +
                                   " (paid " + QString::fromStdString(order.date) + ")", id);
    }
}

void CashierScreen::onPaymentModeChanged(const QString& mode)
{
    m_merchantCombo->clear();
    if (mode == "cash")
    {
        m_merchantCombo->addItem("Cash Counter");
        m_merchantCombo->setEnabled(false);
        m_tenderedSpin->setEnabled(true);
    }
    else if (mode == "card")
    {
        m_merchantCombo->addItems(QStringList() << "HBL" << "Meezan Bank" << "Bank Alfalah" << "UBL" << "MCB" << "Allied Bank");
        m_merchantCombo->setEnabled(true);
        const Order& order = m_orderService->getCurrentOrder();
        m_tenderedSpin->setValue(std::max(0.0, m_orderService->getTotalPayable() -
            (m_orderService->isEditingPaidOrder() ? order.payment.amount : 0.0)));
    }
    else
    {
        m_merchantCombo->addItems(QStringList() << "JazzCash" << "EasyPaisa" << "SadaPay" << "NayaPay");
        m_merchantCombo->setEnabled(true);
        const Order& order = m_orderService->getCurrentOrder();
        m_tenderedSpin->setValue(std::max(0.0, m_orderService->getTotalPayable() -
            (m_orderService->isEditingPaidOrder() ? order.payment.amount : 0.0)));
    }
}

void CashierScreen::onCheckoutClicked()
{
    try
    {
        if (m_orderService->isEditingPaidOrder())
        {
            const double difference = m_orderService->getTotalPayable() -
                m_orderService->getCurrentOrder().payment.amount;
            if (difference < -0.005 && QMessageBox::question(
                    this, "Confirm refund",
                    "This edit reduces the paid total by PKR " + QString::number(-difference, 'f', 2) +
                    ". Confirm the refund?") != QMessageBox::Yes)
                return;
        }
        const QString custName = m_customerNameInput->text().trimmed().isEmpty()
            ? "Walk"
            : m_customerNameInput->text().trimmed();
        m_orderService->setCustomerDetails(
            custName.toStdString(),
            m_customerPhoneInput->text().trimmed().toStdString());

        const std::string mode = m_paymentModeCombo->currentText().toStdString();
        const std::string merchant = m_merchantCombo->currentText().toStdString();
        const double tendered = m_tenderedSpin->value();
        const bool editingPaidOrder = m_orderService->isEditingPaidOrder();
        const Payment oldPayment = m_orderService->getCurrentOrder().payment;
        const double previouslyPaid = oldPayment.amount;
        const double adjustment = m_orderService->getTotalPayable() - previouslyPaid;

        const Bill bill = m_orderService->checkoutAndGenerateBill(mode, tendered, merchant);
        const Payment paymentSummary = editingPaidOrder && adjustment < -0.005
            ? Payment(bill.totalPayable, oldPayment.mode, oldPayment.merchant)
            : Payment(bill.totalPayable, mode, merchant);

        const double receiptTendered = editingPaidOrder
            ? (tendered > 0.0 ? previouslyPaid + tendered : 0.0) : tendered;
        BillDialog dialog(bill, paymentSummary, receiptTendered, this);
        dialog.exec();

        m_customerNameInput->clear();
        m_customerPhoneInput->clear();
        m_tenderedSpin->setValue(0.0);
        refreshOrderTable();
        refreshHeldOrders();
        refreshPaidOrders();
        m_checkoutBtn->setText("Complete Payment && Generate Bill");
        m_holdBtn->setEnabled(true);
        setStatusMessage("Bill #" + QString::fromStdString(bill.billID) + " saved, paid, and stock updated.");
    }
    catch (const std::exception& e)
    {
        setStatusMessage(QString::fromUtf8(e.what()), true);
        QMessageBox::warning(this, "Checkout Error", QString::fromUtf8(e.what()));
    }
}

void CashierScreen::refreshOrderTable()
{
    const Order& order = m_orderService->getCurrentOrder();
    m_orderIdLabel->setText("Order: " + QString::fromStdString(order.orderID));

    m_cartTable->setRowCount(0);
    for (const auto& item : order.items)
    {
        const int row = m_cartTable->rowCount();
        m_cartTable->insertRow(row);

        const BillLine line(item.product, item.quantity);
        const double lineTotal = PricingService::getLineTotal(line);

        m_cartTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(item.product.barcode)));
        m_cartTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(item.product.name)));
        m_cartTable->setItem(row, 2, new QTableWidgetItem("PKR " + QString::number(item.product.unitPrice, 'f', 2)));
        m_cartTable->setItem(row, 3, new QTableWidgetItem("PKR " + QString::number(item.product.tax, 'f', 2)));
        m_cartTable->setItem(row, 4, new QTableWidgetItem("PKR " + QString::number(item.product.discount, 'f', 2)));
        m_cartTable->setItem(row, 5, new QTableWidgetItem(QString::number(item.quantity)));
        m_cartTable->setItem(row, 6, new QTableWidgetItem("PKR " + QString::number(lineTotal, 'f', 2)));
    }

    const double payable = m_orderService->getTotalPayable();
    m_subtotalLabel->setText("PKR " + QString::number(m_orderService->getSubtotal(), 'f', 2));
    m_taxLabel->setText("PKR " + QString::number(m_orderService->getTotalTax(), 'f', 2));
    m_discountLabel->setText("PKR " + QString::number(m_orderService->getTotalDiscount(), 'f', 2));
    m_payableLabel->setText("PKR " + QString::number(payable, 'f', 2));

    const double amountToCollect = m_orderService->isEditingPaidOrder()
        ? std::max(0.0, payable - order.payment.amount) : payable;
    m_tenderedSpin->setValue(amountToCollect);
}
