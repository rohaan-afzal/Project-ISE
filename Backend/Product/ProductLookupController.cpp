#include "ProductLookupController.h"
#include "ProductDescriptionView.h"

ProductLookupController::ProductLookupController(ProductService* service,
    ProductDescriptionView* view)
{
    productService = service;
    descriptionView = view;
}

void ProductLookupController::showProductDescription(const std::string& barcode)
{
    if (productService == 0 || descriptionView == 0)
    {
        return;
    }

    Product product;

    if (productService->getProductByBarcode(barcode, product))
    {
        descriptionView->displayProduct(product);
    }
    else
    {
        descriptionView->displayNotFound(barcode);
    }
}

