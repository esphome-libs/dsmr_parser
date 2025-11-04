// This code tests that the header has all necessary dependencies included in its headers.
// We check that the code compiles.

#include "dsmr_parser/aes128gcm_mbedtls.h"

using namespace dsmr_parser;

void Aes128GcmMbedtls_some_function() {
  Aes128GcmMbedTls aes;
  const auto& key = Aes128GcmEncryptionKey::from_hex("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
  aes.set_encryption_key(*key);
}
