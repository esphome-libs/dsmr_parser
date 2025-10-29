#pragma once
#include "util.h"
#include <array>
#include <mbedtls/gcm.h>
#include <optional>
#include <span>
#include <vector>

namespace dsmr_parser {

// Decrypts DLMS packets encrypted with AES-128-GCM.
// The encryption is described in the "specs/Luxembourg Smarty P1 specification v1.1.3.pdf" chapter "3.2.5 P1 software – Channel security".
class DlmsPacketDecryptor final {

#pragma pack(push, 1)
  // The packet has the following structure:
  //   Header (18 bytes) | Encrypted Telegram | GCM Tag (12 bytes)
  struct DlmsPacket final {
  private:
    struct Header {
      uint8_t tag;                              // always = 0xDB
      uint8_t system_title_length;              // always = 0x08
      uint8_t system_title[8];                  // arbitrary sequence. For example: ['S', 'Y', 'S', 'T', 'E', 'M', 'I', 'D']
      uint8_t long_form_length_indicator;       // always = 0x82
      uint8_t total_length_big_endian[2];       // SecurityControlFieldLength + InvocationCounterLength + EncryptedTelegramLength + GcmTagLength
      uint8_t security_control_field;           // always = 0x30
      uint8_t invocation_counter_big_endian[4]; // also called "frame counter"
    } header;
    uint8_t encrypted_telegram_with_gcm_tag[1]; // GCM Tag is 12 bytes

  public:
    // Also called "IV"
    auto nonce() const {
      // nonce = SystemTitle (8 bytes) + InvocationCounter (4 bytes)
      const auto& st = header.system_title;
      const auto& ic = header.invocation_counter_big_endian;
      return std::array<uint8_t, 12>{st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7], ic[0], ic[1], ic[2], ic[3]};
    }

    std::span<const uint8_t> encrypted_telegram() const { return {encrypted_telegram_with_gcm_tag, telegram_length()}; }
    std::span<const uint8_t> gcm_tag() const { return {encrypted_telegram_with_gcm_tag + telegram_length(), 12}; }

    bool check_consistency(const size_t dlms_packet_length) const {
      // Check the values of the constant fields and that the length is correct
      return header.tag == 0xDB && header.system_title_length == 0x08 && header.long_form_length_indicator == 0x82 && header.security_control_field == 0x30 &&
             dlms_packet_length == sizeof(Header) + /* gcm_tag length */ 12 + telegram_length();
    }

    // encrypted and decrypted telegrams have the same length
    size_t telegram_length() const {
      // Convert from big-endian to the host's little-endian
      const auto total_length = (header.total_length_big_endian[0] << 8) | (header.total_length_big_endian[1]);
      return static_cast<size_t>(total_length - 5 - 12); // 5 = SecurityControlFieldLength + InvocationCounterLength. 12 = GcmTagLength
    }
  };
#pragma pack(pop)
  static_assert(sizeof(DlmsPacket) == 19, "EncryptedPacket struct must be 19 bytes");

  class MbedTlsAes128GcmDecryptor final : NonCopyableAndNonMovable {
    mbedtls_gcm_context gcm;

  public:
    MbedTlsAes128GcmDecryptor() { mbedtls_gcm_init(&gcm); }

    bool set_encryption_key(const std::span<const uint8_t> key) { return mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key.data(), 128) == 0; }

    bool decrypt(std::span<const uint8_t> iv, std::span<const uint8_t> ciphertext, std::span<const uint8_t> tag, std::span<char> decrypted_output_buffer) {
      // aad = AdditionalAuthenticatedData = SecurityControlField + AuthenticationKey.
      //   SecurityControlField is always 0x30.
      //   AuthenticationKey = "00112233445566778899AABBCCDDEEFF". It is hardcoded and is the same for all DSMR devices.
      constexpr uint8_t aad[] = {0x30, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

      const auto& res = mbedtls_gcm_auth_decrypt(&gcm, ciphertext.size(), iv.data(), iv.size(), aad, std::size(aad), tag.data(), tag.size(), ciphertext.data(),
                                                 reinterpret_cast<unsigned char*>(decrypted_output_buffer.data()));
      return res == 0;
    }

    ~MbedTlsAes128GcmDecryptor() { mbedtls_gcm_free(&gcm); }
  };

  std::span<char> _decrypted_dsmr_telegram_buffer;

public:
  class EncryptionKey final {
    friend DlmsPacketDecryptor;

    std::array<uint8_t, 16> key{};
    EncryptionKey() = default;

  public:
    // key_hex is a string like "00112233445566778899AABBCCDDEEFF"
    static std::optional<EncryptionKey> FromHex(std::string_view key_hex) {
      if (key_hex.size() != 32) {
        return {};
      }

      EncryptionKey res;

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

  enum class Error { EncryptedTelegramIsTooSmall, DecryptedTelegramBufferIsTooSmall, HeaderCorrupted, FailedToSetEncryptionKey, DecryptionFailed };

  class Result final {
    friend DlmsPacketDecryptor;

    std::optional<std::string_view> _dsmr_telegram;
    std::optional<Error> _error;

    Result() = default;
    Result(std::string_view dsmr_telegram) : _dsmr_telegram(dsmr_telegram) {}
    Result(Error error) : _error(error) {}

  public:
    auto dsmr_telegram() const { return _dsmr_telegram; }
    auto error() const { return _error; }
  };

  explicit DlmsPacketDecryptor(std::span<char> decrypted_dsmr_telegram_buffer) : _decrypted_dsmr_telegram_buffer(decrypted_dsmr_telegram_buffer) {}

  Result decrypt(std::span<const uint8_t> dlms_packet_bytes, const EncryptionKey& encryption_key) const {
    if (dlms_packet_bytes.size() < sizeof(DlmsPacket)) {
      return Error::EncryptedTelegramIsTooSmall;
    }

    const auto& dlms_packet = *reinterpret_cast<const DlmsPacket*>(dlms_packet_bytes.data());
    if (!dlms_packet.check_consistency(dlms_packet_bytes.size())) {
      return Error::HeaderCorrupted;
    }

    if (_decrypted_dsmr_telegram_buffer.size() < dlms_packet.encrypted_telegram().size())
      return Error::DecryptedTelegramBufferIsTooSmall;

    MbedTlsAes128GcmDecryptor decryptor;

    if (!decryptor.set_encryption_key(encryption_key.key)) {
      return Error::FailedToSetEncryptionKey;
    }

    if (!decryptor.decrypt(dlms_packet.nonce(), dlms_packet.encrypted_telegram(), dlms_packet.gcm_tag(), _decrypted_dsmr_telegram_buffer)) {
      return Error::DecryptionFailed;
    }

    return std::string_view(_decrypted_dsmr_telegram_buffer.data(), dlms_packet.telegram_length());
  }
};

inline const char* to_string(const DlmsPacketDecryptor::Error error) {
  switch (error) {
  case DlmsPacketDecryptor::Error::EncryptedTelegramIsTooSmall:
    return "EncryptedTelegramIsTooSmall";
  case DlmsPacketDecryptor::Error::DecryptedTelegramBufferIsTooSmall:
    return "DecryptedTelegramBufferIsTooSmall";
  case DlmsPacketDecryptor::Error::HeaderCorrupted:
    return "HeaderCorrupted";
  case DlmsPacketDecryptor::Error::FailedToSetEncryptionKey:
    return "FailedToSetEncryptionKey";
  case DlmsPacketDecryptor::Error::DecryptionFailed:
    return "DecryptionFailed";
  }
  return "Unknown error";
}

}
