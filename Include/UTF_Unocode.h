#pragma once
#include <clocale>
#include <codecvt>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <locale.h>

#if _HAS_CXX17
#include <string_view>
inline const std::wstring UTF8_Decode(std::string_view aStr)
{
    const size_t& wstr_len = aStr.size() + 1;
    std::unique_ptr<wchar_t[]> twPtr(new wchar_t[wstr_len]);
    if (MultiByteToWideChar(CP_UTF8, WC_DEFAULTCHAR | MB_ERR_INVALID_CHARS, aStr.data(), -1, twPtr.get(), static_cast<DWORD>(wstr_len)) < wstr_len)
    {
        return {};
    }
    return std::wstring(twPtr.get(), wstr_len - 1);
}

inline const std::string UTF8_Encode(std::wstring_view wStr)
{
    const size_t& astr_len = wStr.size() + 1;
    std::unique_ptr<char[]> taPtr(new char[astr_len]);
    if(WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | MB_ERR_INVALID_CHARS, wStr.data(), -1, taPtr.get(), static_cast<DWORD>(astr_len), 0, 0) < astr_len)
    {
        return {};
    };
    return std::string(taPtr.get(), astr_len - 1);
}
#else
inline const std::wstring UTF8_Decode(const std::string &aStr)
{
    const size_t& wstr_len = aStr.size() + 1;
    std::unique_ptr<wchar_t[]> twPtr(new wchar_t[wstr_len]);
    if (MultiByteToWideChar(CP_UTF8, 0, aStr.data(), -1, twPtr.get(), static_cast<int>(wstr_len)) < wstr_len)
    {
        return {};
    }
    return std::wstring(twPtr.get(), wstr_len - 1);
}

inline const std::string UTF8_Encode(const std::wstring &wStr)
{
    const size_t& astr_len = wStr.size() + 1;
    std::unique_ptr<char[]> taPtr(new char[astr_len]);
    if (WideCharToMultiByte(CP_ACP, 0, wStr.data(), -1, taPtr.get(), static_cast<int>(astr_len), 0, 0) < astr_len)
    {
        return {};
    };
    return std::string(taPtr.get(), astr_len - 1);
}
#endif