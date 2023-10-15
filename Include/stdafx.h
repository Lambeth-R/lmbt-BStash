#pragma once
#include <vcruntime.h>
#if defined(_HAS_CXX17)
    #define _CRT_SECURE_NO_WARNINGS
    #include <stdio.h>
    typedef int(__cdecl* sprintf_foo)(const void *, size_t, const void*, ...);
#endif
#include <string>
#include <memory>

#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

#pragma region string_format
#if defined(_HAS_CXX17)
    template<typename T, typename ... Args>
    const std::basic_string<T> string_format(const std::basic_string_view<T> format, Args&& ...args)
    {
        sprintf_foo printing = nullptr;
        if (typeid(T) == typeid(char))
        {
            printing = reinterpret_cast<sprintf_foo>(_snprintf);
        }
        else if (typeid(T) == typeid(wchar_t))
        {
            printing = reinterpret_cast<sprintf_foo>(_snwprintf);
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
#else
    template<typename T, typename ... Args>
    const std::string string_format(const std::string &format, Args&& ...args)
    {
        int size_s = _snprintf(nullptr, 0, format.data(), args ...) + 1;
        if (size_s <= 0) return {};
        std::unique_ptr<char[]> buf(new char[static_cast<size_t>(size_s)]);
        _snprintf(buf.get(), static_cast<size_t>(size_s), format.data(), args ...);
        return std::string(buf.get(), buf.get() + static_cast<size_t>(size_s) - 1);
    }

    template<typename T, typename ... Args>
    const std::wstring string_format(const std::wstring &format, Args&& ...args)
    {
        int size_s = _snwprintf(nullptr, 0, format.c_str(), args ...) + 1;
        if (size_s <= 0) return {};
        auto size = static_cast<size_t>(size_s);
        std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
        _snwprintf(buf.get(), size, format.c_str(), args ...);
        return std::wstring(buf.get(), buf.get() + size - 1);
    }
#endif
#pragma endregion

template <typename Fn>
struct scope_exit_impl : Fn
{
    ~scope_exit_impl()
    {
        (*this)();
    }
};

struct make_scope_exit {
    template <typename Fn>
    auto operator->*(const Fn& fn) const {
        return scope_exit_impl(fn);
    }
};

#define MakeScopeGuard(Foo) \
    make_scope_exit{} ->* Foo;

template<class T>
void SafeDelete(T **ptr)
{
    if (*ptr)
    {
        delete *ptr;
        *ptr = nullptr;
    }
}

#define USE_LOGGER
#include "LogClass.h"