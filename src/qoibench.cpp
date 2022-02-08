#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#elif defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push)
#pragma warning(disable: 4820 4365 4505)
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_LINEAR
#include"stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include"stb_image_write.h"
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

#include"qoixx.hpp"

#define QOI_NO_STDIO
#define QOI_IMPLEMENTATION
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(push)
#pragma warning(disable: 4820 4365 4244 4242)
#endif
#include"qoi.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#include<chrono>
#include<iostream>
#include<filesystem>
#include<string_view>
#include<ranges>
#include<fstream>
#include<cstddef>
#include<vector>
#include<memory>
#include<utility>
#include<cstdint>
#include<iomanip>

struct options{
  bool warmup = true;
  bool verify = true;
  bool reference = true;
  bool encode = true;
  bool decode = true;
  bool recurse = true;
  bool only_totals = false;
  unsigned runs;
  bool parse_option(std::string_view argv){
    if(argv == "--nowramup")
      this->warmup = false;
    else if(argv == "--noverify")
      this->verify = false;
    else if(argv == "--noreference")
      this->reference = false;
    else if(argv == "--noencode")
      this->encode = false;
    else if(argv == "--nodecode")
      this->decode = false;
    else if(argv == "--norecurse")
      this->recurse = false;
    else if(argv == "--onlytotals")
      this->only_totals = true;
    else
      return false;
    return true;
  }
};

struct benchmark_result_t{
  struct lib_t{
    std::size_t size;
    std::chrono::duration<double, std::nano> encode_time;
    std::chrono::duration<double, std::nano> decode_time;
  };
  std::size_t count;
  std::size_t raw_size, px;
  std::uint32_t w, h;
  std::uint8_t c;
  lib_t qoi, qoixx;
  benchmark_result_t():count{0}, raw_size{0}, px{0}, qoi{0, {}, {}}, qoixx{0, {}, {}}{}
  benchmark_result_t(const qoixx::qoi::desc& dc):count{1}, raw_size{static_cast<std::size_t>(dc.width)*dc.height*dc.channels}, px{static_cast<std::size_t>(dc.width)*dc.height}, w{dc.width}, h{dc.height}, c{dc.channels}, qoi{}, qoixx{}{}
  benchmark_result_t& operator+=(const benchmark_result_t& rhs)noexcept{
    this->count += rhs.count;
    this->raw_size += rhs.raw_size;
    this->px += rhs.px;
    this->qoi.size += rhs.qoi.size;
    this->qoi.encode_time += rhs.qoi.encode_time;
    this->qoi.decode_time += rhs.qoi.decode_time;
    this->qoixx.size += rhs.qoixx.size;
    this->qoixx.encode_time += rhs.qoixx.encode_time;
    this->qoixx.decode_time += rhs.qoixx.decode_time;
    return *this;
  }
  struct printer{
    const benchmark_result_t* result;
    const options* opt;
    struct manip{
      int w;
      int prec = 0;
      friend std::ostream& operator<<(std::ostream& os, const manip& m){
        os << std::fixed << std::setw(m.w);
        if(m.prec != 0)
          os << std::setprecision(m.prec);
        return os;
      }
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& printer){
      const auto& res = *printer.result;
      const auto px = static_cast<double>(res.px) / res.count;
      const auto raw_size = res.raw_size / res.count;
      const auto qoi_size = res.qoi.size / res.count;
      const auto qoixx_size = res.qoixx.size / res.count;
      const auto qoi_etime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(res.qoi.encode_time) / res.count;
      const auto qoi_dtime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(res.qoi.decode_time) / res.count;
      const auto qoi_empps = res.qoi.encode_time.count() != 0 ? px / std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(qoi_etime).count() : 0.;
      const auto qoi_dmpps = res.qoi.decode_time.count() != 0 ? px / std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(qoi_dtime).count() : 0.;
      const auto qoixx_etime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(res.qoixx.encode_time) / res.count;
      const auto qoixx_dtime = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(res.qoixx.decode_time) / res.count;
      const auto qoixx_empps = res.qoixx.encode_time.count() != 0 ? px / std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(qoixx_etime).count() : 0.;
      const auto qoixx_dmpps = res.qoixx.decode_time.count() != 0 ? px / std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(qoixx_dtime).count() : 0.;
      os << "        decode ms   encode ms   decode mpps   encode mpps   size kb    rate\n";
      if(printer.opt->reference)
        std::cout << "qoi:     " << manip{8, 4} << qoi_dtime.count() << "    " << manip{8, 4} << qoi_etime.count() << "      " << manip{8, 3} << qoi_dmpps << "      " << manip{8, 3} << qoi_empps << "  " << manip{8} << qoi_size/1024 << "   " << manip{4, 1} << static_cast<double>(qoi_size)/raw_size*100. << "%\n";
      std::cout << "qoixx:   " << manip{8, 4} << qoixx_dtime.count() << "    " << manip{8, 4} << qoixx_etime.count() << "      " << manip{8, 3} << qoixx_dmpps << "      " << manip{8, 3} << qoixx_empps << "  " << manip{8} << qoixx_size/1024 << "   " << manip{4, 1} << static_cast<double>(qoixx_size)/raw_size*100. << "%\n";
      return os;
    }
  };
  printer print(const options& opt)const{
    return printer{this, &opt};
  }
};

