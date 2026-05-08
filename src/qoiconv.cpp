#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wunused-function"
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

#include<filesystem>
#include<vector>
#include<cstddef>
#include<fstream>
#include<iostream>
#include<cstdlib>
#include<memory>
#include<variant>
#include<string>
#include<string_view>

static inline std::vector<std::byte> load_file(const std::filesystem::path& path){
  std::vector<std::byte> bytes(std::filesystem::file_size(path));
  std::ifstream ifs{path, std::ios::binary};
  ifs.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
  return bytes;
}

static inline void save_file(const std::filesystem::path& path, const std::vector<std::byte>& bytes){
  std::ofstream ofs{path, std::ios::binary};
  ofs.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

struct stbi_png{
  std::unique_ptr<::stbi_uc[], decltype(&::stbi_image_free)> pixels;
  int width;
  int height;
  int channels;
};

using image = std::variant<stbi_png, std::pair<std::vector<std::byte>, qoixx::qoi::desc>>;

static inline stbi_png read_png(const std::filesystem::path& file_path){
  int w, h, c;
  if(!::stbi_info(file_path.string().c_str(), &w, &h, &c))
    throw std::runtime_error("decode_png: Couldn't read header " + file_path.string());

  if(c != 3)
    c = 4;

  auto loaded = ::stbi_load(file_path.string().c_str(), &w, &h, nullptr, c);
  if(loaded == nullptr)
    throw std::runtime_error("decode_png: Couldn't load/decode " + file_path.string());

  auto pixels = std::unique_ptr<::stbi_uc[], decltype(&::stbi_image_free)>{loaded, &::stbi_image_free};
  return {std::move(pixels), w, h, c};
}

static inline std::pair<std::vector<std::byte>, qoixx::qoi::desc> read_qoi(const std::filesystem::path& file_path){
  const auto qoi = load_file(file_path);
  return qoixx::qoi::decode<std::vector<std::byte>>(qoi);
}

template<typename... Fs>
struct overloaded : Fs...{
  using Fs::operator()...;
};
template<typename... Fs> overloaded(Fs...) -> overloaded<Fs...>;

static inline void write_png(const std::filesystem::path& file_path, const image& image){
  auto [ptr, w, h, c] = std::visit(overloaded{
    [](const stbi_png& image){
      return std::make_tuple(reinterpret_cast<const void*>(image.pixels.get()), image.width, image.height, image.channels);
    },
    [](const std::pair<std::vector<std::byte>, qoixx::qoi::desc>& image){
      return std::make_tuple(
        reinterpret_cast<const void*>(image.first.data()),
        static_cast<int>(image.second.width),
        static_cast<int>(image.second.height),
        static_cast<int>(image.second.channels)
      );
    }
  }, image);
  if(!::stbi_write_png(file_path.string().c_str(), w, h, c, ptr, 0))
    throw std::runtime_error("write_png: Couldn't write/encode " + file_path.string());
}

static inline void write_qoi(const std::filesystem::path& file_path, const image& image){
  auto [ptr, size, desc] = std::visit(overloaded{
    [](const stbi_png& image){
      return std::make_tuple(
        reinterpret_cast<const std::byte*>(image.pixels.get()),
        static_cast<std::size_t>(image.width) * image.height * image.channels,
        qoixx::qoi::desc{
          .width = static_cast<std::uint32_t>(image.width),
          .height = static_cast<std::uint32_t>(image.height),
          .channels = static_cast<std::uint8_t>(image.channels),
          .colorspace = qoixx::qoi::colorspace::srgb
        }
      );
    },
    [](const std::pair<std::vector<std::byte>, qoixx::qoi::desc>& image){
      return std::make_tuple(image.first.data(), image.first.size(), image.second);
    }
  }, image);
  const auto encoded = qoixx::qoi::encode<std::vector<std::byte>>(ptr, size, desc);
  save_file(file_path, encoded);
}

int main(int argc, char **argv)try{
  if(argc < 3){
    std::cout << "Usage: " << argv[0] << " <infile> <outfile>\n"
                 "Examples:\n"
                 "  " << argv[0] << " input.png output.qoi\n"
                 "  " << argv[0] << " input.qoi output.png" << std::endl;
    return EXIT_FAILURE;
  }

  const std::string_view in{argv[1]}, out{argv[2]};
  const auto im = [&in]()->image{
    if(in.ends_with(".png"))
      return read_png(in);
    else if(in.ends_with(".qoi"))
      return read_qoi(in);
    else
      throw std::runtime_error("infile " + std::string{in} + " is not png/qoi file");
  }();

  if(out.ends_with(".png"))
    write_png(out, im);
  else if(out.ends_with(".qoi"))
    write_qoi(out, im);
  else
    throw std::runtime_error("outfile " + std::string{out} + " is not png/qoi file name");
}catch(std::exception& e){
  std::cerr << e.what() << std::endl;
  return EXIT_FAILURE;
}
