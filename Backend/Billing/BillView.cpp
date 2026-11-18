#include "BillView.h"
#include "BillCalc.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QFont>
using namespace std;

QString BillView::formatMoney(double value)
{
    return "PKR " + QString::number(value, 'f', 2);
}

QLabel* BillView::createValueLabel(const QString& objectName)
{
    QLabel* label = new QLabel("-");
    label->setObjectName(objectName);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

BillView::BillView(QWidget* parent) : QWidget(parent)
{
    buildLayout();
}

void BillView::buildLayout()
{
    billIDLabel = createValueLabel("billIDLabel");
    customerLabel = createValueLabel("customerLabel");
    orderIDLabel = createValueLabel("orderIDLabel");
    statusLabel = createValueLabel("statusLabel");
    totalWithoutTaxLabel = createValueLabel("totalWithoutTaxLabel");
    totalTaxLabel = createValueLabel("totalTaxLabel");
    totalDiscountLabel = createValueLabel("totalDiscountLabel");
    totalPayableLabel = createValueLabel("totalPayableLabel");

    QFont boldFont = totalPayableLabel->font();
    boldFont.setBold(true);
    boldFont.setPointSize(boldFont.pointSize() + 2);
    totalPayableLabel->setFont(boldFont);

    QFormLayout* headerLayout = new QFormLayout();
    headerLayout->addRow("Bill ID:", billIDLabel);
    headerLayout->addRow("Customer:", customerLabel);
    headerLayout->addRow("Order ID:", orderIDLabel);
    headerLayout->addRow("Payment status:", statusLabel);

    table = new QTableWidget(0, 6);
    table->setObjectName("billTable");
    table->setHorizontalHeaderLabels(
        QStringList() << "Product" << "Unit price" << "Discount" << "Tax" << "Qty" << "Line total");
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QFormLayout* totalsLayout = new QFormLayout();
    totalsLayout->addRow("Total without tax:", totalWithoutTaxLabel);
    totalsLayout->addRow("Total tax:", totalTaxLabel);
    totalsLayout->addRow("Total discount:", totalDiscountLabel);
    totalsLayout->addRow("Total payable:", totalPayableLabel);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(headerLayout);
    mainLayout->addWidget(table);
    mainLayout->addLayout(totalsLayout);
}

void BillView::addLineToTable(const BillLine& line)
{
    int row = table->rowCount();
    table->insertRow(row);

    table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(line.productAcronym)));
    table->setItem(row, 1, new QTableWidgetItem(formatMoney(line.unitPrice)));
    table->setItem(row, 2, new QTableWidgetItem(formatMoney(line.discount)));
    table->setItem(row, 3, new QTableWidgetItem(formatMoney(line.tax)));
    table->setItem(row, 4, new QTableWidgetItem(QString::number(line.quantity)));
    table->setItem(row, 5, new QTableWidgetItem(formatMoney(PricingService::getLineTotal(line))));

    for (int column = 1; column < 6; column++)
    {
        table->item(row, column)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
}

void BillView::showBill(const Bill& bill)
{
    clear();

    billIDLabel->setText(QString::fromStdString(bill.billID));
    customerLabel->setText(QString::fromStdString(bill.customerName));
    orderIDLabel->setText(QString::fromStdString(bill.orderID));
    statusLabel->setText(QString::fromStdString(bill.paymentStatus));

    for (int i = 0; i < (int)bill.lines.size(); i++)
    {
        addLineToTable(bill.lines[i]);
    }

    totalWithoutTaxLabel->setText(formatMoney(bill.totalWithoutTax));
    totalTaxLabel->setText(formatMoney(bill.totalTax));
    totalDiscountLabel->setText(formatMoney(bill.totalDiscount));
    totalPayableLabel->setText(formatMoney(bill.totalPayable));
}

void BillView::clear()
{
    billIDLabel->setText("-");
    customerLabel->setText("-");
    orderIDLabel->setText("-");
    statusLabel->setText("-");
    table->setRowCount(0);
    totalWithoutTaxLabel->setText("-");
    totalTaxLabel->setText("-");
    totalDiscountLabel->setText("-");
    totalPayableLabel->setText("-");
}