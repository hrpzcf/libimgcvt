#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "imgdecode.h"
#include "imgencode.h"

imgcconfig_t *ImgcCreateConfig(void)
{
    return malloc(sizeof(imgcconfig_t));
}

/// @brief 仅用于释放 ImgcCreateConfig 的返回值，否则请勿调用此函数
/// @param config 释放由 ImgcCreateConfig 函数分配的 imgcconfig_t 内存块
void ImgcDeleteConfig(imgcconfig_t *config)
{
    ImgcFreeConfig(config);
    free(config);
}

int ImgcInitConfig(imgcconfig_t *config)
{
    if (NULL != config)
    {
        memset(config, 0, sizeof(imgcconfig_t));
        config->level = -1;
        config->quality = 80;
        return 1;
    }
    return 0;
}

int ImgcAllocConfigImages(imgcconfig_t *config)
{
    if (NULL != config && config->count > 0)
    {
        config->images = calloc(config->count, sizeof(imgcimage_t));
        return NULL != config->images;
    }
    return 0;
}

int ImgcAllocConfigImagesRgba(imgcconfig_t *config)
{
    if (NULL != config && config->count > 0)
    {
        for (size_t i = 0; i < config->count; ++i)
        {
            config->images[i].rgba = malloc(config->images[i].stride *
                                            config->images[i].height);
            if (NULL == config->images[i].rgba)
            {
                goto RollBackAndExit;
            }
        }
        return 1;
    RollBackAndExit:
        for (size_t j = 0; j < config->count && NULL != config->images[j].rgba; ++j)
        {
            free(config->images[j].rgba);
            config->images[j].rgba = NULL;
        }
    }
    return 0;
}

void ImgcFreeConfig(imgcconfig_t *config)
{
    if (NULL != config && config->count > 0 && NULL != config->images)
    {
        for (size_t i = 0; i < config->count; ++i)
        {
            if (NULL != config->images[i].rgba)
            {
                free(config->images[i].rgba);
                config->images[i].rgba = NULL;
            }
        }
        free(config->images);
        config->count = 0;
        config->images = NULL;
    }
}

int ImgcExpandGrayScale(imgcconfig_t *config)
{
    size_t pixel_count;
    uint8_t gray_scale, *origin_rgba, *new_rgba;
    int old_channels, new_channels;
    if (NULL != config && config->count > 0 && NULL != config->images)
    {
        for (int i = 0; i < config->count; ++i)
        {
            imgcimage_t *image = config->images + i;
            origin_rgba = image->rgba;
            pixel_count = image->width * image->height;
            switch (image->clr)
            {
            case IMGC_CLR_GSC:
            {
                old_channels = 1;
                new_channels = 3;
                if (NULL != (new_rgba = malloc(pixel_count * new_channels)))
                {
                    for (size_t j = 0; j < pixel_count; ++j)
                    {
                        new_rgba[j * new_channels + 0] = origin_rgba[j];
                        new_rgba[j * new_channels + 1] = origin_rgba[j];
                        new_rgba[j * new_channels + 2] = origin_rgba[j];
                    }
                    image->rgba = new_rgba;
                    image->clr = IMGC_CLR_RGB;
                    image->stride = new_channels * image->width;
                    free(origin_rgba);
                }
                break;
            }
            case IMGC_CLR_GSCA:
            {
                old_channels = 2;
                new_channels = 4;
                if (NULL != (new_rgba = malloc(pixel_count * new_channels)))
                {
                    for (size_t j = 0; j < pixel_count; ++j)
                    {
                        new_rgba[j * new_channels + 0] = origin_rgba[j * old_channels + 0];
                        new_rgba[j * new_channels + 1] = origin_rgba[j * old_channels + 0];
                        new_rgba[j * new_channels + 2] = origin_rgba[j * old_channels + 0];
                        new_rgba[j * new_channels + 3] = origin_rgba[j * old_channels + 1];
                    }
                    image->rgba = new_rgba;
                    image->clr = IMGC_CLR_RGBA;
                    image->stride = new_channels * image->width;
                    free(origin_rgba);
                }
                break;
            }
            default:
                continue;
            }
        }
        return 1;
    }
    return 0;
}

