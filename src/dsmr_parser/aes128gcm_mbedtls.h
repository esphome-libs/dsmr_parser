#pragma once
#include "aes128gcm.h"
#include "util.h"
#include <mbedtls/gcm.h>
#include <span>

namespace dsmr_parser {

class Aes128GcmMbedTls final : NonCopyableAndNonMovable {
  mbedtls_gcm_context gcm;

public:
  Aes128GcmMbedTls() { mbedtls_gcm_init(&gcm); }

  void set_encryption_key(const Aes128GcmEncryptionKey& key) { mbedtls_gcm_setkey(&gcm, MBEDTLS_CIPHER_ID_AES, key.data(), 128); }

  bool decrypt_inplace(const std::array<const uint8_t, 17>& aad, const std::array<const uint8_t, 12>& nonce, std::span<uint8_t> ciphertext,
                       const std::array<const uint8_t, 12>& tag) {
    const auto& res = mbedtls_gcm_auth_decrypt(&gcm, ciphertext.size(), nonce.data(), nonce.size(), aad.data(), aad.size(), tag.data(), tag.size(),
                                               ciphertext.data(), ciphertext.data());
    return res == 0;
  }

  ~Aes128GcmMbedTls() { mbedtls_gcm_free(&gcm); }
};

}
