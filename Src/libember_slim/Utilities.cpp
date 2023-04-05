/// <summary>
/// ユーティリティ
/// </summary>
#include "Utilities.h"
#include "Output.h"
#include <cassert>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <regex>
#include <algorithm>
#include <locale>
#ifndef WIN32
#include <sys/stat.h>
#include <sys/unistd.h>
#endif

#undef min
#undef max


/// <summary>
/// utilities 名前空間
/// </summary>
namespace utilities
{
    // ====================================================================
    // 出力手続
    // ====================================================================

    /// <summary>
    /// ガイダンス
    /// </summary>
    /// <param name="pFormat"></param>
    /// <param name=""></param>
    void Guidance(const char* pFormat, ...)
    {
        if (!CClientConfig::GetInstance()->bOutputTrace())return;
        std::thread::id threadId = std::this_thread::get_id();
        va_list arg;
        va_start(arg, pFormat);
        COutput::Guidance(threadId, pFormat, arg);
        va_end(arg);
    }
    /// <summary>
    /// トレース
    /// </summary>
    /// <param name="pFileName"></param>
    /// <param name="nLineNumber"></param>
    /// <param name="pFuncName"></param>
    /// <param name="pFormat"></param>
    /// <param name=""></param>
    void Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...)
    {
        if (!CClientConfig::GetInstance()->bOutputTrace())return;
        std::thread::id threadId = std::this_thread::get_id();
        va_list arg;
        va_start(arg, pFormat);
        COutput::Trace(pFileName, nLineNumber, pFuncName, threadId, pFormat, arg);
        va_end(arg);
    }
    /// <summary>
    /// エラー手続
    /// </summary>
    /// <param name="pFileName"></param>
    /// <param name="nLineNumber"></param>
    /// <param name="pFuncName"></param>
    /// <param name="pFormat"></param>
    /// <param name=""></param>
    void ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...)
    {
        std::thread::id threadId = std::this_thread::get_id();
        va_list arg;
        va_start(arg, pFormat);
        COutput::ErrorHandler(pFileName, nLineNumber, pFuncName, threadId, pFormat, arg);
        va_end(arg);
    }


    // ====================================================================
    // 文字列操作
    // ====================================================================

    /// <summary>日時文字列生成</summary>
    /// <param name="value"></param>
    /// <param name="format">strftime 準拠書式</param>
    /// <returns></returns>
    /// <remarks>
    /// 秒未満非対応
    /// </remarks>
    std::string DateTimeFormat(const std::chrono::system_clock::time_point value, std::string format)
    {
        std::string sTime = "";
        if (format.empty())
            return sTime;

        std::unique_ptr<char[]> pBuff;
        try
        {
            auto t = std::chrono::system_clock::to_time_t(value);
            struct tm _tm {};
            auto ptm = localtime_r(&t, &_tm);
            if (ptm)	// ptm == &_tm
            {
                size_t buffSize = format.size() + 32;
                pBuff.reset(new char[buffSize] { 0 });
                char* pBuffRaw = pBuff.get();

                memset(pBuffRaw, 0, buffSize);
                std::strftime(pBuffRaw, buffSize, format.c_str(), ptm);
                if (strlen(pBuffRaw))
                    sTime = pBuffRaw;
            }
        }
        catch (const std::exception ex)
        {
            ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
        }
        pBuff.reset();

        return sTime;
    }

    // 
    // トリミング
    // 
    /// <summary>先頭トリミング</summary>
    /// <param name="value"></param>
    /// <returns></returns>
    std::string TrimStart(const std::string value) { return std::regex_replace(value, std::regex("^\\s+"), std::string("")); }
    /// <summary>末尾トリミング</summary>
    /// <param name="value"></param>
    /// <returns></returns>
    std::string TrimEnd(const std::string value) { return std::regex_replace(value, std::regex("\\s+$"), std::string("")); }
    /// <summary>先頭末尾トリミング</summary>
    /// <param name="value"></param>
    /// <returns></returns>
    std::string Trim(const std::string value) { return TrimStart(TrimEnd(value)); }


    // 
    // 大文字小文字変換
    // 
    /// <summary>
    /// 文字列小文字変換
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    /// <remarks>
    /// ASCII外非対応
    /// </remarks>
    std::string ToLower(std::string value)
    {
        if (!value.empty())
        {
            std::transform
            (
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c) { return tolower(c); }
            );
        }
        return value;
    }
    /// <summary>
    /// 文字列大文字変換
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    /// <remarks>
    /// ASCII外非対応
    /// </remarks>
    std::string ToUpper(std::string value)
    {
        if (!value.empty())
        {
            std::transform
            (
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c) { return toupper(c); }
            );
        }
        return value;
    }


    //
    // 文字列判断
    //
    /// <summary>
    /// 正規表現判断
    /// </summary>
    /// <param name="value"></param>
    /// <param name="pFormat"></param>
    /// <returns></returns>
    bool IsMatch(const std::string value, const std::regex format)
    {
        return (!value.empty() && std::regex_match(value, format));
    }
    /// <summary>
    /// IPv4 判断
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    bool IsIPv4(std::string value)
    {
        // Regex expression for validating IPv4
        const std::regex ipv4("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
        return IsMatch(value, ipv4);
    }
    /// <summary>
    /// IPv6 判断
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    bool IsIPv6(std::string value)
    {
        // Regex expression for validating IPv6
        const std::regex ipv6("((([0-9a-fA-F]){1,4})\\:){7}([0-9a-fA-F]){1,4}");
        return IsMatch(value, ipv6);
    }


    // 
    // 文字列→数値変換
    // 
    /// <summary>
    /// 文字列→unsigned 変換
    /// </summary>
    /// <param name="str"></param>
    /// <param name="idx"></param>
    /// <param name="base"></param>
    /// <returns></returns>
    /// <remarks>
    /// std::string に定義のない数値変換関数を補完
    /// </remarks>
    unsigned stou(const std::string& str, size_t* idx, int base)
    {
        unsigned long val = stoul(str, idx, base);
        if (std::numeric_limits<unsigned>::max() < val) {
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
            val = std::numeric_limits<unsigned>::max();
#else
            throw std::out_of_range("stou");
#endif
        }
        return static_cast<unsigned>(val);
    }
    /// <summary>
    /// 文字列→unsigned short変換
    /// </summary>
    /// <param name="str"></param>
    /// <param name="idx"></param>
    /// <param name="base"></param>
    /// <returns></returns>
    /// <remarks>
    /// std::string に定義のない数値変換関数を補完
    /// </remarks>
    unsigned short stous(const std::string& str, size_t* idx, int base)
    {
        unsigned int val = stoul(str, idx, base);
        if (std::numeric_limits<unsigned short>::max() < val) {
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
            val = std::numeric_limits<unsigned short>::max();
#else
            throw std::out_of_range("stous");
#endif
        }
        return static_cast<unsigned short>(val);
    }
    /// <summary>
    /// 文字列→short変換
    /// </summary>
    /// <param name="str"></param>
    /// <param name="idx"></param>
    /// <param name="base"></param>
    /// <returns></returns>
    /// <remarks>
    /// std::string に定義のない数値変換関数を補完
    /// </remarks>
    short stos(const std::string& str, size_t* idx, int base)
    {
        int val = stoi(str, idx, base);
        if (std::numeric_limits<short>::max() < val) {
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
            val = std::numeric_limits<short>::max();
#else
            throw std::out_of_range("stos");
#endif
        }
        return static_cast<short>(val);
    }

    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, short& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stos(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, unsigned short& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stous(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, int& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stoi(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, unsigned& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stou(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, long& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stol(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, unsigned long& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stoul(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, long long& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stoll(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, unsigned long long& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stoull(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, float& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stof(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    bool ToNumber(std::string value, double& num)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                auto _num = stod(value);
                num = _num;
                result = true;
            }
            catch (...) {}
        }
        return result;
    }

    /// <summary>
    /// 文字列→ブール値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="boolean">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    /// <remarks>
    /// 文字列が整数変換可能な場合
    ///   0 → false
    ///   0以外 → true
    ///   どちらも正常変換と判断
    /// 文字列が整数変換不可の場合
    ///   小文字変換した文字列が "f" もしくは "false" → false
    ///   小文字変換した文字列が "t" もしくは "true" → true
    ///   上記は正常変換と判断
    ///   上記以外は変換不可と判断
    /// 文字列がない場合
    ///   false
    ///   正常変換と判断
    /// </remarks>
    bool ToBool(std::string value, bool& boolean)
    {
        auto result = false;
        if (!value.empty())
        {
            try
            {
                long long _num = 0LL;
                result = ToNumber(value, _num);
                if (result)
                {
                    boolean = (_num != 0LL);
                }
                else
                {
                    auto _str = ToLower(Trim(value));
                    if ((value == "t") || (_str == "true"))
                    {
                        boolean = true;
                        result = true;
                    }
                    else if ((_str == "f") || (_str == "false"))
                    {
                        boolean = false;
                        result = true;
                    }
                }
            }
            catch (...) {}
        }
        else
        {
            boolean = false;
            result = true;
        }

        return result;
    }


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
    /// <summary>wstring → multibyte stream</summary>
    /// <param name="value"></param>
    /// <param name="ostream"></param>
    /// <returns></returns>
    int CreateMultibyteStream(const std::wstring value, std::stringstream& ostream)
    {
        ostream.clear();
        if (value.empty())
            return 0;

	    //int buffSize = (int)value.size() * sizeof(wchar_t) + 1;
	    int buffSize = (int)value.size() * 8 + 1;
        Trace(__FILE__, __LINE__, __FUNCTION__, "start, value size = %d, buffer size = %d\n", value.size(), buffSize);
        char* buff = new char[buffSize];
        memset(buff, 0, buffSize);
        int newValueSize = 0;

        setlocale(LC_ALL, "jpn");
        //setlocale(LC_CTYPE, "ja_JP.UTF-8");
#ifdef _WIN32
        std::size_t converted{};
        int eno = wcstombs_s(&converted, buff, buffSize - 1, value.c_str(), _TRUNCATE);
        if (errno != 0)
#else
        // 現状、日本語が混ざると変換に失敗している
        int len = wcstombs(buff, value.c_str(), buffSize - 1);
        int eno = errno;

        //Trace(__FILE__, __LINE__, __FUNCTION__, "called wcstombs, result = %d, errno = %d\n", len, eno);
    	if (len <= 0)
#endif
        {
            ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed convert, errno = %d\n", eno);
        }
        else
        {
            auto _value = std::string(buff);
            ostream << _value;
            newValueSize = (int)_value.size();
        }

        delete[] buff;

        Trace(__FILE__, __LINE__, __FUNCTION__, "exit, length = %d\n", newValueSize);
        return newValueSize;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif


    // ====================================================================
    // ファイル
    // ====================================================================

    /// <summary>
    /// ディレクトリ／ファイル有無
    /// </summary>
    /// <param name="path"></param>
    /// <returns></returns>
    bool PathExists(const std::string path)
    {
        if (path.empty())
            return false;
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }
    /// <summary>
    /// ディレクトリ／ファイル有無
    /// </summary>
    /// <param name="path"></param>
    /// <returns></returns>
    bool PathExists(const std::filesystem::path& path)
    {
        return !path.empty() && std::filesystem::exists(path);
    }
    /// <summary>
    /// ファイルコピー
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <returns></returns>
    bool FileCopy(const std::string dest, const std::string src)
    {
        if (dest.empty() || src.empty())
            return false;

        std::ifstream ifs(src.c_str(), std::ios::in | std::ios_base::binary);
        std::ofstream ofs(dest.c_str(), std::ios::out | std::ios_base::binary);

        // コピー
        ofs << ifs.rdbuf();

        return PathExists(dest);
    }
    /// <summary>
    /// ファイルコピー
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <returns></returns>
    bool FileCopy(const std::filesystem::path& dest, const std::filesystem::path& src)
    {
        if (dest.empty()
         || src.empty()
         || !std::filesystem::exists(src)
         || std::filesystem::is_directory(src))
            return false;
        return std::filesystem::copy_file(src, dest);
    }
    /// <summary>
    /// ファイル削除
    /// </summary>
    /// <param name="path"></param>
    /// <param name="errcode"></param>
    /// <returns></returns>
    bool FileRemove(const std::filesystem::path& path, std::error_code& errcode)
    {
        errcode.clear();
        bool res = false;
        if (path.empty())
            return res;

        res = std::filesystem::remove(path, errcode);
        return res;
    }
    /// <summary>
    /// ファイル名変更
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <param name="errcode"></param>
    /// <returns></returns>
    bool FileRename(const std::filesystem::path& dest, const std::filesystem::path& src, std::error_code& errcode)
    {
        errcode.clear();
        bool res = false;
        if (dest.empty()
         || src.empty()
         || (dest.string() == src.string())
         || !std::filesystem::exists(src)
         || std::filesystem::is_directory(src))
            return res;

        std::filesystem::rename(src, dest, errcode);
        return std::filesystem::exists(dest) && !std::filesystem::exists(src);
    }
    /// <summary>
    /// ファイル移動
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <param name="errcode"></param>
    /// <returns></returns>
    bool FileMove(const std::filesystem::path& dest, const std::filesystem::path& src, std::error_code& errcode)
    {
        return FileRename(dest, src, errcode);
    }

    // ====================================================================

    // split string to vector
    std::vector<std::string> split(const std::string& text, char sep) {
        std::vector<std::string> tokens;
        std::size_t start = 0, end = 0;
        while ((end = text.find(sep, start)) != std::string::npos) {
            if (end != start) {
                tokens.push_back(text.substr(start, end - start));
            }
            start = end + 1;
        }
        if (end != start) {
            tokens.push_back(text.substr(start));
        }
        return tokens;
    }

}

extern "C" void freeMemory(void* pMemory);

/// <summary>
/// ガイダンス
/// </summary>
/// <param name="pFormat"></param>
/// <param name=""></param>
extern "C" void __Guidance(const char* pFormat, ...)
{
    std::thread::id threadId = std::this_thread::get_id();
    va_list arg;
    va_start(arg, pFormat);
    COutput::Guidance(threadId, pFormat, arg);
    va_end(arg);
}
/// <summary>
/// トレース
/// </summary>
/// <param name="pFileName"></param>
/// <param name="nLineNumber"></param>
/// <param name="pFuncName"></param>
/// <param name="pFormat"></param>
/// <param name=""></param>
extern "C" void __Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...)
{
    std::thread::id threadId = std::this_thread::get_id();
    va_list arg;
    va_start(arg, pFormat);
    //COutput::Trace(pFileName, nLineNumber, pFuncName, threadId, pFormat, arg);
    va_end(arg);
}
/// <summary>
/// エラー手続
/// </summary>
/// <param name="pFileName"></param>
/// <param name="nLineNumber"></param>
/// <param name="pFuncName"></param>
/// <param name="pFormat"></param>
/// <param name=""></param>
extern "C" void __ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...)
{
    std::thread::id threadId = std::this_thread::get_id();
    va_list arg;
    va_start(arg, pFormat);
    COutput::ErrorHandler(pFileName, nLineNumber, pFuncName, threadId, pFormat, arg);
    va_end(arg);
}

/// <summary>
/// 整数変換
/// </summary>
/// <param name="pSrc"></param>
/// <param name="pDest"></param>
/// <returns></returns>
extern "C" bool __CharToLongLong(const char* pSrc, long long* pDest)
{
    bool res = false;
    if (!pSrc || !pDest)
        return res;
    *pDest = 0;

#if true
    std::string value(pSrc);
    long long num = 0;
    if (!utilities::ToNumber(value, num))
        return false;
#else
    errno = 0;
    char* end;
    long long num = strtoll(pSrc, &end, 10);
    if ((errno != 0) || (pSrc == end) || (*end != 0))
        return false;
#endif

    *pDest = num;
    return true;
}
/// <summary>
/// 実数変換
/// </summary>
/// <param name="pSrc"></param>
/// <param name="pDest"></param>
/// <returns></returns>
extern "C" bool __CharToDouble(const char* pSrc, double* pDest)
{
    if (!pSrc || !pDest)
        return false;
    *pDest = 0.0;

#if true
    std::string value(pSrc);
    double num = 0.0;
    if (!utilities::ToNumber(value, num))
        return false;
#else
    errno = 0;
    char* end;
    double num = strtod(pSrc, &end);
    if ((errno != 0) || (pSrc == end) || (*end != 0))
        return false;
#endif

    *pDest = num;
    return true;
}


#if defined WIN32
/// <summary>
/// 
/// </summary>
/// <remarks>
/// スレッドセーフ localtime 定義が
///   clang : struct tm * localtime_r (const time_t * clock, struct tm* result)
///     gcc : struct tm * localtime_r (const time_t *__restrict __timer, struct tm* __restrict __tp) __THROW;
///     msc : errno_t __CRTDECL localtime_s(_Out_ struct tm* const _Tm, _In_  time_t const* const _Time)
/// につき、localtime_r の代替を定義
/// </remarks>
struct tm* localtime_r(time_t* clock, struct tm* result)
{
    if (!result)
        return nullptr;

    if (localtime_s(result, clock) != 0)
        return nullptr;

    return result;
}
#endif
