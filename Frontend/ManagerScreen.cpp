#include "ManagerScreen.h"
#include "../Backend/Product/ProductService.h"
#include "../Backend/Encoder/Encoder.h"
#include "../Backend/imageGenerator/BarcodeImageGenerator.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QPixmap>
#include <QFileDialog>
#include <QMessageBox>
#include <QFrame>
#include <stdexcept>
#include <vector>

ManagerScreen::ManagerScreen(ProductService* productService,
                             const QString& productFilePath,
                             QWidget* parent)
    : QWidget(parent),
    m_productService(productService),
    m_productFilePath(productFilePath)
{
    setupUi();
    refreshProductTable();
}

void ManagerScreen::setupUi()
{
    QHBoxLayout* rootLayout = new QHBoxLayout(this);

    // Left panel: Add Product + Barcode Generator
    QGroupBox* formGroup = new QGroupBox("Add New Product to Catalog", this);
    QVBoxLayout* leftLayout = new QVBoxLayout(formGroup);

    QFormLayout* form = new QFormLayout();

    m_idInput = new QLineEdit(this);
    m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));

    m_barcodeInput = new QLineEdit(this);
    m_barcodeInput->setPlaceholderText("6-digit barcode (first digit 1-9)");
    m_barcodeInput->setMaxLength(6);
    m_barcodeInput->setValidator(
        new QRegularExpressionValidator(QRegularExpression("[1-9][0-9]{0,5}"), m_barcodeInput));

    m_acronymInput = new QLineEdit(this);
    m_acronymInput->setPlaceholderText("e.g. TEA");

    m_nameInput = new QLineEdit(this);
    m_nameInput->setPlaceholderText("e.g. GreenTea");

    m_unitCombo = new QComboBox(this);
    m_unitCombo->addItems(QStringList() << "Pack" << "Kg" << "Gram" << "Litre" << "ml" << "Piece" << "Box" << "Bottle" << "Dozen");

    m_priceSpin = new QDoubleSpinBox(this);
    m_priceSpin->setRange(0.0, 1000000.0);
    m_priceSpin->setDecimals(2);
    m_priceSpin->setPrefix("PKR ");

    m_taxSpin = new QDoubleSpinBox(this);
    m_taxSpin->setRange(0.0, 100000.0);
    m_taxSpin->setDecimals(2);

    m_discountSpin = new QDoubleSpinBox(this);
    m_discountSpin->setRange(0.0, 100000.0);
    m_discountSpin->setDecimals(2);

    m_quantitySpin = new QSpinBox(this);
    m_quantitySpin->setRange(0, 100000);
    m_quantitySpin->setValue(10);

    m_imagePathInput = new QLineEdit(this);
    m_imagePathInput->setPlaceholderText("e.g. tea.png (optional)");

    form->addRow("Product ID:", m_idInput);
    form->addRow("Barcode (6-digit):", m_barcodeInput);
    form->addRow("Acronym:", m_acronymInput);
    form->addRow("Name:", m_nameInput);
    form->addRow("Unit:", m_unitCombo);
    form->addRow("Unit Price:", m_priceSpin);
    form->addRow("Tax:", m_taxSpin);
    form->addRow("Discount:", m_discountSpin);
    form->addRow("Stock Quantity:", m_quantitySpin);
    form->addRow("Barcode Image:", m_imagePathInput);
    leftLayout->addLayout(form);

    QHBoxLayout* barcodeBtns = new QHBoxLayout();
    m_genBarcodeBtn = new QPushButton("Preview Barcode", this);
    m_saveBarcodeBtn = new QPushButton("Save Barcode PNG", this);
    m_saveBarcodeBtn->setEnabled(false);
    barcodeBtns->addWidget(m_genBarcodeBtn);
    barcodeBtns->addWidget(m_saveBarcodeBtn);
    leftLayout->addLayout(barcodeBtns);

    m_barcodePreview = new QLabel("Barcode preview", this);
    m_barcodePreview->setAlignment(Qt::AlignCenter);
    m_barcodePreview->setMinimumHeight(100);
    m_barcodePreview->setFrameShape(QFrame::Box);
    leftLayout->addWidget(m_barcodePreview);

    m_addProductBtn = new QPushButton("Save Product to Catalog", this);
    leftLayout->addWidget(m_addProductBtn);

    m_statusLabel = new QLabel("Ready.", this);
    m_statusLabel->setWordWrap(true);
    leftLayout->addWidget(m_statusLabel);

    rootLayout->addWidget(formGroup, 4);

    // Right panel: Current Product Catalog Table + Restock / Remove
    QGroupBox* catalogGroup = new QGroupBox("Current Product Catalog (Data/Product.txt)", this);
    QVBoxLayout* rightLayout = new QVBoxLayout(catalogGroup);

    m_productTable = new QTableWidget(0, 9, this);
    m_productTable->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Barcode" << "Acronym" << "Name" << "Unit"
                      << "Price" << "Tax" << "Discount" << "Stock");
    m_productTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_productTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_productTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_productTable->verticalHeader()->setVisible(false);
    m_productTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    rightLayout->addWidget(m_productTable, 1);

    QHBoxLayout* stockActionsRow = new QHBoxLayout();
    m_restockSpin = new QSpinBox(this);
    m_restockSpin->setRange(1, 100000);
    m_restockSpin->setValue(10);
    m_restockSpin->setPrefix("+ Add Stock: ");

    m_restockBtn = new QPushButton("Restock Selected", this);
    m_removeProductBtn = new QPushButton("Remove Selected Product", this);
    m_refreshBtn = new QPushButton("Refresh", this);

    stockActionsRow->addWidget(m_restockSpin);
    stockActionsRow->addWidget(m_restockBtn);
    stockActionsRow->addWidget(m_removeProductBtn);
    stockActionsRow->addWidget(m_refreshBtn);
    rightLayout->addLayout(stockActionsRow);

    rootLayout->addWidget(catalogGroup, 6);

    connect(m_genBarcodeBtn, &QPushButton::clicked, this, &ManagerScreen::onGenerateBarcodeClicked);
    connect(m_saveBarcodeBtn, &QPushButton::clicked, this, &ManagerScreen::onSaveBarcodeImageClicked);
    connect(m_addProductBtn, &QPushButton::clicked, this, &ManagerScreen::onAddProductClicked);
    connect(m_restockBtn, &QPushButton::clicked, this, &ManagerScreen::onRestockClicked);
    connect(m_removeProductBtn, &QPushButton::clicked, this, &ManagerScreen::onRemoveProductClicked);
    connect(m_refreshBtn, &QPushButton::clicked, this, &ManagerScreen::refreshProductTable);
}

