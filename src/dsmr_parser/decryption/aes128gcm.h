#pragma once
#include "../util.h"
#include <charconv>
#include <optional>
#include <span>

namespace dsmr_parser {

class Aes128GcmDecryptionKey final {
  std::array<uint8_t, 16> key{};
  explicit Aes128GcmDecryptionKey(const std::array<uint8_t, 16> k) : key(k) {}

public:
  // hex is a string like "00112233445566778899AABBCCDDEEFF"
  static std::optional<Aes128GcmDecryptionKey> from_hex(const std::string_view hex) {
    if (hex.size() != 32)
      return std::nullopt;
    std::array<uint8_t, 16> arr{};
    for (size_t i = 0; i < 16; ++i) {
      auto [ptr, ec] = std::from_chars(hex.data() + i * 2, hex.data() + i * 2 + 2, arr[i], 16);
      if (ec != std::errc{})
        return std::nullopt;
    }
    return Aes128GcmDecryptionKey(arr);
  }

  const uint8_t* data() const { return key.data(); }
};

class Aes128GcmDecryptor {
public:
  virtual void set_encryption_key(const Aes128GcmDecryptionKey& key) = 0;
  virtual bool decrypt_inplace(std::span<const uint8_t, 17> aad, std::span<const uint8_t, 12> nonce, std::span<uint8_t> ciphertext,
                               std::span<const uint8_t, 12> tag) = 0;

protected:
  virtual ~Aes128GcmDecryptor() = default;
  std::optional<Aes128GcmDecryptionKey> decryption_key_;
};

}
