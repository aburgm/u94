#include "u94.h"
#include <assert.h>

#define FETCH_N(N) \
    assert(N <= 8); \
    if(boff > N) { \
        num = bytes[bnum] >> (boff - N); \
        boff -= N; \
    } \
    else { \
        num = ((bytes[bnum] & ((1 << boff) - 1)) << (N - boff)) | (bytes[bnum + 1] >> (8 + boff - N)); \
        boff += 8 - N; \
        ++bnum; \
    }

#define WRITE_N(N, X) \
    assert(N <= 8); \
    assert(X < (1 << N)); \
    if (boff > N) { \
        out[bnum] = (out[bnum] & ~((1 << boff) - 1)) | (X << (boff - N)); \
        boff -= N; \
    } \
    else { \
        out[bnum] = (out[bnum] & ~((1 << boff) - 1)) | (X >> (N - boff)); \
        out[bnum + 1] = (X & ((1 << (N - boff)) - 1)) << (8 + boff - N); \
        boff += 8 - N; \
        ++bnum; \
    }

#define ENCODE_NEXT \
    FETCH_N(7) \
    assert (num <= 0x80); \
    if (num >= 0x20 || num == '\n' || num == '\r') { \
        *out++ = num; \
    } \
    else { \
        assert(num != '\r'); \
        assert(num != '\n'); \
        if (num < '\n') ++num; \
        if (num < '\r') ++num; \
        assert(num < 0x20); \
        assert(num >= 2); \
        *out++ = num | 0b11000000; \
        FETCH_N(6) \
        *out++ = num | 0b10000000; \
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

#define DECODE_NEXT(N7, N6) \
    assert(N7 > 0); \
    if ((*pos & 0x80) == 0) { \
        unsigned int num = *pos >> (7 - N7); \
        WRITE_N(N7, num); \
        ++pos; \
    } \
    else { \
        if (pos + 1 >= end || (*pos & 0xe0) != 0xc0 || (*pos & 0b00011110) == 0) \
            return 0; \
        unsigned int num = (*pos) & 0x1f; \
        unsigned int num2 = (*(pos+1)) & 0x3f; \
        assert(num >= 2); \
        if (num <= '\r') --num; \
        if (num <= '\n') --num; \
        assert(num != '\r'); \
        assert(num != '\n'); \
        assert(num < 0x20); \
        num >>= 7 - N7; \
        num2 >>= 6 - N6; \
        WRITE_N(N7, num); \
        if(N6 > 0) { \
            WRITE_N(N6, num2); \
        } \
        pos += 2; \
    }

/* TODO: streaming interface */

/* The decoder can be called with untrusted input.
 * It returns 0 if the byte sequence is not correctly
 * encoded u94 or invalid UTF-8. */

size_t u94dec(unsigned char* out, size_t outlen, const unsigned char* bytes, size_t len)
{
    assert(outlen >= 2);

    const unsigned char* end = bytes + len;
    const unsigned char* pos = bytes;

    unsigned int bnum = 0;
    unsigned int boff = 8;

    /* outlen must be known for this to work. Can't reliably
     * detect end of stream otherwise... */

    while (bnum < outlen - 2 && pos != end) {
        DECODE_NEXT(7, 6)
    }

    while (bnum < outlen && pos != end) {
        unsigned int N7 = boff + 8*(outlen - bnum - 1);
        unsigned int N6 = 0;

        if (N7 > 7) {
            N6 = N7 - 7;
            N7 = 7;
            if (N6 > 6) N6 = 6;
        }

        DECODE_NEXT(N7, N6)
    }

    assert(pos == end);
    assert(bnum == outlen);
    assert(boff == 8);

    return bnum;
}

size_t u94enc(unsigned char* out, size_t outlen, const unsigned char* bytes, size_t len)
{
    assert(len >= 2);

    const unsigned char* const orig_out = out;
    unsigned int bnum = 0;
    unsigned int boff = 8;
    unsigned int num;

    while (bnum < len - 2) {
        ENCODE_NEXT
    }

    const unsigned char last[4] = { bytes[len - 2], bytes[len - 1], 0, 0 };

    bnum -= (len - 2);
    bytes = last;
    while (bnum < 2) {
        ENCODE_NEXT
    }

    return out - orig_out;
}
