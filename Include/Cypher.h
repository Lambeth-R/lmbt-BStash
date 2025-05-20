#pragma once

#include <array>
#include <vector>

#include <bcrypt.h>
#include <Windows.h>
#include <wincrypt.h>

#include "common.h"

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
constexpr static size_t AES_BLOCK_SIZE = 32UL;

// Legacy AES-256
// Works perfectly well with all kind of data.
// Output data: Keys:           Hex Array
//              EncryptData:    Hex Array
// Crypto keys is stored as private parts of class. XXX rewrite as CNG.
class LIB_EXPORT AES
{
private:
    typedef struct AES256KEYBLOB_ {
        BLOBHEADER bhHdr;
        DWORD dwKeySize;
        BYTE szBytes[AES_BLOCK_SIZE] = { 0 };
    } AES256KEYBLOB;

    std::array<uint8_t, AES_BLOCK_SIZE>     m_Key;
    std::array<uint8_t, AES_BLOCK_SIZE / 2> m_IV;

    bool                InitContext(HCRYPTPROV& provider, HCRYPTKEY& key) const;

public:
                        AES(void)   = default;
                        ~AES()      = default;

                        AES(const AES& rhs);
    AES&                operator= (const AES&);

    bool                EncryptInPlace(std::vector<uint8_t> &iData) const;
    bool                EncryptInPlace(std::string &iData) const;

    bool                DecryptInPlace(std::vector<uint8_t> &iData) const;
    bool                DecryptInPlace(std::string &iData) const;

    bool                ImportKeys(const std::vector<uint8_t> &key, const std::vector<uint8_t> &iv);
    template <typename T>
    bool                ExportKeys(T &key, T &iv) const;
};

class LIB_EXPORT BaseCNG
{
protected:
              BCRYPT_ALG_HANDLE InitAlgorithm(const wchar_t* alg);

public:
    // Random number generator with selected data length,
    // uses kernel to select generate bytes
    static                 bool FillBufferRNG(size_t iLen, std::vector<uint8_t> &oData);
    static std::vector<uint8_t> CalculateBase64(const std::vector<uint8_t> &iData);
};

// CNG based RSA impl.
// Can be used to Encrypt Decrypt data with zeroed padding.
// Output data: Keys:           ASN.1 PKCS#1
//              EncryptData:    Hex Array (Padding dependent if on - SHA-256, off - zeroed)
// Crypto keys is stored only in kernel. From moment of generation or import.
class LIB_EXPORT RSA : BaseCNG
{
public:
    enum KeyTypeFlags : uint8_t
    {
        KeyType_Undefined =  0,
        KeyType_Public    =  1,
        KeyType_Full      =  2,
        KeyType_BER       =  4,
        KeyType_DER       =  8,
        KeyType_PEM       =  16
    };
    using KeyType = FlagWrap<KeyTypeFlags>;


private:
    static constexpr char c_PemAddition[]   = "-----";
    static constexpr char c_PemBegin[]      = "BEGIN";
    static constexpr char c_PemEnd[]        = "END";

    static constexpr char c_PublicKeyId[]   = "RSA PUBLIC KEY";
    static constexpr char c_PrivateKeyId[]  = "RSA PRIVATE KEY";

    static constexpr bool ValidKeyType(const KeyType &flags)
    {
        // Can`t export Public & Full at the same time.
        // Can`t have both diffent PEM & DER / BER types of key at the same time
        // Undefined means go & define it.
        return 
               (flags  & KeyType_PEM)  ?  (!(flags & KeyType_DER)    || !(flags & KeyType_BER)) :
               ((flags & KeyType_DER) ||  (flags   & KeyType_BER))    ? !(flags & KeyType_PEM)  :
               (flags  & KeyType_Full) ? !(flags   & KeyType_Public)  : !(flags & KeyType_Full);
    }
    struct KeyParams
    {
        const char    *StructType = nullptr;
        const wchar_t *BlobType   = nullptr;
    };

    const BCRYPT_OAEP_PADDING_INFO  c_PaddingInfo       = { BCRYPT_SHA256_ALGORITHM, NULL, 0 };   // Padding struct
    BCRYPT_KEY_HANDLE               m_KeyHandle         = nullptr;                                // Kernel RSA key object.
    KeyType                         m_Keytype           = KeyType_Undefined;                     // Key type. Defined along with aquiring BCRYPT_KEY_HANDLE
    bool                            m_ExplicitPadding   = false;

    const     KeyParams FillParams(KeyType type) const;
    bool                PEM_Encode(KeyType type, std::vector<uint8_t> &data);
    bool                PEM_Decode(KeyType type, std::vector<uint8_t> &data);

public:
                        RSA(bool usePadding = false) : m_ExplicitPadding(usePadding) {}
                        RSA(const RSA&)        = delete;
                        RSA(const RSA&&)       = delete;
              auto     &operator=(const RSA&)  = delete;
              auto     &operator=(const RSA&&) = delete;
                        ~RSA();

    bool                GenerateKeyPair(uint16_t keySize);
    bool                ImportKeyInfo(KeyType type, const std::vector<uint8_t> &keyData);
    bool                ExportKeyInfo(KeyType type, std::vector<uint8_t> &keyData);
    bool                Encrypt(const std::vector<uint8_t> &iString, std::vector<uint8_t> &oString) const;
    bool                Decrypt(const std::vector<uint8_t> &iString, std::vector<uint8_t> &oString) const;
    void                ReleaseKeyPair();
};

// Polymorph class for multiple types of hashes
// Allows to Init & calculate via single string
// Hash_id().CalculateHash(data)
class LIB_EXPORT Hash : BaseCNG
{
private:
    // TODO: extend
    enum InnerHashType
    {
        Hash_Undefined  = 0,
        Hash_Crc32      = 1,
    };
    InnerHashType     m_ExternType = Hash_Undefined;
    BCRYPT_ALG_HANDLE m_AlgHandle  = nullptr;

                                      Hash() = delete;
                                      Hash(InnerHashType extenrT);
                                      Hash(const wchar_t *alg);
                                      Hash(const Hash&)       = delete;
                                      Hash(const Hash&&)      = delete;
           const Hash &               operator=(const Hash&)  = delete;
           const Hash &               operator=(const Hash&&) = delete;
           const std::vector<uint8_t> InnerHash(const std::vector<uint8_t> &iData) const;
public:
                                     ~Hash();
    static const Hash                 SHA_1();
    static const Hash                 SHA_256();
    static const Hash                 SHA_384();
    static const Hash                 SHA_512();

    static const Hash                 MD2();
    static const Hash                 MD4();
    static const Hash                 MD5();

    static const Hash                 Crc32();

           const std::vector<uint8_t> CalculateHash(const std::vector<uint8_t> &iData, const std::vector<uint8_t> &iSalt = {}) const;
           bool                       VerifyHashData(const std::vector<uint8_t> &hashData, const std::vector<uint8_t> &iData, const std::vector<uint8_t> &iSalt = {}) const;
};