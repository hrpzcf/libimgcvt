#ifndef IMGDECODE_H_
#define IMGDECODE_H_

#include <stdint.h>
#include "libimagecvt.h"

int DecodeJpg(imgcconfig_t *, FILE *, char *);
int DecodePng(imgcconfig_t *, FILE *, char *);
int DecodeWebp(imgcconfig_t *, FILE *, char *);
int DecodeGif(imgcconfig_t *, FILE *, char *);

#endif // IMGDECODE_H_
