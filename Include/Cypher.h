#pragma once

#include <string>
#include <array>
#include <Windows.h>
#include <wincrypt.h>

#define AES_BLOCK_SIZE 32UL

typedef struct AES256KEYBLOB_ {
    BLOBHEADER bhHdr;
    DWORD dwKeySize;
    BYTE szBytes[AES_BLOCK_SIZE] = {0};
} AES256KEYBLOB;

class AES
{
private:
    std::array<uint8_t, AES_BLOCK_SIZE> m_Key;
    std::array<uint8_t, AES_BLOCK_SIZE / 2> m_IV;
    bool InitContext(HCRYPTPROV& provider, HCRYPTKEY& key);

public:
    AES() = default;
    ~AES() = default;
    AES(const AES& rhs)
    {
        *this = rhs;
    };
    AES& operator= (const AES& rhs)
    {
        std::copy(std::begin(rhs.m_Key), std::end(rhs.m_Key), std::begin(m_Key));
        std::copy(std::begin(rhs.m_IV), std::end(rhs.m_IV), std::begin(m_IV));
        return *this;
    }
    bool EncryptInPlace(std::string& iString);
    bool DecryptInPlace(std::string& iString);
    inline bool Encrypt(std::string& iString, std::string& oString) { oString = iString; return EncryptInPlace(oString); }
    inline bool Decrypt(std::string& iString, std::string& oString) { oString = iString; return DecryptInPlace(oString); }
    bool ImportKeys(const std::string& key, const std::string& iv);
    bool ExportKeys(std::string& key, std::string& iv);
};

