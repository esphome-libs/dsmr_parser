#pragma once
#include "util.h"
#include <optional>

namespace dsmr_parser {

class Aes128GcmEncryptionKey final {
  std::array<uint8_t, 16> key{};

  Aes128GcmEncryptionKey() = default;

public:
  // key_hex is a string like "00112233445566778899AABBCCDDEEFF"
  static std::optional<Aes128GcmEncryptionKey> from_hex(std::string_view key_hex) {
    if (key_hex.size() != 32) {
      return {};
    }

    Aes128GcmEncryptionKey res;

    for (size_t i = 0; i < 16; ++i) {
      const auto hi = to_hex_value(key_hex[2 * i]);
      const auto lo = to_hex_value(key_hex[2 * i + 1]);
      if (!hi || !lo) {
        return {};
      }
      res.key[i] = static_cast<uint8_t>((*hi << 4) | *lo);
    }

    return res;
  }

  const uint8_t* data() const { return key.data(); }

private:
  static std::optional<uint8_t> to_hex_value(const char c) {
    if (c >= '0' && c <= '9')
      return static_cast<uint8_t>(c - '0');
    if (c >= 'a' && c <= 'f')
      return static_cast<uint8_t>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F')
      return static_cast<uint8_t>(c - 'A' + 10);
    return {};
  }
};

}
