#include "EditOrderDialog.h"
#include "BillDialog.h"
#include "../Backend/Order/OrderService.h"
#include "../Backend/Product/ProductService.h"
#include "../Backend/Billing/BillCalc3.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <stdexcept>

EditOrderDialog::EditOrderDialog(OrderService* orderService,
    ProductService* productService,
    QWidget* parent)
    : QDialog(parent),
    m_orderService(orderService),
    m_productService(productService)
{
    setWindowTitle("Edit Paid Order (Within 7 Days - Manager PIN Required)");
    resize(760, 520);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QHBoxLayout* selectRow = new QHBoxLayout();
    selectRow->addWidget(new QLabel("Select Paid Order:", this));
    m_orderCombo = new QComboBox(this);
    selectRow->addWidget(m_orderCombo, 1);
    mainLayout->addLayout(selectRow);

    m_orderMetaLabel = new QLabel("Select an order above to edit its items.", this);
    mainLayout->addWidget(m_orderMetaLabel);

    m_itemsTable = new QTableWidget(0, 5, this);
    m_itemsTable->setHorizontalHeaderLabels(
        QStringList() << "Barcode" << "Product" << "Unit Price" << "Qty" << "Line Total");
    m_itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_itemsTable->verticalHeader()->setVisible(false);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mainLayout->addWidget(m_itemsTable, 1);

    QHBoxLayout* editControls = new QHBoxLayout();
    m_barcodeInput = new QLineEdit(this);
    m_barcodeInput->setPlaceholderText("6-digit barcode");
    m_barcodeInput->setMaxLength(6);

    m_qtySpin = new QSpinBox(this);
    m_qtySpin->setRange(1, 999);
    m_qtySpin->setValue(1);
    m_qtySpin->setPrefix("Qty: ");

    QPushButton* addBtn = new QPushButton("Add / Increase Item", this);
    QPushButton* setQtyBtn = new QPushButton("Set Selected Qty", this);
    QPushButton* removeBtn = new QPushButton("Remove Selected", this);

    editControls->addWidget(m_barcodeInput, 1);
    editControls->addWidget(m_qtySpin);
    editControls->addWidget(addBtn);
    editControls->addWidget(setQtyBtn);
    editControls->addWidget(removeBtn);
    mainLayout->addLayout(editControls);

    QFormLayout* authForm = new QFormLayout();
    m_specialDiscSpin = new QDoubleSpinBox(this);
    m_specialDiscSpin->setRange(0.0, 8.0);
    m_specialDiscSpin->setDecimals(2);
    m_specialDiscSpin->setSuffix(" %");

    m_pinInput = new QLineEdit(this);
    m_pinInput->setEchoMode(QLineEdit::Password);
    m_pinInput->setPlaceholderText("Enter Manager PIN (default: 1234)");

    m_totalLabel = new QLabel("PKR 0.00", this);

    authForm->addRow("Special Discount (<=8%):", m_specialDiscSpin);
    authForm->addRow("Manager PIN:", m_pinInput);
    authForm->addRow("Revised Base Total:", m_totalLabel);
    mainLayout->addLayout(authForm);

    QHBoxLayout* footer = new QHBoxLayout();
    QPushButton* saveBtn = new QPushButton("Authorize && Save Revised Order", this);
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    footer->addWidget(saveBtn);
    footer->addStretch();
    footer->addWidget(cancelBtn);
    mainLayout->addLayout(footer);

    connect(m_orderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &EditOrderDialog::onOrderSelected);
    connect(addBtn, &QPushButton::clicked, this, &EditOrderDialog::onAddItemClicked);
    connect(setQtyBtn, &QPushButton::clicked, this, &EditOrderDialog::onUpdateQtyClicked);
    connect(removeBtn, &QPushButton::clicked, this, &EditOrderDialog::onRemoveItemClicked);
    connect(saveBtn, &QPushButton::clicked, this, &EditOrderDialog::onSaveEditedOrderClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    loadEligibleOrders();
}

void EditOrderDialog::loadEligibleOrders()
{
    m_orderCombo->clear();
    const std::vector<Order> eligible = m_orderService->getEditablePaidOrders();
    for (const auto& ord : eligible)
    {
        const QString id = QString::fromStdString(ord.orderID);
        const QString label = QString("%1 | Date: %2 | Customer: %3 | Paid: PKR %4")
            .arg(id,
                QString::fromStdString(ord.date),
                QString::fromStdString(ord.customerName),
                QString::number(ord.payment.amount, 'f', 2));
        m_orderCombo->addItem(label, id);
    }

    if (m_orderCombo->count() == 0)
        m_orderMetaLabel->setText("No paid orders found within the last 7 days.");
}

void EditOrderDialog::onOrderSelected(int index)
{
    m_workingItems.clear();
    if (index < 0)
    {
        refreshItemsTable();
        return;
    }

    const std::string orderID = m_orderCombo->itemData(index).toString().toStdString();
    try
    {
        const Order fullOrder = m_orderService->getOrderWithDetails(orderID);
        m_workingItems = fullOrder.items;
        m_orderMetaLabel->setText(QString("Editing %1 (Customer: %2 | Original Date: %3)")
            .arg(QString::fromStdString(fullOrder.orderID),
                QString::fromStdString(fullOrder.customerName),
                QString::fromStdString(fullOrder.date)));
        refreshItemsTable();
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, "Error", QString::fromUtf8(e.what()));
    }
}