int ImgcAlphaBlending(imgcconfig_t *config, uint8_t light)
{
    size_t pixel_count;
    int old_channels, new_channels;
    double light_part;
    double fg_r, fg_g, fg_b, fg_a;
    double mx_r, mx_g, mx_b;
    uint8_t *origin_rgba, *mixed_rgb;
    if (NULL != config && config->count > 0 && NULL != config->images)
    {
        for (int i = 0; i < config->count; ++i)
        {
            imgcimage_t *image = config->images + i;
            origin_rgba = image->rgba;
            pixel_count = image->width * image->height;
            switch (image->clr)
            {
            case IMGC_CLR_GSCA:
            {
                old_channels = 2;
                new_channels = 1;
                if (NULL != (mixed_rgb = malloc(pixel_count)))
                {
                    for (size_t j = 0; j < pixel_count; ++j)
                    {
                        fg_g = origin_rgba[j * old_channels + 0];
                        fg_a = origin_rgba[j * old_channels + 1];
                        light_part = light * (255 - fg_a);
                        mx_g = fg_g * fg_a + light_part;
                        mixed_rgb[j] = (uint8_t)round(mx_g / 255);
                    }
                    image->rgba = mixed_rgb;
                    image->clr = IMGC_CLR_GSC;
                    image->stride = image->width * new_channels;
                    free(origin_rgba);
                }
                break;
            }
            case IMGC_CLR_RGBA:
            {
                old_channels = 4;
                new_channels = 3;
                if (NULL != (mixed_rgb = malloc(pixel_count * new_channels)))
                {
                    for (size_t j = 0; j < pixel_count; ++j)
                    {
                        fg_r = origin_rgba[j * old_channels + 0];
                        fg_g = origin_rgba[j * old_channels + 1];
                        fg_b = origin_rgba[j * old_channels + 2];
                        fg_a = origin_rgba[j * old_channels + 3];
                        light_part = light * (255 - fg_a);
                        mx_r = fg_r * fg_a + light_part;
                        mx_g = fg_g * fg_a + light_part;
                        mx_b = fg_b * fg_a + light_part;
                        mixed_rgb[j * new_channels + 0] = (uint8_t)round(mx_r / 255);
                        mixed_rgb[j * new_channels + 1] = (uint8_t)round(mx_g / 255);
                        mixed_rgb[j * new_channels + 2] = (uint8_t)round(mx_b / 255);
                    }
                    image->rgba = mixed_rgb;
                    image->clr = IMGC_CLR_RGB;
                    image->stride = image->width * new_channels;
                    free(origin_rgba);
                }
                break;
            }
            default:
                continue;
            }
        }
        return 1;
    }
FinalizeAndExit:
    return 0;
}

int ImgcGetFileData(const char *in_file, uint8_t **out_data, size_t *data_size)
{
    int result = 0;
    FILE *file_pointer = fopen(in_file, "rb");
    if (NULL == file_pointer)
    {
        return result;
    }
    fseek(file_pointer, 0, SEEK_END);
    *data_size = ftell(file_pointer);
    if (0 == *data_size)
    {
        goto FinalizeAndExit;
    }
    if (NULL == (*out_data = malloc(*data_size)))
    {
        *data_size = 0;
        goto FinalizeAndExit;
    }
    fseek(file_pointer, 0, SEEK_SET);
    if ((fread(*out_data, sizeof(uint8_t), *data_size, file_pointer)) != *data_size)
    {
        *data_size = 0;
        free(*out_data);
        goto FinalizeAndExit;
    }
    result = 1;
FinalizeAndExit:
    if (NULL != file_pointer)
    {
        fclose(file_pointer);
    }
    return result;
}

int ImgcDecodeFile(const char *in_file, imgcfmt_t in_fmt, imgcconfig_t *config, char *error)
{
    int result = 0;
    FILE *file_pointer = NULL;
    if (NULL == in_file || NULL == config)
    {
        sprintf(error, "[解码] 文件路径或解码配置为空");
        return result;
    }
    if (in_fmt < IMGC_FMT_JPG || in_fmt > IMGC_FMT_GIF)
    {
        sprintf(error, "[解码] 文件格式参数超出范围");
        return result;
    }
    if (NULL == (file_pointer = fopen(in_file, "rb")))
    {
        sprintf(error, "[解码] 无法打开指定路径的文件");
        return result;
    }
    if (config->level == -2)
    {
        long long file_size;
        if (0 != _fseeki64(file_pointer, 0, SEEK_END) ||
            -1 == (file_size = _ftelli64(file_pointer)) ||
            0 != _fseeki64(file_pointer, 0, SEEK_SET))
        {
            sprintf(error, "[解码] 无法获知输入文件的大小");
            goto FinalizeAndExit;
        }
        if (file_size > INT_MAX)
        {
            sprintf(error, "[解码] 输入文件过大，无法作为输出大小目标");
            goto FinalizeAndExit;
        }
        config->quality = (int)file_size;
    }
    switch (in_fmt)
    {
    case IMGC_FMT_JPG:
        result = DecodeJpg(config, file_pointer, error);
        break;
    case IMGC_FMT_PNG:
        result = DecodePng(config, file_pointer, error);
        break;
    case IMGC_FMT_WEBP:
        result = DecodeWebp(config, file_pointer, error);
        break;
    case IMGC_FMT_GIF:
        result = DecodeGif(config, file_pointer, error);
        break;
    }
FinalizeAndExit:
    if (NULL != file_pointer)
    {
        fclose(file_pointer);
    }
    return result;
}

int ImgcEncodeFile(const char *out_file, imgcfmt_t out_fmt, imgcconfig_t *config, char *error)
{
    int result = 0;
    FILE *file_pointer = NULL;
    if (NULL == out_file || NULL == config)
    {
        sprintf(error, "[编码] 文件路径或编码配置为空");
        return result;
    }
    if (out_fmt < IMGC_FMT_JPG || out_fmt > IMGC_FMT_GIF)
    {
        sprintf(error, "[编码] 文件格式参数超出范围");
        return result;
    }
    if (NULL == (file_pointer = fopen(out_file, "wb")))
    {
        sprintf(error, "[编码] 无法在指定路径创建文件");
        return result;
    }
    switch (out_fmt)
    {
    case IMGC_FMT_JPG:
        result = EncodeJpg(config, file_pointer, error);
        break;
    case IMGC_FMT_PNG:
        result = EncodePng(config, file_pointer, error);
        break;
    case IMGC_FMT_WEBP:
        result = EncodeWebp(config, file_pointer, error);
        break;
    case IMGC_FMT_GIF:
        result = EncodeGif(config, file_pointer, error);
        break;
    }
FinalizeAndExit:
    if (NULL != file_pointer)
    {
        fclose(file_pointer);
    }
    return result;
}
