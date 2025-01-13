cmake_minimum_required(VERSION 3.30)
project(DirectFitGirlDownloader)

set(CMAKE_CXX_STANDARD 26)

# Set pkg-config path for MSYS2
if(WIN32)
    set(ENV{PKG_CONFIG_PATH} "C:/msys64/ucrt64/lib/pkgconfig")
endif()

# Find required packages using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(CURL REQUIRED libcurl)
pkg_check_modules(LIBXML REQUIRED libxml-2.0)

add_executable(DirectFitGirlDownloader main.cpp
        LinkGenerator.cpp
        LinkGenerator.h
        CurlHelper.h)

# Include headers
target_include_directories(DirectFitGirlDownloader PRIVATE
        ${CURL_INCLUDE_DIRS}
        ${LIBXML_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(DirectFitGirlDownloader PRIVATE
        ${CURL_LIBRARIES}
        ${LIBXML_LIBRARIES}
)

# Add library directories for Windows
if(WIN32)
    target_link_directories(DirectFitGirlDownloader PRIVATE C:/msys64/ucrt64/lib)
endif()