#include "ReportsScreen.h"
#include "../Backend/Billing/BillCalc3.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QDir>
#include <QColor>
#include <QBrush>
#include <stdexcept>

ReportsScreen::ReportsScreen(const QString& dataDirPath, QWidget* parent)
    : QWidget(parent),
    m_reportService(
        QDir(dataDirPath).filePath("Bill.txt").toStdString(),
        QDir(dataDirPath).filePath("BillDetail.txt").toStdString(),
        QDir(dataDirPath).filePath("Product.txt").toStdString())
{
    setupUi();
    refreshAllReports();
}

void ReportsScreen::setupUi()
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setSpacing(12);

    // ================= 1. SALES & DISCOUNT REPORT =================
    QGroupBox* salesGroup = new QGroupBox("1. Sales, Tax & Discount Report (Data/Bill.txt)", this);
    QVBoxLayout* salesLayout = new QVBoxLayout(salesGroup);

    QHBoxLayout* salesFilterRow = new QHBoxLayout();
    m_filterDateCheck = new QCheckBox("Filter by Date Range (DD-MM-YYYY):", this);
    m_filterDateCheck->setChecked(false);

    const QString today = QString::fromStdString(BillService::getCurrentDate());
    m_startDateInput = new QLineEdit(today, this);
    m_startDateInput->setPlaceholderText("Start DD-MM-YYYY");
    m_startDateInput->setMaxLength(10);
    m_startDateInput->setMaximumWidth(130);

    m_endDateInput = new QLineEdit(today, this);
    m_endDateInput->setPlaceholderText("End DD-MM-YYYY");
    m_endDateInput->setMaxLength(10);
    m_endDateInput->setMaximumWidth(130);

    m_genSalesBtn = new QPushButton("Generate Sales Report", this);
    m_exportSalesBtn = new QPushButton("Export Sales CSV", this);

    salesFilterRow->addWidget(m_filterDateCheck);
    salesFilterRow->addWidget(m_startDateInput);
    salesFilterRow->addWidget(new QLabel("to", this));
    salesFilterRow->addWidget(m_endDateInput);
    salesFilterRow->addWidget(m_genSalesBtn);
    salesFilterRow->addStretch();
    salesFilterRow->addWidget(m_exportSalesBtn);
    salesLayout->addLayout(salesFilterRow);

    m_salesSummaryLabel = new QLabel("Bills: 0 | Gross: PKR 0.00 | Tax: PKR 0.00 | Discounts: PKR 0.00 | Net Revenue: PKR 0.00", this);
    m_salesSummaryLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    salesLayout->addWidget(m_salesSummaryLabel);

    m_salesTable = new QTableWidget(0, 10, this);
    m_salesTable->setHorizontalHeaderLabels(
        QStringList() << "Bill ID" << "Order ID" << "Customer" << "Date" << "Items"
        << "Gross" << "Tax" << "Prod Disc" << "Season + Spec Disc" << "Net Payable");
    m_salesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_salesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_salesTable->verticalHeader()->setVisible(false);
    m_salesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    salesLayout->addWidget(m_salesTable, 1);

    root->addWidget(salesGroup, 1);

    // ================= 2. INVENTORY & LOW-STOCK REPORT =================
    QGroupBox* invGroup = new QGroupBox("2. Inventory Valuation & Low-Stock Alert Report (Data/Product.txt)", this);
    QVBoxLayout* invLayout = new QVBoxLayout(invGroup);

    QHBoxLayout* invFilterRow = new QHBoxLayout();
    invFilterRow->addWidget(new QLabel("Low-Stock Alert Threshold (<= units):", this));
    m_lowStockSpin = new QSpinBox(this);
    m_lowStockSpin->setRange(0, 10000);
    m_lowStockSpin->setValue(5);

    m_refreshInvBtn = new QPushButton("Refresh Inventory Report", this);
    m_exportInvBtn = new QPushButton("Export Inventory CSV", this);

    invFilterRow->addWidget(m_lowStockSpin);
    invFilterRow->addWidget(m_refreshInvBtn);
    invFilterRow->addStretch();
    invFilterRow->addWidget(m_exportInvBtn);
    invLayout->addLayout(invFilterRow);

    m_invSummaryLabel = new QLabel("SKUs: 0 | Total Units: 0 | Low-Stock Items: 0 | Stock Valuation: PKR 0.00", this);
    m_invSummaryLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    invLayout->addWidget(m_invSummaryLabel);

    m_invTable = new QTableWidget(0, 8, this);
    m_invTable->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Barcode" << "Name" << "Unit" << "Unit Price"
        << "In Stock" << "Stock Value" << "Stock Status");
    m_invTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_invTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_invTable->verticalHeader()->setVisible(false);
    m_invTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    invLayout->addWidget(m_invTable, 1);

    root->addWidget(invGroup, 1);

    connect(m_genSalesBtn, &QPushButton::clicked, this, &ReportsScreen::onGenerateSalesClicked);
    connect(m_exportSalesBtn, &QPushButton::clicked, this, &ReportsScreen::onExportSalesCsvClicked);
    connect(m_refreshInvBtn, &QPushButton::clicked, this, &ReportsScreen::onRefreshInventoryClicked);
    connect(m_exportInvBtn, &QPushButton::clicked, this, &ReportsScreen::onExportInventoryCsvClicked);
    connect(m_startDateInput, &QLineEdit::textEdited, this, [this]() { m_filterDateCheck->setChecked(true); });
    connect(m_endDateInput, &QLineEdit::textEdited, this, [this]() { m_filterDateCheck->setChecked(true); });
}

