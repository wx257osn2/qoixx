#ifndef QOIXX_HPP_INCLUDED_
#define QOIXX_HPP_INCLUDED_

#include<cstdint>
#include<cstddef>
#include<cstdlib>
#include<cstring>

#ifndef QOI_MALLOC
#define QOIXX_MALLOC_DEFAULT
#define QOI_MALLOC(sz) malloc(sz)
#define QOI_FREE(p)    free(p)
#endif

namespace qoixx{

struct qoi{
  enum class colorspace : std::uint8_t{
    srgb = 0,
    linear = 1,
  };
  struct desc{
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t channels;
    qoi::colorspace colorspace;
  };
  enum chunk_tag : std::uint32_t{
    index = 0x00u,
    diff  = 0x40u,
    luma  = 0x80u,
    run   = 0xc0u,
    rgb   = 0xfeu,
    rgba  = 0xffu,
  };
  struct rgba_t{
    std::uint8_t r, g, b, a;
    std::uint32_t v()const{
      static_assert(sizeof(rgba_t) == sizeof(std::uint32_t));
      std::uint32_t ret;
      std::memcpy(&ret, this, sizeof(std::uint32_t));
      return ret;
    }
    constexpr auto hash()const{
      return r*3 + g*5 + b*7 + a*11;
    }
    bool operator==(const rgba_t& rhs)const{
      return v() == rhs.v();
    }
    bool operator!=(const rgba_t& rhs)const{
      return v() != rhs.v();
    }
  };
  static constexpr std::uint32_t magic = 
    static_cast<std::uint32_t>('q') << 24 |
    static_cast<std::uint32_t>('o') << 16 |
    static_cast<std::uint32_t>('i') <<  8 |
    static_cast<std::uint32_t>('f');
  static constexpr std::size_t header_size =
    sizeof(magic) +
    sizeof(std::declval<desc>().width) +
    sizeof(std::declval<desc>().height) +
    sizeof(std::declval<desc>().channels) +
    sizeof(std::declval<desc>().colorspace);
  static constexpr std::size_t pixels_max = 400000000u;
  static constexpr std::uint8_t padding[8] = {0, 0, 0, 0, 0, 0, 0, 1};
  static constexpr std::uint32_t read_32(const std::uint8_t* ptr, std::size_t& index){
    const auto _1 = ptr[index++];
    const auto _2 = ptr[index++];
    const auto _3 = ptr[index++];
    const auto _4 = ptr[index++];
    return static_cast<std::uint32_t>(_1 << 24 | _2 << 16 | _3 << 8 | _4);
  }
  static constexpr void write_32(std::uint8_t* ptr, std::size_t& index, std::uint32_t value){
    ptr[index++] = (value & 0xff000000) >> 24;
    ptr[index++] = (value & 0x00ff0000) >> 16;
    ptr[index++] = (value & 0x0000ff00) >> 8;
    ptr[index++] =  value & 0x000000ff;
  }

  static void* encode(const std::uint8_t* pixels, const desc& desc, std::size_t *out_len){
    if(pixels == NULL || out_len == NULL || desc.width == 0 || desc.height == 0 || desc.channels < 3 || desc.channels > 4 || desc.height >= pixels_max / desc.width)
      return NULL;

    const auto max_size = static_cast<std::size_t>(desc.width) * desc.height * (desc.channels + 1) + header_size + sizeof(padding);
    std::size_t p = 0;
    std::uint8_t* bytes = static_cast<std::uint8_t*>(QOI_MALLOC(max_size));
    if(!bytes)
      return NULL;

    write_32(bytes, p, magic);
    write_32(bytes, p, desc.width);
    write_32(bytes, p, desc.height);
    bytes[p++] = desc.channels;
    bytes[p++] = static_cast<std::uint8_t>(desc.colorspace);

    rgba_t index[64] = {};

    unsigned int run = 0;
    rgba_t px_prev = {0, 0, 0, 255};
    rgba_t px = px_prev;

    const std::size_t px_len = desc.width * desc.height * desc.channels;
    for(std::size_t px_pos = 0; px_pos < px_len;){
      if(desc.channels == 4){
        std::memcpy(&px, pixels+px_pos, sizeof(rgba_t));
        px_pos += sizeof(rgba_t);
      }
      else{
        px.r = pixels[px_pos++];
        px.g = pixels[px_pos++];
        px.b = pixels[px_pos++];
      }

      if(px == px_prev){
        ++run;
        if(run == 62 || px_pos == px_len){
          bytes[p++] = chunk_tag::run | (run-1);
          run = 0;
        }
      }
      else{
        if(run > 0){
          bytes[p++] = chunk_tag::run | (run-1);
          run = 0;
        }

        const auto index_pos = px.hash() % 64;

        if(index[index_pos] == px)
          bytes[p++] = chunk_tag::index | index_pos;
        else{
          index[index_pos] = px;

          if(px.a == px_prev.a){
            const auto vr = static_cast<int>(px.r) - static_cast<int>(px_prev.r);
            const auto vg = static_cast<int>(px.g) - static_cast<int>(px_prev.g);
            const auto vb = static_cast<int>(px.b) - static_cast<int>(px_prev.b);

            const auto vg_r = vr - vg;
            const auto vg_b = vb - vg;

            if(
              (256-3 < vr || (-3 < vr && vr < 2)) &&
              (256-3 < vg || (-3 < vg && vg < 2)) &&
              (256-3 < vb || (-3 < vb && vb < 2))
            )
              bytes[p++] = static_cast<std::uint8_t>(chunk_tag::diff | (vr+2) << 4 | (vg+2) << 2 | (vb+2));
            else if(
              -9  < vg_r && vg_r < 8  &&
              (256-33 < vg || (-33 < vg && vg < 32)) &&
              -9  < vg_b && vg_b < 8
            ){
              bytes[p++] = chunk_tag::luma | (vg+32);
              bytes[p++] = (vg_r+8) << 4 | (vg_b+8);
            }
            else{
              bytes[p++] = chunk_tag::rgb;
              bytes[p++] = px.r;
              bytes[p++] = px.g;
              bytes[p++] = px.b;
            }
          }
          else{
            bytes[p++] = chunk_tag::rgba;
            bytes[p++] = px.r;
            bytes[p++] = px.g;
            bytes[p++] = px.b;
            bytes[p++] = px.a;
          }
        }
      }
      px_prev = px;
    }

    std::memcpy(bytes + p, padding, sizeof(padding));
    p += sizeof(padding);

    *out_len = p;
    return bytes;
  }

