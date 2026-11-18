#pragma once

#include <QMainWindow>
#include <QImage>
#include <QString>
#include <atomic>
#include <memory>
#include <thread>

// Forward Declarations
class QTabWidget;
class QLineEdit;
class QLabel;
class QPushButton;
class QWidget;
class ProductService;
class OrderRepository;
class OrderDetailRepository;
class FileBillRepository;
class BillService;
class OrderService;
class CashierScreen;
class ManagerScreen;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    QLineEdit* m_Input;
    QPushButton* m_Generate;
    QPushButton* m_saveBtn;
    QLabel* m_Preview;
    QLabel* m_Status;
    QImage m_currentImage;
    QTabWidget* m_tabs;

    QPushButton* m_uploadBtn;
    QLabel* m_readPreview;
    QLabel* m_decodedLabel;
    QLabel* m_readStatus;
    QLabel* m_productDetails;
    QPushButton* m_cameraBtn;
    QPushButton* m_descriptionBtn;
    QString m_scannedBarcode;

    // Backend Services & Repositories
    QString m_dataDirPath;
    std::unique_ptr<ProductService> m_productService;
    std::unique_ptr<OrderRepository> m_orderRepository;
    std::unique_ptr<OrderDetailRepository> m_orderDetailRepository;
    std::unique_ptr<FileBillRepository> m_billRepository;
    std::unique_ptr<BillService> m_billService;
    std::unique_ptr<OrderService> m_orderService;

    // POS Screens
    CashierScreen* m_cashierScreen;
    ManagerScreen* m_managerScreen;

    bool m_scanning;
    std::atomic_bool m_cancelScan;
    std::thread m_scanThread;

    QString resolveDataDirectory() const;
    QWidget* createGenerateTab();
    QWidget* createReadTab();

private slots:

    void onTabChanged(int index);
    void onGenerateClicked();
    void onSaveClicked();
};