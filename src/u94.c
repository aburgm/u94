#include "u94.h"
#include <assert.h>

#define MIN(A,B) ((A) < (B) ? (A) : (B))

#define FETCH_N(N) \
    assert(N <= 8); \
    if(boff >= N) { \
        num = (bytes[bnum] >> (boff - N)) & ((1 << N) - 1); \
        boff = ((boff - N + 7) % 8) + 1; \
        bnum += (boff == 8); \
    } \
    else { \
        num = ((bytes[bnum] << (N - boff)) & ((1 << N) - 1)) | (bytes[bnum + 1] >> (8 + boff - N)); \
        boff += 8 - N; \
        ++bnum; \
    }

#define WRITE_N(N, X) \
    assert(N <= 8); \
    assert(X < (1 << N)); \
    if (boff >= N) { \
        out[bnum] = (out[bnum] & ~((1 << boff) - 1)) | (X << (boff - N)); \
        boff = ((boff - N + 7) % 8) + 1; \
        bnum += (boff == 8); \
    } \
    else { \
        out[bnum] = (out[bnum] & ~((1 << boff) - 1)) | (X >> (N - boff)); \
        out[bnum + 1] = (X & ((1 << (N - boff)) - 1)) << (8 + boff - N); \
        boff += 8 - N; \
        ++bnum; \
    }

/*
 * Note that in case N6 == 0, it doesn't matter
 * what we are padding the 6 bits with, so just
 * re-use `num`...
 */

#define ENCODE_NEXT(N7, M6, O1, P6) \
    assert(N7 > 0); \
    FETCH_N(N7) \
    assert (num < 0x80); \
    if (num >= 0x20 && num != '\"' && num != '\\') { \
	++A1; \
        *out++ = num; \
    } \
    else if (num < 30) { \
	++A2; \
        num += 2; \
        *out++ = num | 0b11000000; \
        if (M6 > 0) { \
          FETCH_N(M6) \
        } \
        *out++ = num | 0b10000000; \
    } \
    else { \
      ++A3; \
      unsigned int prenum; \
      switch (num) { \
      case 30: prenum = 0b11100010; break; \
      case 31: prenum = 0b11100110; break; \
      case '\"': prenum = 0b11101010; break; \
      case '\\': prenum = 0b11101110; break; \
      default: assert(0); break; \
      } \
      if (M6 > 0) { \
        FETCH_N((M6 + O1)); \
      } \
      *out++ = prenum | (num >> 6); \
      *out++ = 0b10000000 | (num & 0b00111111); \
      if (P6 > 0) { \
        FETCH_N(P6) \
      } \
      *out++ = 0b10000000 | (num & 0b00111111); \
    }

/*
 * Don't validate 1-byte sequences:
 * if (*pos < 0x20 && *pos != '\r' && *pos != '\n') return 0;
 * Costs about 15% decoding performance and does not make the
 * decoder vulnerable to untrusted input.
 * Just decodes some additional sequences that can not occur
 * in properly encoded u94. */

/* The if check in the 2-byte-sequence case:
 * It does not seem to have a large impact on performance. The
 * (pos + 1 >= end) part is needed for completely untrusted
 * input. If the caller makes sure that the input is valid UTF-8,
 * then it is not needed. */

#define DECODE_NEXT(N7, M6, O1, P6) \
    assert(N7 > 0); \
    if ((*pos & 0x80) == 0) { \
        unsigned int num = *pos; \
        WRITE_N(N7, num) \
        ++pos; \
    } \
    else if ((*pos & 0xe0) == 0xc0) { \
        if (pos + 1 >= end || (*pos & 0b00011110) == 0) \
            return 0; \
        unsigned int num = (*pos) & 0x1f; \
        unsigned int num2 = (*(pos+1)) & 0x3f; \
        assert(num >= 2); \
        num -= 2; \
        assert(num < 0x20); \
        WRITE_N(N7, num) \
        if(M6 > 0) { \
            WRITE_N(M6, num2) \
        } \
        pos += 2; \
    } \
    else { \
        if (pos + 2 >= end || (*pos & 0xf0) != 0xe0 || (*pos & 0b00000010) != 0b00000010) \
            return 0; \
        unsigned int num = ((*pos) & 0b00001100) >> 2; \
        unsigned int num2 = ((*pos & 0x01) << 6) | ((*(pos + 1)) & 0x3f); \
        unsigned int num3 = (*(pos + 2)) & 0x3f; \
        switch(num) { \
        case 0: num = 30; break; \
        case 1: num = 31; break; \
        case 2: num = '\"'; break; \
        case 3: num = '\\'; break; \
        default: assert(0); break; \
        } \
        WRITE_N(N7, num) \
        if (M6 > 0) { \
            WRITE_N((M6 + O1), num2) \
        } \
        if (P6 > 0) { \
            WRITE_N(P6, num3) \
        } \
        pos += 3; \
    }

#define GET_N7M6O1P6(len) \
    unsigned int rem = boff + 8*(len - bnum - 1); \
    unsigned int N7 = MIN(rem, 7); \
    unsigned int M6 = MIN(rem - MIN(7, rem), 6); \
    unsigned int O1 = MIN(rem - MIN(13, rem), 1); \
    unsigned int P6 = MIN(rem - MIN(14, rem), 6);

/* TODO: streaming interface */

/* The decoder can be called with untrusted input.
 * It returns 0 if the byte sequence is not correctly
 * encoded u94 or invalid UTF-8. */

size_t u94dec(unsigned char* out, size_t outlen, const unsigned char* bytes, size_t len)
{
    const unsigned char* end = bytes + len;
    const unsigned char* pos = bytes;

    unsigned int bnum = 0;
    unsigned int boff = 8;

    /* outlen must be known for this to work. Can't reliably
     * detect end of stream otherwise... */

    if (outlen > 3) {
        while (bnum < outlen - 3 && pos != end) {
            DECODE_NEXT(7, 6, 1, 6)
        }
    }

    while (bnum < outlen && pos != end) {
        GET_N7M6O1P6(outlen)
        DECODE_NEXT(N7, M6, O1, P6)
    }

    // Both streams should end at the same time or we got not properly encoded input data
    if (pos != end || bnum != outlen || boff != 8)
        return 0;

    return bnum;
}

size_t u94enc(unsigned char* out, size_t outlen, const unsigned char* bytes, size_t len)
{
    unsigned int A1 = 0, A2 = 0, A3 = 0;
    const unsigned char* const orig_out = out;
    unsigned int bnum = 0;
    unsigned int boff = 8;
    unsigned int num;

    if (len > 3) {
        while (bnum < len - 3) {
            ENCODE_NEXT(7, 6, 1, 6)
        }
    }
    while (bnum < len) {
        GET_N7M6O1P6(len)
        ENCODE_NEXT(N7, M6, O1, P6)
    }

    return out - orig_out;
}