void ReportsScreen::refreshAllReports()
{
    onGenerateSalesClicked();
    onRefreshInventoryClicked();
}

void ReportsScreen::onGenerateSalesClicked()
{
    try
    {
        const bool byDate = m_filterDateCheck->isChecked();
        const std::string startStr = m_startDateInput->text().trimmed().toStdString();
        const std::string endStr = m_endDateInput->text().trimmed().toStdString();

        m_currentBills = m_reportService.getFilteredBills(startStr, endStr, byDate);
        m_currentSalesSummary = m_reportService.computeSalesSummary(m_currentBills);

        m_salesTable->setRowCount(0);
        for (const auto& b : m_currentBills)
        {
            const int r = m_salesTable->rowCount();
            m_salesTable->insertRow(r);

            int itemCount = 0;
            for (const auto& l : b.lines)
                itemCount += l.quantity;

            const double extraDisc = b.seasonalDiscount + b.specialDiscount;

            m_salesTable->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(b.billID)));
            m_salesTable->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(b.orderID)));
            m_salesTable->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(b.customerName)));
            m_salesTable->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(b.billDate)));
            m_salesTable->setItem(r, 4, new QTableWidgetItem(QString::number(itemCount)));
            m_salesTable->setItem(r, 5, new QTableWidgetItem("PKR " + QString::number(b.totalWithoutTax, 'f', 2)));
            m_salesTable->setItem(r, 6, new QTableWidgetItem("PKR " + QString::number(b.totalTax, 'f', 2)));
            m_salesTable->setItem(r, 7, new QTableWidgetItem("PKR " + QString::number(b.totalDiscount, 'f', 2)));
            m_salesTable->setItem(r, 8, new QTableWidgetItem("PKR " + QString::number(extraDisc, 'f', 2)));
            m_salesTable->setItem(r, 9, new QTableWidgetItem("PKR " + QString::number(b.totalPayable, 'f', 2)));
        }

        const double totalAllDiscounts = m_currentSalesSummary.totalProductDiscount +
            m_currentSalesSummary.totalSeasonalDiscount +
            m_currentSalesSummary.totalSpecialDiscount;

        m_salesSummaryLabel->setText(
            QString("Bills: %1 | Items Sold: %2 | Gross: PKR %3 | Tax: PKR %4 | Total Discounts: PKR %5 | Net Revenue: PKR %6")
            .arg(m_currentSalesSummary.billCount)
            .arg(m_currentSalesSummary.totalItemsSold)
            .arg(QString::number(m_currentSalesSummary.totalGrossWithoutTax, 'f', 2))
            .arg(QString::number(m_currentSalesSummary.totalTax, 'f', 2))
            .arg(QString::number(totalAllDiscounts, 'f', 2))
            .arg(QString::number(m_currentSalesSummary.totalNetRevenue, 'f', 2)));
    }
    catch (const std::exception& e)
    {
        QMessageBox::warning(this, "Sales Report Error", QString::fromUtf8(e.what()));
    }
}

