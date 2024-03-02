#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imgdecode.h"
#include "jpeglib.h"
#include "png.h"
#include "pngstruct.h"
#include "pnginfo.h"
#include "webp/decode.h"

void jerror_handler(j_common_ptr cinfo)
{
    (*cinfo->err->output_message)(cinfo);
}

int DecodeJpg(imgcconfig_t *config, FILE *file_ptr, char *error)
{
    int result = 0;
    int decompress_created = 0;
    int decompress_started = 0;
    struct jpeg_decompress_struct jinfo;
    struct jpeg_error_mgr jerr;
    if (NULL == config || NULL == file_ptr)
    {
        sprintf(error, "[JPEG 解码] 解码配置或文件句柄为空");
        return result;
    }
    config->count = 1;
    if (!ImgcAllocConfigImages(config))
    {
        sprintf(error, "[JPEG 解码] 为图像分配内存空间失败");
        return result;
    }
    config->images->clr = config->clr;
    switch (config->clr)
    {
    case IMGC_CLR_RGB:
        jinfo.out_color_space = JCS_RGB;
        break;
    case IMGC_CLR_GSC:
        jinfo.out_color_space = JCS_GRAYSCALE;
        break;
    case IMGC_CLR_ANY:
        int grayscale = jinfo.jpeg_color_space == JCS_GRAYSCALE;
        jinfo.out_color_space = grayscale ? JCS_GRAYSCALE : JCS_RGB;
        config->images->clr = grayscale ? IMGC_CLR_GSC : IMGC_CLR_RGB;
        break;
    default:
        sprintf(error, "[JPEG 解码] 不支持指定的颜色空间");
        goto FinalizeAndExit;
    }
    jinfo.err = jpeg_std_error(&jerr);
    jinfo.err->error_exit = jerror_handler;
    jpeg_create_decompress(&jinfo);
    decompress_created = 1;
    jpeg_stdio_src(&jinfo, file_ptr);
    if (JPEG_HEADER_OK != jpeg_read_header(&jinfo, TRUE))
    {
        sprintf(error, "[JPEG 解码] 是一个无效的 JPEG 文件");
        goto FinalizeAndExit;
    }
    if (!jpeg_start_decompress(&jinfo))
    {
        sprintf(error, "[JPEG 解码] 无法开始对文件进行解码");
        goto FinalizeAndExit;
    }
    decompress_started = 1;
    if (jinfo.output_components < 1)
    {
        sprintf(error, "[JPEG 解码] 图片的颜色通道数小于 1");
        goto FinalizeAndExit;
    }
    if (jinfo.output_width < 1 || jinfo.output_height < 1)
    {
        sprintf(error, "[JPEG 解码] 图片的宽或图片的高小于 1");
        goto FinalizeAndExit;
    }
    config->images->width = jinfo.output_width;
    config->images->height = jinfo.output_height;
    config->images->stride = jinfo.output_width * jinfo.output_components;
    if (!ImgcAllocConfigImagesRgba(config))
    {
        sprintf(error, "[JPEG 解码] 为图像数据分配内存空间失败");
        goto FinalizeAndExit;
    }
    JSAMPROW row_pointer = config->images->rgba;
    for (int i = 0; i < jinfo.output_height; ++i)
    {
        if (jpeg_read_scanlines(&jinfo, &row_pointer, 1) != 1)
        {
            sprintf(error, "[JPEG 解码] 从 JPEG 图像中读取数据失败");
            goto FinalizeAndExit;
        }
        row_pointer += config->images->stride;
    }
    result = 1;
FinalizeAndExit:
    if (decompress_started)
    {
        jpeg_finish_decompress(&jinfo);
    }
    if (decompress_created)
    {
        jpeg_destroy_decompress(&jinfo);
    }
    return result;
}

