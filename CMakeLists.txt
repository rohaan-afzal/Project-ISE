cmake_minimum_required(VERSION 3.19)
project(increment111 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
#Each dev has to configure OPENCV in QTCreator(Initial Configuration)
#and also as an environmental variable DO NOT HARDCODE PATH
if((NOT OpenCV_DIR OR OpenCV_DIR MATCHES "-NOTFOUND$") AND DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR "$ENV{OpenCV_DIR}")
endif()

# Use the repository's bundled OpenCV build when Qt Creator has no valid
# OpenCV_DIR cache entry (or has cached OpenCV_DIR-NOTFOUND).
if((NOT OpenCV_DIR OR OpenCV_DIR MATCHES "-NOTFOUND$") AND MINGW AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/opencv-install/OpenCVConfig.cmake")
    set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/opencv-install" CACHE PATH
        "Bundled OpenCV package compatible with the selected MinGW compiler" FORCE)
endif()

# Qt's MinGW kits cannot link against OpenCV's MSVC (vcXX) libraries. If a
# stale Qt Creator cache points at an MSVC package, prefer this project's
# bundled MinGW build when it is available.
if(MINGW AND OpenCV_DIR MATCHES "/vc[0-9]+/" AND
   EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/opencv-install/OpenCVConfig.cmake")
    set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/opencv-install" CACHE PATH
        "OpenCV package compatible with the selected MinGW compiler" FORCE)
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio objdetect)

set(CMAKE_AUTOMOC ON)

qt_add_executable(increment111
    WIN32 MACOSX_BUNDLE
    Frontend/main.cpp
    Frontend/mainwindow.h
    Frontend/mainwindow.cpp
    Backend/Encoder/Encoder.h
    Backend/Encoder/Encoder.cpp
    Backend/imageGenerator/BarcodeImageGenerator.h
    Backend/imageGenerator/BarcodeImageGenerator.cpp

    Backend/Decoder/Decoder.h
    Backend/Decoder/Decoder.cpp

    Backend/ImageProcessor/ImageProcessor.h
    Backend/ImageProcessor/ImageProcessor.cpp

    Backend/BarcodeReader/BarcodeReader.h
    Backend/BarcodeReader/CameraBarcodeReader.h
    Backend/BarcodeReader/CameraBarcodeReader.cpp

    Backend/Product/Product.h
    Backend/Product/BarcodeGenerator.h
    Backend/Product/ProductBarcodeGenerator.h
    Backend/Product/ProductBarcodeGenerator.cpp
    Backend/Product/ProductDescriptionView.h
    Backend/Product/ProductDescriptionView.cpp
    Backend/Product/ProductLookupController.h
    Backend/Product/ProductLookupController.cpp
    Backend/Product/ProductRepository.h
    Backend/Product/ProductRepository.cpp
    Backend/Product/ProductService.h
    Backend/Product/ProductService.cpp

    Backend/Billing/Record-Format.h
    Backend/Order/Input-Validator.h
    Backend/Order/Input-Validator.cpp
    Backend/Billing/Bill.h
    Backend/Billing/Bill.cpp
    Backend/Billing/BillCalc.h
    Backend/Billing/BillCalc.cpp
    Backend/Billing/BillRepo.h
    Backend/Billing/BillRepo.cpp
    Backend/Billing/BillView.h
    Backend/Billing/BillView.cpp

    Backend/Order/Payment.h
    Backend/Order/Order.h
    Backend/Order/Order.cpp
    Backend/Order/PaymentProcessor.h
    Backend/Order/PaymentProcessor.cpp
    Backend/Order/OrderRepository.h
    Backend/Order/OrderRepository.cpp
    Backend/Order/OrderService.h
    Backend/Order/OrderService.cpp
    Backend/Order/InventoryService.h
    Backend/Order/InventoryService.cpp
    Backend/Order/OrderWorkflowValidator.h
    Backend/Order/OrderWorkflowValidator.cpp
    Backend/Order/OrderApprovalService.h
    Backend/Order/OrderApprovalService.cpp
    Backend/Order/PaymentService.h
    Backend/Order/PaymentService.cpp

    Frontend/CameraWidget.h
    Frontend/DescriptionButton.h
    Frontend/CamerWidget.cpp
    Frontend/BillDialog.h
    Frontend/BillDialog.cpp
    Frontend/CashierScreen.h
    Frontend/CashierScreen.cpp
    Frontend/ManagerScreen.h
    Frontend/ManagerScreen.cpp
)

target_link_libraries(increment111
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${OpenCV_LIBS}
)
target_include_directories(increment111 PRIVATE
    Backend
    Backend/imageGenerator
    Backend/BarcodeReader
    Backend/Product
    Frontend
    Backend/Encoder
    Backend/Billing
    Backend/Order
    ${OpenCV_INCLUDE_DIRS}
)

# OpenCV's DLLs are required beside the executable when launched outside Qt Creator.
foreach(opencv_target IN LISTS OpenCV_LIBS)
    if(TARGET ${opencv_target})
        get_target_property(opencv_runtime_dll ${opencv_target} IMPORTED_LOCATION_RELEASE)
        if(opencv_runtime_dll)
            add_custom_command(TARGET increment111 POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${opencv_runtime_dll}" "$<TARGET_FILE_DIR:increment111>")
        endif()
    endif()
endforeach()
