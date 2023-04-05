#pragma once

#include "Utilities.h"
#include "ClientConfig.h"
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <chrono>
#include <ctime>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <sstream>


// ====================================================================

/// <summary>
/// 
/// </summary>
enum class OutputMode : uint8_t
{
	/// <summary></summary>
	OUTPUT_NORMAL = 0,

	/// <summary></summary>
	OUTPUT_TRACE,
	/// <summary></summary>
	OUTPUT_ERROR,


	/// <summary>モード数</summary>
	OUTPUT_COUNT,
	/// <summary>不定</summary>
	OUTPUT_UNKNOWN = INT8_MAX,
};

/// <summary>
/// 
/// </summary>
class OutputContent
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	OutputContent() :
		id(0),
		m_tPoint(),
		m_nThreadId(),
		m_eMode(OutputMode::OUTPUT_NORMAL),
		m_sFilePath(""),
		m_nLineNumber(0),
		m_sFuncName(""),
		m_sValue("")
	{
	}
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="timePoint"></param>
	/// <param name="nThreadId"></param>
	/// <param name="eMode"></param>
	/// <param name="sFilePath"></param>
	/// <param name="nLineNumber"></param>
	/// <param name="sFuncName"></param>
	/// <param name="sValue"></param>
	OutputContent(
		std::chrono::system_clock::time_point timePoint,
		std::thread::id nThreadId,
		OutputMode eMode,
		std::string sFilePath,
		int nLineNumber,
		std::string sFuncName,
		std::string sValue)
	{
		id = 0;
		m_tPoint = timePoint;
		m_nThreadId = nThreadId;
		m_eMode = eMode;
		m_sFilePath = sFilePath;
		m_nLineNumber = nLineNumber;
		m_sFuncName = sFuncName;
		m_sValue = sValue;
	}

	/// <summary>識別</summary>
	int id;

	/// <summary></summary>
	std::chrono::system_clock::time_point m_tPoint;

	/// <summary></summary>
	std::thread::id m_nThreadId;

	/// <summary></summary>
	OutputMode m_eMode;

	/// <summary></summary>
	std::string m_sFilePath;
	/// <summary></summary>
	int m_nLineNumber;
	/// <summary></summary>
	std::string m_sFuncName;

	/// <summary></summary>
	std::string m_sValue;
};


// ====================================================================

/// <summary>
/// </summary>
class COutput
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="outputTime"></param>
	COutput(bool outputTime = false);

	/// <summary>
	/// デストラクタ
	/// </summary>
	~COutput();

	/// <summary>初期化済</summary>
	/// <returns></returns>
	bool Initialized() { return m_bInitialized; }
	/// <summary>有無効</summary>
	/// <returns></returns>
	bool Enabled() { return m_bInitialized /* && m_ptWatcher */ && !m_bCancelRequest; }

	/// <summary>ログファイル出力有無</summary>
	/// <returns></returns>
	bool IsOutputLogFile() { return m_pClientConfig && m_pClientConfig->LogFileEnabled(); }

	/// <summary></summary>
	/// <returns></returns>
	bool CancelRequest();
	/// <summary>離脱要求取得</summary>
		/// <returns></returns>
	bool IsCancelRequest();

	/// <summary>出力プレフィックス生成</summary>
	/// <param name="sFileName"></param>
	/// <param name="nLineNumber"></param>
	/// <param name="sFuncName"></param>
	/// <param name="sPrefix"></param>
	/// <param name="isError"></param>
	static void CreateTracePrefix(const std::string sFileName, const int nLineNumber, const std::string sFuncName, std::string& sPrefix, bool isError = false);

	/// <summary>出力</summary>
	/// <param name="sPath"></param>
	/// <param name="sValue"></param>
	static void Output(std::string sPath, std::string sValue);
	/// <summary>出力</summary>
	/// <param name="sValue"></param>
	/// <param name="isError"></param>
	/// <returns></returns>
	static void Output(std::string sValue, bool isError = false);
	/// <summary>
	/// ガイダンス
	/// </summary>
	/// <param name="threadId"></param>
	/// <param name="pFormat"></param>
	/// <param name="pArgList"></param>
