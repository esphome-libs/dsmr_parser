// This code tests that the encrypted_packet_accumulator header has all necessary dependencies included in its headers.
// We check that the code compiles.

#include "dsmr_parser/dlms_packet_decryptor.h"

std::array<uint8_t, 1000> encrypted_packet_buffer;
std::array<char, 1000> decrypted_packet_buffer;
const auto encryption_key = *dsmr_parser::DlmsPacketDecryptor::EncryptionKey::FromHex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
void DlmsPacketDecryptor_some_function() {
  const auto& decryptor = dsmr_parser::DlmsPacketDecryptor(decrypted_packet_buffer);
  decryptor.decrypt(encrypted_packet_buffer, encryption_key);
}
