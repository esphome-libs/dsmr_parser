if(ESP_PLATFORM)
   idf_component_register(INCLUDE_DIRS "src") # header-only
   return()
endif()

cmake_minimum_required(VERSION 3.20)
project(dsmr_parser)
include(FetchContent)

# doctest
file(DOWNLOAD
  https://github.com/doctest/doctest/releases/download/v2.4.12/doctest.h
  ${CMAKE_BINARY_DIR}/doctest/doctest.h
  EXPECTED_HASH SHA512=5a69cc6a5e874691877fbd1cea661ec736c2d78d38791cb22a12398c2731ea6d07e7edda4bd1efde2a65df9e4ddd4836f6571d9b04e0241d520790573634065a)

# mbedtls
FetchContent_Populate(
  mded_tls
  URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.4/mbedtls-3.6.4.tar.bz2
  URL_HASH SHA512=6671fb8fcaa832e5b115dfdce8f78baa6a4aea71f5c89a640583634cdee27aefe3bf4be075744da91f7c3ae5ea4e0c765c8fc3937b5cfd9ea73d87ef496524da)
set(ENABLE_PROGRAMS OFF CACHE INTERNAL "")
set(ENABLE_TESTING OFF CACHE INTERNAL "")
add_subdirectory(${mded_tls_SOURCE_DIR})

# bearssl
FetchContent_Populate(
  bearsll
  URL https://bearssl.org/bearssl-0.6.tar.gz
  URL_HASH SHA512=f9ed25683cfc6c4abe7f1203a2b82ed101ee4c9e0f9ab60755b6a09c8d1e8e4f64d413624e7bb9c4b0033f909a2e4568a1d916cc6ce4736222900691e1f8359a)
file(GLOB_RECURSE bearssl_sources CONFIGURE_DEPENDS "${bearsll_SOURCE_DIR}/src/*.c")
add_library(bearssl STATIC ${bearssl_sources})
target_include_directories(bearssl SYSTEM PUBLIC "${bearsll_SOURCE_DIR}/inc" PRIVATE "${bearsll_SOURCE_DIR}/src")

# cmake_template for warnings and sanitizers
FetchContent_Populate(
  cmake_template
  URL https://github.com/cpp-best-practices/cmake_template/archive/1015c6b88410df411c0cc0413e3e64c33d7a8331.zip
  URL_HASH SHA512=b7ad5b05eb21e5cb8159f3bd8b05615ee55489e5ab6a543bfe50ba05c8c8935106a6870b438c97ca3894108650a4a542a810707954cbe6b3beb9c6de82f1bde7)
include(${cmake_template_SOURCE_DIR}/cmake/CompilerWarnings.cmake)
include(${cmake_template_SOURCE_DIR}/cmake/Sanitizers.cmake)

# dsmr_parser_test
file(GLOB_RECURSE dsmr_parser_test_src_files CONFIGURE_DEPENDS "src/*.h" "src/*.cpp" "tests/*.h" "tests/*.cpp")
add_executable(dsmr_parser_test ${dsmr_parser_test_src_files})
target_include_directories(dsmr_parser_test PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/doctest)
target_compile_features(dsmr_parser_test PRIVATE cxx_std_20)
target_link_libraries(dsmr_parser_test PRIVATE mbedtls bearssl)
target_include_directories(dsmr_parser_test SYSTEM PUBLIC $<TARGET_PROPERTY:mbedtls,INTERFACE_INCLUDE_DIRECTORIES>) # disable warnings for mbedtls headers
target_compile_options(dsmr_parser_test PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>) # enable __VA_OPT__ on MSVC

# enable warnings
add_library(dsmr_parser_test_warnings INTERFACE)
myproject_set_project_warnings(dsmr_parser_test_warnings ON "" "" "" "")
target_link_libraries(dsmr_parser_test PRIVATE dsmr_parser_test_warnings)

# enable sanitizers: address, leak, undefined behaviour
add_library(dsmr_parser_test_sanitizers INTERFACE)
myproject_enable_sanitizers(dsmr_parser_test_sanitizers ON ON ON OFF OFF)
target_link_libraries(dsmr_parser_test PRIVATE dsmr_parser_test_sanitizers)
