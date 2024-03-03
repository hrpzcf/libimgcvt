#include <stdio.h>

#include "imgencode.h"
#include "png.h"
#include "jpeglib.h"
#include "webp/encode.h"

extern void jerror_handler(j_common_ptr cinfo);

int EncodeJpg(imgcconfig_t *config, FILE *file_pointer, char *error)
{
    int result = 0;
    if (config->count < 1)
    {
        sprintf(error, "[JPEG 编码] 没有图像数据");
        return result;
    }
    else if (config->count > 1)
    {
        sprintf(error, "[JPEG 编码]  JPEG 不支持动态图像");
        return result;
    }
    struct jpeg_compress_struct jinfo;
    struct jpeg_error_mgr jerr;
    jinfo.err = jpeg_std_error(&jerr);
    jinfo.err->error_exit = jerror_handler;
    jpeg_create_compress(&jinfo);
    jinfo.image_width = config->images->width;
    jinfo.image_height = config->images->height;
    switch (config->images->clr)
    {
    case IMGC_CLR_GSC:
        jinfo.in_color_space = JCS_GRAYSCALE;
        break;
    case IMGC_CLR_GSCA:
        if (!ImgcAlphaBlending(config, 0xFF))
        {
            sprintf(error, "[JPEG 编码] 无法对数据进行透明混合");
            return result;
        }
        jinfo.in_color_space = JCS_GRAYSCALE;
        break;
    case IMGC_CLR_RGB:
        jinfo.in_color_space = JCS_RGB;
        break;
    case IMGC_CLR_RGBA:
        if (!ImgcAlphaBlending(config, 0xFF))
        {
            sprintf(error, "[JPEG 编码] 无法对数据进行透明混合");
            return result;
        }
        jinfo.in_color_space = JCS_RGB;
        break;
    default:
        sprintf(error, "[JPEG 编码] 不支持的颜色空间");
        return result;
    }
    jinfo.num_components = config->images->stride /
                           config->images->width;
    jinfo.input_components = jinfo.num_components;
    jpeg_stdio_dest(&jinfo, file_pointer);
    jpeg_set_defaults(&jinfo);
    jpeg_set_quality(&jinfo, config->quality, TRUE);
    jpeg_start_compress(&jinfo, TRUE);
    JSAMPROW row_pointer = config->images->rgba;
    while (jinfo.next_scanline < jinfo.image_height)
    {
        if (jpeg_write_scanlines(&jinfo, &row_pointer, 1) != 1)
        {
            sprintf(error, "[JPEG 编码] 将数据编码为 JPEG 图像时出错");
            goto FinalizeAndExit;
        }
        row_pointer += config->images->stride;
    }
    result = 1;
FinalizeAndExit:
    jpeg_finish_compress(&jinfo);
    jpeg_destroy_compress(&jinfo);
    return result;
}

int EncodePng(imgcconfig_t *config, FILE *file_pointer, char *error)
{
    int result = 0;
    png_infop info_pointer = NULL;
    png_structp png_pointer = NULL;
    if (config->count < 1)
    {
        sprintf(error, "[PNG 编码] 没有图像数据");
        return result;
    }
    else if (config->count > 1)
    {
        sprintf(error, "[PNG 编码] 不支持编码动态 PNG 图像");
        return result;
    }
    int png_color_type;
    switch (config->images->clr)
    {
    case IMGC_CLR_GSC:
        png_color_type = PNG_COLOR_TYPE_GRAY;
        break;
    case IMGC_CLR_GSCA:
        png_color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        break;
    case IMGC_CLR_RGB:
        png_color_type = PNG_COLOR_TYPE_RGB;
        break;
    case IMGC_CLR_RGBA:
        png_color_type = PNG_COLOR_TYPE_RGBA;
        break;
    default:
        sprintf(error, "[PNG 编码] 不支持的颜色空间");
        return result;
    }
    png_pointer = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                          NULL, NULL, NULL);
    if (NULL == png_pointer)
    {
        sprintf(error, "编码 PNG 文件时无法创建写入结构");
        return result;
    }
    if ((info_pointer = png_create_info_struct(png_pointer)) == NULL)
    {
        sprintf(error, "编码 PNG 文件时无法创建信息结构");
        goto FinalizeAndExit;
    }
    if (setjmp(png_jmpbuf(png_pointer)))
    {
        goto FinalizeAndExit;
    }
    png_init_io(png_pointer, file_pointer);
    png_set_IHDR(png_pointer, info_pointer,
                 config->images->width,
                 config->images->height,
                 8, png_color_type,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_pointer, info_pointer);
    png_bytep row_pointer = config->images->rgba;
    for (int row_number = 0; row_number < config->images->height; ++row_number)
    {
        png_write_row(png_pointer, row_pointer);
        row_pointer += config->images->stride;
    }
    png_write_end(png_pointer, info_pointer);
    result = 1;