void ManagerScreen::setStatusMessage(const QString& text, bool isError)
{
    m_statusLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
    m_statusLabel->setText(text);
}

QString ManagerScreen::selectedCatalogBarcode() const
{
    const int row = m_productTable->currentRow();
    if (row < 0 || !m_productTable->item(row, 1))
        return QString();
    return m_productTable->item(row, 1)->text();
}

void ManagerScreen::onGenerateBarcodeClicked()
{
    const QString code = m_barcodeInput->text().trimmed();
    try
    {
        const std::string bits = Encoder::encode(code.toStdString());
        BarcodeImageGenerator generator;
        m_generatedBarcodeImage = generator.generate(bits);

        m_barcodePreview->setPixmap(QPixmap::fromImage(m_generatedBarcodeImage).scaled(
            m_barcodePreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_saveBarcodeBtn->setEnabled(true);
        if (m_imagePathInput->text().trimmed().isEmpty())
            m_imagePathInput->setText("barcode_" + code + ".png");

        setStatusMessage("Generated barcode preview for " + code + ".");
    }
    catch (const std::exception& e)
    {
        m_saveBarcodeBtn->setEnabled(false);
        setStatusMessage(QString::fromUtf8(e.what()), true);
    }
}

void ManagerScreen::onSaveBarcodeImageClicked()
{
    if (m_generatedBarcodeImage.isNull())
        return;

    const QString defaultName = m_imagePathInput->text().trimmed().isEmpty()
                                    ? ("barcode_" + m_barcodeInput->text().trimmed() + ".png")
                                    : m_imagePathInput->text().trimmed();

    const QString path = QFileDialog::getSaveFileName(
        this, "Save Barcode Image", defaultName, "PNG Image (*.png)");
    if (path.isEmpty())
        return;

    if (m_generatedBarcodeImage.save(path, "PNG"))
        setStatusMessage("Saved barcode image to " + path);
    else
        setStatusMessage("Failed to save barcode image.", true);
}

void ManagerScreen::onAddProductClicked()
{
    Product p;
    p.productID = m_idInput->text().trimmed().toStdString();
    p.barcode = m_barcodeInput->text().trimmed().toStdString();
    p.acronym = m_acronymInput->text().trimmed().toStdString();
    p.name = m_nameInput->text().trimmed().replace(' ', '_').toStdString();
    p.unit = m_unitCombo->currentText().toStdString();
    p.unitPrice = m_priceSpin->value();
    p.tax = m_taxSpin->value();
    p.discount = m_discountSpin->value();
    p.quantity = m_quantitySpin->value();
    p.barcodeImagePath = m_imagePathInput->text().trimmed().toStdString();

    if (m_productService->addProduct(p))
    {
        setStatusMessage("Added product " + QString::fromStdString(p.name) + " (" + QString::fromStdString(p.barcode) + ").");
        m_barcodeInput->clear();
        m_acronymInput->clear();
        m_nameInput->clear();
        m_imagePathInput->clear();
        refreshProductTable();
        m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));
    }
    else
    {
        setStatusMessage("Failed to add product. Ensure ID and 6-digit barcode are unique and fields are valid.", true);
    }
}

