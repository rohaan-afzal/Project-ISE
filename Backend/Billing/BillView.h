#ifndef BILLVIEW_H
#define BILLVIEW_H

#include <QWidget>
#include <QString>
#include <QLabel>
#include <QTableWidget>
#include "Bill.h"

class BillView : public QWidget
{
private:
    QLabel* billIDLabel;
    QLabel* customerLabel;
    QLabel* orderIDLabel;
    QLabel* statusLabel;
    QTableWidget* table;
    QLabel* totalWithoutTaxLabel;
    QLabel* totalTaxLabel;
    QLabel* totalDiscountLabel;
    QLabel* totalPayableLabel;

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