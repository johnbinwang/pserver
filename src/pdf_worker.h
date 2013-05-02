#ifndef PDF_WORKER_H 
#define PDF_WORKER_H
#ifdef  __cplusplus
extern "C" {
#endif
#include "fitz.h"
#include <wchar.h>
#include <string.h>

#define ARRAY_SIZE (1000)


int render(const char* path, unsigned char *data, int len, int zoom, int rotation);
#endif
#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
