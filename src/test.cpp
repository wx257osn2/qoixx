#include <qoixx.hpp>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

template<typename T, typename U>
static bool equals(const T& t, const U& u){
  const auto t_size = qoixx::container_operator<T>::size(t);
  {
    const auto u_size = qoixx::container_operator<U>::size(u);
    if(t_size != u_size)
      return false;
  }
  auto t_puller = qoixx::container_operator<T>::create_puller(t);
  auto u_puller = qoixx::container_operator<U>::create_puller(u);
  for(std::size_t i = 0; i < t_size; ++i)
    if(t_puller.pull() != u_puller.pull())
      return false;
  return true;
}

TEST_CASE("3-channel image"){
  constexpr qoixx::qoi::desc d{
    .width = 8,
    .height = 4,
    .channels = 3,
    .colorspace = qoixx::qoi::colorspace::srgb,
  };
  const std::vector<std::uint8_t> image = {
    130,   0, 212, 124, 204,  88,  79,  26, 210, 104, 117,   4, 137, 191,  80, 204,
     65, 175,  38, 160, 207, 182, 174,  59,  83,  18, 227,   4, 234, 150,  97, 131,
     62,  95, 167, 236, 132, 143,  78, 175,  86, 172, 237, 113, 195,  87, 227, 242,
     13, 189, 125,  33,  16,  79, 165, 247, 216, 193, 192, 113, 254, 176, 172, 227,
     94, 105, 146, 232, 150,  39, 148, 238, 105,  65,  23,   4,  33, 252, 243, 111,
    120,  32, 150, 144,  96,  66,   9, 102, 226, 245, 145, 153, 240, 183,  60, 132
  };
  const std::vector<std::uint8_t> expected = {
    113, 111, 105, 102,   0,   0,   0,   8,   0,   0,   0,   4,   3,   0, 254, 130,
      0, 212, 254, 124, 204,  88, 254,  79,  26, 210, 254, 104, 117,   4, 254, 137,
    191,  80, 254, 204,  65, 175, 254,  38, 160, 207, 254, 182, 174,  59, 254,  83,
     18, 227, 254,   4, 234, 150, 254,  97, 131,  62, 254,  95, 167, 236, 254, 132,
    143,  78, 254, 175,  86, 172, 254, 237, 113, 195, 254,  87, 227, 242, 254,  13,
    189, 125, 254,  33,  16,  79, 254, 165, 247, 216, 254, 193, 192, 113, 254, 254,
    176, 172, 254, 227,  94, 105, 254, 146, 232, 150, 254,  39, 148, 238, 254, 105,
     65,  23, 254,   4,  33, 252, 254, 243, 111, 120, 254,  32, 150, 144, 254,  96,
     66,   9, 254, 102, 226, 245, 254, 145, 153, 240, 254, 183,  60, 132,   0,   0,
      0,   0,   0,   0,   0,   1
  };
  REQUIRE(image.size() == d.width * d.height * d.channels);
  SUBCASE("encode std::vector<std::uint8_t>, output as std::vector<std::uint8_t>"){
    const auto actual = qoixx::qoi::encode<std::vector<std::uint8_t>>(image, d);
    CHECK(actual == expected);
  }
  SUBCASE("encode std::vector<std::uint8_t>, output as std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>"){
    const auto actual = qoixx::qoi::encode<std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>(image, d);
    CHECK(equals(actual, expected));
  }
  SUBCASE("encode std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>, output as std::vector<std::uint8_t>"){
    auto ptr = std::make_unique<std::uint8_t[]>(image.size());
    std::ranges::copy(image, ptr.get());
    const auto actual = qoixx::qoi::encode<std::vector<std::uint8_t>>(std::make_pair(std::move(ptr), image.size()), d);
    CHECK(equals(actual, expected));
  }
  SUBCASE("encode std::uint8_t*, output as std::vector<std::byte>"){
    const auto actual = qoixx::qoi::encode<std::vector<std::byte>>(image.data(), image.size(), d);
    CHECK(equals(actual, expected));
  }
  SUBCASE("decode std::vector<std::uint8_t>, output as std::vector<std::uint8_t>"){
    const auto [actual, desc] = qoixx::qoi::decode<std::vector<std::uint8_t>>(expected);
    CHECK(d == desc);
    CHECK(actual == image);
  }
  SUBCASE("decode std::vector<std::uint8_t>, output as std::pair<std::unique_ptr<std::uint8_t[], std::size_t>>"){
    const auto [actual, desc] = qoixx::qoi::decode<std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>(expected);
    CHECK(d == desc);
    CHECK(equals(actual, image));
  }
  SUBCASE("decode std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>, output as std::vector<std::uint8_t>"){
    auto ptr = std::make_unique<std::uint8_t[]>(expected.size());
    std::ranges::copy(expected, ptr.get());
    const auto [actual, desc] = qoixx::qoi::decode<std::vector<std::uint8_t>>(std::make_pair(std::move(ptr), expected.size()));
    CHECK(d == desc);
    CHECK(equals(actual, image));
  }
  SUBCASE("decode std::uint8_t*, output as std::vector<std::byte>"){
    const auto [actual, desc] = qoixx::qoi::decode<std::vector<std::byte>>(expected.data(), expected.size());
    CHECK(equals(actual, image));
  }
}

