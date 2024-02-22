#ifndef LIBWEBPCVT_H_
#define LIBWEBPCVT_H_

typedef enum
{
    WC_JPG,
    WC_PNG,
} outfmt_t;

int WebpConvertTo(const char *in_file, const char *out_file, outfmt_t fmt, int quality, int *reason);

#endif // LIBWEBPCVT_H_
