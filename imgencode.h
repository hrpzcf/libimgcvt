#ifndef IMGENCODE_H_
#define IMGENCODE_H_

#include <stdio.h>
#include "libimagecvt.h"

int EncodeJpg(const imgcconfig_t *, FILE *, char *);
int EncodePng(const imgcconfig_t *, FILE *, char *);
int EncodeWebp(const imgcconfig_t *, FILE *, char *);
int EncodeGif(const imgcconfig_t *, FILE *, char *);

#endif // IMGENCODE_H_
