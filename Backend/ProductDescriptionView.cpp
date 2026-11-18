#include "ProductDescriptionView.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QString>

ProductDescriptionView::ProductDescriptionView(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    statusLabel = new QLabel("Scan a barcode to view product details.", this);
    productIDLabel = new QLabel(this);
    barcodeLabel = new QLabel(this);
    acronymLabel = new QLabel(this);
    nameLabel = new QLabel(this);
    unitLabel = new QLabel(this);
    priceLabel = new QLabel(this);
    taxLabel = new QLabel(this);
    discountLabel = new QLabel(this);
    quantityLabel = new QLabel(this);
    imagePathLabel = new QLabel(this);

    layout->addWidget(statusLabel);
    layout->addWidget(productIDLabel);
    layout->addWidget(barcodeLabel);
    layout->addWidget(acronymLabel);
    layout->addWidget(nameLabel);
    layout->addWidget(unitLabel);
    layout->addWidget(priceLabel);
    layout->addWidget(taxLabel);
    layout->addWidget(discountLabel);
    layout->addWidget(quantityLabel);
    layout->addWidget(imagePathLabel);

    setLayout(layout);
}

void ProductDescriptionView::displayProduct(const Product& product)
{
    statusLabel->setText("Product found");
    productIDLabel->setText("Product ID: " + QString::fromStdString(product.productID));
    barcodeLabel->setText("Barcode: " + QString::fromStdString(product.barcode));
    acronymLabel->setText("Acronym: " + QString::fromStdString(product.acronym));
    nameLabel->setText("Name: " + QString::fromStdString(product.name));
    unitLabel->setText("Unit: " + QString::fromStdString(product.unit));
    priceLabel->setText("Unit Price: Rs. " + QString::number(product.unitPrice, 'f', 2));
    taxLabel->setText("Tax: " + QString::number(product.tax, 'f', 2) + "%");
    discountLabel->setText("Discount: " + QString::number(product.discount, 'f', 2) + "%");
    quantityLabel->setText("Available Quantity: " + QString::number(product.quantity));
    imagePathLabel->setText("Barcode Image: " + QString::fromStdString(product.barcodeImagePath));
}

void ProductDescriptionView::displayNotFound(const std::string& barcode)
{
    statusLabel->setText("Product not found for barcode: " + QString::fromStdString(barcode));
    productIDLabel->clear();
    barcodeLabel->clear();
    acronymLabel->clear();
    nameLabel->clear();
    unitLabel->clear();
    priceLabel->clear();
    taxLabel->clear();
    discountLabel->clear();
    quantityLabel->clear();
    imagePathLabel->clear();
}

void ProductDescriptionView::displayError(const std::string& message)
{
    statusLabel->setText(QString::fromStdString(message));
    productIDLabel->clear();
    barcodeLabel->clear();
    acronymLabel->clear();
    nameLabel->clear();
    unitLabel->clear();
    priceLabel->clear();
    taxLabel->clear();
    discountLabel->clear();
    quantityLabel->clear();
    imagePathLabel->clear();
}

