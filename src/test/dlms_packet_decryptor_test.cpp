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

const auto& dlms_packet =
    read_binary_file(std::filesystem::path(std::source_location::current().file_name()).parent_path() / "test_data" / "encrypted_packet.bin");

TEST_CASE("EncryptionKey FromHex method works correctly") {
  // success cases
  REQUIRE(DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
  REQUIRE(DlmsPacketDecryptor::EncryptionKey::FromHex("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));

  // failure cases
  REQUIRE(!DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAA"));                      // key too short
  REQUIRE(!DlmsPacketDecryptor::EncryptionKey::FromHex("GAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA")); // non hex symbols in key
}

TEST_CASE("Can decrypt a correct packet") {
  std::array<char, 2000> decrypted_dsmr_telegram_buffer{};
  auto decryptor = DlmsPacketDecryptor(decrypted_dsmr_telegram_buffer);
  const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

  const auto& res = decryptor.decrypt(dlms_packet, encryption_key);
  REQUIRE(!res.error());

  const auto dsmr_telegram = std::string(*res.dsmr_telegram());
  REQUIRE(dsmr_telegram.ends_with("1-0:4.7.0(000000166*var)\r\n!7EF9\r\n"));
  REQUIRE(dsmr_telegram.starts_with("/EST5\\253710000_A\r\n"));
}

TEST_CASE("Fail to decrypt corrupted packet") {
  std::array<char, 2000> decrypted_dsmr_telegram_buffer{};
  auto decryptor = DlmsPacketDecryptor(decrypted_dsmr_telegram_buffer);
  const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

  auto corrupted_packet = dlms_packet;
  corrupted_packet[50] ^= 0xFF;

  const auto& res = decryptor.decrypt(corrupted_packet, encryption_key);
  REQUIRE(res.error());
  REQUIRE(*res.error() == DlmsPacketDecryptor::Error::DecryptionFailed);
}

TEST_CASE("Fail to decrypt packet with corrupted header") {
  std::array<char, 2000> decrypted_dsmr_telegram_buffer{};
  auto decryptor = DlmsPacketDecryptor(decrypted_dsmr_telegram_buffer);
  const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

  auto corrupted_packet = dlms_packet;
  corrupted_packet[0] = 0;

  const auto& res = decryptor.decrypt(corrupted_packet, encryption_key);
  REQUIRE(res.error());
  REQUIRE(*res.error() == DlmsPacketDecryptor::Error::HeaderCorrupted);
}

TEST_CASE("Decription fails because specified length in dlms packet is smaller than the received data") {
  std::array<char, 2000> decrypted_dsmr_telegram_buffer{};
  auto decryptor = DlmsPacketDecryptor(decrypted_dsmr_telegram_buffer);
  const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

  const auto& res = decryptor.decrypt({dlms_packet.data(), dlms_packet.size() - 1}, encryption_key);
  REQUIRE(res.error());
  REQUIRE(*res.error() == DlmsPacketDecryptor::Error::HeaderCorrupted);
}

TEST_CASE("Decription fails if the dlms packet is too small") {
  std::array<char, 2000> decrypted_dsmr_telegram_buffer{};
  auto decryptor = DlmsPacketDecryptor(decrypted_dsmr_telegram_buffer);
  const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

  std::vector<uint8_t> small_dlms_packet(10);
  const auto& res = decryptor.decrypt(small_dlms_packet, encryption_key);
  REQUIRE(res.error());
  REQUIRE(*res.error() == DlmsPacketDecryptor::Error::EncryptedTelegramIsTooSmall);
}

TEST_CASE("Decription fails if the decryption buffer is too small") {
  std::array<char, 10> decrypted_dsmr_telegram_buffer{};
  auto decryptor = DlmsPacketDecryptor(decrypted_dsmr_telegram_buffer);
  const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

  const auto& res = decryptor.decrypt(dlms_packet, encryption_key);
  REQUIRE(res.error());
  REQUIRE(*res.error() == DlmsPacketDecryptor::Error::DecryptedTelegramBufferIsTooSmall);
}
