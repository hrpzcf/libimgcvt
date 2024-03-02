#include <stdio.h>
#include <stdlib.h>

#include "libimagecvt.h"

/// @brief 把指定格式的输入图片转换为指定格式的输出图片
/// @param in_file 输入文件的完整路径
/// @param in_fmt 输出文件类型，具体请参考 imgcfmt_t 枚举
/// @param out_file 输出文件的完整路径
/// @param out_fmt 输出文件类型，具体请参考 imgcfmt_t 枚举
/// @param fit_dyn 当输入文件是动图时，自行把输出格式改为支持动图的格式
/// @param level 输出文件的压缩方式或级别，见 EncodeWebp 函数
/// @param quality 输出文件的图像质量，输出 JPEG 时、输出 WEBP 且 level=-1 时有效
/// @param error 转换失败时向此缓冲器写入 UTF-8 编码的失败原因
/// @return 成功返回非 0，失败返回 0
int ConvertImageFormat(const char *in_file, imgcfmt_t in_fmt, const char *out_file,
                       imgcfmt_t out_fmt, int fit_dyn, int level, int quality, char *error)
{
    int result = 0;
    imgcconfig_t imgc_config;
    if (NULL == error)
    {
        return result;
    }
    if (NULL == in_file || NULL == out_file)
    {
        sprintf(error, "[转换] 输入或输出文件路径为空");
        return result;
    }
    if (in_fmt < IMGC_FMT_JPG || in_fmt > IMGC_FMT_GIF)
    {
        sprintf(error, "[转换] 输入文件格式参数不正确");
        return result;
    }
    if (out_fmt < IMGC_FMT_JPG || out_fmt > IMGC_FMT_GIF)
    {
        sprintf(error, "[转换] 输出文件格式参数不正确");
        return result;
    }
    if (in_fmt == out_fmt && quality >= 100)
    {
        sprintf(error, "[转换] 质量 100 时输入和输出文件格式参数不能相同");
        return result;
    }
    if (level < -2 || level > 2)
    {
        sprintf(error, "[转换] 无效的压缩级别参数");
        return result;
    }
    if (!ImgcInitConfig(&imgc_config))
    {
        sprintf(error, "[转换] 初始化图像转换配置失败");
        return result;
    }
    imgc_config.level = level;
    imgc_config.quality = quality;
    if (!(result = ImgcDecodeFile(in_file, in_fmt, &imgc_config, error)))
    {
        goto FinalizeAndExit;
    }
    if (fit_dyn && imgc_config.count > 1)
    {
        out_fmt == in_fmt == IMGC_FMT_GIF ? IMGC_FMT_WEBP : IMGC_FMT_GIF;
    }
    if (!(result = ImgcEncodeFile(out_file, out_fmt, &imgc_config, error)))
    {
        goto FinalizeAndExit;
    }
FinalizeAndExit:
    ImgcFreeConfig(&imgc_config);
    return result;
}
