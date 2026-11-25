#ifndef BILLVIEW3_H
#define BILLVIEW3_H

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QTableWidget>
#include "Bill3.h"

class BillView : public QWidget
{
private:
    QLabel* billIDLabel;
    QLabel* customerLabel;
    QLabel* orderIDLabel;
    QLabel* statusLabel;
    QLabel* dateTimeLabel;
    QTableWidget* table;
    QLabel* totalWithoutTaxLabel;
    QLabel* totalTaxLabel;
    QLabel* totalDiscountLabel;
    QLabel* seasonalDiscountLabel;
    QLabel* specialDiscountLabel;
    QLabel* totalPayableLabel;
    QLabel* amountReceivedLabel;
    QLabel* balanceReturnedLabel;

    static QString formatMoney(double value);
    static QLabel* createValueLabel(const QString& objectName);
    void buildLayout();
    void addLineToTable(const BillLine& line);

public:
    explicit BillView(QWidget* parent = nullptr);

    void showBill(const Bill& bill);
    void clear();
};

#endif