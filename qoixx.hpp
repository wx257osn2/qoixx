#ifndef QOIXX_HPP_INCLUDED_
#define QOIXX_HPP_INCLUDED_

#include<cstdint>
#include<cstddef>
#include<cstring>
#include<vector>
#include<type_traits>
#include<stdexcept>

namespace qoixx{

namespace detail{

template<typename T>
struct default_container_operator;

template<typename T, typename A>
requires(sizeof(T) == 1)
struct default_container_operator<std::vector<T, A>>{
  using target_type = std::vector<T, A>;
  static target_type construct(std::size_t size){
    target_type t(size);
    return t;
  }
  struct pusher{
    static constexpr bool is_contiguous = true;
    target_type* t;
    std::size_t i = 0;
    void push(std::uint8_t x)noexcept{
      (*t)[i++] = static_cast<T>(x);
    }
    target_type finalize()noexcept{
      t->resize(i);
      return std::move(*t);
    }
    std::uint8_t* raw_pointer()noexcept{
      return static_cast<std::uint8_t*>(t->data())+i;
    }
    void advance(std::size_t n)noexcept{
      i += n;
    }
  };
  static constexpr pusher create_pusher(target_type& t)noexcept{
    return {&t};
  }
  struct puller{
    static constexpr bool is_contiguous = true;
    const T* t;
    std::size_t i = 0;
    std::uint8_t pull()noexcept{
      return static_cast<std::uint8_t>(t[i++]);
    }
    const std::uint8_t* raw_pointer()noexcept{
      return static_cast<const std::uint8_t*>(t)+i;
    }
    void advance(std::size_t n)noexcept{
      i += n;
    }
    std::size_t count()const noexcept{
      return i;
    }
  };
  static constexpr puller create_puller(const target_type& t)noexcept{
    return {t.data()};
  }
  static std::size_t size(const target_type& t)noexcept{
    return t.size();
  }
  static constexpr bool valid(const target_type&)noexcept{
    return true;
  }
};

template<typename T>
requires(sizeof(T) == 1)
struct default_container_operator<std::pair<T*, std::size_t>>{
  using target_type = std::pair<T*, std::size_t>;
  struct puller{
    static constexpr bool is_contiguous = true;
    const T* ptr;
    std::size_t i = 0;
    std::uint8_t pull()noexcept{
      return static_cast<std::uint8_t>(ptr[i++]);
    }
    const std::uint8_t* raw_pointer()noexcept{
      return static_cast<const std::uint8_t*>(ptr)+i;
    }
    void advance(std::size_t n)noexcept{
      i += n;
    }
    std::size_t count()const noexcept{
      return i;
    }
  };
  static constexpr puller create_puller(const target_type& t)noexcept{
    return {t.first};
  }
  static std::size_t size(const target_type& t)noexcept{
    return t.second;
  }
  static bool valid(const target_type& t)noexcept{
    return t.first != nullptr;
  }
};

}

template<typename T>
struct container_operator : detail::default_container_operator<T>{};

class qoi{
  template<typename T>
  static void push(T& dst, const void* src, std::size_t size){
    if constexpr(T::is_contiguous){
      auto*const ptr = dst.raw_pointer();
      dst.advance(size);
      std::memcpy(ptr, src, size);
    }
    else{
      const auto* ptr = static_cast<const std::uint8_t*>(src);
      while(size --> 0)
        dst.push(*ptr++);
    }
  }
  template<typename T>
  static void pull(void* dst, T& src, std::size_t size){
    if constexpr(T::is_contiguous){
      const auto*const ptr = src.raw_pointer();
      src.advance(size);
      std::memcpy(dst, ptr, size);
    }
    else{
      auto* ptr = static_cast<std::uint8_t*>(dst);
      while(size --> 0)
        *ptr++ = src.pull();
    }
  }
  enum chunk_tag : std::uint32_t{
    index = 0b0000'0000u,
    diff  = 0b0100'0000u,
    luma  = 0b1000'0000u,
    run   = 0b1100'0000u,
    rgb   = 0b1111'1110u,
    rgba  = 0b1111'1111u,
  };
  static constexpr std::size_t index_size = 64u;
 public:
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
  struct rgba_t{
    std::uint8_t r, g, b, a;
    std::uint32_t v()const{
      static_assert(sizeof(rgba_t) == sizeof(std::uint32_t));
      std::uint32_t ret;
      std::memcpy(&ret, this, sizeof(std::uint32_t));
      return ret;
    }
    auto hash()const{
      static constexpr std::uint64_t constant =
        static_cast<std::uint64_t>(3u) << 56 |
                                   5u  << 16 |
        static_cast<std::uint64_t>(7u) << 40 |
                                  11u;
      const auto v = static_cast<std::uint64_t>(this->v());
      return (((v<<32|v)&0xFF00FF0000FF00FF)*constant)>>56;
    }
    bool operator==(const rgba_t& rhs)const{
      return v() == rhs.v();
    }
    bool operator!=(const rgba_t& rhs)const{
      return v() != rhs.v();
    }
  };
  static constexpr std::uint32_t magic = 
    113u /*q*/ << 24 | 111u /*o*/ << 16 | 105u /*i*/ <<  8 | 102u /*f*/ ;
  static constexpr std::size_t header_size =
    sizeof(magic) +
    sizeof(std::declval<desc>().width) +
    sizeof(std::declval<desc>().height) +
    sizeof(std::declval<desc>().channels) +
    sizeof(std::declval<desc>().colorspace);
  static constexpr std::size_t pixels_max = 400000000u;
  static constexpr std::uint8_t padding[8] = {0, 0, 0, 0, 0, 0, 0, 1};
  template<typename Puller>
  static constexpr std::uint32_t read_32(Puller& p){
    const auto _1 = p.pull();
    const auto _2 = p.pull();
    const auto _3 = p.pull();
    const auto _4 = p.pull();
    return static_cast<std::uint32_t>(_1 << 24 | _2 << 16 | _3 << 8 | _4);
  }
  template<typename Pusher>
  static constexpr void write_32(Pusher& p, std::uint32_t value){
    p.push((value & 0xff000000) >> 24);
    p.push((value & 0x00ff0000) >> 16);
    p.push((value & 0x0000ff00) >>  8);
    p.push( value & 0x000000ff       );
  }
 private:
  template<typename Pusher, typename Puller>
  static void encode_impl(Pusher& p, Puller& pixels, const desc& desc){
    write_32(p, magic);
    write_32(p, desc.width);
    write_32(p, desc.height);
    p.push(desc.channels);
    p.push(static_cast<std::uint8_t>(desc.colorspace));

    rgba_t index[index_size] = {};

    std::size_t run = 0;
    rgba_t px_prev = {.r = 0, .g = 0, .b = 0, .a = 255};

    const std::size_t px_len = desc.width * desc.height;
    const auto f = [&run, &index, &p](rgba_t px, rgba_t px_prev){
      if(px == px_prev){
        ++run;
        return;
      }
      if(run > 0){
        while(run >= 62){
          static constexpr std::uint8_t x = chunk_tag::run | 61;
          p.push(x);
          run -= 62;
        }
        if(run > 0){
          p.push(chunk_tag::run | (run-1));
          run = 0;
        }
      }

      const auto index_pos = px.hash() % index_size;

      if(index[index_pos] == px){
        p.push(chunk_tag::index | index_pos);
        return;
      }
      index[index_pos] = px;

      if(px.a != px_prev.a){
        p.push(chunk_tag::rgba);
        push(p, &px, 4);
        return;
      }
      const auto vr = static_cast<int>(px.r) - static_cast<int>(px_prev.r);
      const auto vg = static_cast<int>(px.g) - static_cast<int>(px_prev.g);
      const auto vb = static_cast<int>(px.b) - static_cast<int>(px_prev.b);

      const auto vg_r = vr - vg;
      const auto vg_b = vb - vg;

      if(
        (256-3 < vr || (-3 < vr && vr < 2) || vr < -256+2) &&
        (256-3 < vg || (-3 < vg && vg < 2) || vg < -256+2) &&
        (256-3 < vb || (-3 < vb && vb < 2) || vb < -256+2)
      )
        p.push(chunk_tag::diff | (vr+2) << 4 | (vg+2) << 2 | (vb+2));
      else if(
        -9  < vg_r && vg_r < 8  &&
        (256-33 < vg || (-33 < vg && vg < 32) || vg < -256+32) &&
        -9  < vg_b && vg_b < 8
      ){
        p.push(chunk_tag::luma | (vg+32));
        p.push((vg_r+8) << 4 | (vg_b+8));
      }
      else{
        p.push(chunk_tag::rgb);
        push(p, &px, 3);
      }
    };
    std::size_t px_pos = 0;
    static constexpr std::size_t blocking_size = 4;
    const auto rb_end = px_len/blocking_size*blocking_size;
    for(; px_pos < rb_end; px_pos += blocking_size){
      rgba_t pxs[blocking_size];
      if(desc.channels == 4)
        pull(pxs, pixels, 4*blocking_size);
      else{
        pull(pxs, pixels, 3);
        pull(pxs+1, pixels, 3);
        pull(pxs+2, pixels, 3);
        pull(pxs+3, pixels, 3);
        pxs[0].a = pxs[1].a = pxs[2].a = pxs[3].a = 255;
      }
      f(pxs[0], px_prev);
      f(pxs[1], pxs[0]);
      f(pxs[2], pxs[1]);
      f(pxs[3], pxs[2]);
      px_prev = pxs[3];
    }
    auto px = px_prev;
    for(; px_pos < px_len; ++px_pos){
      px_prev = px;
      pull(&px, pixels, desc.channels);
      f(px, px_prev);
    }
    if(px == px_prev){
      while(run >= 62){
        static constexpr std::uint8_t x = chunk_tag::run | 61;
        p.push(x);
        run -= 62;
      }
      if(run > 0){
        p.push(chunk_tag::run | (run-1));
        run = 0;
      }
    }

    push(p, padding, sizeof(padding));
  }

