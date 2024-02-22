cmake_minimum_required (VERSION 3.27.0)
project("webpcvt" VERSION 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/Output")

find_package(ZLIB)

add_library(${PROJECT_NAME} SHARED
    "exports.def"
    "libwebpcvt.c"
)

target_link_directories(${PROJECT_NAME} PUBLIC
    "libjpeg/lib"
    "libpng/lib"
    "libwebp/lib"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "libjpeg/include"
    "libpng/include"
    "libwebp/include"
)

target_link_libraries(${PROJECT_NAME}
    libpng16.a
    libturbojpeg.a
    libwebpdecoder.a
    libz.a
    -Wl,-Bstatic,--whole-archive
    -lwinpthread
    -Wl,-Bdynamic,--no-whole-archive
    -static-libgcc
 )

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
