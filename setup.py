from distutils.core import setup, Extension

u94 = Extension('u94',
                sources=['src/u94.c', 'u94/u94-py.c'],
                include_dirs=['src'])

setup(name='u94',
      version='0.1',
      description='u94 encoder and decoder',
      ext_modules=[u94])
