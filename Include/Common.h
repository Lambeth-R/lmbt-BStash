//
// Contains common functions for code formatting / control
//

#pragma once

#include <functional>
#include <memory>
#include <string>

#include <stdio.h>
#include <stdint.h>
#include <vcruntime.h>
#if _HAS_CXX17
    #define _CRT_SECURE_NO_WARNINGS
    typedef int(__cdecl* sprintf_foo)(const void *, size_t, const void*, ...);
#endif

#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

#pragma region string_format
#if _HAS_CXX17
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
    template<typename T>
    const std::basic_string<T> string_format_l(const std::basic_string_view<T> format, va_list args)
    {
        sprintf_foo printing = nullptr;
        if (typeid(T) == typeid(char))
        {
            printing = reinterpret_cast<sprintf_foo>(_vsnprintf_c_l);
        }
        else if (typeid(T) == typeid(wchar_t))
        {
            printing = reinterpret_cast<sprintf_foo>(_vsnwprintf_l);

        }
        if (!printing)
        {
            return {};
        }
        int size_s = printing(nullptr, 0, format.data(), nullptr, args) + 1;
        if (size_s <= 0) return {};
        std::unique_ptr<T[]> buf(new T[static_cast<size_t>(size_s)]);
        printing(buf.get(), static_cast<size_t>(size_s), format.data(), nullptr, args);
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

class ScopeGuard
{
private:
    std::function<void()> m_Cleanup;
public:
    ScopeGuard(void)              = default;
    ScopeGuard(ScopeGuard&)       = delete;
    ScopeGuard(ScopeGuard&&)      = delete;
    auto operator= (ScopeGuard&)  = delete;
    auto operator= (ScopeGuard&&) = delete;

    auto operator->* (const std::function<void()> &releaser)
    {
        m_Cleanup = releaser;
    }
    ~ScopeGuard()
    {
        m_Cleanup();
    }
};

#define MakeScopeGuard(Foo) \
    ScopeGuard() ->* Foo;

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


#define USE_LOGGER