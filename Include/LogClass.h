//
// Implementation of stream based logger.
// All main types are present.
// Works mainly with wchar_t data. Has option for extern Utf8 translation.
//

#pragma once
#include <sstream>
#include <fstream>
#include <Windows.h>

#define FEATURE_STATIC_LINKING 0

std::wstring ParseLastError();
std::wstring ForamtSystemMessage(const uint32_t messageId) noexcept;

#if _HAS_CXX17
    #define PARSE_ERROR(val) std::hex << val << " " << ForamtSystemMessage(val)
#else
    #define PARSE_ERROR(val) std::hex << val << " " << ForamtSystemMessage(val).data()
#endif

#ifdef USE_LOGGER
    #ifdef MODULE_NAME
        #define LOGSTREAM(lvl, stream)      LogClass().Log(lvl) << L"(" << MODULE_NAME<< L")\t" << stream
        #define LOGPRINT(lvl, format, ...)  LogClass().LogPrint(lvl, MODULE_NAME, format, __VA_ARGS__)
    #else
        #define LOGSTREAM(lvl, stream)      LogClass().Log(lvl) << stream
        #define LOGPRINT(lvl, format, ...)  LogClass().LogPrint(lvl, nullptr,format, __VA_ARGS__)
    #endif

#else
    #define LOGSTREAM()
    #define LOGPRINT()
#endif // USE_LOGGER

#define Logstream_Error(str)       LOGSTREAM(LogClass::LogLevel_Error,   str)
#define Logstream_Warning(str)     LOGSTREAM(LogClass::LogLevel_Warning, str)
#define Logstream_Info(str)        LOGSTREAM(LogClass::LogLevel_Info,    str)
#define Logstream_Debug(str)       LOGSTREAM(LogClass::LogLevel_Debug,   str)

#define Log_Error(...)     LOGPRINT(LogClass::LogLevel_Error,    __VA_ARGS__)
#define Log_Warning(...)   LOGPRINT(LogClass::LogLevel_Warning,  __VA_ARGS__)
#define Log_Info(...)      LOGPRINT(LogClass::LogLevel_Info,     __VA_ARGS__)
#define Log_Debug(...)     LOGPRINT(LogClass::LogLevel_Debug,    __VA_ARGS__)

struct ConsoleColor
{
    enum Color
    {
        Color_NC = -1,
        Color_Black,
        Color_Red,
        Color_Green,
        Color_Yellow,
        Color_Blue,
        Color_Magenta,
        Color_Cyan,
        Color_White,
    };
    static constexpr  Color         Lvl_to_Color(uint8_t lvl);
    static const      std::wstring  SetColors(const std::wstring& iStream, int16_t front, int16_t back = -1);
};

class LogClass
{
public:
    enum LogLevel
    {
        LogLevel_None       = 0,
        LogLevel_Error      = 1,
        LogLevel_Warning    = 2,
        LogLevel_Info       = 3,
        LogLevel_Debug      = 4,
        LogLevel_All        = 10
    };

    enum LogType
    {
        LogType_None      = 0,
        LogType_Debug     = 2,
        LogType_Print     = 4,
        LogType_File      = 8,
        LogType_Callback  = 16
    };

    struct CallbackEx
    {
        typedef void (*Log_Callback) (void* ex, const std::wstring& string);

        Log_Callback                *callback   = nullptr;
        void                        *callbackEx = nullptr;
    };
private:

    static LogLevel                 s_CurLogLvl;
    static std::wofstream           s_OutputFile;
    static LogType                  s_LogFlags;
    static CallbackEx               s_Callback;

           LogLevel                 m_LogLevel;
           std::wstringstream       m_Data;

    static void                     SetConsoleUTF8();
public:

    static void                     InitConsole();
    static void                     InitLogger(LogLevel eLogLvl, LogType eFlags, const std::string& fName = "", const CallbackEx &callback = {});

    static void                     DumpPtr(const std::string &dumpName, void *ptr, size_t dumpSize);

                                    LogClass(void) = default;
                                    ~LogClass();

    std::wstringstream&             Log(LogLevel elvl);
    std::wstringstream&             LogPrint(LogLevel elvl, const wchar_t *moduleName, const wchar_t *format, ...);

#if _HAS_CXX17
    friend std::wostream&           operator<<(std::wostream &iStream, std::string_view  iString);
    friend std::wostream&           operator<<(std::wostream &iStream, std::wstring_view iString);
#endif
    friend std::wostream&           operator<<(std::wostream &iStream, const char *data);
    friend std::wostream&           operator<<(std::wostream &iStream, const char data);
    friend std::wostream&           operator<<(std::wostream &iStream, const wchar_t *data);
    friend std::wostream&           operator<<(std::wostream &iStream, const wchar_t data);

private:
    const wchar_t*                  GenerateAppendix() const;
};
DEFINE_ENUM_FLAG_OPERATORS(LogClass::LogType);