  static void *decode(const std::uint8_t* bytes, std::size_t size, desc& desc, int channels){
    if((channels != 0 && channels != 3 && channels != 4) || size < header_size + sizeof(padding) || bytes == nullptr)
      return NULL;

    std::size_t p = 0;

    const auto magic_ = read_32(bytes, p);
    desc.width = read_32(bytes, p);
    desc.height = read_32(bytes, p);
    desc.channels = bytes[p++];
    desc.colorspace = static_cast<qoi::colorspace>(bytes[p++]);

    if(
      desc.width == 0 || desc.height == 0 || magic_ != magic || 
      desc.channels < 3 || desc.channels > 4 ||
      desc.height >= pixels_max / desc.width
    )
      return NULL;

    if(channels == 0)
      channels = desc.channels;

    const std::size_t px_len = static_cast<std::size_t>(desc.width) * desc.height * channels;
    std::uint8_t* pixels = static_cast<std::uint8_t*>(QOI_MALLOC(px_len));
    if(!pixels)
      return NULL;

    rgba_t px = {0, 0, 0, 255};
    rgba_t index[64] = {};

    unsigned int run = 0;
    const std::size_t chunks_len = size - sizeof(padding);
    for(std::size_t px_pos = 0; px_pos < px_len; px_pos += channels){
      if(run > 0)
        --run;
      else if(p < chunks_len){
        static constexpr std::uint8_t mask_2 = 0xc0;
        const auto b1 = bytes[p++];

        if(b1 == chunk_tag::rgb){
          px.r = bytes[p++];
          px.g = bytes[p++];
          px.b = bytes[p++];
        }
        else if(b1 == chunk_tag::rgba){
          px.r = bytes[p++];
          px.g = bytes[p++];
          px.b = bytes[p++];
          px.a = bytes[p++];
        }
        else if((b1 & mask_2) == chunk_tag::index)
          px = index[b1];
        else if((b1 & mask_2) == chunk_tag::diff){
          px.r += ((b1 >> 4) & 0x03) - 2;
          px.g += ((b1 >> 2) & 0x03) - 2;
          px.b += ( b1       & 0x03) - 2;
        }
        else if((b1 & mask_2) == chunk_tag::luma){
          const auto b2 = bytes[p++];
          const int vg = (b1 & 0x3f) - 32;
          px.r += vg - 8 + ((b2 >> 4) & 0x0f);
          px.g += vg;
          px.b += vg - 8 + ( b2       & 0x0f);
        }
        else if((b1 & mask_2) == chunk_tag::run)
          run = b1 & 0x3f;

        index[px.hash() % 64] = px;
      }

      if(channels == 4)
        std::memcpy(pixels+px_pos, &px, sizeof(rgba_t));
      else{
        pixels[px_pos] = px.r;
        pixels[px_pos+1] = px.g;
        pixels[px_pos+2] = px.b;
      }
    }

    return pixels;
  }
};

}

#ifdef QOIXX_MALLOC_DEFAULT
#undef QOI_FREE
#undef QOI_MALLOC
#undef QOIXX_MALLOC_DEFAULT
#endif

#endif //QOIXX_HPP_INCLUDED_
