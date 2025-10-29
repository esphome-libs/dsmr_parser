// This code tests that the encrypted_packet_accumulator header has all necessary dependencies included in its headers.
// We check that the code compiles.

#include "dsmr_parser/encrypted_packet_accumulator.h"

std::array<uint8_t, 1000> encrypted_packet_buffer;
std::array<char, 1000> decrypted_packet_buffer;
void EncryptedPacketAccumulator_some_function() { dsmr_parser::EncryptedPacketAccumulator(encrypted_packet_buffer, decrypted_packet_buffer); }