void EditOrderDialog::onAddItemClicked()
{
    const std::string barcode = m_barcodeInput->text().trimmed().toStdString();
    if (barcode.size() != 6)
    {
        QMessageBox::information(this, "Invalid Barcode", "Enter a 6-digit product barcode.");
        return;
    }

    Product prod;
    if (!m_productService->getProductByBarcode(barcode, prod))
    {
        QMessageBox::warning(this, "Not Found", "Product not found for barcode: " + QString::fromStdString(barcode));
        return;
    }

    const int qty = m_qtySpin->value();
    for (auto& item : m_workingItems)
    {
        if (item.product.barcode == barcode)
        {
            item.quantity += qty;
            m_barcodeInput->clear();
            refreshItemsTable();
            return;
        }
    }

    m_workingItems.emplace_back(prod, qty);
    m_barcodeInput->clear();
    refreshItemsTable();
}

void EditOrderDialog::onUpdateQtyClicked()
{
    const int row = m_itemsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_workingItems.size()))
        return;

    m_workingItems[row].quantity = m_qtySpin->value();
    refreshItemsTable();
}

void EditOrderDialog::onRemoveItemClicked()
{
    const int row = m_itemsTable->currentRow();
    if (row < 0 || row >= static_cast<int>(m_workingItems.size()))
        return;

    m_workingItems.erase(m_workingItems.begin() + row);
    refreshItemsTable();
}

void EditOrderDialog::refreshItemsTable()
{
    m_itemsTable->setRowCount(0);
    std::vector<BillLine> lines;

    for (const auto& item : m_workingItems)
    {
        const int r = m_itemsTable->rowCount();
        m_itemsTable->insertRow(r);

        const BillLine line(item.product, item.quantity);
        lines.push_back(line);

        m_itemsTable->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(item.product.barcode)));
        m_itemsTable->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(item.product.name)));
        m_itemsTable->setItem(r, 2, new QTableWidgetItem("PKR " + QString::number(item.product.unitPrice, 'f', 2)));
        m_itemsTable->setItem(r, 3, new QTableWidgetItem(QString::number(item.quantity)));
        m_itemsTable->setItem(r, 4, new QTableWidgetItem("PKR " + QString::number(PricingService::getLineAmount(line), 'f', 2)));
    }

    const double baseTotal = PricingService::getAmountAfterProductDiscount(lines);
    m_totalLabel->setText("PKR " + QString::number(baseTotal, 'f', 2));
}

void EditOrderDialog::onSaveEditedOrderClicked()
{
    if (m_orderCombo->currentIndex() < 0)
        return;

    const std::string orderID = m_orderCombo->currentData().toString().toStdString();
    const std::string pin = m_pinInput->text().toStdString();
    const double specialPct = m_specialDiscSpin->value();

    try
    {
        const Bill revisedBill = m_orderService->saveEditedPaidOrder(
            orderID, m_workingItems, pin, specialPct);

        QMessageBox::information(this, "Order Updated",
            "Order " + QString::fromStdString(orderID) + " updated, stock adjusted, and revised bill #" +
            QString::fromStdString(revisedBill.billID) + " generated.");

        Payment summary(revisedBill.totalPayable, "edited", "-");
        BillDialog receipt(revisedBill, summary, revisedBill.totalPayable, this);
        receipt.exec();
        accept();
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, "Edit Rejected", QString::fromUtf8(e.what()));
    }
}