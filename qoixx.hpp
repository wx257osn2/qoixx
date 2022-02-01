#ifndef QOIXX_HPP_INCLUDED_
#define QOIXX_HPP_INCLUDED_

#include<cstdint>
#include<cstddef>
#include<cstring>
#include<vector>
#include<type_traits>
#include<stdexcept>
#include<bit>

namespace qoixx{

namespace detail{

template<typename T>
struct default_container_operator;

template<typename T, typename A>
requires(sizeof(T) == 1)
struct default_container_operator<std::vector<T, A>>{
  using target_type = std::vector<T, A>;
  static inline target_type construct(std::size_t size){
    target_type t(size);
    return t;
  }
  struct pusher{
    static constexpr bool is_contiguous = true;
    target_type* t;
    std::size_t i = 0;
    inline void push(std::uint8_t x)noexcept{
      (*t)[i++] = static_cast<T>(x);
    }
    inline target_type finalize()noexcept{
      t->resize(i);
      return std::move(*t);
    }
    inline std::uint8_t* raw_pointer()noexcept{
      return static_cast<std::uint8_t*>(t->data())+i;
    }
    inline void advance(std::size_t n)noexcept{
      i += n;
    }
  };
  static constexpr pusher create_pusher(target_type& t)noexcept{
    return {&t};
  }
  struct puller{
    static constexpr bool is_contiguous = true;
    const T* t;
    inline std::uint8_t pull()noexcept{
      return static_cast<std::uint8_t>(*t++);
    }
    inline const std::uint8_t* raw_pointer()noexcept{
      return static_cast<const std::uint8_t*>(t);
    }
    inline void advance(std::size_t n)noexcept{
      t += n;
    }
  };
  static constexpr puller create_puller(const target_type& t)noexcept{
    return {t.data()};
  }
  static inline std::size_t size(const target_type& t)noexcept{
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
    inline std::uint8_t pull()noexcept{
      return static_cast<std::uint8_t>(*ptr++);
    }
    inline const std::uint8_t* raw_pointer()noexcept{
      return static_cast<const std::uint8_t*>(ptr);
    }
    inline void advance(std::size_t n)noexcept{
      ptr += n;
    }
  };
  static constexpr puller create_puller(const target_type& t)noexcept{
    return {t.first};
  }
  static inline std::size_t size(const target_type& t)noexcept{
    return t.second;
  }
  static inline bool valid(const target_type& t)noexcept{
    return t.first != nullptr;
  }
};

}

template<typename T>
struct container_operator : detail::default_container_operator<T>{};

class qoi{
  template<std::size_t Size, typename T>
  static inline void push(T& dst, const void* src){
    if constexpr(T::is_contiguous){
      auto*const ptr = dst.raw_pointer();
      dst.advance(Size);
      if constexpr(Size == 3){
        std::memcpy(ptr, src, 2);
        std::memcpy(ptr+2, static_cast<const std::uint8_t*>(src)+2, 1);
      }
      else
        std::memcpy(ptr, src, Size);
    }
    else{
      const auto* ptr = static_cast<const std::uint8_t*>(src);
      auto size = Size;
      while(size --> 0)
        dst.push(*ptr++);
    }
  }
  template<std::size_t Size, typename T>
  static inline void pull(void* dst, T& src){
    if constexpr(T::is_contiguous){
      const auto*const ptr = src.raw_pointer();
      src.advance(Size);
      if constexpr(Size == 3){
        std::memcpy(dst, ptr, 2);
        std::memcpy(static_cast<std::uint8_t*>(dst)+2, ptr+2, 1);
      }
      else
        std::memcpy(dst, ptr, Size);
    }
    else{
      auto* ptr = static_cast<std::uint8_t*>(dst);
      auto size = Size;
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
  struct rgb_t{
    std::uint8_t r, g, b;
    inline auto hash()const{
      static constexpr std::uint64_t constant =
        static_cast<std::uint64_t>(3u) << 56 |
                                   5u  << 16 |
        static_cast<std::uint64_t>(7u) << 40 |
                                  11u;
      const auto v =
        static_cast<std::uint64_t>(r)          |
        static_cast<std::uint64_t>(g)    << 40 |
        static_cast<std::uint64_t>(b)    << 16 |
        static_cast<std::uint64_t>(0xff) << 56 ;
      return (v*constant)>>56;
    }
    constexpr bool operator==(const rgb_t& r)const{
      return ((this->r^r.r)|(this->g^r.g)|(this->b^r.b)) == 0;
    }
  };
  struct rgba_t{
    std::uint8_t r, g, b, a;
    inline std::uint32_t v()const{
      static_assert(sizeof(rgba_t) == sizeof(std::uint32_t));
      if constexpr(std::endian::native == std::endian::little){
        std::uint32_t x;
        std::memcpy(&x, this, sizeof(std::uint32_t));
        return x;
      }
      else
        return std::uint32_t{r}       |
               std::uint32_t{g} <<  8 |
               std::uint32_t{b} << 16 |
               std::uint32_t{a} << 24;
    }
    inline auto hash()const{
      static constexpr std::uint64_t constant =
        static_cast<std::uint64_t>(3u) << 56 |
                                   5u  << 16 |
        static_cast<std::uint64_t>(7u) << 40 |
                                  11u;
      const auto v = static_cast<std::uint64_t>(this->v());
      return (((v<<32|v)&0xFF00FF0000FF00FF)*constant)>>56;
    }
    inline bool operator==(const rgba_t& rhs)const{
      return v() == rhs.v();
    }
    inline bool operator!=(const rgba_t& rhs)const{
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
  static inline std::uint32_t read_32(Puller& p){
    if constexpr(std::endian::native == std::endian::big && Puller::is_contiguous){
      std::uint32_t x;
      pull<sizeof(x)>(&x, p);
      return x;
    }
    else{
      const auto _1 = p.pull();
      const auto _2 = p.pull();
      const auto _3 = p.pull();
      const auto _4 = p.pull();
      return static_cast<std::uint32_t>(_1 << 24 | _2 << 16 | _3 << 8 | _4);
    }
  }
  template<typename Pusher>
  static inline void write_32(Pusher& p, std::uint32_t value){
    if constexpr(std::endian::native == std::endian::big && Pusher::is_contiguous)
      push<sizeof(value)>(p, value);
    else{
      p.push((value & 0xff000000) >> 24);
      p.push((value & 0x00ff0000) >> 16);
      p.push((value & 0x0000ff00) >>  8);
      p.push( value & 0x000000ff       );
    }
  }
 private:
  template<std::uint_fast8_t Channels, typename Pusher, typename Puller>
  static inline void encode_impl(Pusher& p, Puller& pixels, const desc& desc){
    write_32(p, magic);
    write_32(p, desc.width);
    write_32(p, desc.height);
    p.push(Channels);
    p.push(static_cast<std::uint8_t>(desc.colorspace));

    using rgba_t = std::conditional_t<Channels == 4, qoi::qoi::rgba_t, qoi::qoi::rgb_t>;

    rgba_t index[index_size] = {};

    std::size_t run = 0;
    rgba_t px_prev = {};
    if constexpr(std::is_same<rgba_t, qoi::qoi::rgba_t>::value){
      px_prev.a = 255;
      index[(0*3+0*5+0*7+255*11)%index_size] = px_prev;
    }

    std::size_t px_len = desc.width * desc.height;
    const auto f = [&run, &index, &p](rgba_t px, rgba_t px_prev){
      if(px == px_prev){
        ++run;
        return;
      }
      if(run > 0){
        while(run >= 62)[[unlikely]]{
          static constexpr std::uint8_t x = chunk_tag::run | 61;
          p.push(x);
          run -= 62;
        }
        if(run == 1){
          const auto index_pos = px_prev.hash() % index_size;
          p.push(chunk_tag::index | index_pos);
          run = 0;
        }
        else if(run > 0){
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

      if constexpr(Channels == 4)
        if(px.a != px_prev.a){
          p.push(chunk_tag::rgba);
          push<4>(p, &px);
          return;
        }
      const auto vr = static_cast<int>(px.r) - static_cast<int>(px_prev.r) + 2;
      const auto vg = static_cast<int>(px.g) - static_cast<int>(px_prev.g) + 2;
      const auto vb = static_cast<int>(px.b) - static_cast<int>(px_prev.b) + 2;

      if(const std::uint8_t v = vr|vg|vb; v < 4){
        p.push(chunk_tag::diff | vr << 4 | vg << 2 | vb);
        return;
      }
      const auto vg_r = vr - vg + 8;
      const auto vg_b = vb - vg + 8;
      if(const int v = vg_r|vg_b, g = vg+30; ((v&0xf0)|(g&0xc0)) == 0){
        p.push(chunk_tag::luma | g);
        p.push(vg_r << 4 | vg_b);
      }
      else{
        p.push(chunk_tag::rgb);
        push<3>(p, &px);
      }
    };
    auto px = px_prev;
    while(px_len--)[[likely]]{
      px_prev = px;
      pull<Channels>(&px, pixels);
      f(px, px_prev);
    }
    if(px == px_prev){
      while(run >= 62)[[unlikely]]{
        static constexpr std::uint8_t x = chunk_tag::run | 61;
        p.push(x);
        run -= 62;
      }
      if(run > 0){
        p.push(chunk_tag::run | (run-1));
        run = 0;
      }
    }

    push<sizeof(padding)>(p, padding);
  }

  template<typename Puller>
  static inline desc decode_header(Puller& p){
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
    )[[unlikely]]
      throw std::runtime_error("qoixx::qoi::decode: invalid header");
    return d;
  }

  template<std::size_t Channels, typename Pusher, typename Puller>
  static inline void decode_impl(Pusher& pixels, Puller& p, std::size_t px_len, std::size_t size){
    using rgba_t = std::conditional_t<Channels == 4, qoi::qoi::rgba_t, qoi::qoi::rgb_t>;

    rgba_t px = {};
    if constexpr(std::is_same<rgba_t, qoi::qoi::rgba_t>::value)
      px.a = 255;
    rgba_t index[index_size] = {};
    if constexpr(std::is_same<rgba_t, qoi::qoi::rgba_t>::value)
      index[(0*3+0*5+0*7+255*11)%index_size] = px;

    const auto f = [&pixels, &p, &px_len, &size, &px, &index]{
      static constexpr std::uint32_t mask_tail_6 = 0b0011'1111u;
      static constexpr std::uint32_t mask_tail_4 = 0b0000'1111u;
      static constexpr std::uint32_t mask_tail_2 = 0b0000'0011u;
      const auto b1 = p.pull();
      --size;

#define QOIXX_HPP_DECODE_SWITCH(...) \
      if(b1 >= chunk_tag::run){ \
        switch(b1){ \
          __VA_ARGS__ \
          case chunk_tag::rgb: \
            pull<3>(&px, p); \
            size -= 3; \
            break; \
        default: \
          /*run*/ \
          std::size_t run = b1 & mask_tail_6; \
          if(run >= px_len)[[unlikely]] \
            run = px_len; \
          px_len -= run; \
          do{ \
            push<Channels>(pixels, &px); \
          }while(run--); \
          return; \
        } \
      } \
      else if(b1 >= chunk_tag::luma){ \
        /*luma*/ \
        const auto b2 = p.pull(); \
        --size; \
        static constexpr int vgv = chunk_tag::luma+40; \
        const int vg = b1 - vgv; \
        px.r += vg + (b2 >> 4); \
        px.g += vg + 8; \
        px.b += vg + (b2 & mask_tail_4); \
      } \
      else if(b1 >= chunk_tag::diff){ \
        /*diff*/ \
        px.r += ((b1 >> 4) & mask_tail_2) - 2; \
        px.g += ((b1 >> 2) & mask_tail_2) - 2; \
        px.b += ( b1       & mask_tail_2) - 2; \
      } \
      else{ \
        /*index*/ \
        if constexpr(std::is_same<rgba_t, qoi::qoi::rgba_t>::value) \
          px = index[b1]; \
        else{ \
          const auto src = index + b1; \
          std::memcpy(&px, src, 2); \
          px.b = src->b; \
        } \
        push<Channels>(pixels, &px); \
        return; \
      }
      if constexpr(Channels == 4)
        QOIXX_HPP_DECODE_SWITCH(
          case chunk_tag::rgba:
            pull<4>(&px, p);
            size -= 4;
            break;
        )
      else
        QOIXX_HPP_DECODE_SWITCH(
          [[unlikely]] case chunk_tag::rgba:
            pull<3>(&px, p);
            p.advance(1);
            size -= 4;
            break;
        )
#undef QOIXX_HPP_DECODE_SWITCH
      if constexpr(std::is_same<rgba_t, qoi::qoi::rgba_t>::value)
        index[px.hash() % index_size] = px;
      else{
        const auto dst = index + px.hash() % index_size;
        std::memcpy(dst, &px, 2);
        dst->b = px.b;
      }

      push<Channels>(pixels, &px);
    };

    while(px_len--){
      f();
      if(size < sizeof(padding))[[unlikely]]{
        throw std::runtime_error("qoixx::qoi::decode: insufficient input data");
      }
    }
  }
 public:
  template<typename T, typename U>
  static inline T encode(const U& u, const desc& desc){
    using coU = container_operator<U>;
    if(!coU::valid(u) || coU::size(u) < desc.width*desc.height*desc.channels || desc.width == 0 || desc.height == 0 || desc.channels < 3 || desc.channels > 4 || desc.height >= pixels_max / desc.width)[[unlikely]]
      throw std::invalid_argument{"qoixx::qoi::encode: invalid argument"};

    const auto max_size = static_cast<std::size_t>(desc.width) * desc.height * (desc.channels + 1) + header_size + sizeof(padding);
    using coT = container_operator<T>;
    T data = coT::construct(max_size);
    auto p = coT::create_pusher(data);
    auto puller = coU::create_puller(u);

    if(desc.channels == 4)
      encode_impl<4>(p, puller, desc);
    else
      encode_impl<3>(p, puller, desc);

    return p.finalize();
  }
  template<typename T, typename U>
  requires(sizeof(U) == 1)
  static inline T encode(const U* pixels, std::size_t size, const desc& desc){
    return encode<T>(std::make_pair(pixels, size), desc);
  }
  template<typename T, typename U>
  static inline std::pair<T, desc> decode(const U& u, std::uint8_t channels = 0){
    using coU = container_operator<U>;
    const auto size = coU::size(u);
    if(!coU::valid(u) || size < header_size + sizeof(padding) || (channels != 0 && channels != 3 && channels != 4))[[unlikely]]
      throw std::invalid_argument{"qoixx::qoi::decode: invalid argument"};
    auto puller = coU::create_puller(u);

    const auto d = decode_header(puller);
    if(channels == 0)
      channels = d.channels;

    const std::size_t px_len = static_cast<std::size_t>(d.width) * d.height;
    using coT = container_operator<T>;
    T data = coT::construct(px_len*channels);
    auto p = coT::create_pusher(data);

    if(channels == 4)
      decode_impl<4>(p, puller, px_len, size);
    else
      decode_impl<3>(p, puller, px_len, size);

    return std::make_pair(std::move(p.finalize()), d);
  }
  template<typename T, typename U>
  requires(sizeof(U) == 1)
  static inline std::pair<T, desc> decode(const U* pixels, std::size_t size, std::uint8_t channels = 0){
    return decode<T>(std::make_pair(pixels, size), channels);
  }
};

}

#endif //QOIXX_HPP_INCLUDED_
