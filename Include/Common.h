#pragma once


// https://blog.kowalczyk.info/article/j/guide-to-predefined-macros-in-c-compilers-gcc-clang-msvc-etc..html
// Platform separation.
#if defined __ANDROID__
#define PLATFORM_ANDROID
#elif defined __linux__ && !defined __ANDROID__
#define PLATFORM_LINUX
#elif defined __APPLE__
#define PLATFORM_APPLE
#elif defined _WIN32 && !defined _WIN64
#define PLATFORM_WIN32
#elif defined _WIN64
#define PLATFORM_WIN64
#else
#define PLATFORM_FREEBSD
#endif

// Compiler separation
#if defined _MSC_VER
#define COMPILER_MSVC
#elif defined __GNUC__
#define COMPILER_GNUC
#elif defined __clang__
#define COMPILER_CLANG
#elif defined __MINGW32__
#define COMPILER_MINGW32
#elif defined __MINGW64__
#define COMPILER_MINGW64
#endif


#if defined(_MSVC_LANG)
#define _CPP_VERSION _MSVC_LANG
#else 
#define _CPP_VERSION __cplusplus
#endif

#if _CPP_VERSION >= 201402L && _CPP_VERSION < 201703L
#define IS_CPP_14O // C+14 Only
#endif
#if _CPP_VERSION >= 201703L
#define IS_CPP_17G // C+17 & greater
#elif _CPP_VERSION >= 201703L && _CPP_VERSION < 202002L
#define IS_CPP_17O // C+17 Only
#endif
#if _CPP_VERSION >= 202002L
#define IS_CPP_20G // C+20 & greater
#endif

#if defined _WINDLL
#define LIB_EXPORT __declspec(dllexport)
#else
#define LIB_EXPORT 
#endif

#ifdef _MSC_VER
#include <stdio.h>
#include <vcruntime.h>
#define _CRT_SECURE_NO_WARNINGS
#endif 

#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <stdint.h>
#include <cstdio>
#include <wchar.h>
#if defined IS_CPP_17G
#include <string_view>
#endif
#if defined IS_CPP_20G
#include <span>
#endif

#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

#if !defined(__CRTDECL)
#define __CRTDECL
#endif

#if defined IS_CPP_17G
#define _CRT_SECURE_NO_WARNINGS
typedef int(__CRTDECL *sprintf_foo)(const void *, size_t, const void *, ...);

typedef int(__CRTDECL *sprintf_foo_l) (void*    const, const size_t, const void    * const, va_list args);
typedef int(__CRTDECL *sprintf_fooA_l)(char*    const, const size_t, const char    * const, va_list args);
typedef int(__CRTDECL *sprintf_fooW_l)(wchar_t* const, const size_t, const wchar_t * const, va_list args);
#endif

#if defined IS_CPP_17G
template<typename T, typename ... Args>
const std::basic_string<T> string_format(const std::basic_string_view<T> format, Args&& ...args)
{
    sprintf_foo printing = nullptr;
    if (typeid(T) == typeid(char))
    {
        printing = reinterpret_cast<sprintf_foo>(snprintf);
    }
    else if (typeid(T) == typeid(wchar_t))
    {
        printing = reinterpret_cast<sprintf_foo>(swprintf);
    }
    if (!printing)
    {
        return {};
    }
    int size_s = printing(nullptr, 0, format.data(), args ...) + 1;
    if (size_s <= 0) return {};
    std::unique_ptr<T[]> buf(new T[static_cast<size_t>(size_s)]);
    printing(buf.get(), static_cast<size_t>(size_s), format.data(), args...);
    return std::basic_string<T>(buf.get(), buf.get() + static_cast<size_t>(size_s) - 1);
}
template<typename T>
const std::basic_string<T> string_format_l(const std::basic_string_view<T> format, va_list args)
{
    sprintf_foo_l printing = nullptr;
    if (typeid(T) == typeid(char))
    {
        printing = reinterpret_cast<sprintf_foo_l>(static_cast<sprintf_fooA_l>(vsnprintf));
    }
    else if (typeid(T) == typeid(wchar_t))
    {
        printing = reinterpret_cast<sprintf_foo_l>(static_cast<sprintf_fooW_l>(vswprintf));
    }
    if (!printing)
    {
        return {};
    }
    int size_s = printing(nullptr, 0, format.data(), args) + 1;
    if (size_s <= 0) return {};
    std::unique_ptr<T[]> buf(new T[static_cast<size_t>(size_s)]);
    printing(buf.get(), static_cast<size_t>(size_s), format.data(), args);
    return std::basic_string<T>(buf.get(), buf.get() + static_cast<size_t>(size_s) - 1);
}

#else
template<typename T, typename ... Args>
const std::string string_format(const std::string &format, Args&& ...args)
{
    int size_s = snprintf(nullptr, 0, format.data(), args ...) + 1;
    if (size_s <= 0) return {};
    std::unique_ptr<char[]> buf(new char[static_cast<size_t>(size_s)]);
    snprintf(buf.get(), static_cast<size_t>(size_s), format.data(), args ...);
    return std::string(buf.get(), buf.get() + static_cast<size_t>(size_s) - 1);
}