#define BENCHMARK(opt, result, ...) \
do{ \
  std::chrono::nanoseconds time = {}; \
  for(unsigned i = opt.warmup ? 0u : 1u; i <= opt.runs; ++i){ \
    const auto start = std::chrono::high_resolution_clock::now(); \
    __VA_ARGS__ \
    const auto end = std::chrono::high_resolution_clock::now(); \
    if(i > 0) \
      time += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start); \
  } \
  result = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(time)/opt.runs; \
}while(0)

static inline benchmark_result_t benchmark_image(const std::filesystem::path& p, const options& opt){
  int w, h, channels;

  if(!stbi_info(p.string().c_str(), &w, &h, &channels))
    throw std::runtime_error("Error decoding header " + p.string());

  if(channels != 3)
    channels = 4;

  const ::qoi_desc qoi_desc = {
    .width = static_cast<unsigned int>(w),
    .height = static_cast<unsigned int>(h),
    .channels = static_cast<unsigned char>(channels),
    .colorspace = QOI_SRGB,
  };

  const qoixx::qoi::desc qoixx_desc = {
    .width = static_cast<std::uint32_t>(w),
    .height = static_cast<std::uint32_t>(h),
    .channels = static_cast<std::uint8_t>(channels),
    .colorspace = qoixx::qoi::colorspace::srgb,
  };
  const std::size_t raw_size = static_cast<std::size_t>(w) * qoixx_desc.height * qoixx_desc.channels;

  const auto pixels = std::unique_ptr<::stbi_uc[], decltype(&::stbi_image_free)>{::stbi_load(p.string().c_str(), &w, &h, nullptr, channels), &::stbi_image_free};
  const auto encoded_qoixx = qoixx::qoi::encode<std::vector<std::uint8_t>>(pixels.get(), raw_size, qoixx_desc);

  if(!pixels)
    throw std::runtime_error("Error decoding " + p.string());

  if(opt.verify){
    {// qoi.encode -> qoixx.decode == pixels
      int size;
      const auto qoi = std::unique_ptr<std::uint8_t[], decltype(&::free)>{static_cast<std::uint8_t*>(::qoi_encode(pixels.get(), &qoi_desc, &size)), &::free};
      const auto [pixs, desc] = qoixx::qoi::decode<std::vector<std::uint8_t>>(qoi.get(), size);
      if(desc != qoixx_desc || std::memcmp(pixels.get(), pixs.data(), desc.width*desc.height*desc.channels) != 0)
        throw std::runtime_error("QOIxx decoder pixel mismatch for " + p.string());
    }
    {// qoixx.encode -> qoi.decode == pixels
      ::qoi_desc dc;
      const auto pixs = std::unique_ptr<std::uint8_t[], decltype(&::free)>{static_cast<std::uint8_t*>(::qoi_decode(encoded_qoixx.data(), static_cast<int>(encoded_qoixx.size()), &dc, channels)), &::free};
      if(dc.width != qoixx_desc.width || dc.height != qoixx_desc.height || dc.channels != qoixx_desc.channels || dc.colorspace != static_cast<unsigned char>(qoixx_desc.colorspace) || std::memcmp(pixels.get(), pixs.get(), dc.width*dc.height*dc.channels) != 0){
        std::cout << dc.width << ' ' << qoixx_desc.width << '\n'
                  << dc.height << ' ' << qoixx_desc.height << '\n'
                  << +dc.channels << ' ' << +qoixx_desc.channels << '\n'
                  << +dc.colorspace << ' ' << static_cast<int>(qoixx_desc.colorspace) << std::endl;
        auto j = 0;
        for(std::size_t i = 0; i < dc.width*dc.height*dc.channels; ++i)
          if(pixs.get()[i] != pixels.get()[i]){
            std::cout << i << ": " << +pixs[i] << " != " << +pixels[i] << std::endl;
            if(++j == 3)
              break;
          }
        throw std::runtime_error("QOIxx encoder pixel mismatch for " + p.string());
      }
    }
    {// qoixx.encode -> qoixx.decode == pixels
      const auto [pixs, desc] = qoixx::qoi::decode<std::vector<std::uint8_t>>(encoded_qoixx);
      if(desc != qoixx_desc || std::memcmp(pixels.get(), pixs.data(), desc.width*desc.height*desc.channels) != 0){
        for(std::size_t i = 0; i < desc.width*desc.height*desc.channels; ++i)
          if(pixs[i] != pixels.get()[i]){
            std::cout << i << ": " << +pixs[i] << " != " << +pixels[i] << std::endl;
            break;
          }
        throw std::runtime_error("QOIxx roundtrip pixel mismatch for " + p.string());
      }
    }
  }

  benchmark_result_t result{qoixx_desc};
  if(opt.decode){
    if(opt.reference)
      BENCHMARK(opt, result.qoi.decode_time,
        ::qoi_desc dc;
        const std::unique_ptr<std::uint8_t[], decltype(&::free)> pixs{static_cast<std::uint8_t*>(::qoi_decode(encoded_qoixx.data(), static_cast<int>(encoded_qoixx.size()), &dc, channels)), &::free};
      );
    BENCHMARK(opt, result.qoixx.decode_time,
      const auto [pixs, desc] = qoixx::qoi::decode<std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>(encoded_qoixx);
    );
  }

  if(opt.encode){
    if(opt.reference)
      BENCHMARK(opt, result.qoi.encode_time,
        int size;
        const auto qoi = std::unique_ptr<std::uint8_t[], decltype(&::free)>{static_cast<std::uint8_t*>(::qoi_encode(pixels.get(), &qoi_desc, &size)), &::free};
        result.qoi.size = static_cast<std::size_t>(size);
      );
    BENCHMARK(opt, result.qoixx.encode_time,
      const auto encoded_qoixx = qoixx::qoi::encode<std::pair<std::unique_ptr<std::uint8_t[]>, std::size_t>>(pixels.get(), raw_size, qoixx_desc);
      result.qoixx.size = encoded_qoixx.second;
    );
  }

  return result;
}

