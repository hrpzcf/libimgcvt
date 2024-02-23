#include <stdio.h>
#include <stdlib.h>

#include "jpeglib.h"
#include "libpng16/png.h"
#include "libwebpcvt.h"
#include "webp/decode.h"

static int WriteBufferToPngFile(WebPDecoderConfig *config, FILE *handle)
{
    int result = 0;
    if (handle == NULL)
    {
        return result;
    }
    png_infop info_pointer = NULL;
    png_structp png_pointer = NULL;
    png_pointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_pointer == NULL)
    {
        return result;
    }
    if ((info_pointer = png_create_info_struct(png_pointer)) == NULL)
    {
        goto FinalizeAndExit;
    }
    if (setjmp(png_jmpbuf(png_pointer)))
    {
        goto FinalizeAndExit;
    }
    png_init_io(png_pointer, handle);
    png_set_IHDR(png_pointer, info_pointer,
                 config->output.width, config->output.height,
                 8,
                 config->input.has_alpha ? PNG_COLOR_TYPE_RGBA : PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_pointer, info_pointer);
    png_bytep row_pointer = config->output.u.RGBA.rgba;
    for (int y = 0; y < config->output.height; ++y)
    {
        if (row_pointer - config->output.u.RGBA.rgba + config->output.u.RGBA.stride >
            config->output.u.RGBA.size)
        {
            break;
        }
        png_write_row(png_pointer, row_pointer);
        row_pointer += config->output.u.RGBA.stride;
    }
    png_write_end(png_pointer, info_pointer);
    result = 1;
FinalizeAndExit:
    png_destroy_write_struct(&png_pointer, &info_pointer);
    return result;
}

static int WriteBufferToJpgFile(WebPDecoderConfig *config, int quality, FILE *handle)
{
    int result = 0;
    if (handle == NULL || config->output.colorspace != MODE_RGB)
    {
        return result;
    }
    struct jpeg_error_mgr jerr;
    struct jpeg_compress_struct jinfo;
    jinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&jinfo);
    jinfo.image_width = config->output.width;
    jinfo.image_height = config->output.height;
    jinfo.input_components = 3;
    jinfo.in_color_space = JCS_RGB;
    jpeg_stdio_dest(&jinfo, handle);
    jpeg_set_defaults(&jinfo);
    jpeg_set_quality(&jinfo, quality, TRUE);
    jpeg_start_compress(&jinfo, TRUE);
    JSAMPROW row_pointer = config->output.u.RGBA.rgba;
    while (jinfo.next_scanline < jinfo.image_height)
    {
        if (row_pointer - config->output.u.RGBA.rgba + config->output.u.RGBA.stride >
            config->output.u.RGBA.size)
        {
            break;
        }
        jpeg_write_scanlines(&jinfo, &row_pointer, 1);
        row_pointer += config->output.u.RGBA.stride;
    }
    result = jerr.msg_code == 0;
    jpeg_finish_compress(&jinfo);
    jpeg_destroy_compress(&jinfo);
    return result;
}

/// @brief 把 WEBP 文件转换为 JPEG/PNG 文件
/// @param in_file 输入文件的完整路径
/// @param out_file 输出文件的完整路径
/// @param out_fmt 输出文件类型，具体请参考 outfmt_t 枚举
/// @param quality 输出文件的图像质量，仅输出 JPEG 文件时有效
/// @param reason 转换失败时设置失败代码
/// @return 成功返回非 0，失败返回 0
int WebpConvertTo(const char *in_file, const char *out_file, outfmt_t out_fmt, int quality, int *reason)
{
    int result = 0;
    FILE *in_handle = NULL;
    FILE *out_handle = NULL;
    uint8_t *data = NULL;
    WebPDecoderConfig config;
    if (NULL == in_file || NULL == out_file)
    {
        *reason = 1;
        return result;
    }
    if (out_fmt < WC_JPG || out_fmt > WC_PNG)
    {
        *reason = 2;
        return result;
    }
    if (!WebPInitDecoderConfig(&config))
    {
        *reason = 3;
        goto FinalizeAndExit;
    }
    if ((in_handle = fopen(in_file, "rb")) == NULL)
    {
        *reason = 4;
        goto FinalizeAndExit;
    }
    fseek(in_handle, 0L, SEEK_END);
    size_t data_size = ftell(in_handle);
    data = malloc(data_size);
    fseek(in_handle, 0L, SEEK_SET);
    if (data_size != fread(data, 1, data_size, in_handle))
    {
        *reason = 5;
        goto FinalizeAndExit;
    }
    if (WC_JPG == out_fmt)
    {
        config.output.colorspace = MODE_RGB;
    }
    if (WebPDecode(data, data_size, &config) != VP8_STATUS_OK)
    {
        *reason = 6;
        goto FinalizeAndExit;
    }
    if (config.input.has_animation)
    {
        *reason = 7;
        goto FinalizeAndExit;
    }
    if ((out_handle = fopen(out_file, "wb")) == NULL)
    {
        *reason = 8;
        goto FinalizeAndExit;
    }
    switch (out_fmt)
    {
    case WC_PNG:
        result = WriteBufferToPngFile(&config, out_handle);
        break;
    case WC_JPG:
        result = WriteBufferToJpgFile(&config, quality, out_handle);
        break;
    }
    *reason = result ? 0 : 9;
FinalizeAndExit:
    if (NULL != in_handle)
    {
        fclose(in_handle);
    }
    if (NULL != out_handle)
    {
        fclose(out_handle);
    }
    free(data);
    WebPFreeDecBuffer(&config.output);
    return result;
}
