// This code tests that the header has all necessary dependencies included in its headers.
// We check that the code compiles.

#include "dsmr_parser/decryption/aes128gcm_tfpsa.h"

using namespace dsmr_parser;

void Aes128GcmTfPsa_some_function() {
  Aes128GcmTfPsa aes;
  const auto& key = Aes128GcmDecryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  aes.set_encryption_key(*key);
}