static inline benchmark_result_t benchmark_directory(const std::filesystem::path& path, const options& opt){
  if(!std::filesystem::is_directory(path))
    throw std::runtime_error(path.string() + " is not a directory");

  benchmark_result_t results = {};

  if(opt.recurse)
    for(const auto& x : std::ranges::subrange{std::filesystem::directory_iterator{path}, std::filesystem::directory_iterator{}})
      if(x.is_directory())
        results += benchmark_directory(x, opt);

  bool first = true;
  for(const auto& x : std::ranges::subrange{std::filesystem::directory_iterator{path}, std::filesystem::directory_iterator{}}){
    auto xp = x.path();
    if(x.is_directory() || xp.extension() != ".png")
      continue;
    if(std::exchange(first, false))
      std::cout << "## Benchmarking " << path.string() << "/*.png -- " << opt.runs << " runs\n\n";

    const auto result = benchmark_image(xp, opt);
    if(!opt.only_totals)
      std::cout << "## " << xp.string() << " size: " << result.w << 'x' << result.h << ", channels: " << +result.c << '\n'
                << result.print(opt) << std::endl;

    results += result;
  }

  if(results.count > 0)
    std::cout << "## Total for " << path << '\n'
              << results.print(opt) << std::endl;

  return results;
}

static inline int help(const char* argv_0, std::ostream& os = std::cout){
  os << "Usage: " << argv_0 << " <iterations> <directory> [options...]\n"
        "Options:\n"
        "    --nowarmup ... don't perform a warmup run\n"
        "    --noverify ... don't verify qoi roundtrip\n"
        "    --noencode ... don't run encoders\n"
        "    --nodecode ... don't run decoders\n"
        "    --norecurse .. don't descend into directories\n"
        "    --onlytotals . don't print individual image results\n"
        "Examples\n"
        "    ./" << argv_0 << " 10 images/textures/\n"
        "    ./" << argv_0 << " 1 images/textures/ --nowarmup" << std::endl;
  return EXIT_FAILURE;
}

int main(int argc, char** argv)try{
  if(argc < 3)
    return help(argv[0]);
  options opt = {};
  for(int i = 3; i < argc; ++i)
    if(!opt.parse_option(argv[i])){
      std::cout << "Unknown option " << argv[i] << '\n';
      return help(argv[0]);
    }

  const auto runs = std::stoi(argv[1]);
  if(runs <= 0){
    std::cout << "Invalid number of runs " << runs << std::endl;
    return EXIT_FAILURE;
  }
  opt.runs = static_cast<unsigned>(runs);

  const auto result = benchmark_directory(argv[2], opt);
  if(result.count > 0)
    std::cout << "# Grand total for " << argv[2] << '\n'
              << result.print(opt) << std::endl;
  else
    std::cout << "No images found in " << argv[2] << std::endl;
}catch(const std::runtime_error& e){
  std::cout << e.what() << std::endl;
  return EXIT_FAILURE;
}