void ReportsScreen::onExportSalesCsvClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Export Sales Report CSV", "sales_report.csv", "CSV Files (*.csv)");
    if (path.isEmpty())
        return;

    if (m_reportService.exportSalesCsv(path.toStdString(), m_currentBills, m_currentSalesSummary))
        QMessageBox::information(this, "Export Complete", "Sales report exported to:\n" + path);
    else
        QMessageBox::warning(this, "Export Failed", "Could not write CSV file.");
}

void ReportsScreen::onRefreshInventoryClicked()
{
    const int threshold = m_lowStockSpin->value();
    m_currentProducts = m_reportService.getAllProducts();
    m_currentInvSummary = m_reportService.computeInventorySummary(m_currentProducts, threshold);

    m_invTable->setRowCount(0);
    for (const auto& p : m_currentProducts)
    {
        const int r = m_invTable->rowCount();
        m_invTable->insertRow(r);

        const double stockValue = p.unitPrice * p.quantity;
        const bool isOut = (p.quantity == 0);
        const bool isLow = (p.quantity <= threshold);
        const QString statusText = isOut ? "OUT OF STOCK" : (isLow ? "LOW STOCK" : "Healthy");

        m_invTable->setItem(r, 0, new QTableWidgetItem(QString::fromStdString(p.productID)));
        m_invTable->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(p.barcode)));
        m_invTable->setItem(r, 2, new QTableWidgetItem(QString::fromStdString(p.name)));
        m_invTable->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(p.unit)));
        m_invTable->setItem(r, 4, new QTableWidgetItem("PKR " + QString::number(p.unitPrice, 'f', 2)));
        m_invTable->setItem(r, 5, new QTableWidgetItem(QString::number(p.quantity)));
        m_invTable->setItem(r, 6, new QTableWidgetItem("PKR " + QString::number(stockValue, 'f', 2)));

        QTableWidgetItem* statusItem = new QTableWidgetItem(statusText);
        if (isOut)
            statusItem->setForeground(QBrush(QColor("#B5382A")));
        else if (isLow)
            statusItem->setForeground(QBrush(QColor("#C6613F")));
        else
            statusItem->setForeground(QBrush(QColor("#2E7D32")));
        m_invTable->setItem(r, 7, statusItem);
    }

    m_invSummaryLabel->setText(
        QString("Total SKUs: %1 | Total Units in Stock: %2 | Low-Stock Alerts: %3 | Total Inventory Value: PKR %4")
        .arg(m_currentInvSummary.totalSkus)
        .arg(m_currentInvSummary.totalUnitsInStock)
        .arg(m_currentInvSummary.lowStockCount)
        .arg(QString::number(m_currentInvSummary.totalStockValuation, 'f', 2)));
}

void ReportsScreen::onExportInventoryCsvClicked()
{
    const QString path = QFileDialog::getSaveFileName(
        this, "Export Inventory Report CSV", "inventory_report.csv", "CSV Files (*.csv)");
    if (path.isEmpty())
        return;

    if (m_reportService.exportInventoryCsv(
        path.toStdString(), m_currentProducts, m_lowStockSpin->value(), m_currentInvSummary))
        QMessageBox::information(this, "Export Complete", "Inventory report exported to:\n" + path);
    else
        QMessageBox::warning(this, "Export Failed", "Could not write CSV file.");
}