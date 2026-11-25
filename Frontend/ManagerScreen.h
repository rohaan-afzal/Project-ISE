#pragma once
#ifndef MANAGERSCREEN_H
#define MANAGERSCREEN_H

#include <QWidget>
#include <QImage>
#include <QString>

class QLineEdit;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QPushButton;
class QLabel;
class QTableWidget;
class ProductService;
class CameraWidget;

class ManagerScreen : public QWidget
{
    Q_OBJECT

public:
    explicit ManagerScreen(ProductService* productService,
        const QString& productFilePath,
        QWidget* parent = nullptr);

public slots:
    void refreshProductTable();

private slots:
    void onAddProductClicked();
    void onGenerateBarcodeClicked();
    void onSaveBarcodeImageClicked();
    void onRestockClicked();
    void onRemoveProductClicked();
    void onBarcodeScanned(const QString& barcode);

private:
    ProductService* m_productService;
    QString m_productFilePath;

    QLineEdit* m_idInput;
    QLineEdit* m_barcodeInput;
    QComboBox* m_categoryCombo;
    QLineEdit* m_acronymInput;
    QLineEdit* m_nameInput;
    QComboBox* m_unitCombo;
    QDoubleSpinBox* m_priceSpin;
    QDoubleSpinBox* m_taxSpin;
    QDoubleSpinBox* m_discountSpin;
    QSpinBox* m_quantitySpin;
    QLineEdit* m_imagePathInput;

    QPushButton* m_genBarcodeBtn;
    QPushButton* m_saveBarcodeBtn;
    QPushButton* m_addProductBtn;
    QPushButton* m_refreshBtn;

    QSpinBox* m_restockSpin;
    QPushButton* m_restockBtn;
    QPushButton* m_removeProductBtn;

    QLabel* m_barcodePreview;
    QLabel* m_statusLabel;
    CameraWidget* m_cameraWidget;
    QTableWidget* m_productTable;
    QImage m_generatedBarcodeImage;

    void setupUi();
    void setStatusMessage(const QString& text, bool isError = false);
    QString selectedCatalogBarcode() const;
};

#endif
