cmake_minimum_required(VERSION 3.16)
project(BarcodeProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Gui Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Widgets)

add_library(encoder src/Encoder.cpp)
target_include_directories(encoder PUBLIC src)

add_library(recordformat src/RecordFormat.cpp)
target_include_directories(recordformat PUBLIC src)

add_library(productcatalog
    src/ProductFileParser.cpp
    src/FileProductRepository.cpp
    src/ProductService.cpp)
target_include_directories(productcatalog PUBLIC src)
target_link_libraries(productcatalog PUBLIC encoder recordformat)

add_library(billing
    Billing/Bill.cpp
    Billing/BillCalc.cpp
    Billing/BillRepo.cpp)
target_include_directories(billing PUBLIC Billing src)
target_link_libraries(billing PUBLIC recordformat)

add_library(billview src/BillView.cpp)
target_include_directories(billview PUBLIC src)
target_link_libraries(billview PUBLIC billing Qt${QT_VERSION_MAJOR}::Widgets)

add_library(imagegenerator src/BarcodeImageGenerator.cpp)
target_include_directories(imagegenerator PUBLIC src)
target_link_libraries(imagegenerator PUBLIC Qt${QT_VERSION_MAJOR}::Gui)

add_executable(barcode_generator src/main.cpp)
target_link_libraries(barcode_generator PRIVATE encoder imagegenerator)

enable_testing()

add_executable(encoder_tests tests/test_encoder.cpp)
target_link_libraries(encoder_tests PRIVATE encoder)
add_test(NAME encoder_tests COMMAND encoder_tests)

add_executable(image_tests tests/test_image.cpp)
target_link_libraries(image_tests PRIVATE encoder imagegenerator)
add_test(NAME image_tests COMMAND image_tests)

add_executable(product_tests tests/test_product.cpp)
target_link_libraries(product_tests PRIVATE productcatalog)
add_test(NAME product_tests COMMAND product_tests)

add_executable(bill_tests tests/test_bill.cpp)
target_link_libraries(bill_tests PRIVATE billing)
add_test(NAME bill_tests COMMAND bill_tests)

add_executable(bill_view_tests tests/test_bill_view.cpp)
target_link_libraries(bill_view_tests PRIVATE billview)
add_test(NAME bill_view_tests COMMAND bill_view_tests)
set_tests_properties(bill_view_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")