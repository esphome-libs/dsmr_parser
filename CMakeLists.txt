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
  EXPECTED_MD5 0671bbca9fb00cb19a168cffa89ac184)

# mbedtls
FetchContent_Populate(
  mded_tls
  URL https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.4/mbedtls-3.6.4.tar.bz2
  URL_HASH MD5=eb965a5bb8044bc43a49adb435fa72ee)
set(ENABLE_PROGRAMS OFF CACHE INTERNAL "")
set(ENABLE_TESTING OFF CACHE INTERNAL "")
add_subdirectory(${mded_tls_SOURCE_DIR})

# bearssl
FetchContent_Populate(
  bearsll
  URL https://bearssl.org/bearssl-0.6.tar.gz
  URL_HASH MD5=1513f9828c5b174ea409ca581cb45c98)
file(GLOB_RECURSE bearssl_sources CONFIGURE_DEPENDS "${bearsll_SOURCE_DIR}/src/*.c")
add_library(bearssl STATIC ${bearssl_sources})
target_include_directories(bearssl SYSTEM PUBLIC "${bearsll_SOURCE_DIR}/inc" PRIVATE "${bearsll_SOURCE_DIR}/src")

# cmake_template for warnings and sanitizers
FetchContent_Populate(
  cmake_template
  URL https://github.com/cpp-best-practices/cmake_template/archive/refs/heads/main.zip
  URL_HASH MD5=c391b4c21eeabac1d5142bcf404c740c)
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
