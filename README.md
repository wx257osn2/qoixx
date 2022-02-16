# qoixx

qoixx is a high performance MIT licensed single-header [QOI](https://github.com/phoboslab/qoi) implementation written in C++20.

## Features

qoixx has the features below:

- single-header library
- MIT licensed
- no dependencies, except for the standard library and architecture-specific headers included with the common compilers
- extremely fast implementation
    - encoder: SIMD-based implementation
        - On x86_64, qoixx uses AVX2 if available
        - On ARMv8, qoixx uses ARM SIMD(NEON) if available
        - If not available, qoixx encoder runs without SIMD instructions (but the scalar implementation is still faster than the [original implementation](https://github.com/phoboslab/qoi))
        - When you don't want to use SIMD implementation or want to break the dependency to architecture-specific headers, you can use `QOIXX_NO_SIMD` macro. `QOIXX_NO_SIMD` forces qoixx encoder to use scalar implementation.
    - decoder: Optimized scalar implementation, averagely fast
        - With some input, [original implementation](https://github.com/phoboslab/qoi) is faster

## Performance

```
# Grand total for qoi benchmark suite, 20 iterations
#   https://qoiformat.org/benchmark/qoi_benchmark_suite.tar
        decode ms   encode ms   decode mpps   encode mpps   size kb    rate
## Intel Core i9-10900
qoi:       2.2226      2.5331       208.843       183.247       463   28.2%
qoixx:     1.3753      1.5134       337.518       306.706       463   28.2%

## AMD Ryzen7 2700X
qoi:       2.1826      2.7039       212.673       171.671       463   28.2%
qoixx:     1.6523      1.5448       280.928       300.470       463   28.2%

## AMD Ryzen9 3950X
qoi:       2.2756      2.5869       203.978       179.435       463   28.2%
qoixx:     1.5453      1.5821       300.381       293.402       463   28.2%

## Qualcomm Kyro 585
qoi:       2.5395      2.7791       182.785       167.027       463   28.2%
qoixx:     1.7685      1.8228       262.473       254.648       463   28.2%

## Apple M1 Max
qoi:       1.7461      2.1248       265.840       218.461       463   28.2%
qoixx:     1.1440      1.4430       405.750       321.683       463   28.2%
```

## License

[MIT](https://github.com/wx257osn2/qoixx/blob/master/LICENSE)

## FAQ

- Q. The decoder built with MSVC is not fast.
    - A. I know that. You can choose [MinGW-w64](https://www.mingw-w64.org/).
- Q. Can I port it to older C++?
    - A. Of course, you can fork this repository and port it to older C++.
         However, in this repository I never uses older C++.
