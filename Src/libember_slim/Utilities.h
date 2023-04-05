/// <summary>
/// ユーティリティ
/// </summary>
#pragma once

#include <cstdint>
#include <string>
#include <regex>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <filesystem>

#ifndef _WIN32
#define _strdup(pStr) strdup(pStr)
#endif
#define stringDup(pStr) \
   (pStr != NULL ? _strdup(pStr) : NULL)


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
    extern void Guidance(const char* pFormat, ...);
    /// <summary>
    /// トレース
    /// </summary>
    /// <param name="pFileName"></param>
    /// <param name="nLineNumber"></param>
    /// <param name="pFuncName"></param>
    /// <param name="pFormat"></param>
    /// <param name=""></param>
    extern void Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...);
    /// <summary>
    /// エラー手続
    /// </summary>
    /// <param name="pFileName"></param>
    /// <param name="nLineNumber"></param>
    /// <param name="pFuncName"></param>
    /// <param name="pFormat"></param>
    /// <param name=""></param>
    extern void ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...);


    // ====================================================================
    // 数値操作
    // ====================================================================

    /// <summary>
    /// 範囲補正
    /// </summary>
    /// <param name="value"></param>
    /// <param name="minValue"></param>
    /// <param name="maxValue"></param>
    /// <returns></returns>
    template<typename T>
    T Clip(const T value, const T minValue, const T maxValue)
    {
        return std::min<T>(std::max<T>(value, minValue), maxValue);
    }

    /// <summary>
    /// 範囲確認
    /// </summary>
    /// <param name="value"></param>
    /// <param name="minValue"></param>
    /// <param name="maxValue"></param>
    /// <returns></returns>
    template<typename T>
    bool IsRange(const T value, const T minValue, const T maxValue)
    {
        return Clip(value, minValue, maxValue) == value;
    }


    // ====================================================================
    // 文字列操作
    // ====================================================================

    /// <summary>書式指定</summary>
    /// <typeparam name="...Args"></typeparam>
    /// <param name="fmt"></param>
    /// <param name="...args"></param>
    /// <returns></returns>
    /// <remarks>
    /// 揮発性インスタンスのため、戻りは直接参照せず別インスタンスに受けて使用すること
    /// </remarks>
    template <typename ... Args>
    std::string Format(const std::string& fmt, Args ... args)
    {
        size_t len = std::snprintf(nullptr, 0, fmt.c_str(), args ...);
        std::vector<char> buf(len + 1);
        std::snprintf(&buf[0], len + 1, fmt.c_str(), args ...);
        return std::string(&buf[0], &buf[0] + len);
    }
    /// <summary>日時文字列生成</summary>
    /// <param name="value"></param>
    /// <param name="format">strftime 準拠書式</param>
    /// <returns></returns>
    /// <remarks>
    /// 秒未満非対応
    /// </remarks>
    std::string DateTimeFormat(const std::chrono::system_clock::time_point value, std::string format);

    // 
    // トリミング
    // 
    /// <summary>先頭トリミング</summary>
    /// <param name="value"></param>
    /// <returns></returns>
    extern std::string TrimStart(const std::string value);
    /// <summary>末尾トリミング</summary>
    /// <param name="value"></param>
    /// <returns></returns>
    extern std::string TrimEnd(const std::string value);
    /// <summary>先頭末尾トリミング</summary>
    /// <param name="value"></param>
    /// <returns></returns>
    extern std::string Trim(const std::string value);

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
    extern std::string ToLower(std::string value);
    /// <summary>
    /// 文字列大文字変換
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    /// <remarks>
    /// ASCII外非対応
    /// </remarks>
    extern std::string ToUpper(std::string value);

    //
    // 文字列判断
    //
    /// <summary>
    /// 正規表現判断
    /// </summary>
    /// <param name="value"></param>
    /// <param name="format"></param>
    /// <returns></returns>
    extern bool IsMatch(const std::string value, const std::regex format);
    /// <summary>
    /// IPv4 判断
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    extern bool IsIPv4(std::string value);
    /// <summary>
    /// IPv6 判断
    /// </summary>
    /// <param name="value"></param>
    /// <returns></returns>
    extern bool IsIPv6(std::string value);

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
    extern unsigned stou(const std::string& str, size_t* idx = 0, int base = 10);
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
    extern unsigned short stous(const std::string& str, size_t* idx = 0, int base = 10);
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
    extern short stos(const std::string& str, size_t* idx = 0, int base = 10);

    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, short& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, unsigned short& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, int& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, unsigned& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, long& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, unsigned long& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, long long& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, unsigned long long& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, float& num);
    /// <summary>
    /// 文字列→数値変換
    /// </summary>
    /// <param name="value"></param>
    /// <param name="num">変換値</param>
    /// <returns>
    /// false : 変換不可、変換値非保証
    /// true : 正常変換
    /// </returns>
    extern bool ToNumber(const std::string value, double& num);

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
    extern bool ToBool(const std::string value, bool& boolean);

    /// <summary>ストリーム → wstring</summary>
    /// <param name="pStream"></param>
    /// <returns></returns>
    extern std::wstring StreamReadW(std::istream* pStream);
    /// <summary>wstring → multibyte stream</summary>
    /// <param name="value"></param>
    /// <param name="ostream"></param>
    /// <returns></returns>
    extern int CreateMultibyteStream(const std::wstring value, std::stringstream& ostream);


    // ====================================================================
    // map
    // ====================================================================

    /// <summary>
    /// マップ逆引き
    /// </summary>
    /// <typeparam name="TValue"></typeparam>
    /// <typeparam name="TKey"></typeparam>
    /// <param name="map"></param>
    /// <param name="value"></param>
    /// <param name="key"></param>
    /// <returns></returns>
    template<typename TKey, typename TValue>
    bool ToKey(const std::unordered_map<TKey, TValue>& map, const TValue& value, TKey& key)
    {
        // key の初期化はしない

        bool result = false;
        if (map.empty())
            return result;

        auto itr = std::find_if(map.cbegin(), map.cend(), [&value](const std::pair<TKey, TValue>& p) { return p.second == value; });
        if (itr != map.cend())
        {
            key = (*itr).first;
            result = true;
        }

        return true;
    }


    // ====================================================================
    // ファイル
    // ====================================================================
    
    /// <summary>
    /// ディレクトリ／ファイル有無
    /// </summary>
    /// <param name="path"></param>
    /// <returns></returns>
    extern bool PathExists(const std::string path);
    /// <summary>
    /// ディレクトリ／ファイル有無
    /// </summary>
    /// <param name="path"></param>
    /// <returns></returns>
    extern bool PathExists(const std::filesystem::path& path);
    /// <summary>
    /// ファイルコピー
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <returns></returns>
    extern bool FileCopy(const std::string dest, const std::string src);
    /// <summary>
    /// ファイルコピー
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <returns></returns>
    extern bool FileCopy(const std::filesystem::path& dest, const std::filesystem::path& src);
    /// <summary>
    /// ファイル削除
    /// </summary>
    /// <param name="path"></param>
    /// <param name="errcode"></param>
    /// <returns></returns>
    extern bool FileRemove(const std::filesystem::path& path, std::error_code& errcode);
    /// <summary>
    /// ファイル名変更
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <param name="errcode"></param>
    /// <returns></returns>
    extern bool FileRename(const std::filesystem::path& dest, const std::filesystem::path& src, std::error_code& errcode);
    /// <summary>
    /// ファイル移動
    /// </summary>
    /// <param name="dest"></param>
    /// <param name="src"></param>
    /// <param name="errcode"></param>
    /// <returns></returns>
    extern bool FileMove(const std::filesystem::path& dest, const std::filesystem::path& src, std::error_code& errcode);

    // split string
    std::vector<std::string> split(const std::string& text, char sep);

    // ====================================================================
    // 設定ファイル操作 - ReadIni.cpp
    // ====================================================================
    /// <summary>
    /// ファイルから読み込み
    /// </summary>
    /// <param name="path">ファイルフルパス</param>
    /// <param name="lines">行単位ファイル内文字列</param>
    /// <returns>
    /// 0 以上 : 有効行数,  
    /// -1 : 指定ファイル不正
    /// </returns>
    extern int CommReadIniFile(std::string path, std::vector<std::string>& lines);

    /// <summary>
    /// INI-FILE解析
    /// </summary>
    /// <param name="lines"></param>
    /// <param name="section"></param>
    /// <param name="key"></param>
    /// <param name="value"></param>
    /// <returns>
    ///  0 : データ取得成功（空文字列含）
    /// -1 : 指定データ取得失敗
    /// </returns>
    extern int CommGetIniFileData(const std::vector<std::string> lines, std::string section, std::string key, std::string& value);
    /// <summary>
    /// INI-FILE解析
    /// </summary>
    /// <param name="path">ファイルパス</param>
    /// <param name="section">セクション（空文字列許容）</param>
    /// <param name="key">キー</param>
    /// <param name="value">返却取得値</param>
    /// <returns>
    ///  0 : データ取得成功（空文字列含）
    /// -1 : 指定データ取得失敗
    /// </returns>
    extern int CommGetIniFileData(std::string path, std::string section, std::string key, std::string& value);


    // ====================================================================
    // CSVファイル操作 - ReadCsv.cpp
    // ====================================================================
    /// <summary>
    /// ファイルから読み込み
    /// </summary>
    /// <param name="path">ファイルフルパス</param>
    /// <param name="lines">行単位ファイル内文字列</param>
    /// <param name="useMultibyte">マルチバイト想定ファイル是非</param>
    /// <returns>
    /// 0 以上 : 有効行数,  
    /// -1 : 指定ファイル不正
    /// </returns>
    extern int CommReadCsvFile(std::string path, std::vector<std::vector<std::string>>& lines, bool useMultibyte);

    /// <summary>
    /// 指定カラム位置取得
    /// </summary>
    /// <param name="columns"></param>
    /// <param name="value"></param>
    /// <returns></returns>
    extern int CommGetColumnPosition(const std::vector<std::string> columns, std::string value);
    /// <summary>
    /// 指定位置カラム内容取得
    /// </summary>
    /// <param name="columns"></param>
    /// <param name="position"></param>
    /// <param name="value"></param>
    /// <returns>
    /// true/false
    /// </returns>
    extern bool CommGetColumnValue(const std::vector<std::string> columns, int position, std::string& value);

    // ====================================================================
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
extern struct tm* localtime_r(time_t* clock, struct tm* result);
#endif
