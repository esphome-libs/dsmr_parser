#include "dsmr_parser/aes128gcm_bearssl.h"
#include "dsmr_parser/aes128gcm_mbedtls.h"
#include "dsmr_parser/dlms_packet_decryptor.h"
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <source_location>

using namespace dsmr_parser;

inline std::vector<std::uint8_t> read_binary_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

inline std::vector<std::uint8_t> get_test_encrypted_packet() {
  return read_binary_file(std::filesystem::path(std::source_location::current().file_name()).parent_path() / "test_data" / "encrypted_packet.bin");
}

TEST_CASE("EncryptionKey FromHex method works correctly") {
  // success cases
  REQUIRE(Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  REQUIRE(Aes128GcmEncryptionKey::from_hex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

  // failure cases
  REQUIRE(!Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAA"));                      // key too short
  REQUIRE(!Aes128GcmEncryptionKey::from_hex("GAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")); // non hex symbols in key
}

TEST_CASE("Can decrypt a correct packet") {
  const auto encryption_key = *Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  const auto packet_starts = std::string_view{"/EST5\\253710000_A\r\n"};
  const auto packet_ends = std::string_view{"1-0:4.7.0(000000166*var)\r\n!7EF9\r\n"};

  SUBCASE("MbedTls") {
    DlmsPacketDecryptor<Aes128GcmMbedTls> decryptor;
    decryptor.set_encryption_key(encryption_key);
    auto packet = get_test_encrypted_packet();

    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->starts_with(packet_starts));
    REQUIRE(dsmr_telegram->ends_with(packet_ends));
  }

  SUBCASE("BearSsl") {
    DlmsPacketDecryptor<Aes128GcmBearSsl> decryptor;
    decryptor.set_encryption_key(encryption_key);
    auto packet = get_test_encrypted_packet();

    const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
    REQUIRE(dsmr_telegram);
    REQUIRE(dsmr_telegram->starts_with(packet_starts));
    REQUIRE(dsmr_telegram->ends_with(packet_ends));
  }
}

TEST_CASE("Fail to decrypt corrupted packet") {
  const auto encryption_key = *Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  auto packet = get_test_encrypted_packet();
  packet[50] ^= 0xFF;

  SUBCASE("MbedTls") {
    DlmsPacketDecryptor<Aes128GcmMbedTls> decryptor;
    decryptor.set_encryption_key(encryption_key);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }

  SUBCASE("BearSsl") {
    DlmsPacketDecryptor<Aes128GcmBearSsl> decryptor;
    decryptor.set_encryption_key(encryption_key);
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }
}

TEST_CASE("Fail to decrypt packet with corrupted header") {
  DlmsPacketDecryptor<Aes128GcmMbedTls> decryptor;
  const auto encryption_key = *Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  auto packet = get_test_encrypted_packet();
  decryptor.set_encryption_key(encryption_key);

  packet[0] = 0;

  const auto dsmr_telegram = decryptor.decrypt_inplace({packet.data(), packet.size()});
  REQUIRE_FALSE(dsmr_telegram);
}

TEST_CASE("Decryption fails if the dlms packet is too small") {
  DlmsPacketDecryptor<Aes128GcmMbedTls> decryptor;
  const auto encryption_key = *Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  std::vector<uint8_t> small_dlms_packet(10);
  decryptor.set_encryption_key(encryption_key);

  const auto dsmr_telegram = decryptor.decrypt_inplace({small_dlms_packet.data(), small_dlms_packet.size()});
  REQUIRE_FALSE(dsmr_telegram);
}

TEST_CASE("Does not crash if decrypt_inplace is called without set_encryption_key") {
  auto packet = get_test_encrypted_packet();

  SUBCASE("MbedTls") {
    DlmsPacketDecryptor<Aes128GcmMbedTls> decryptor;
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }

  SUBCASE("BearSsl") {
    DlmsPacketDecryptor<Aes128GcmBearSsl> decryptor;
    REQUIRE_FALSE(decryptor.decrypt_inplace({packet.data(), packet.size()}));
  }
}
