cmake_minimum_required (VERSION 3.27.0)
project(imgcvt VERSION 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/Output")

# add_subdirectory("giflib")
add_subdirectory("libpng")
add_subdirectory("zlib")
add_subdirectory("libwebp")

add_library(${PROJECT_NAME} SHARED
    "imgcvtutils.c"
    "imgdecode.c"
    "imgencode.c"
    "libimagecvt.c"
    "exports.def"
)

target_link_directories(${PROJECT_NAME} PUBLIC
    "libjpeg/lib"
)

target_include_directories(${PROJECT_NAME} PUBLIC
    "giflib"
    "libjpeg/include"
    "libpng"
    "libwebp"
)

target_link_libraries(${PROJECT_NAME}
    # giflib
    png_static
    zlibstatic
    webp
    sharpyuv
    libturbojpeg.a
    -Wl,-Bstatic,--whole-archive
    -lwinpthread
    -Wl,-Bdynamic,--no-whole-archive
    -static-libgcc
 )
