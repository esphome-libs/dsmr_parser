#include "dsmr_parser/dlms_packet_decryptor.h"
#include "dsmr_parser/fields.h"
#include "dsmr_parser/parser.h"
#include <doctest.h>
#include <iostream>

// Dummy functions to make the example compile
long millis() { return 0; }
struct Uart {
  size_t available() { return 0; }
  uint8_t readByte() { return 0; }
};

using namespace dsmr_parser;

std::array<uint8_t, 4000> dlms_packet_buffer; // Buffer to store the incoming bytes from the P1 port.
size_t dlms_packet_buffer_position = 0;       // needed to accumulate bytes

std::array<char, 4000> decrypted_telegram_buffer; // Buffer in which the decrypted dsmr telegram will be stored.

DlmsPacketDecryptor decryptor(decrypted_telegram_buffer);

long last_read = 0; // timestamp of the last byte received. Needed to detect inter-frame gaps.

Uart uart; // UART connected to P1 port

// This encryption key is unique per smart meter and must be provided by the electricity company.
const auto encryption_key = *DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

// Main loop that reads data from the P1 port and decrypts packets.
inline void loop() {
  // To receive the encrypted packets, we need to rely on the "inter-frame delay" technique.
  // The packets don't have any start and stop bytes. However, there is a guaranteed 10-second delay between packets.

  while (uart.available()) {
    // reset buffer if overflow.
    if (dlms_packet_buffer_position >= dlms_packet_buffer.size()) {
      dlms_packet_buffer_position = 0;
    }

    dlms_packet_buffer[dlms_packet_buffer_position] = uart.readByte();
    dlms_packet_buffer_position++;

    // save the time when we received the last byte
    last_read = millis();
  }

  // detect inter-frame delay. If no byte is received for more than 1 second, then the packet is complete
  if ((millis() - last_read) > 1000 && dlms_packet_buffer_position > 0) {
    auto res = decryptor.decrypt({dlms_packet_buffer.data(), dlms_packet_buffer_position}, encryption_key);

    // check that decryption was successful
    if (res.error()) {
      printf("Failed to decrypt packet: %s\n", to_string(*res.error()));
      return;
    }

    std::string_view decrypted_dsmr_telegram = *res.dsmr_telegram();
    // decrypted_dsmr_telegram is a normal DSMR telegram with a CRC checksum at the end.
    printf("Decrypted DSMR telegram:\n%s\n", std::string(decrypted_dsmr_telegram).c_str());
    // Parse it using P1Parser::parse() method.
  }
}
