#ifndef LIBIMGCVT_H_
#define LIBIMGCVT_H_

#include <stdint.h>

typedef enum imgcclr
{
    IMGC_CLR_ANY,  // 解码图像时转为与以下颜色空间
    IMGC_CLR_RGB,  // 图像数据是 RGB 颜色空间
    IMGC_CLR_RGBA, // 图像数据是 RGBA 颜色空间
    IMGC_CLR_GSC,  // 图像数据是 GRAY 颜色空间
    IMGC_CLR_GSCA, // 图像数据是 GRAY ALPHA 颜色空间
} imgcclr_t;

typedef enum imgcfmt
{
    IMGC_FMT_JPG,
    IMGC_FMT_PNG,
    IMGC_FMT_WEBP,
    IMGC_FMT_GIF,
} imgcfmt_t;

typedef struct imgcimage
{
    int width;     // 图像的横向像素数
    int height;    // 图像的纵向像素数
    imgcclr_t clr; // rgba 的颜色空间类型
    int stride;    // rgba 每行字节数
    uint8_t *rgba; // 解码后的字节数据
} imgcimage_t;

typedef struct imgcconfig
{
    // WEBP 的压缩方式，初始化后默认值是 -1
    // -2 表示压缩图像时尽量使输出文件大小接近原文件
    // -1 表示使用有损压缩
    // 0 表示最弱最快无损压缩
    // 1 表示无损压缩中间值
    // 2 表示最强最慢无损压缩
    // 0 对应 webp 无损压缩级别 2，1 对应 6，2 对应 9
    int level;

    // 图像的输出质量，经过初始化后默认值是 80
    // 对于 JPEG 文件:
    // 1 最低质量，100 最佳质量
    // 对于 WEBP 文件：
    // level>=0: WEBP 无损压缩，0 更快更大，100 更慢更小
    // level=-1: WEBP 有损压缩，0 最差，100 最佳，典型值 75
    // level=-2: 压缩时尝试使输出文件接近此大小
    int quality;

    // 期望解码得到此颜色空间类型的数据，初始化后默认 0
    imgcclr_t clr;

    // 解码后 images 的元素数量，初始化后默认值是 0
    size_t count;

    // 解码得到的 imgcimage_t 指针，初始化后默认值 NULL
    imgcimage_t *images;
} imgcconfig_t;

imgcconfig_t *ImgcCreateConfig(void);
void ImgcDeleteConfig(imgcconfig_t *);
int ImgcInitConfig(imgcconfig_t *);
int ImgcAllocConfigImages(imgcconfig_t *config);
int ImgcAllocConfigImagesRgba(imgcconfig_t *config);
void ImgcFreeConfig(imgcconfig_t *);

int ImgcGetFileData(const char *, uint8_t **, size_t *);
int ImgcDecodeFile(const char *, imgcfmt_t, imgcconfig_t *, char *);
int ImgcEncodeFile(const char *, imgcfmt_t, imgcconfig_t *, char *);
int ImgcAlphaBlending(imgcconfig_t *, uint8_t);

int ConvertImageFormat(const char *, imgcfmt_t, const char *, imgcfmt_t, int,
                       int, int, char *);

#endif // LIBIMGCVT_H_
