#pragma once
#ifndef PRODUCTDESCRIPTIONVIEW_H
#define PRODUCTDESCRIPTIONVIEW_H

#include <QWidget>
#include "Product.h"

class QLabel;
class QVBoxLayout;

class ProductDescriptionView : public QWidget
{
private:
    QLabel* statusLabel;
    QLabel* productIDLabel;
    QLabel* barcodeLabel;
    QLabel* acronymLabel;
    QLabel* nameLabel;
    QLabel* unitLabel;
    QLabel* priceLabel;
    QLabel* taxLabel;
    QLabel* discountLabel;
    QLabel* quantityLabel;
    QLabel* imagePathLabel;

public:
    explicit ProductDescriptionView(QWidget* parent = 0);

    void displayProduct(const Product& product);
    void displayNotFound(const std::string& barcode);
    void displayError(const std::string& message);
};

#endif
