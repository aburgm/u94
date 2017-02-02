
#ifndef _U94_U94_H
#define _U94_U94_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t u94dec(unsigned char* out,
              size_t outlen,
              const unsigned char* bytes,
              size_t len);

size_t u94enc(unsigned char* out,
              size_t outlen,
              const unsigned char* bytes,
              size_t len);

#ifdef __cplusplus
}
#endif

#endif
