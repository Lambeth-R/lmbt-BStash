#pragma once
#include "Common.h"

namespace
{
    template<typename T>
    std::string UTF8_EncodeEx(const T &data)
    {
        const size_t result_size = (data.size() / sizeof(T::value_type)) + 1;
        std::string result_string(result_size, 0x00);
        size_t converted_data = 0;
        auto err = wcstombs_s(&converted_data, result_string.data(), result_size, reinterpret_cast<const wchar_t *>(data.data()), _TRUNCATE);
        result_string.resize(converted_data);
        return result_string;
    }

    template<typename T>
    std::wstring UTF8_DecodeEx(const T &data)
    {
        const size_t result_size = (data.size() / sizeof(T::value_type)) + 1;
        std::wstring result_string(result_size, 0x00);
        size_t converted_data = 0;
        auto err = mbstowcs_s(&converted_data, result_string.data(), result_size, reinterpret_cast<const char *>(data.data()), _TRUNCATE);
        result_string.resize(converted_data);
        return result_string;
    }
}

namespace ConvertUTF
{
    inline
#if defined IS_CPP_14
    std::string UTF8_Encode(const std::wstring &iData)
#else
    std::string UTF8_Encode(std::wstring_view iData)
#endif
    {
        return UTF8_EncodeEx(iData);
    }

    inline
#if defined IS_CPP_14
    std::wstring UTF8_Decode(const std::string &iData)
#else
    std::wstring UTF8_Decode(std::string_view iData)
#endif
    {
        return UTF8_DecodeEx(iData);
    }

    inline std::string UTF8_Encode(const std::vector<uint8_t> &iData)
    {
        return UTF8_EncodeEx(iData);
    }

    inline std::wstring UTF8_Decode(const std::vector<uint8_t> &iData)
    {
        return UTF8_DecodeEx(iData);
    }
}