FinalizeAndExit:
    png_destroy_write_struct(&png_pointer, &info_pointer);
    return result;
}

static int FileWriter(const uint8_t *data, size_t data_size,
                      const WebPPicture *const pic)
{
    return data_size ? (fwrite(data, data_size, 1, pic->custom_ptr) == 1) : 1;
}

int EncodeWebp(imgcconfig_t *config, FILE *file_pointer, char *error)
{
    int result = 0;
    WebPConfig wp_config;
    WebPPicture wp_picture;
    if (config->count < 1)
    {
        sprintf(error, "[WEBP 编码] 没有图像数据");
        return result;
    }
    else if (config->count > 1)
    {
        sprintf(error, "[WEBP 编码] 暂不支持编码动态 WEBP 图像");
        return result;
    }
    if (!WebPPictureInit(&wp_picture))
    {
        sprintf(error, "[WEBP 编码] 初始化图像结构失败");
        return result;
    }
    if (!WebPConfigInit(&wp_config))
    {
        sprintf(error, "[WEBP 编码] 初始化编码配置失败");
        return result;
    }
    wp_config.thread_level = 1;
    if (config->level == -2)
    {
        wp_picture.use_argb = 0;
        wp_config.target_size = config->quality;
    }
    else if (config->level == -1)
    {
        wp_picture.use_argb = 0;
        wp_config.quality = config->quality;
    }
    else if (config->level >= 0 && config->level <= 2)
    {
        int lossless;
        switch (config->level)
        {
        case 0:
            lossless = 2;
            break;
        default:
        case 1:
            lossless = 6;
            break;
        case 2:
            lossless = 9;
            break;
        }
        wp_picture.use_argb = 1;
        wp_config.near_lossless = 60;
        if (!WebPConfigLosslessPreset(&wp_config, lossless))
        {
            sprintf(error, "[WEBP 编码] 无法配置为预设的无损编码级别");
            goto FinalizeAndExit;
        }
    }
    else
    {
        sprintf(error, "[WEBP 编码] 无效的压缩方法：%d", config->level);
        goto FinalizeAndExit;
    }
    if (!WebPValidateConfig(&wp_config))
    {
        sprintf(error, "[WEBP 编码] 编码配置不正确");
        goto FinalizeAndExit;
    }
    wp_picture.custom_ptr = file_pointer;
    wp_picture.writer = FileWriter;
    wp_picture.width = config->images->width;
    wp_picture.height = config->images->height;
    switch (config->images->clr)
    {
    case IMGC_CLR_RGB:
        result = WebPPictureImportRGB(&wp_picture, config->images->rgba, config->images->stride);
        break;
    case IMGC_CLR_RGBA:
        result = WebPPictureImportRGBA(&wp_picture, config->images->rgba, config->images->stride);
        break;
    default:
        sprintf(error, "[WEBP 编码] 不支持的颜色空间");
        goto FinalizeAndExit;
    }
    if (!result)
    {
        sprintf(error, "[WEBP 编码] 导入已解码的图像数据失败");
        goto FinalizeAndExit;
    }
    if (!(result = WebPEncode(&wp_config, &wp_picture)))
    {
        sprintf(error, "[WEBP 编码] 对数据的编码过程出现错误");
    }
FinalizeAndExit:
    return result;
}

int EncodeGif(imgcconfig_t *config, FILE *file_pointer, char *error)
{
    int result = 0;
    sprintf(error, "[GIF 编码] 暂未实现功能");
FinalizeAndExit:
    return result;
}
