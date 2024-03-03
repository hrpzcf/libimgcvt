#ifndef IMGENCODE_H_
#define IMGENCODE_H_

#include <stdio.h>
#include "libimagecvt.h"

int EncodeJpg(imgcconfig_t *, FILE *, char *);
int EncodePng(imgcconfig_t *, FILE *, char *);
int EncodeWebp(imgcconfig_t *, FILE *, char *);
int EncodeGif(imgcconfig_t *, FILE *, char *);

#endif // IMGENCODE_H_
