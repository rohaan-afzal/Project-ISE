#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QString>
#include <memory>

class QTabWidget;
class QLineEdit;
class QPushButton;
class QLabel;
class CashierScreen;
class ManagerScreen;
class ProductService;
class OrderRepository;
class OrderDetailRepository;
class BillRepository;
class BillDetailRepository;
class FileSeasonRepository;
class SeasonalDiscountService;
class BillService;
class OrderService;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onTabChanged(int index);
    void onGenerateClicked();
    void onSaveClicked();

private:
    QString resolveDataDirectory() const;
    QWidget* createGenerateTab();

    QString m_dataDirPath;

    std::unique_ptr<ProductService> m_productService;
    std::unique_ptr<OrderRepository> m_orderRepository;
    std::unique_ptr<OrderDetailRepository> m_orderDetailRepository;
    std::unique_ptr<BillRepository> m_billRepository;
    std::unique_ptr<BillDetailRepository> m_billDetailRepository;
    std::unique_ptr<FileSeasonRepository> m_seasonRepository;
    std::unique_ptr<SeasonalDiscountService> m_seasonalDiscountService;
    std::unique_ptr<BillService> m_billService;
    std::unique_ptr<OrderService> m_orderService;

    QTabWidget* m_tabs = nullptr;
    CashierScreen* m_cashierScreen = nullptr;
    ManagerScreen* m_managerScreen = nullptr;


    QLineEdit* m_Input = nullptr;
    QPushButton* m_Generate = nullptr;
    QLabel* m_Preview = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QLabel* m_Status = nullptr;
    QImage m_currentImage;
};

#endif