TEST_CASE("4-channel image"){
  constexpr qoixx::qoi::desc d{
    .width = 8,
    .height = 4,
    .channels = 4,
    .colorspace = qoixx::qoi::colorspace::srgb,
  };
  const std::vector<std::uint8_t> image = {
    227,  18,  59,  13, 114, 145, 116,  65, 253,  13,  51,   2,  59, 127, 119, 230,
    130,  55, 122,  13, 136, 141,  55, 200, 251, 177,  49, 112, 173, 183, 103,  36,
    222,  40,  87,  30, 158,  80,  63,  71, 237,  95, 111,  66, 197,  97, 199, 150,
      4, 219,  66, 124, 130, 119, 209, 109,  62, 184,  17, 197,   1, 158,  17, 144,
    213, 121, 105,  79, 135,  23, 237,  52,  36, 240, 218, 108, 203, 171, 129, 236,
      9, 124,  40,  10, 251,  87, 171, 127, 118, 254, 215, 136, 202, 241, 141, 111,
    252, 185,  28, 179,  57, 236, 121,  96,  11, 150, 167, 113, 154,  81, 167, 125,
    245,  28, 216, 181, 107,  14, 134,  46,  39,  19,  62,  59,  22, 253, 148,  38
  };
  const std::vector<std::uint8_t> expected = {
    113, 111, 105, 102,   0,   0,   0,   8,   0,   0,   0,   4,   4,   0, 255, 227,
     18,  59,  13, 255, 114, 145, 116,  65, 255, 253,  13,  51,   2, 255,  59, 127,
    119, 230, 255, 130,  55, 122,  13, 255, 136, 141,  55, 200, 255, 251, 177,  49,
    112, 255, 173, 183, 103,  36, 255, 222,  40,  87,  30, 255, 158,  80,  63,  71,
    255, 237,  95, 111,  66, 255, 197,  97, 199, 150, 255,   4, 219,  66, 124, 255,
    130, 119, 209, 109, 255,  62, 184,  17, 197, 255,   1, 158,  17, 144, 255, 213,
    121, 105,  79, 255, 135,  23, 237,  52, 255,  36, 240, 218, 108, 255, 203, 171,
    129, 236, 255,   9, 124,  40,  10, 255, 251,  87, 171, 127, 255, 118, 254, 215,
    136, 255, 202, 241, 141, 111, 255, 252, 185,  28, 179, 255,  57, 236, 121,  96,
    255,  11, 150, 167, 113, 255, 154,  81, 167, 125, 255, 245,  28, 216, 181, 255,
    107,  14, 134,  46, 255,  39,  19,  62,  59, 255,  22, 253, 148,  38,   0,   0,
      0,   0,   0,   0,   0,   1
  };
  REQUIRE(image.size() == d.width * d.height * d.channels);
  SUBCASE("encode std::vector<std::uint8_t>, output as std::vector<std::uint8_t>"){
    const auto actual = qoixx::qoi::encode<std::vector<std::uint8_t>>(image, d);
    CHECK(actual == expected);
  }
  SUBCASE("encode std::vector<std::uint8_t>, output as std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>"){
    const auto actual = qoixx::qoi::encode<std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>(image, d);
    CHECK(equals(actual, expected));
  }
  SUBCASE("encode std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>, output as std::vector<std::uint8_t>"){
    auto ptr = std::make_unique<std::uint8_t[]>(image.size());
    std::ranges::copy(image, ptr.get());
    const auto actual = qoixx::qoi::encode<std::vector<std::uint8_t>>(std::make_pair(std::move(ptr), image.size()), d);
    CHECK(equals(actual, expected));
  }
  SUBCASE("encode std::uint8_t*, output as std::vector<std::byte>"){
    const auto actual = qoixx::qoi::encode<std::vector<std::byte>>(image.data(), image.size(), d);
    CHECK(equals(actual, expected));
  }
  SUBCASE("decode std::vector<std::uint8_t>, output as std::vector<std::uint8_t>"){
    const auto [actual, desc] = qoixx::qoi::decode<std::vector<std::uint8_t>>(expected);
    CHECK(d == desc);
    CHECK(actual == image);
  }
  SUBCASE("decode std::vector<std::uint8_t>, output as std::pair<std::unique_ptr<std::uint8_t[], std::size_t>>"){
    const auto [actual, desc] = qoixx::qoi::decode<std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>(expected);
    CHECK(d == desc);
    CHECK(equals(actual, image));
  }
  SUBCASE("decode std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>, output as std::vector<std::uint8_t>"){
    auto ptr = std::make_unique<std::uint8_t[]>(expected.size());
    std::ranges::copy(expected, ptr.get());
    const auto [actual, desc] = qoixx::qoi::decode<std::vector<std::uint8_t>>(std::make_pair(std::move(ptr), expected.size()));
    CHECK(d == desc);
    CHECK(equals(actual, image));
  }
  SUBCASE("decode std::uint8_t*, output as std::vector<std::byte>"){
    const auto [actual, desc] = qoixx::qoi::decode<std::vector<std::byte>>(expected.data(), expected.size());
    CHECK(equals(actual, image));
  }
}