int DecodePng(imgcconfig_t *config, FILE *file_ptr, char *error)
{
    int result = 0;
    png_infop info_pointer;
    png_structp png_pointer;
    int chanels = 0;
    if (NULL == config || NULL == file_ptr)
    {
        sprintf(error, "[PNG 解码] 解码配置或文件句柄为空");
        return result;
    }
    config->count = 1;
    if (!ImgcAllocConfigImages(config))
    {
        sprintf(error, "[PNG 解码] 无法为解码配置分配图像内存");
        return result;
    }
    if (NULL == (png_pointer = png_create_read_struct(
                     PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
    {
        sprintf(error, "[PNG 解码] 无法创建 PNG 解码结构体");
        return result;
    }
    if (NULL == (info_pointer = png_create_info_struct(png_pointer)))
    {
        sprintf(error, "[PNG 解码] 无法创建 PNG 信息结构体");
        goto FinalizeAndExit;
    }
    if (setjmp(png_jmpbuf(png_pointer)))
    {
        sprintf(error, "[PNG 解码] 解码过程中出现了错误，解码失败");
        goto FinalizeAndExit;
    }
    png_init_io(png_pointer, file_ptr);
    png_read_info(png_pointer, info_pointer);
    int bit_depth = png_get_bit_depth(png_pointer, info_pointer);
    int color_type = png_get_color_type(png_pointer, info_pointer);
    if (bit_depth > 8)
    {
        png_set_scale_16(png_pointer);
    }
    if (bit_depth < 8 || color_type == PNG_COLOR_TYPE_PALETTE)
    {
        png_set_expand(png_pointer);
    }
    png_read_update_info(png_pointer, info_pointer);
    color_type = png_get_color_type(png_pointer, info_pointer);
    switch (color_type)
    {
    case PNG_COLOR_TYPE_GRAY: // 0
        config->images->clr = IMGC_CLR_GSC;
        break;
    case PNG_COLOR_TYPE_GRAY_ALPHA: // 4
        config->images->clr = IMGC_CLR_GSCA;
        break;
    case PNG_COLOR_TYPE_RGB: // 2
        config->images->clr = IMGC_CLR_RGB;
        break;
    case PNG_COLOR_TYPE_RGB_ALPHA: // 6
        config->images->clr = IMGC_CLR_RGBA;
        break;
    default:
        sprintf(error, "[PNG 解码] 不支持的 PNG 颜色空间");
        goto FinalizeAndExit;
    }
    if ((chanels = png_get_channels(png_pointer, info_pointer)) < 1)
    {
        sprintf(error, "[PNG 解码] PNG 图像的颜色通道数小于 1");
        goto FinalizeAndExit;
    }
    size_t row_bytes = png_get_rowbytes(png_pointer, info_pointer);
    if (row_bytes > INT_MAX)
    {
        sprintf(error, "[PNG 解码] PNG 行像素数过大，解码失败");
        goto FinalizeAndExit;
    }
    config->images->stride = (int)row_bytes;
    config->images->width = png_get_image_width(png_pointer, info_pointer);
    config->images->height = png_get_image_height(png_pointer, info_pointer);
    if (row_bytes != chanels * config->images->width)
    {
        sprintf(error, "[PNG 解码] PNG 行像素数与预期不一致");
        goto FinalizeAndExit;
    }
    if (config->images->width < 1 || config->images->height < 1)
    {
        sprintf(error, "[PNG 解码] PNG 图像的宽度或高度小于 1");
        goto FinalizeAndExit;
    }
    if (!ImgcAllocConfigImagesRgba(config))
    {
        sprintf(error, "[PNG 解码] 无法为解码配置分配图像数据内存");
        goto FinalizeAndExit;
    }
    png_bytep row_pointer = config->images->rgba;
    for (int row_count = 0; row_count < config->images->height; ++row_count)
    {
        png_read_row(png_pointer, row_pointer, NULL);
        row_pointer += config->images->stride;
    }
    png_read_end(png_pointer, info_pointer);
    result = 1;
FinalizeAndExit:
    png_destroy_read_struct(&png_pointer, &info_pointer, NULL);
    return result;
}

int DecodeWebp(imgcconfig_t *config, FILE *file_ptr, char *error)
{
    int result = 0;
    WebPDecoderConfig wp_config;
    WebPDecBuffer *const output = &wp_config.output;
    WebPBitstreamFeatures *const input = &wp_config.input;
    uint8_t *data = NULL;
    size_t data_size = 0;
    if (NULL == config || NULL == file_ptr)
    {
        sprintf(error, "[WEBP 解码] 解码配置或文件句柄为空");
        return result;
    }
    if (0 != fseek(file_ptr, 0, SEEK_END))
    {
        sprintf(error, "[WEBP 解码] 无法移动文件指针");
        return result;
    }
    if (-1L == (data_size = ftell(file_ptr)))
    {
        sprintf(error, "[WEBP 解码] 无法获取文件指针未知");
        return result;
    }
    if (NULL == (data = malloc(data_size)))
    {
        sprintf(error, "[WEBP 解码] 无法分配文件数据缓存");
        return result;
    }
    if (0 != fseek(file_ptr, 0, SEEK_SET))
    {
        sprintf(error, "[WEBP 解码] 无法移动文件指针");
        return result;
    }
    if (data_size != fread(data, sizeof(uint8_t), data_size, file_ptr))
    {
        sprintf(error, "[WEBP 解码] 读取的文件数据长度不正确");
        return result;
    }
    if (!WebPInitDecoderConfig(&wp_config))
    {
        sprintf(error, "[WEBP 解码] 初始化解码配置失败");
        return result;
    }
    if (VP8_STATUS_OK != WebPGetFeatures(data, data_size, input))
    {
        sprintf(error, "[WEBP 解码] 无法获取图像的特征");
        goto FinalizeAndExit;
    }
    if (input->has_animation)
    {
        sprintf(error, "[WEBP 解码] 暂不支持解码动态 webp 图像");
        goto FinalizeAndExit;
    }
    config->count = 1;
    if (!ImgcAllocConfigImages(config))
    {
        sprintf(error, "[WEBP 解码] 无法为图像数据分配内存空间");
        goto FinalizeAndExit;
    }
    switch (config->clr)
    {
    case IMGC_CLR_RGB:
        config->images->clr = IMGC_CLR_RGB;
        wp_config.output.colorspace == MODE_RGB;
        break;
    case IMGC_CLR_RGBA:
        config->images->clr = IMGC_CLR_RGBA;
        wp_config.output.colorspace == MODE_RGBA;
        break;
    case IMGC_CLR_ANY:
        if (!input->has_alpha)
        {
            config->images->clr = IMGC_CLR_RGB;
            wp_config.output.colorspace = MODE_RGB;
        }
        else
        {
            config->images->clr = IMGC_CLR_RGBA;
            wp_config.output.colorspace = MODE_RGBA;
        }
        break;
    default:
        sprintf(error, "[WEBP 解码] 不支持指定的颜色空间");
        goto FinalizeAndExit;
    }
    result = VP8_STATUS_OK == WebPDecode(data, data_size, &wp_config);
    if (!result)
    {
        sprintf(error, "[WEBP 解码] 对数据的解码操作失败");
        goto FinalizeAndExit;
    }
    if (output->u.RGBA.size > 0 && NULL != output->u.RGBA.rgba)
    {
        config->images->width = output->width;
        config->images->height = output->height;
        config->images->stride = output->u.RGBA.stride;
        if (!ImgcAllocConfigImagesRgba(config))
        {
            sprintf(error, "[WEBP 解码] 无法为图像数据分配内存空间");
            goto FinalizeAndExit;
        }
        memcpy(config->images->rgba, output->u.RGBA.rgba, output->u.RGBA.size);
        result = 1;
    }
FinalizeAndExit:
    free(data);
    WebPFreeDecBuffer(output);
    return result;
}

int DecodeGif(imgcconfig_t *config, FILE *file_ptr, char *error)
{
    int result = 0;
    sprintf(error, "[GIF 解码] 暂未实现功能");
    if (NULL == config || NULL == file_ptr)
    {
        return result;
    }
FinalizeAndExit:
    return result;
}
