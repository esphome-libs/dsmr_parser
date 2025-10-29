#!/usr/bin/env python3
# Generate an encrypted DLMS packet using AES128-GCM encryption.
# It is a packet that is sent by Luxembourg Smarty P1.
# The packet format is described in the file "specs/Luxembourg Smarty P1 specification v1.1.3.pdf" chapter "3.2.5 P1 software – Channel security"

from binascii import unhexlify
from dlms_cosem.protocol.xdlms import GeneralGlobalCipher
from dlms_cosem.security import SecurityControlField, encrypt

TELEGRAM_SAGEMCOM_T210_D_R = (
    '/EST5\\253710000_A\r\n'
    '\r\n'
    '1-3:0.2.8(50)\r\n'
    '0-0:1.0.0(221006155014S)\r\n'
    '1-0:1.8.0(006545766*Wh)\r\n'
    '1-0:1.8.1(005017120*Wh)\r\n'
    '1-0:1.8.2(001528646*Wh)\r\n'
    '1-0:1.7.0(000000286*W)\r\n'
    '1-0:2.8.0(000000058*Wh)\r\n'
    '1-0:2.8.1(000000000*Wh)\r\n'
    '1-0:2.8.2(000000058*Wh)\r\n'
    '1-0:2.7.0(000000000*W)\r\n'
    '1-0:3.8.0(000000747*varh)\r\n'
    '1-0:3.8.1(000000000*varh)\r\n'
    '1-0:3.8.2(000000747*varh)\r\n'
    '1-0:3.7.0(000000000*var)\r\n'
    '1-0:4.8.0(003897726*varh)\r\n'
    '1-0:4.8.1(002692848*varh)\r\n'
    '1-0:4.8.2(001204878*varh)\r\n'
    '1-0:4.7.0(000000166*var)\r\n'
    '!7EF9\r\n')

# The code below creates full_frame with the format:
# Tag (0xDB) | SystemTitleLength (0x08) | SystemTitle (8 bytes) | LongFormLengthIndicator (0x82) | TotalLength (2 bytes, big endian) (SecurityControlFieldLength + InvocationCounterLength + CiphertextLength + GcmTagLength) | SecurityControlField (0x30) | InvocationCounter (4 bytes, big endian) (also called "frame counter") | EncryptedTelegram | GcmTag (12 bytes)

system_title = "SYSTEMID".encode("ascii")  # must be 8 bytes
invocation_counter = [0x10, 0x00, 0x00, 0x01]  # must be 4 bytes (can have any value) big endian

securityControlField = SecurityControlField(security_suite=0, authenticated=True, encrypted=True)
encryptedTelegramWithGcmTag = encrypt(security_control=securityControlField,
                                      key=unhexlify("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"),      # the encryption key is supposed to be provided by the electricity provider.

                                      auth_key=unhexlify("00112233445566778899AABBCCDDEEFF"), # the auth_key is hardcoded and specified in "specs/Luxembourg Smarty P1 specification v1.1.3.pdf"
                                                                                              # chapter "3.2.5 P1 software – Channel security".
                                      system_title=system_title,
                                      invocation_counter=int.from_bytes(invocation_counter),
                                      plain_text=TELEGRAM_SAGEMCOM_T210_D_R.encode("ascii"))

full_frame = bytearray(GeneralGlobalCipher.TAG.to_bytes(1, "big", signed=False)) # = 0xDB
full_frame.extend([0x08]) # length of the title
full_frame.extend(system_title) # 8 bytes string
full_frame.extend([0x82]) # it means 2 length bytes follow

total_len = 1 + 4 + len(encryptedTelegramWithGcmTag) # TotalLength = SecurityControlFieldLength + InvocationCounterLength + CiphertextWithGcmTagLength
full_frame.extend(total_len.to_bytes(2, "big", signed=False))
full_frame.extend(securityControlField.to_bytes()) # = 0x30
full_frame.extend(invocation_counter) # invocation counter (any 4 bytes value, big endian)
full_frame.extend(encryptedTelegramWithGcmTag)

with open("encrypted_packet.bin", "wb") as f:
    f.write(full_frame)
