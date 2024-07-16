cmake_minimum_required(VERSION 3.14)
project(qoixx LANGUAGES CXX)

option(QOIXX_USE_TABLES "QOIXX will use precomputed tables to increase runtime preformance." ON)
option(QOIXX_BUILD_TESTS "QOIXX will build samples and tests." ON)

add_library(qoixx INTERFACE)
add_library(qoixx::qoixx ALIAS qoixx)
target_include_directories(qoixx INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_definitions(qoixx INTERFACE QOIXX_DECODE_WITH_TABLES=${QOIXX_USE_TABLES})
set_target_properties(qoixx PROPERTIES CXX_STANDARD 20)

if (QOIXX_BUILD_TESTS)
  set(STB ".dependencies/stb")
  set(QOI ".dependencies/qoi")
  set(DOCTEST ".dependencies/doctest/doctest")

  add_executable(qoibench "src/qoibench.cpp")
  target_link_libraries(qoibench qoixx::qoixx)
  target_include_directories(qoibench PUBLIC ${STB} ${QOI})
  set_target_properties(qoibench PROPERTIES CXX_STANDARD 20)

  add_executable(qoiconv "src/qoiconv.cpp")
  target_link_libraries(qoiconv qoixx::qoixx)
  target_include_directories(qoiconv PUBLIC ${STB} ${QOI})
  set_target_properties(qoiconv PROPERTIES CXX_STANDARD 20)

  add_executable(qoitest "src/test.cpp")
  target_link_libraries(qoitest qoixx::qoixx)
  target_include_directories(qoitest PUBLIC ${DOCTEST})
  set_target_properties(qoitest PROPERTIES CXX_STANDARD 20)
endif()
