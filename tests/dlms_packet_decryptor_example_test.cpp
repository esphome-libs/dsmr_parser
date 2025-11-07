#include "dsmr_parser/aes128gcm_mbedtls.h" // or "dsmr_parser/aes128gcm_bearssl.h"
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

std::array<uint8_t, 4000> dlms_packet_buffer; // Buffer to store the incoming bytes from the P1 port and the decrypted dsmr telegram
size_t dlms_packet_buffer_position = 0;       // needed to accumulate bytes

DlmsPacketDecryptor<Aes128GcmMbedTls> decryptor;

long last_read_timestamp = 0; // timestamp of the last byte received. Needed to detect inter-frame gaps.

Uart uart; // UART connected to P1 port

// This encryption key is unique per smart meter and must be provided by the electricity company.
const auto encryption_key = *Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");

// You must set the encryption key before calling `decryptor.decrypt_inplace` method.
inline void set_encryption_key() { decryptor.set_encryption_key(encryption_key); }

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
    last_read_timestamp = millis();
  }

  // detect inter-frame delay. If no byte is received for more than 1 second, then the packet is complete
  if ((millis() - last_read_timestamp) > 1000 && dlms_packet_buffer_position > 0) {
    std::optional<std::string_view> dsmr_telegram = decryptor.decrypt_inplace({dlms_packet_buffer.data(), dlms_packet_buffer_position});
    dlms_packet_buffer_position = 0; // reset for the next packet

    // check that decryption was successful
    if (!dsmr_telegram) {
      printf("Failed to decrypt packet\n");
      return;
    }

    // decrypted_dsmr_telegram is a normal DSMR telegram with a CRC checksum at the end.
    printf("Decrypted DSMR telegram:\n%s\n", std::string(*dsmr_telegram).c_str());
    // Parse it using P1Parser::parse() method.
  }
}
