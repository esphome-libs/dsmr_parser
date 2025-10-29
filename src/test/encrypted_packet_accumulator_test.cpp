#include "dsmr_parser/encrypted_packet_accumulator.h"
#include <doctest.h>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <source_location>

using namespace dsmr_parser;

template <class T, class... Vecs>
std::vector<T> concat(const std::vector<T>& first, const Vecs&... rest) {
  std::vector<T> out;
  out.reserve(first.size() + (rest.size() + ... + 0));
  out.insert(out.end(), first.begin(), first.end());
  (out.insert(out.end(), rest.begin(), rest.end()), ...);
  return out;
}

inline std::vector<std::uint8_t> read_binary_file(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

const auto& encrypted_packet =
    read_binary_file(std::filesystem::path(std::source_location::current().file_name()).parent_path() / "test_data" / "encrypted_packet.bin");

static void change_length(std::vector<std::uint8_t>& packet, const std::uint16_t total_len) {
  packet[11] = static_cast<std::uint8_t>((total_len >> 8) & 0xFF);
  packet[12] = static_cast<std::uint8_t>(total_len & 0xFF);
}

TEST_CASE("Can receive correct packet") {
  std::array<std::uint8_t, 2000> encrypted_packet_buffer;
  std::array<char, 2000> decrypted_packet_buffer;
  auto accumulator = EncryptedPacketAccumulator(encrypted_packet_buffer, decrypted_packet_buffer);
  REQUIRE(!accumulator.set_encryption_key("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));

  for (const auto& byte : encrypted_packet) {
    const auto& res = accumulator.process_byte(byte);
    REQUIRE(res.error().has_value() == false);

    if (res.packet()) {
      REQUIRE(std::string(*res.packet()).ends_with("1-0:4.7.0(000000166*var)\r\n!7EF9\r\n"));
      REQUIRE(std::string(*res.packet()).starts_with("/EST5\\253710000_A\r\n"));
      return;
    }
  }

  REQUIRE(false);
}

TEST_CASE("Error on corrupted packet") {
  std::array<std::uint8_t, 2000> encrypted_packet_buffer;
  std::array<char, 2000> decrypted_packet_buffer;

  auto corrupted_packet = encrypted_packet;
  corrupted_packet[50] ^= 0xFF;

  auto accumulator = EncryptedPacketAccumulator(encrypted_packet_buffer, decrypted_packet_buffer);
  REQUIRE(!accumulator.set_encryption_key("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA").has_value());

  for (const auto& byte : corrupted_packet) {
    const auto& res = accumulator.process_byte(byte);
    if (res.error()) {
      REQUIRE(*res.error() == EncryptedPacketAccumulator::Error::DecryptionFailed);
      return;
    }
  }

  REQUIRE(false);
}

TEST_CASE("Encryption key validation") {
  std::array<std::uint8_t, 2000> encrypted_packet_buffer;
  std::array<char, 2000> decrypted_packet_buffer;

  auto accumulator = EncryptedPacketAccumulator(encrypted_packet_buffer, decrypted_packet_buffer);
  REQUIRE(!accumulator.set_encryption_key("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA").has_value());
  REQUIRE(!accumulator.set_encryption_key("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa").has_value());
  REQUIRE(*accumulator.set_encryption_key("AAAAAAAAAAA") == EncryptedPacketAccumulator::SetEncryptionKeyError::EncryptionKeyLengthIsNot32Bytes);
  REQUIRE(*accumulator.set_encryption_key("GAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA") ==
          EncryptedPacketAccumulator::SetEncryptionKeyError::EncryptionKeyContainsNonHexSymbols);
}

TEST_CASE("BufferOverflow when telegram length exceeds capacity") {
  std::array<std::uint8_t, 10> encrypted_packet_buffer;
  std::array<char, 10> decrypted_packet_buffer;

  EncryptedPacketAccumulator acc(encrypted_packet_buffer, decrypted_packet_buffer);
  for (const auto byte : encrypted_packet) {
    const auto& res = acc.process_byte(byte);
    if (res.error()) {
      REQUIRE(*res.error() == EncryptedPacketAccumulator::Error::BufferOverflow);
      return;
    }
  }
  REQUIRE(false);
}

TEST_CASE("Telegram is too small") {
  std::array<std::uint8_t, 2000> encrypted_packet_buffer;
  std::array<char, 2000> decrypted_packet_buffer;

  EncryptedPacketAccumulator acc(encrypted_packet_buffer, decrypted_packet_buffer);
  auto too_small_packet = encrypted_packet;
  change_length(too_small_packet, 16);

  for (const auto byte : too_small_packet) {
    const auto& res = acc.process_byte(byte);
    if (res.error()) {
      REQUIRE(*res.error() == EncryptedPacketAccumulator::Error::HeaderCorrupted);
      return;
    }
  }
  REQUIRE(false);
}

TEST_CASE("Receive many packets") {
  std::array<std::uint8_t, 500> encrypted_packet_buffer;
  std::array<char, 500> decrypted_packet_buffer;

  auto accumulator = EncryptedPacketAccumulator(encrypted_packet_buffer, decrypted_packet_buffer);
  REQUIRE(!accumulator.set_encryption_key("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA").has_value());

  const auto& garbage = std::vector<std::uint8_t>(100, 0x55);

  auto packet_corrupted = encrypted_packet;
  packet_corrupted[50] ^= 0xFF;

  auto packet_too_short_length = encrypted_packet;
  change_length(packet_too_short_length, 16);

  auto packet_too_long_length = encrypted_packet;
  change_length(packet_too_long_length, 2000);

  size_t received_packets = 0;
  std::vector<EncryptedPacketAccumulator::Error> occurred_errors;

  for (const auto byte : concat(garbage, encrypted_packet, garbage, packet_too_short_length, packet_corrupted, encrypted_packet, packet_corrupted,
                                encrypted_packet, packet_too_long_length, encrypted_packet)) {
    auto res = accumulator.process_byte(byte);

    if (res.packet()) {
      received_packets++;
    }

    if (res.error()) {
      occurred_errors.push_back(*res.error());
    }
  }

  REQUIRE(received_packets == 4);

  using enum EncryptedPacketAccumulator::Error;
  REQUIRE(occurred_errors == std::vector{HeaderCorrupted, HeaderCorrupted, HeaderCorrupted, HeaderCorrupted, DecryptionFailed, DecryptionFailed, BufferOverflow,
                                         HeaderCorrupted, HeaderCorrupted, HeaderCorrupted});
}
