#include <u94.h>

#include <stdio.h>

int main()
{
#define INSIZ 2
    unsigned char buf[INSIZ] = "ab";
    unsigned char outbuf[4];
    size_t siz = u94enc(outbuf, 4, buf, INSIZ);

    unsigned char decbuf[INSIZ];
    u94dec(decbuf, INSIZ, outbuf, siz);

    for (int i = 0; i < siz; ++i)
    {
      printf("%x ", outbuf[i]);
    }
    printf("\n");

    printf("%.*s\n", INSIZ, decbuf);
    return 0;
}
