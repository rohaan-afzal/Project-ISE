#pragma once
#ifndef REPORTSSCREEN_H
#define REPORTSSCREEN_H

#include <QWidget>
#include <QString>
#include "../Backend/Reports/ReportService.h"

class QLineEdit;
class QCheckBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QTableWidget;

class ReportsScreen : public QWidget
{
    Q_OBJECT

public:
    explicit ReportsScreen(const QString& dataDirPath, QWidget* parent = nullptr);

public slots:
    void refreshAllReports();

private slots:
    void onGenerateSalesClicked();
    void onExportSalesCsvClicked();
    void onRefreshInventoryClicked();
    void onExportInventoryCsvClicked();

private:
    void setupUi();

    ReportService m_reportService;
    std::vector<Bill> m_currentBills;
    SalesSummary m_currentSalesSummary;
    std::vector<Product> m_currentProducts;
    InventorySummary m_currentInvSummary;

    // Sales Controls
    QCheckBox* m_filterDateCheck = nullptr;
    QLineEdit* m_startDateInput = nullptr;
    QLineEdit* m_endDateInput = nullptr;
    QPushButton* m_genSalesBtn = nullptr;
    QPushButton* m_exportSalesBtn = nullptr;
    QLabel* m_salesSummaryLabel = nullptr;
    QTableWidget* m_salesTable = nullptr;

    // Inventory Controls
    QSpinBox* m_lowStockSpin = nullptr;
    QPushButton* m_refreshInvBtn = nullptr;
    QPushButton* m_exportInvBtn = nullptr;
    QLabel* m_invSummaryLabel = nullptr;
    QTableWidget* m_invTable = nullptr;
};

#endif