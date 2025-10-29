#pragma once
#include "util.h"
#include <cstdint>
#include <optional>
#include <string_view>
#include <span>

namespace dsmr_parser {

// Receives unencrypted DSMR packets.
class PacketAccumulator {
  class DsmrPacketBuffer {
    std::span<char> _buffer;
    std::size_t _packetSize = 0;

  public:
    explicit DsmrPacketBuffer(std::span<char> buffer) : _buffer{buffer} {}

    std::string_view packet() const { return std::string_view(_buffer.data(), _packetSize); }

    void add(char byte) {
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

  class CrcAccumulator {
    uint16_t crc = 0;
    size_t amount_of_crc_nibbles = 0;

  public:
    bool add_to_crc(char byte) {
      if (byte >= '0' && byte <= '9') {
        byte = byte - '0';
      } else if (byte >= 'A' && byte <= 'F') {
        byte = static_cast<char>(byte - 'A' + 10);
      } else if (byte >= 'a' && byte <= 'f') {
        byte = static_cast<char>(byte - 'a' + 10);
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
  std::span<char> _raw_buffer;
  DsmrPacketBuffer _buf;
  CrcAccumulator _crc_accumulator;
  bool _check_crc;

public:
  enum class Error {
    BufferOverflow,
    PacketStartSymbolInPacket,
    IncorrectCrcCharacter,
    CrcMismatch,
  };

  class Result {
    friend class PacketAccumulator;

    std::optional<std::string_view> _packet;
    std::optional<Error> _error;

    Result() = default;
    Result(std::string_view packet) : _packet(packet) {}
    Result(Error error) : _error(error) {}

  public:
    auto packet() const { return _packet; }
    auto error() const { return _error; }
  };

  PacketAccumulator(std::span<char> buffer, bool check_crc) : _raw_buffer(buffer), _buf(buffer), _check_crc(check_crc) {}

  Result process_byte(const char byte) {
    if (!_buf.has_space()) {
      _buf = DsmrPacketBuffer(_raw_buffer);
      _state = State::WaitingForPacketStartSymbol;
      if (byte != '/') {
        return Error::BufferOverflow;
      }
    }

    if (byte == '/') {
      _buf = DsmrPacketBuffer(_raw_buffer);
      _buf.add(byte);
      const auto prev_state = _state;
      _state = State::WaitingForPacketEndSymbol;

      if (prev_state == State::WaitingForPacketEndSymbol || prev_state == State::WaitingForCrc) {
        return Error::PacketStartSymbolInPacket;
      }
      return {};
    }

    switch (_state) {
    case State::WaitingForPacketStartSymbol:
      return {};

    case State::WaitingForPacketEndSymbol:
      _buf.add(byte);

      if (byte != '!') {
        return {};
      }

      if (!_check_crc) {
        _state = State::WaitingForPacketStartSymbol;
        return Result(_buf.packet());
      }

      _state = State::WaitingForCrc;
      _crc_accumulator = CrcAccumulator();
      return {};

    case State::WaitingForCrc:
      if (!_crc_accumulator.add_to_crc(byte)) {
        _state = State::WaitingForPacketStartSymbol;
        return Error::IncorrectCrcCharacter;
      }

      if (!_crc_accumulator.has_full_crc()) {
        return {};
      }

      _state = State::WaitingForPacketStartSymbol;

      if (_crc_accumulator.crc_value() == _buf.calculate_crc16()) {
        return _buf.packet();
      }

      return Error::CrcMismatch;
    }

    // unreachable
    return {};
  }
};

inline const char* to_string(const PacketAccumulator::Error error) {
  switch (error) {
  case PacketAccumulator::Error::BufferOverflow:
    return "BufferOverflow";
  case PacketAccumulator::Error::PacketStartSymbolInPacket:
    return "PacketStartSymbolInPacket";
  case PacketAccumulator::Error::IncorrectCrcCharacter:
    return "IncorrectCrcCharacter";
  case PacketAccumulator::Error::CrcMismatch:
    return "CrcMismatch";
  }

  // unreachable
  return "Unknown error";
}

}
