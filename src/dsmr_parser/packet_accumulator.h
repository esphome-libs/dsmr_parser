#pragma once
#include "util.h"
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace dsmr_parser {

// Receives unencrypted DSMR packets.
class PacketAccumulator final {
  class DsmrPacketBuffer final {
    std::span<uint8_t> _buffer;
    std::size_t _packetSize = 0;

  public:
    explicit DsmrPacketBuffer(std::span<uint8_t> buffer) : _buffer{buffer} {}

    std::string_view packet() const { return std::string_view(reinterpret_cast<const char*>(_buffer.data()), _packetSize); }

    void add(uint8_t byte) {
      _buffer[_packetSize] = byte;
      _packetSize++;
    }

    bool has_space() const { return _packetSize < _buffer.size(); }

    uint16_t calculate_crc16() const {
      uint16_t crc = 0;
      for (std::size_t i = 0; i < _packetSize; ++i) {
        crc ^= static_cast<uint8_t>(_buffer[i]);
        for (std::size_t bit = 0; bit < 8; bit++) {
          if (crc & 1)
            crc = (crc >> 1) ^ 0xa001;
          else
            crc = (crc >> 1);
        }
      }
      return crc;
    }
  };

  class CrcAccumulator final {
    uint16_t crc = 0;
    size_t amount_of_crc_nibbles = 0;

  public:
    bool add_to_crc(uint8_t byte) {
      if (byte >= '0' && byte <= '9') {
        byte = byte - '0';
      } else if (byte >= 'A' && byte <= 'F') {
        byte = static_cast<uint8_t>(byte - 'A' + 10);
      } else if (byte >= 'a' && byte <= 'f') {
        byte = static_cast<uint8_t>(byte - 'a' + 10);
      } else {
        return false;
      }

      crc = static_cast<uint16_t>((crc << 4) | (byte & 0xF));
      amount_of_crc_nibbles++;
      return true;
    }

    bool has_full_crc() const { return amount_of_crc_nibbles == 4; }

    uint16_t crc_value() const { return crc; }
  };

  enum class State { WaitingForPacketStartSymbol, WaitingForPacketEndSymbol, WaitingForCrc };
  State _state = State::WaitingForPacketStartSymbol;
  std::span<uint8_t> _raw_buffer;
  DsmrPacketBuffer _buf;
  CrcAccumulator _crc_accumulator;
  bool _check_crc;

public:
  PacketAccumulator(std::span<uint8_t> buffer, bool check_crc) : _raw_buffer(buffer), _buf(buffer), _check_crc(check_crc) {}

  std::optional<DsmrUnencryptedTelegram> process_byte(const uint8_t byte) {
    if (!_buf.has_space()) {
      Logger::log(LogLevel::DEBUG, "Buffer overflow. Discarding the accumulated data");
      _buf = DsmrPacketBuffer(_raw_buffer);
      _state = State::WaitingForPacketStartSymbol;
    }

    if (byte == '/') {
      Logger::log(LogLevel::VERBOSE, "Found telegram start symbol '/'");
      _buf = DsmrPacketBuffer(_raw_buffer);
      _buf.add(byte);
      _state = State::WaitingForPacketEndSymbol;
      return std::nullopt;
    }

    switch (_state) {
    case State::WaitingForPacketStartSymbol:
      return std::nullopt;

    case State::WaitingForPacketEndSymbol:
      _buf.add(byte);

      if (byte != '!') {
        return std::nullopt;
      }

      Logger::log(LogLevel::VERBOSE, "Found telegram end symbol '!'");
      if (!_check_crc) {
        _state = State::WaitingForPacketStartSymbol;
        Logger::log(LogLevel::VERBOSE, "Successfully received the telegram without CRC check");
        return DsmrUnencryptedTelegram(_buf.packet());
      }

      _state = State::WaitingForCrc;
      _crc_accumulator = CrcAccumulator();
      return std::nullopt;

    case State::WaitingForCrc:
      if (!_crc_accumulator.add_to_crc(byte)) {
        Logger::log(LogLevel::DEBUG, "Incorrect CRC character '%c'", byte);
        _state = State::WaitingForPacketStartSymbol;
        return std::nullopt;
      }

      if (!_crc_accumulator.has_full_crc()) {
        return std::nullopt;
      }

      _state = State::WaitingForPacketStartSymbol;

      if (_crc_accumulator.crc_value() == _buf.calculate_crc16()) {
        Logger::log(LogLevel::VERBOSE, "Successfully received the telegram with correct CRC");
        return DsmrUnencryptedTelegram(_buf.packet());
      }

      Logger::log(LogLevel::DEBUG, "CRC mismatch: expected %04X, got %04X", _crc_accumulator.crc_value(), _buf.calculate_crc16());
      return std::nullopt;
    }

    // unreachable
    return std::nullopt;
  }
};

}
