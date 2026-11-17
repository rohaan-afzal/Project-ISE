
#pragma once

#include <QMainWindow>
#include <QImage>
#include <atomic>
#include <memory>
#include <thread>
//Forward Declartion
class QTabWidget;
class QLineEdit;
class QLabel;
class QPushButton;
class QWidget;
class QTimer;
class ProductService;
class CameraBarcodeReader;

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
    std::unique_ptr<ProductService> m_productService;
    std::unique_ptr<CameraBarcodeReader> m_barcodeReader;
    bool m_scanning;
    std::atomic_bool m_cancelScan;
    std::thread m_scanThread;

    QWidget* createGenerateTab();
    QWidget* createReadTab();

private slots:
    void onGenerateClicked();
    void onSaveClicked();
    void onUploadClicked();
    void onCameraClicked();
    void onDescriptionClicked();
    void showProductForBarcode(const QString& barcode);
};
