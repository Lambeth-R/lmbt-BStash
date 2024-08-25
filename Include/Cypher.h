#pragma once

#include <array>
#include <vector>

#include <bcrypt.h>
#include <Windows.h>
#include <wincrypt.h>

#define AES_BLOCK_SIZE 32UL
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)

// Legacy AES-256
// Works perfectly well with all kind of data.
// Output data: Keys:           Hex Array
//              EncryptData:    Hex Array
// Crypto keys is stored as private parts of class. XXX rewrite as CNG.
class AES
{
private:
    typedef struct AES256KEYBLOB_ {
        BLOBHEADER bhHdr;
        DWORD dwKeySize;
        BYTE szBytes[AES_BLOCK_SIZE] = { 0 };
    } AES256KEYBLOB;

    std::array<uint8_t, AES_BLOCK_SIZE> m_Key;
    std::array<uint8_t, AES_BLOCK_SIZE / 2> m_IV;

    bool                InitContext(HCRYPTPROV& provider, HCRYPTKEY& key) const;

public:
                        AES(void)   = default;
                        ~AES()      = default;

                        AES(const AES& rhs);
    AES&                operator= (const AES&);

    bool                EncryptInPlace(std::vector<uint8_t> &iString) const;
    bool                DecryptInPlace(std::vector<uint8_t> &iString) const;
    bool                Encrypt(const std::vector<uint8_t> &iString, std::vector<uint8_t> &oString) const;
    bool                Decrypt(const std::vector<uint8_t> &iString, std::vector<uint8_t> &oString) const;
    bool                ExportKeys(std::vector<uint8_t> &key, std::vector<uint8_t> &iv) const;
    bool                ImportKeys(const std::vector<uint8_t> &key, const std::vector<uint8_t> &iv);
};

// CNG based RSA impl.
// Can be used to Encrypt Decrypt data with zeroed padding.
// Output data: Keys:           ASN.1 PKCS#1
//              EncryptData:    Hex Array (Padding dependent if on - SHA-256, off - zeroed)
// Crypto keys is stored only in kernel. From moment of generation or import.
class RSA
{
public:
    enum KeyType
    {
        KeyType_Undefined = -1,
        KeyType_Public,
        KeyType_Full
    };
private:
    struct KeyParams
    {
        const char    *StructType = nullptr;
        const wchar_t *BlobType   = nullptr;
    };

    const BCRYPT_OAEP_PADDING_INFO  c_PaddingInfo = { BCRYPT_SHA256_ALGORITHM, NULL, 0 };   // Padding struct
    BCRYPT_KEY_HANDLE               m_KeyHandle   = nullptr;                                // Kernel RSA key object.
    KeyType                         m_Keytype     = KeyType_Undefined;                      // Key type. Defined along with aquiring BCRYPT_KEY_HANDLE
    bool                            m_ExplicitPadding = false;

    constexpr KeyParams FillParams(KeyType type) const;
    BCRYPT_ALG_HANDLE   InitAlgorithm();
public:
                        RSA(bool usePadding = false) : m_ExplicitPadding(usePadding) {};
                        RSA(RSA&)        = delete;
                        RSA(RSA&&)       = delete;
    auto &              operator=(RSA&)  = delete;
    auto &              operator=(RSA&&) = delete;
                        ~RSA();

    bool                GenerateKeyPair(uint16_t keySize);
    bool                ImportKeyInfo(KeyType type, const std::vector<uint8_t> &keyData);
    bool                ExportKeyInfo(KeyType type, std::vector<uint8_t> &keyData);
    bool                Encrypt(const std::vector<uint8_t> &iString, std::vector<uint8_t> &oString) const;
    bool                Decrypt(const std::vector<uint8_t> &iString, std::vector<uint8_t> &oString) const;
    void                ReleaseKeyPair();
};