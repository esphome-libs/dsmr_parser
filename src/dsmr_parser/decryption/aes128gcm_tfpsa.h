#pragma once

#include "../util.h"
#include "aes128gcm.h"
#include <psa/crypto.h>
#include <span>

namespace dsmr_parser {

class Aes128GcmTfPsa final : public Aes128GcmDecryptor, NonCopyableAndNonMovable {
  psa_key_id_t key_id = 0;
  bool initialized = false;

public:
  Aes128GcmTfPsa() { initialized = (psa_crypto_init() == PSA_SUCCESS); }

  void set_encryption_key(const Aes128GcmDecryptionKey& key) override {
    if (!initialized) {
      return;
    }

    if (key_id != 0) {
      psa_destroy_key(key_id);
      key_id = 0;
    }

    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
    psa_set_key_bits(&attributes, 128);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_DECRYPT);
    psa_set_key_algorithm(&attributes, PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, 12));

    const psa_status_t status = psa_import_key(&attributes, key.data(), 16, &key_id);

    psa_reset_key_attributes(&attributes);

    if (status != PSA_SUCCESS) {
      key_id = 0;
    }
  }

  bool decrypt_inplace(std::span<const uint8_t, 17> aad, std::span<const uint8_t, 12> nonce, std::span<uint8_t> ciphertext,
                       std::span<const uint8_t, 12> tag) override {
    if (!initialized || key_id == 0) {
      return false;
    }

    psa_aead_operation_t op = PSA_AEAD_OPERATION_INIT;
    size_t out_len = 0;
    size_t tail_len = 0;

    psa_status_t status = psa_aead_decrypt_setup(&op, key_id, PSA_ALG_AEAD_WITH_SHORTENED_TAG(PSA_ALG_GCM, tag.size()));
    if (status != PSA_SUCCESS) {
      psa_aead_abort(&op);
      return false;
    }

    status = psa_aead_set_nonce(&op, nonce.data(), nonce.size());
    if (status != PSA_SUCCESS) {
      psa_aead_abort(&op);
      return false;
    }

    status = psa_aead_update_ad(&op, aad.data(), aad.size());
    if (status != PSA_SUCCESS) {
      psa_aead_abort(&op);
      return false;
    }

    status = psa_aead_update(&op, ciphertext.data(), ciphertext.size(), ciphertext.data(), ciphertext.size(), &out_len);
    if (status != PSA_SUCCESS || out_len != ciphertext.size()) {
      psa_aead_abort(&op);
      return false;
    }

    status = psa_aead_verify(&op, nullptr, 0, &tail_len, tag.data(), tag.size());

    if (status != PSA_SUCCESS) {
      psa_aead_abort(&op);
      return false;
    }

    if (tail_len != 0) {
      psa_aead_abort(&op);
      return false;
    }

    return true;
  }

  ~Aes128GcmTfPsa() {
    if (key_id != 0) {
      psa_destroy_key(key_id);
    }
  }
};

} // namespace dsmr_parser
