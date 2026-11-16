
cmake_minimum_required(VERSION 3.19)
project(increment111 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
#Each dev has to configure OPENCV in QTCreator(Initial Configuration)
#and also as an environmental variable DO NOT HARDCODE PATH
if(NOT OpenCV_DIR AND DEFINED ENV{OpenCV_DIR})
    set(OpenCV_DIR "$ENV{OpenCV_DIR}")
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio objdetect calib3d features2d flann)

qt_standard_project_setup()


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
    Backend/Product/ProductService.h
    Backend/Product/ProductService.cpp
)

target_link_libraries(increment111
    PRIVATE
        Qt::Core
        Qt::Widgets
        ${OpenCV_LIBS}
)

target_include_directories(increment111 PRIVATE
    Backend
    Backend/Encoder
    Backend/imageGenerator
    Backend/BarcodeReader
    Backend/Product
    Frontend
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