void ManagerScreen::onRestockClicked()
{
    const QString barcode = selectedCatalogBarcode();
    if (barcode.isEmpty())
    {
        setStatusMessage("Select a product row in the catalog table first.", true);
        return;
    }

    const int addedQty = m_restockSpin->value();
    if (m_productService->restockProduct(barcode.toStdString(), addedQty))
    {
        refreshProductTable();
        setStatusMessage("Restocked barcode " + barcode + " by +" + QString::number(addedQty) + " units.");
    }
    else
    {
        setStatusMessage("Failed to restock selected product.", true);
    }
}

void ManagerScreen::onRemoveProductClicked()
{
    const QString barcode = selectedCatalogBarcode();
    if (barcode.isEmpty())
    {
        setStatusMessage("Select a product row in the catalog table to remove.", true);
        return;
    }

    const auto reply = QMessageBox::question(
        this,
        "Remove Product",
        "Remove product with barcode " + barcode + " from Data/Product.txt?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (m_productService->removeProduct(barcode.toStdString()))
    {
        refreshProductTable();
        m_idInput->setText(QString::fromStdString(m_productService->getNextProductID()));
        setStatusMessage("Removed product with barcode " + barcode + ".");
    }
    else
    {
        setStatusMessage("Failed to remove selected product.", true);
    }
}

void ManagerScreen::refreshProductTable()
{
    const std::vector<Product> products = m_productService->getAllProducts();

    m_productTable->setRowCount(0);
    for (const auto& p : products)
    {
        const int row = m_productTable->rowCount();
        m_productTable->insertRow(row);

        m_productTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.productID)));
        m_productTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.barcode)));
        m_productTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(p.acronym)));
        m_productTable->setItem(row, 3, new QTableWidgetItem(QString::fromStdString(p.name)));
        m_productTable->setItem(row, 4, new QTableWidgetItem(QString::fromStdString(p.unit)));
        m_productTable->setItem(row, 5, new QTableWidgetItem(QString::number(p.unitPrice, 'f', 2)));
        m_productTable->setItem(row, 6, new QTableWidgetItem(QString::number(p.tax, 'f', 2)));
        m_productTable->setItem(row, 7, new QTableWidgetItem(QString::number(p.discount, 'f', 2)));
        m_productTable->setItem(row, 8, new QTableWidgetItem(QString::number(p.quantity)));
    }
}