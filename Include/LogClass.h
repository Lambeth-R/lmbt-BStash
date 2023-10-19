#pragma once
#include <sstream>
#include <fstream>
#include <thread>
#include <Windows.h>

inline std::wstring ForamtSystemMessage(const uint32_t _Message_id) noexcept
{
    const auto curr_last_err = GetLastError();
    DWORD lang_id;
    const auto ret = GetLocaleInfoEx(LOCALE_NAME_SYSTEM_DEFAULT, LOCALE_ILANGUAGE | LOCALE_RETURN_NUMBER,
        reinterpret_cast<LPWSTR>(&lang_id), sizeof(lang_id) / sizeof(wchar_t));
    if (ret == 0) { lang_id = 0; }
    wchar_t* inner_allocated_buf = nullptr;
    std::shared_ptr<void> _{nullptr, [&](void* ptr) { if (inner_allocated_buf) { LocalFree(inner_allocated_buf); } }};
    const auto ret_size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, _Message_id, lang_id, (wchar_t*) & inner_allocated_buf, 0, nullptr);
    SetLastError(curr_last_err);
    return std::wstring(inner_allocated_buf, static_cast<const size_t>(ret_size) - 2);
}

#if _HAS_CXX17
    #define PARSE_ERROR(val) std::hex << val << " " << ForamtSystemMessage(val)
#else
    #define PARSE_ERROR(val) std::hex << val << " " << ForamtSystemMessage(val).data()
#endif

#ifdef USE_LOGGER
    #ifdef MODULE_NAME
        #define LOG(lvl, stream) LogClass().Log(lvl) << "(" << MODULE_NAME<< ")\t" << stream
    #else
        #define LOG(lvl, stream) LogClass().Log(lvl) << stream
    #endif

#else
    #define LOG()
#endif // USE_LOGGER

enum class eLogType
{
    None = 0,
    Debug = 2,
    Print = 4,
    File = 8
};
DEFINE_ENUM_FLAG_OPERATORS(eLogType);

enum class eLogLevels
{
    None = 0,
    Error = 1,
    Warning = 2,
    Info = 3,
    Debug = 4,
    All = 10
};

class ConsoleColour
{
    enum class eConsoleColour
    {
        NC = -1,
        Black,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        White,
    };
public:
    static constexpr eConsoleColour Lvl_to_Colour(eLogLevels lvl)
    {
        switch (lvl)
        {
        case eLogLevels::Error:
        case eLogLevels::Warning:
        case eLogLevels::Info:
        case eLogLevels::Debug:
            return eConsoleColour((uint16_t)lvl);
            break;
        case eLogLevels::None:
        case eLogLevels::All:
        default:
            return eConsoleColour::NC;
        }
    }
    static const std::wstring SetColours(const std::wstring& iStream, int16_t front, int16_t back = -1)
    {
        if (front == -1)
        {
            return iStream;
        }
        std::wstringstream ws;
        static wchar_t fCode[20] = { 0 };
        static wchar_t bCode[20] = { 0 };
        static wchar_t cleanCode[] = L"\033[0m";
        if (back > 0)
        {
            swprintf_s(fCode, L"\033[%d;%dm", (uint16_t)front + 30, (uint16_t)back + 40);
        }
        else
        {
            swprintf_s(fCode, L"\033[%dm", (uint16_t)front + 30);
        }
        ws << fCode;
        ws << iStream;
        ws << cleanCode;
        return ws.str();
    }
};

class LogClass
{
    #pragma region Static

    static eLogLevels       s_eCurLogLvl;
    static std::wofstream   s_OutputFile;
    static eLogType         s_LogFlags;
    static DWORD            s_Error;
    static std::string      s_Filename;
    static bool             s_ConsoleFixed;
    static bool EraseFlag(DWORD flag);
    static void SetConsoleUTF8();
public:
    static void InitConsole();
    static void InitLogger(eLogLevels eLogLvl, const std::string& fName = "", eLogType eFlags = eLogType::Debug);
    #pragma endregion

    #pragma region Public
    LogClass() = default;
    ~LogClass();
    std::wstringstream& Log(eLogLevels elvl);
    #pragma endregion

    #pragma region Private
private:
    
    const std::wstring GenerateAppendix() const;
    eLogLevels          m_elogLevel;
    std::wstringstream  m_Data;
    
    #pragma endregion
};

#if _HAS_CXX17
std::wostream& operator<<(std::wostream& iStream, std::string_view iString);
std::wostream& operator<<(std::wostream& iStream, std::wstring_view iString);
#else
std::wostream& operator<<(std::wostream& iStream, const char* data);
std::wostream& operator<<(std::wostream& iStream, const char data);
std::wostream& operator<<(std::wostream& iStream, const wchar_t* data);
std::wostream& operator<<(std::wostream& iStream, const wchar_t data);
#endif