  template<typename Puller>
  static desc decode_header(Puller& p){
    desc d;
    const auto magic_ = read_32(p);
    d.width = read_32(p);
    d.height = read_32(p);
    d.channels = p.pull();
    d.colorspace = static_cast<qoi::colorspace>(p.pull());
    if(
      d.width == 0 || d.height == 0 || magic_ != magic ||
      d.channels < 3 || d.channels > 4 ||
      d.height >= pixels_max / d.width
    )
      throw std::runtime_error("qoixx::qoi::decode: invalid header");
    return d;
  }

  template<typename Pusher, typename Puller>
  static void decode_impl(Pusher& pixels, Puller& p, std::size_t px_len, std::size_t size, std::size_t channels){
    rgba_t px = {0, 0, 0, 255};
    rgba_t index[index_size] = {};

    std::uint_fast8_t run = 0;
    const std::size_t chunks_len = size - sizeof(padding);
    for(std::size_t px_pos = 0; px_pos < px_len; px_pos += channels){
      if(run > 0)
        --run;
      else if(p.count() < chunks_len){
        static constexpr std::uint32_t mask_head_2 = 0b1100'0000u;
        static constexpr std::uint32_t mask_tail_6 = 0b0011'1111u;
        static constexpr std::uint32_t mask_tail_4 = 0b0000'1111u;
        static constexpr std::uint32_t mask_tail_2 = 0b0000'0011u;
        const auto b1 = p.pull();

        if(b1 == chunk_tag::rgb)
          pull(&px, p, 3);
        else if(b1 == chunk_tag::rgba)
          pull(&px, p, 4);
        else if((b1 & mask_head_2) == chunk_tag::index)
          px = index[b1];
        else if((b1 & mask_head_2) == chunk_tag::diff){
          px.r += ((b1 >> 4) & mask_tail_2) - 2;
          px.g += ((b1 >> 2) & mask_tail_2) - 2;
          px.b += ( b1       & mask_tail_2) - 2;
        }
        else if((b1 & mask_head_2) == chunk_tag::luma){
          const auto b2 = p.pull();
          const int vg = (b1 & mask_tail_6) - 32;
          px.r += vg - 8 + ((b2 >> 4) & mask_tail_4);
          px.g += vg;
          px.b += vg - 8 + ( b2       & mask_tail_4);
        }
        else if((b1 & mask_head_2) == chunk_tag::run)
          run = b1 & mask_tail_6;

        index[px.hash() % index_size] = px;
      }

      push(pixels, &px, channels);
    }
  }
 public:
  template<typename T, typename U>
  static T encode(const U& u, const desc& desc){
    using coU = container_operator<U>;
    if(!coU::valid(u) || coU::size(u) < desc.width*desc.height*desc.channels || desc.width == 0 || desc.height == 0 || desc.channels < 3 || desc.channels > 4 || desc.height >= pixels_max / desc.width)
      throw std::invalid_argument{"qoixx::qoi::encode: invalid argument"};

    const auto max_size = static_cast<std::size_t>(desc.width) * desc.height * (desc.channels + 1) + header_size + sizeof(padding);
    using coT = container_operator<T>;
    T data = coT::construct(max_size);
    auto p = coT::create_pusher(data);
    auto puller = coU::create_puller(u);

    encode_impl(p, puller, desc);

    return p.finalize();
  }
  template<typename T, typename U>
  requires(sizeof(U) == 1)
  static T encode(const U* pixels, std::size_t size, const desc& desc){
    return encode<T>(std::make_pair(pixels, size), desc);
  }
  template<typename T, typename U>
  static std::pair<T, desc> decode(const U& u, std::uint8_t channels = 0){
    using coU = container_operator<U>;
    const auto size = coU::size(u);
    if(!coU::valid(u) || size < header_size + sizeof(padding) || (channels != 0 && channels != 3 && channels != 4))
      throw std::invalid_argument{"qoixx::qoi::decode: invalid argument"};
    auto puller = coU::create_puller(u);

    const auto d = decode_header(puller);
    if(channels == 0)
      channels = d.channels;

    const std::size_t px_len = static_cast<std::size_t>(d.width) * d.height * channels;
    using coT = container_operator<T>;
    T data = coT::construct(px_len);
    auto p = coT::create_pusher(data);

    decode_impl(p, puller, px_len, size, channels);

    return std::make_pair(std::move(p.finalize()), d);
  }
  template<typename T, typename U>
  requires(sizeof(U) == 1)
  static std::pair<T, desc> decode(const U* pixels, std::size_t size, std::uint8_t channels = 0){
    return decode<T>(std::make_pair(pixels, size), channels);
  }
};

}

#endif //QOIXX_HPP_INCLUDED_
