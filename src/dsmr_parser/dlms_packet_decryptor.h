#pragma once
#include "aes128gcm.h"
#include "util.h"
#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace dsmr_parser {

// Decrypts DLMS packets encrypted with AES-128-GCM.
// The encryption is described in the "specs/Luxembourg Smarty P1 specification v1.1.3.pdf" chapter "3.2.5 P1 software – Channel security".
template <typename Aes128Gcm>
class DlmsPacketDecryptor final {

#pragma pack(push, 1)
  // The packet has the following structure:
  //   Header (18 bytes) | Encrypted Telegram | GCM Tag (12 bytes)
  struct DlmsPacket final {
  private:
    struct {
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
    static DlmsPacket* from_bytes(const std::span<uint8_t> bytes) {
      if (bytes.size() < sizeof(header) + /* tag length */ 12) {
        return nullptr;
      }

      auto& packet = *reinterpret_cast<DlmsPacket*>(bytes.data());

      const auto& length_correct = bytes.size() == sizeof(header) + /* tag length */ 12 + packet.telegram_length();
      const auto& header_bytes_consistent = packet.header.tag == 0xDB && packet.header.system_title_length == 0x08 &&
                                            packet.header.long_form_length_indicator == 0x82 && packet.header.security_control_field == 0x30;
      if (!length_correct || !header_bytes_consistent) {
        return nullptr;
      }

      return &packet;
    }

    // Also called "IV"
    std::array<const uint8_t, 12> nonce() const {
      // nonce = SystemTitle (8 bytes) + InvocationCounter (4 bytes)
      const auto& st = header.system_title;
      const auto& ic = header.invocation_counter_big_endian;
      return {st[0], st[1], st[2], st[3], st[4], st[5], st[6], st[7], ic[0], ic[1], ic[2], ic[3]};
    }

    std::span<uint8_t> encrypted_telegram() { return {encrypted_telegram_with_gcm_tag, telegram_length()}; }
    std::array<const uint8_t, 12> gcm_tag() const {
      const uint8_t* p = encrypted_telegram_with_gcm_tag + telegram_length();
      return {p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11]};
    }

  private:
    // encrypted and decrypted telegrams have the same length
    size_t telegram_length() const {
      // Convert from big-endian to the host's little-endian
      const auto total_length = (header.total_length_big_endian[0] << 8) | (header.total_length_big_endian[1]);
      return static_cast<size_t>(total_length - 5 - 12); // 5 = SecurityControlFieldLength + InvocationCounterLength. 12 = GcmTagLength
    }
  };
#pragma pack(pop)
  static_assert(sizeof(DlmsPacket) == 19, "EncryptedPacket struct must be 19 bytes");

  Aes128Gcm decryptor;

public:
  void set_encryption_key(const Aes128GcmEncryptionKey& key) { decryptor.set_encryption_key(key); }

  std::optional<std::string_view> decrypt_inplace(std::span<uint8_t> dlms_packet_bytes) {
    auto dlms_packet = DlmsPacket::from_bytes(dlms_packet_bytes);
    if (dlms_packet == nullptr) {
      return {};
    }

    // aad = AdditionalAuthenticatedData = SecurityControlField + AuthenticationKey.
    //   SecurityControlField is always 0x30.
    //   AuthenticationKey = "00112233445566778899AABBCCDDEEFF". It is hardcoded and is the same for all DSMR devices.
    constexpr std::array<const uint8_t, 17> aad{0x30, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

    const bool res = decryptor.decrypt_inplace(aad, dlms_packet->nonce(), dlms_packet->encrypted_telegram(), dlms_packet->gcm_tag());
    if (!res) {
      return {};
    }

    return std::string_view{reinterpret_cast<const char*>(dlms_packet->encrypted_telegram().data()), dlms_packet->encrypted_telegram().size()};
  }
};

}