#if defined(__aarch64__) || defined(__arm__) 
	static void Guidance(const std::thread::id threadId, const char* pFormat, va_list pArgList);
#else
	static void Guidance(const std::thread::id threadId, const char* pFormat, va_list pArgList = nullptr);
#endif
	/// <summary>
	/// トレース
	/// </summary>
	/// <param name="pFileName"></param>
	/// <param name="nLineNumber"></param>
	/// <param name="pFuncName"></param>
	/// <param name="threadId"></param>
	/// <param name="pFormat"></param>
	/// <param name="pArgList"></param>
#if defined(__aarch64__) || defined(__arm__) 
	static void Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList);
#else
	static void Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList = nullptr);
#endif
	/// <summary>
	/// エラー手続
	/// </summary>
	/// <param name="pFileName"></param>
	/// <param name="nLineNumber"></param>
	/// <param name="pFuncName"></param>
	/// <param name="threadId"></param>
	/// <param name="pFormat"></param>
	/// <param name="pArgList"></param>
#if defined(__aarch64__) || defined(__arm__)
	static void ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList);
#else
	static void ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList = nullptr);
#endif
	/// <summary>
	/// インスタンス取得
	/// </summary>
	/// <returns></returns>
	static COutput* GetInstance();

private:
	/// <summary>
	/// メンバ初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 出力要求識別発行
	/// </summary>
	/// <returns></returns>
	int IssueRequestId();
	/// <summary>出力要求追加</summary>
	/// <param name="pRequest"></param>
	/// <returns></returns>
	int AddRequest(std::unique_ptr<OutputContent> pRequest);
	/// <summary>出力要求追加</summary>
	/// <param name="eMode"></param>
	/// <param name="pFileName"></param>
	/// <param name="nLineNumber"></param>
	/// <param name="pFuncName"></param>
	/// <param name="threadId"></param>
	/// <param name="pFormat"></param>
	/// <param name="pArgList"></param>
	/// <returns></returns>
#if defined(__aarch64__) || defined(__arm__)
	int AddRequest(OutputMode eMode, const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList);
#else
	int AddRequest(OutputMode eMode, const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList = nullptr);
#endif
	/// <summary>出力要求取得</summary>
	/// <returns></returns>
	std::unique_ptr<OutputContent> GetRequest();

	/// <summary>出力</summary>
	/// <param name="sValue"></param>
	/// <param name="pRequest"></param>
	void Output(std::unique_ptr<OutputContent> pRequest);

	/// <summary></summary>
	/// <param name="instance"></param>
	static void Watcher(COutput* instance);

	/// <summary>クライアント用情報クラスインスタンス</summary>
	CClientConfig* m_pClientConfig;

	/// <summary></summary>
	std::unique_ptr <std::thread> m_ptWatcher;
	/// <summary></summary>
	std::mutex m_mtxCancelRequest;
	/// <summary></summary>
	bool m_bCancelRequest;

	/// <summary></summary>
	std::mutex m_mtxPreRequest;
	/// <summary></summary>
	std::mutex m_mtxRequest;
	/// <summary></summary>
	std::mutex m_mtxOutput;
	/// <summary>出力要求最終識別</summary>
	int m_nLastRequestId;
	/// <summary>出力要求済</summary>
	std::deque<std::unique_ptr<OutputContent>> m_qRequests;

	/// <summary>ログファイル出力有無</summary>
	bool m_bOutputLogFile;
	/// <summary>初期化済</summary>
	bool m_bInitialized;

	/// <summary>
	/// 単一インスタンス（宣言）
	/// </summary>
	static std::unique_ptr<COutput> m_pInstance;
};
