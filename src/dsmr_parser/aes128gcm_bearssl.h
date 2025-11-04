#pragma once
#include "aes128gcm.h"
#include "util.h"
#include <bearssl.h>
#include <span>

namespace dsmr_parser {

class Aes128GcmBearSsl final : NonCopyableAndNonMovable {
  br_gcm_context gcm;
  br_aes_ct_ctr_keys aes;
  bool initialized = false;

public:
  Aes128GcmBearSsl() = default;

  void set_encryption_key(const Aes128GcmEncryptionKey& key) {
    br_aes_ct_ctr_init(&aes, key.data(), 16);
    br_gcm_init(&gcm, &aes.vtable, br_ghash_ctmul32);
    initialized = true;
  }

  bool decrypt_inplace(const std::array<const uint8_t, 17>& aad, const std::array<const uint8_t, 12>& nonce, std::span<uint8_t> ciphertext,
                       const std::array<const uint8_t, 12>& tag) {
    if (!initialized) {
      return false;
    }

    br_gcm_reset(&gcm, nonce.data(), nonce.size());
    br_gcm_aad_inject(&gcm, aad.data(), aad.size());
    br_gcm_flip(&gcm);
    br_gcm_run(&gcm, 0, ciphertext.data(), ciphertext.size());
    return br_gcm_check_tag_trunc(&gcm, tag.data(), tag.size());
  }

  ~Aes128GcmBearSsl() = default;
};

}