template<typename T, typename ... Args>
const std::wstring string_format(const std::wstring &format, Args&& ...args)
{
    int size_s = swprintf(nullptr, 0, format.c_str(), args ...) + 1;
    if (size_s <= 0) return {};
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
    swprintf(buf.get(), size, format.c_str(), args ...);
    return std::wstring(buf.get(), buf.get() + size - 1);
}
#endif

template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
class FlagWrap
{
    T m_Value = {};
public:

    FlagWrap(const T &init) : m_Value(init) {};

    constexpr inline FlagWrap(std::initializer_list<T> flags) :
        m_Value(initializer_list_helper(flags.begin(), flags.end())) {}

    FlagWrap &operator=(const T &rhs) { m_Value = static_cast<T>(rhs); return *this; }

    inline constexpr T operator&(const T &b) const
    {
        return static_cast<T>(static_cast<uint8_t>(m_Value) & static_cast<uint8_t>(b));
    }

    inline constexpr T operator&(const FlagWrap &b) const
    {
        return static_cast<T>(static_cast<uint8_t>(m_Value) & static_cast<uint8_t>(b.m_Value));
    }

    inline constexpr T &operator&=(const FlagWrap &b)
    {
        return *this = static_cast<T>(static_cast<uint8_t>(m_Value) & static_cast<uint8_t>(b));
    }

    inline constexpr T &operator|=(const FlagWrap &b)
    {
        return *this = static_cast<T>(static_cast<uint8_t>(m_Value) | static_cast<uint8_t>(b));
    }

    inline constexpr T operator|(const FlagWrap &b) const
    {
        return static_cast<T>(static_cast<uint8_t>(m_Value) | static_cast<uint8_t>(b.m_Value));
    }

    inline constexpr T operator|(const T &b) const
    {
        return static_cast<T>(static_cast<uint8_t>(m_Value) | static_cast<uint8_t>(b));
    }

private:
    constexpr static inline T initializer_list_helper(typename std::initializer_list<T>::const_iterator it,
                                                      typename std::initializer_list<T>::const_iterator end)
    {
        return it == end ? static_cast<T>(0x00) : (static_cast<T>(*it) | initializer_list_helper(it + 1, end));
    }
};

class ScopeGuard
{
private:
    std::function<void()> m_Cleanup;
public:
    ScopeGuard(void)                    = default;
    ScopeGuard(const ScopeGuard&)       = default;
    ScopeGuard(const ScopeGuard&&)      = delete;
    const ScopeGuard &operator= (const ScopeGuard&)  = delete;
    const ScopeGuard &operator= (const ScopeGuard&&) = delete;

    ScopeGuard(const std::function<void()> &releaser) : m_Cleanup(releaser) {}

    ~ScopeGuard()
    {
        m_Cleanup();
    }
};

#define DEFINE_BIT_OPS(FlagType, BaseType) \
    inline static constexpr FlagType  operator|  (FlagType  a, FlagType b) { return static_cast<FlagType >(static_cast<BaseType >(a) |  static_cast<BaseType>(b)); } \
    inline static constexpr FlagType  operator&  (FlagType  a, FlagType b) { return static_cast<FlagType >(static_cast<BaseType >(a) &  static_cast<BaseType>(b)); } \
    inline static constexpr FlagType  operator^  (FlagType  a, FlagType b) { return static_cast<FlagType >(static_cast<BaseType >(a) ^  static_cast<BaseType>(b)); } \
    inline static           FlagType &operator|= (FlagType &a, FlagType b) { return reinterpret_cast<FlagType&>(reinterpret_cast<BaseType&>(a) |= static_cast<BaseType>(b)); } \
    inline static           FlagType &operator&= (FlagType &a, FlagType b) { return reinterpret_cast<FlagType&>(reinterpret_cast<BaseType&>(a) &= static_cast<BaseType>(b)); } \
    inline static           FlagType &operator^= (FlagType &a, FlagType b) { return reinterpret_cast<FlagType&>(reinterpret_cast<BaseType&>(a) ^= static_cast<BaseType>(b)); } \
    inline static constexpr FlagType  operator~  (FlagType a)              { return static_cast<FlagType >( ~static_cast<BaseType>(a)); }


#define STRINGLIZE_HELP(a) #a
#define STRINGLIZE(a) STRINGLIZE_HELP(a)

#define CONCAT_HELP(X,Y) X##Y
#define CONCAT(X,Y) CONCAT_HELP(X,Y)

#define MakeScopeGuard(Foo) \
  const auto CONCAT(guard_, __LINE__) = \
    ScopeGuard(Foo)

#define IF_NOT_CND_BREAK(cnd, res) \
    if (!cnd) { res = false; break; }

template<class T>
void SafeDelete(T **ptr)
{
    if (*ptr)
    {
        delete *ptr;
        *ptr = nullptr;
    }
}

template <typename T>
void SafeRelease(T **obj)
{
    if (*obj)
    {
        (*obj)->Release();
         *obj = nullptr;
    }
}

#define LOG_FEATURE_LOCATION
#define USE_LOGGER