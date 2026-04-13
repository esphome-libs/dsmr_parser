#pragma once

#include <array>
#include <cstdarg>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#if defined(_MSC_VER)
#include <sal.h>
#endif

namespace dsmr_parser {

class NonCopyable {
protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

public:
  NonCopyable(NonCopyable&&) = default;
  NonCopyable& operator=(NonCopyable&&) = default;

  NonCopyable(const NonCopyable&) = delete;
  NonCopyable& operator=(const NonCopyable&) = delete;
};

class NonCopyableAndNonMovable : NonCopyable {
protected:
  NonCopyableAndNonMovable() = default;
  ~NonCopyableAndNonMovable() = default;

public:
  NonCopyableAndNonMovable(NonCopyableAndNonMovable&&) = delete;
  NonCopyableAndNonMovable& operator=(NonCopyableAndNonMovable&&) = delete;
};

// An OBIS ID like: "1-0:1.7.0.255"
struct ObisId final {
  std::array<uint8_t, 6> v{};
  constexpr explicit ObisId(const uint8_t a, const uint8_t b = 255, const uint8_t c = 255, const uint8_t d = 255, const uint8_t e = 255,
                            const uint8_t f = 255) noexcept
      : v{a, b, c, d, e, f} {};
  ObisId() = default;
  bool operator==(const ObisId&) const = default;
};

// Represents an unencrypted DSMR telegram that starts with '/' and ends with '!' without CRC at the end.
// Example:
//  "/AAA5MTR\r\n"
//  "\r\n"
//  "1-0:1.7.0(00.100*kW)\r\n"
//  "1-0:1.7.0(00.200*kW)\r\n"
//  "!";
class DsmrUnencryptedTelegram final {
  std::string_view data;
public:
  explicit DsmrUnencryptedTelegram(std::string_view telegram) : data(telegram) {}
  std::string_view content() const { return data; }
};

enum class LogLevel {
  VERY_VERBOSE,
  VERBOSE,
  DEBUG,
  INFO,
  WARNING,
  ERROR,
};

class Logger final {
public:
  static void set_log_function(std::function<void(LogLevel log_level, const char* fmt, va_list args)> func) { _log_function = std::move(func); }

#if defined(_MSC_VER)
  static void log(LogLevel log_level, _In_z_ _Printf_format_string_ const char* fmt, ...) {
#elif defined(__clang__) || defined(__GNUC__)
  __attribute__((format(printf, 2, 3)))
  static void log(LogLevel log_level, const char* fmt, ...) {
#else
  static void log(LogLevel log_level, const char* fmt, ...) {
#endif
    va_list args;
    va_start(args, fmt);
    _log_function(log_level, fmt, args);
    va_end(args);
  }

private:
  Logger() = default;

  inline static std::function<void(LogLevel log_level, const char* fmt, va_list args)> _log_function = [](LogLevel, const char*, va_list) {};
};

}
