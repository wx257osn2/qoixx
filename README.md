# qoixx

qoixx is a high performance MIT licensed single-header [QOI](https://github.com/phoboslab/qoi) implementation written in C++20.

[![Ubuntu](https://github.com/wx257osn2/qoixx/actions/workflows/linux.yml/badge.svg)](https://github.com/wx257osn2/qoixx/actions/workflows/linux.yml) [![Windows(msys2)](https://github.com/wx257osn2/qoixx/actions/workflows/windows.yml/badge.svg)](https://github.com/wx257osn2/qoixx/actions/workflows/windows.yml)

## Features

qoixx has the features below:

- single-header library
- MIT licensed
- no dependencies, except for the standard library and architecture-specific headers included with the common compilers
- extremely fast implementation
    - encoder: SIMD-based implementation, one of the fastest QOI encoder
        - On x86_64, qoixx uses AVX2 if available
        - On ARMv8 or later, qoixx uses
            - SVE if available
            - ARM SIMD(NEON) if SVE is not available
        - If not available, qoixx encoder runs without SIMD instructions (but the scalar implementation is still faster than the [original implementation](https://github.com/phoboslab/qoi))
        - When you don't want to use SIMD implementation or want to break the dependency to architecture-specific headers, you can use `QOIXX_NO_SIMD` macro. `QOIXX_NO_SIMD` forces qoixx encoder to use the scalar implementation.
    - decoder: Optimized scalar implementation, averagely fast
        - With some input, [original implementation](https://github.com/phoboslab/qoi) is faster
        - If the macro `QOIXX_DECODE_WITH_TABLES` is not 0, the decoder uses precalculated tables
            - In default, `QOIXX_DECODE_WITH_TABLES` is
                - `0` in aarch64
                - `1` in other envirnoment like x86
            - The default `QOIXX_DECODE_WITH_TABLES` value can be overridden.

## Performance

```
# Grand total for qoi benchmark suite, 20 iterations
#   https://qoiformat.org/benchmark/qoi_benchmark_suite.tar
        decode ms   encode ms   decode mpps   encode mpps   size kb    rate
## Intel Core i9-10900, Linux 5.11.0, g++ 11.1.0
qoi:       2.2389      2.5434       207.326       182.504       463   28.2%
qoixx:     1.3477      1.5255       344.431       304.280       463   28.2%

## AMD Ryzen7 2700X, Linux 5.4.0, g++ 11.0.0 20200622 (experimental)
qoi:       2.1203      2.7231       218.924       170.457       463   28.2%
qoixx:     1.4759      1.5711       314.504       295.444       463   28.2%

## AMD Ryzen7 3800X, Linux 5.4.0, g++ 12.0.0 20211219 (experimental)
qoi:       1.9747      2.5249       235.057       183.843       463   28.2%
qoixx:     1.4431      1.4380       321.643       322.785       463   28.2%

## AMD Ryzen9 3950X, Windows + MSYS2 ucrt64, g++ 11.2.0
qoi:       2.1692      2.5216       213.988       184.083       463   28.2%
qoixx:     1.3778      1.5473       336.891       299.991       463   28.2%

## AWS Graviton3, Linux 5.15.0, g++ 11.2.0
qoi:       1.9845      2.9169       233.897       159.134       463   28.2%
qoixx:     1.4420      1.7450       321.902       265.999       463   28.2%

## Qualcomm Kyro 585, Linux 4.19.125, g++ 11.1.0
qoi:       2.5313      2.7970       183.376       165.957       463   28.2%
qoixx:     1.6611      1.8253       279.444       254.306       463   28.2%

## Apple M1 Max, Mac, g++ 11.2.0
qoi:       1.7911      2.1635       259.155       214.545       463   28.2%
qoixx:     1.1020      1.4696       421.211       315.848       463   28.2%
```

## License

[MIT](https://github.com/wx257osn2/qoixx/blob/master/LICENSE)

## FAQ

- Q. The decoder built with MSVC is not fast.
    - A. I know that. You can choose [MinGW-w64](https://www.mingw-w64.org/).
- Q. Can I port it to older C++?
    - A. Of course, you can fork this repository and port it to older C++.
         However, in this repository I never uses older C++.
