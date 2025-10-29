#include "dsmr_parser/encrypted_packet_accumulator.h"
#include "dsmr_parser/fields.h"
#include "dsmr_parser/parser.h"
#include <doctest.h>
#include <iostream>

using namespace dsmr_parser;
using namespace fields;

TEST_CASE("EncryptedPacketAccumulator example") {

  // Buffers to store the incoming bytes and the decrypted packet.
  // Both of these buffers must be large enough to hold the full DSMR message.
  // Advice: define the buffers as global variables, to avoid using stack and heap memory.
  std::array<uint8_t, 4000> encrypted_packet_buffer;
  std::array<char, 4000> decrypted_packet_buffer;

  // For the sake of this example, this variable is supposed to contain data that comes from the P1 port.
  std::vector<uint8_t> encrypted_data_from_p1_port;

  // This class is similar to the PacketAccumulator, but it handles encrypted packets.
  // Use this class only if you have a smart meter that uses encryption.
  // You only need to create this class once.
  EncryptedPacketAccumulator accumulator(encrypted_packet_buffer, decrypted_packet_buffer);

  // Set the encryption key. This key is unique for each smart meter and should be provided by your energy supplier.
  const auto error = accumulator.set_encryption_key("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  if (error) {
    printf("Failed to set encryption key: %s", to_string(*error));
    return;
  }

  // Main loop.
  // We need to read data from P1 port 1 byte at a time.
  for (const auto& byte : encrypted_data_from_p1_port) {
    // feed the byte to the accumulator
    auto res = accumulator.process_byte(byte);

    // During receiving, errors may occur.
    // You can optionally log these errors, or ignore them.
    if (res.error()) {
      printf("Error during receiving a packet: %s", to_string(*res.error()));
    }

    // When a full packet is received, the packet() method returns unencrypted packet.
    if (res.packet()) {
      // Parse the received packet the same way as with the PacketAccumulator example
    }

    // The specification says that packets arrive once every 10 seconds.
    // In case some bytes are lost during transmission, you need to use a timeout to detect when a packet transmission finishes.
    // For example, if you stopped receiving bytes for more than 1 second, but you haven't received a complete packet yet,
    // you should reset EncryptedPacketAccumulator and start receiving a new packet from scratch.
    if (/* timeout */ false) {
      accumulator = EncryptedPacketAccumulator(encrypted_packet_buffer, decrypted_packet_buffer);
    }
  }
}
