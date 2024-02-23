cmake_minimum_required (VERSION 3.27.0)
project(giflib VERSION 0.1.0)

add_library(${PROJECT_NAME} STATIC
    "dgif_lib.c"
    "egif_lib.c"
    "gif_err.c"
    "gif_font.c"
    "gif_hash.c"
    "gifalloc.c"
    "openbsd-reallocarray.c"
    "quantize.c"
)
