#define MODULE_NAME "LmbtSpace"
#include "Include/stdafx.h"
#include "Include/Cypher.h"
#include "Include/UTF_Unocode.h"

int WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, char* lpCmdLine, int nShowCmd)
{
    LogClass::InitConsole();
    LogClass::InitLogger(eLogLevels::All, "Test", eLogType::Debug | eLogType::Print);
    LOG(eLogLevels::Info, L"Привет " << "World!");
    LOG(eLogLevels::All, "All");
    LOG(eLogLevels::Error, "Error");
    LOG(eLogLevels::Warning, "Warning");
    LOG(eLogLevels::Info, "Info");
    LOG(eLogLevels::Debug, "Debug");
    LOG(eLogLevels::Info, L"横尾 太郎");
    //int t = getc(stdin);
    //t -= 48;
    //float tF = 3 / t;
    //*(int*)0 = 0;
    AES aes;
    aes.ImportKeys("970370d71060ec7aa4bf88a98cfc7021c1ab215802f34ede059cd7314d78389c", "80fe42b028d04afa83562b253efa7f4f");
    std::string str_to_encode = "Some string";
    std::string encoded_str, decoded_str;
    aes.Encrypt(str_to_encode, encoded_str);
    aes.Decrypt(encoded_str, decoded_str);
    if (str_to_encode != decoded_str)
    {
        LOG(eLogLevels::Error, "AES failed");
    }
}