#include "PdfBillGenerator3.h"
#include "BillCalc3.h"
#include <QFile>
#include <QPdfWriter>
#include <QPageSize>
#include <QTextDocument>
using namespace std;

QString PdfBillGenerator::formatMoney(double value)
{
    return "PKR " + QString::number(value, 'f', 2);
}

QString PdfBillGenerator::makeRow(const QString& label, double value)
{
    return "<tr><td>" + label + "</td><td align='right'>" + formatMoney(value) + "</td></tr>";
}

QString PdfBillGenerator::buildHtml(const Bill& bill) const
{
    QString html = "<h2>Bill</h2>";

    html += "<p>Bill ID: " + QString::fromStdString(bill.billID).toHtmlEscaped() + "<br>";
    html += "Customer: " + QString::fromStdString(bill.customerName).toHtmlEscaped() + "<br>";
    html += "Order ID: " + QString::fromStdString(bill.orderID).toHtmlEscaped() + "<br>";
    html += "Date: " + QString::fromStdString(bill.billDate).toHtmlEscaped() + "<br>";
    html += "Time: " + QString::fromStdString(bill.billTime).toHtmlEscaped() + "<br>";
    html += "Payment status: " + QString::fromStdString(bill.paymentStatus).toHtmlEscaped() + "</p>";

    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
    html += "<tr><th>Product</th><th>Unit price</th><th>Qty</th><th>Discount</th><th>Amount</th></tr>";

    for (int i = 0; i < (int)bill.lines.size(); i++)
    {
        const BillLine& line = bill.lines[i];

        html += "<tr><td>" + QString::fromStdString(line.productAcronym).toHtmlEscaped() + "</td>";
        html += "<td align='right'>" + formatMoney(line.unitPrice) + "</td>";
        html += "<td align='right'>" + QString::number(line.quantity) + "</td>";
        html += "<td align='right'>" + formatMoney(PricingService::getLineDiscount(line)) + "</td>";
        html += "<td align='right'>" + formatMoney(PricingService::getLineAmount(line)) + "</td></tr>";
    }

    html += "</table><br>";

    html += "<table cellspacing='0' cellpadding='4' width='60%'>";
    html += makeRow("Total without tax", bill.totalWithoutTax);
    html += makeRow("Total tax", bill.totalTax);
    html += makeRow("Product discounts", bill.totalDiscount);
    html += makeRow("Seasonal discount", bill.seasonalDiscount);
    html += makeRow("Special discount", bill.specialDiscount);
    html += "<tr><td><b>Total payable</b></td><td align='right'><b>" + formatMoney(bill.totalPayable) + "</b></td></tr>";
    html += makeRow("Amount received", bill.amountReceived);
    html += makeRow("Balance returned", bill.balanceReturned);
    html += "</table>";

    return html;
}

bool PdfBillGenerator::generate(const Bill& bill, const QString& filePath) const
{
    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));

    QTextDocument document;
    document.setHtml(buildHtml(bill));
    document.print(&writer);

    return QFile::exists(filePath);
}