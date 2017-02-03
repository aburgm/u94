import sys
import os
sys.path.insert(0, 'build/lib.linux-x86_64-3.5')

import u94
import base64

long_seq = os.urandom(1000)
encoded_u94 = u94.encode(long_seq)
encoded_b64 = base64.b64encode(long_seq)

#print(u94.decode(5, b'\x00\x00\x00\x00\x00'))

def test_encode_u94():
    u94.encode(long_seq)

def test_decode_u94():
    u94.decode(1000, encoded_u94)

def test_encode_b64():
    base64.b64encode(long_seq)

def test_decode_b64():
    base64.b64decode(encoded_b64)

if __name__ == '__main__':
    import timeit
    print(timeit.timeit("test_encode_u94()", setup="from __main__ import test_encode_u94;"))
    print(timeit.timeit("test_encode_b64()", setup="from __main__ import test_encode_b64;"))

    print(timeit.timeit("test_decode_u94()", setup="from __main__ import test_decode_u94;"))
    print(timeit.timeit("test_decode_b64()", setup="from __main__ import test_decode_b64;"))
