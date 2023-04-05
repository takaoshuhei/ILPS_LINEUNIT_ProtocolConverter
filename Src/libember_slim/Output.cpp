#include "Output.h"
#include <climits>
#include <cassert>
#include <filesystem>

// stdout の参照がスレッド間でバッティングしている様子が散見されたため
// ロックとキューを利用した出力クラスを用意したが
// pop_front を呼び出した際、例外がでるケースがあった
// - キューに蓄積があること確認した上で呼び出しているにも拘らず
// - pop_front は例外を出さないと謳っているにも拘らず
// 原因未判別
// 
// - 蓄積対象が unique_ptr である
// - 直前にメインループの接続リトライが回数制限で止まる
// - 他起因になりそうなこととは
// 
// キャッチに流れず即断となってしまう様子だったことと
// ロックを入れたことだけで出力のコンタミは見られなくなったので
// キューに積まず出力するようにしておく
// Initialize で実施しているスレッド起動もやめておく
#define _OMIT_USE_QUEUE

using namespace utilities;


/// <summary>
/// コンストラクタ
/// </summary>
COutput::COutput(bool outputTime) :
	m_pClientConfig(nullptr),
	m_bCancelRequest(false),
	m_nLastRequestId(0),
	m_bOutputLogFile(false),
	m_bInitialized(false)
{
	// メンバ初期化
	Initialize();
}
/// <summary>
/// デストラクタ
/// </summary>
COutput::~COutput()
{
	try
	{
		if (!m_qRequests.empty())
		{
			m_qRequests.clear();
		}
	}
	catch (...) {}

#if false	// このタイミングでの reset 呼び出しで反って例外となる様子、後始末は unique_ptr インスタンスに委ねる
	try
	{
		if (m_pInstance)
		{
			m_pInstance.reset();
		}
	}
	catch (...) {}
#endif
}

/// <summary>
/// メンバ初期化
/// </summary>
void COutput::Initialize()
{
	try
	{
		m_bInitialized = false;

		m_pClientConfig = CClientConfig::GetInstance();

		//m_bCancelRequest = false;	// コンストラクタ初期化のみ
		m_qRequests.clear();
		m_nLastRequestId = 0;

		m_bInitialized = true;

#ifndef _OMIT_USE_QUEUE
		if (Initialized() && !IsCancelRequest())
		{
			m_ptWatcher.reset(new std::thread(Watcher, this));
		}
#endif
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
	}
}

///// <summary></summary>
///// <param name="mtx"></param>
///// <returns></returns>
//std::unique_lock<std::mutex> _Lock(std::mutex mtx)
//{
//    return std::unique_lock<std::mutex>(mtx);
//}
#define _Lock(mtx) std::unique_lock<std::mutex>(mtx)
/// <summary></summary>
/// <returns></returns>
bool COutput::CancelRequest()
{
	auto lock = _Lock(m_mtxCancelRequest);
	m_bCancelRequest = true;
	return m_bCancelRequest;
}
/// <summary>離脱要求取得</summary>
/// <returns></returns>
bool COutput::IsCancelRequest()
{
	auto lock = _Lock(m_mtxCancelRequest);
	return m_bCancelRequest;
}

/// <summary>
/// 出力要求識別発行
/// </summary>
/// <returns>1..INT_MAX</returns>
int COutput::IssueRequestId()
{
	if (m_nLastRequestId == INT_MAX)
		m_nLastRequestId = 1;
	else
		++m_nLastRequestId;

	return m_nLastRequestId;
}
/// <summary>出力要求追加</summary>
/// <param name="pRequest"></param>
/// <returns></returns>
int COutput::AddRequest(std::unique_ptr<OutputContent> pRequest)
{
	int id = 0;
	if (IsCancelRequest() || !pRequest)
		return id;

	auto lock = _Lock(m_mtxRequest);

	try
	{
		// 識別を付番した上でキューに追加
		id = IssueRequestId();
		if (id != 0)
		{
			pRequest->id = id;
			m_qRequests.push_back(std::move(pRequest));

			// 追加確認 #back からの参照可能か？
			if (!m_qRequests.empty())
			{
				auto p = m_qRequests.back().get();
				if (!p || (p->id != id))
					id = 0;
			}
			else
				id = 0;

			if (id == 0)
			{
				fprintf(stderr, "%s: failed add request content, id = %d\n", __FUNCTION__, id);
			}
		}
		else
		{
			fprintf(stderr, "%s: failed issue request id\n", __FUNCTION__);
			return id;
		}
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
		id = 0;
	}

	if ((id == 0) && pRequest)
		pRequest.reset();

	return id;
}
/// <summary>出力要求追加</summary>
/// <param name="eMode"></param>
/// <param name="pFileName"></param>
/// <param name="nLineNumber"></param>
/// <param name="pFuncName"></param>
/// <param name="threadId"></param>
/// <param name="pFormat"></param>
/// <param name="pArgList"></param>
/// <returns></returns>
int COutput::AddRequest(OutputMode eMode, const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList)
{
	if (!pFormat || (strlen(pFormat) <= 0))
		return 0;

	std::unique_ptr<OutputContent> pRequest = nullptr;
	std::unique_ptr<char[]> pBuff = nullptr;
	// キューに載せる前の加工自体に時間がかかりそうなのでキューとは別にロックしておく
	auto lock = _Lock(m_mtxPreRequest);

	try
	{
#if true
		std::string sValue = "";
		std::chrono::system_clock::time_point tPoint = std::chrono::system_clock::now();

		const size_t buffSize = 1024;
		pBuff.reset(new char[buffSize + 10]{ 0 });
		char* pBuffRaw = pBuff.get();

		// 可変長データの展開と結合
		memset(pBuffRaw, 0, buffSize + 10);
		vsnprintf(pBuffRaw, buffSize + 10, pFormat, pArgList);
#else
		OutputMode _eMode = eMode;
		//if (_eMode == OutputMode::OUTPUT_TRACE)
		//	_eMode = OutputMode::OUTPUT_NORMAL;
		std::string sValue = "";
		std::chrono::system_clock::time_point tPoint = std::chrono::system_clock::now();

		const size_t buffSize = 1024;
		pBuff.reset(new char[buffSize + 10]{ 0 });
		char* pBuffRaw = pBuff.get();

		// Trace/Error 指定時
		if ((_eMode == OutputMode::OUTPUT_TRACE) || (_eMode == OutputMode::OUTPUT_ERROR))
		{
			// プレフィックス
			std::string sPrefix = "";
			CreateTracePrefix(pFileName, nLineNumber, pFuncName, sPrefix, (_eMode == OutputMode::OUTPUT_ERROR));
			if (!sPrefix.empty())
				sValue += sPrefix;
		}

		// 可変長データの展開と結合
		memset(pBuffRaw, 0, buffSize + 10);
		vsnprintf(pBuffRaw, buffSize + 10, pFormat, pArgList);
#endif

		size_t len = strlen(pBuffRaw);
		if (len > 0)
		{
			if (len > buffSize)
			{
				pBuffRaw[buffSize - 4] = pBuffRaw[buffSize - 3] = pBuffRaw[buffSize - 2] = '.';
				pBuffRaw[buffSize - 1] = '\n';
				pBuffRaw[buffSize] = '\0';
			}
			sValue.append(pBuffRaw);

			std::string sFileName = pFileName ? std::string(pFileName) : "";
			std::string sFuncName = pFuncName ? std::string(pFuncName) : "";
			pRequest.reset(new OutputContent(tPoint, threadId, eMode, sFileName, nLineNumber, sFuncName, sValue));
		}
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
		if (pRequest)
			pRequest.reset();
	}

	pBuff.reset();
	if (!pRequest)
		return 0;

#ifdef _OMIT_USE_QUEUE
	//pRequest.reset();
	Output(std::move(pRequest));
	return 0;
#else
	return AddRequest(std::move(pRequest));
#endif
}
/// <summary>出力要求取得</summary>
/// <returns></returns>
std::unique_ptr<OutputContent> COutput::GetRequest()
{
	std::unique_ptr<OutputContent> pRequest = nullptr;
	auto lock = _Lock(m_mtxRequest);

	// 要求前キューの先頭を参照
	try
	{
		if (IsCancelRequest() || m_qRequests.empty())
			return pRequest;

		pRequest = std::move(m_qRequests.front());
		m_qRequests.pop_front();
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
		if (pRequest)
			pRequest.reset();
	}

	return std::move(pRequest);
}

/// <summary>出力プレフィックス生成</summary>
void COutput::CreateTracePrefix(const std::string sFileName, const int nLineNumber, const std::string sFuncName, std::string& sPrefix, bool isError)
{
	if (!sPrefix.empty())
		sPrefix.clear();
	if (/*isError && */!sFileName.empty())
		sPrefix = Format("%s(%d)", sFileName.c_str(), nLineNumber);
	if (!sFuncName.empty())
	{
		if (!sPrefix.empty())
			sPrefix += " ";
		sPrefix += sFuncName;
	}
	if (!sPrefix.empty())
		sPrefix += " : ";
}

/// <summary>出力</summary>
/// <param name="sPath"></param>
/// <param name="sValue"></param>
void COutput::Output(std::string sPath, std::string sValue)
{
	if (sPath.empty() || sValue.empty())
		return;

	try
	{
		// ディレクトリがなければ生成
		std::filesystem::path pPath = std::filesystem::path(sPath);
		std::filesystem::path pDir = pPath.parent_path();
		if (pDir.empty())
			return;
		if (!PathExists(pDir.string()))
		{
			try
			{
				std::filesystem::create_directories(pDir);
			}
			catch (const std::exception ex)
			{
				fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
				pDir = std::filesystem::path();
			}

			if (pDir.empty() || !PathExists(pDir.string()))
				return;
		}
		
		std::ofstream ofs = std::ofstream(pPath, std::ios_base::app);
		if (ofs.fail() || !ofs.is_open())
			return;
		std::string _sValue = sValue + "\n";
		ofs.write(_sValue.c_str(), _sValue.size());
		ofs.close();
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
	}
}
/// <summary>出力</summary>
/// <param name="sValue"></param>
/// <param name="isError"></param>
/// <returns></returns>
void COutput::Output(std::string sValue, bool isError)
{
	if (sValue.empty())
		return;

	try
	{
		auto fs = isError ? stderr : stdout;
		fprintf(fs, "%s", sValue.c_str());
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
	}
}
/// <summary>出力</summary>
/// <param name="sValue"></param>
/// <param name="pRequest"></param>
void COutput::Output(std::unique_ptr<OutputContent> pRequest)
{
	if (!pRequest || pRequest->m_sValue.empty())
		return;
	auto lock = _Lock(m_mtxOutput);

	try
	{
		// AddRequest から文字列加工を移植
		// プレフィックス
		std::string sPrefix = "";
		CreateTracePrefix(pRequest->m_sFilePath,
						  pRequest->m_nLineNumber,
						  pRequest->m_sFuncName,
						  sPrefix,
						  (pRequest->m_eMode == OutputMode::OUTPUT_ERROR));

		// 標準出力へ出力
		std::string sValue = "";
		if ((pRequest->m_eMode == OutputMode::OUTPUT_TRACE)
		 || (pRequest->m_eMode == OutputMode::OUTPUT_ERROR))
			sValue = sPrefix + pRequest->m_sValue;
		else
			sValue = pRequest->m_sValue;
		Output(sValue, pRequest->m_eMode == OutputMode::OUTPUT_ERROR);

		// ログファイル出力有効
		if (IsOutputLogFile())
		{
			// 行先頭に改行があればすべて削る
			const char _LF = '\n';
			std::string sData = pRequest->m_sValue;
			size_t len = 0;
			while (((len = sData.size()) > 0) && (sData[0] == _LF))
				sData = sData.substr(1);
			// 行末も同様
			while (((len = sData.size()) > 0) && (sData[len - 1] == _LF))
				sData.resize(len - 1);
			if (!sData.empty())
			{
				// 時刻
				auto n = pRequest->m_tPoint;
				sValue = DateTimeFormat(n, "%T");
				if (!sValue.empty())
				{
					int msec = (int)((n.time_since_epoch().count() / 1000000) % 1000);
					std::string sMSec = Format(".%03d ", msec);
					sValue += sMSec;
				}

				// スレッド識別
				std::ostringstream oss;
				oss << std::this_thread::get_id() << std::endl;
				std::string sId = oss.str();
				while (((len = sId.size()) > 0) && (sId[len - 1] == _LF))
					sId.resize(len - 1);
				sValue += Format("[%s] ", sId.c_str());

				// 出力識別
				std::string sMode = "??? ";
				switch (pRequest->m_eMode)
				{
				case OutputMode::OUTPUT_ERROR:	sMode = "err ";	break;
				case OutputMode::OUTPUT_TRACE:	sMode = "inf ";	break;
				case OutputMode::OUTPUT_NORMAL:	sMode = "--- ";	break;
				}
				sValue += sMode;

				// プレフィックス
				if ((pRequest->m_eMode == OutputMode::OUTPUT_TRACE)
				 || (pRequest->m_eMode == OutputMode::OUTPUT_ERROR))
					sValue += sPrefix;

				// 文字列
				sValue += sData;

				// ファイルへ出力
				std::string sPath = DateTimeFormat(n, m_pClientConfig->LogFilePathFormat());
				Output(sPath, sValue);
			}
		}
	}
	catch (const std::exception ex)
	{
		fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
	}

	if (pRequest)
		pRequest.reset();
}

/// <summary>
/// </summary>
void COutput::Watcher(COutput* instance)
{
	if ((instance == nullptr) || !instance->Enabled())
		return;

	auto emptyDelay = std::chrono::milliseconds(instance->m_pClientConfig->MainThreadDelay());
	while (!instance->m_bCancelRequest)
	{
		std::this_thread::sleep_for(emptyDelay);

		std::unique_ptr<OutputContent> pRequest = nullptr;
		try
		{
			// 要求取り出し
			pRequest = instance->GetRequest();
			if (!pRequest)
			{
				//std::this_thread::sleep_for(emptyDelay);
				continue;
			}

			instance->Output(std::move(pRequest));
		}
		catch (const std::exception ex)
		{
			fprintf(stderr, "%s: exception : %s\n", __FUNCTION__, ex.what());
			// 例外出るようなら継続しない
			if (pRequest)
				pRequest.reset();
			break;
		}

		if (pRequest)
			pRequest.reset();
	}
}


// ====================================================================

/// <summary>
/// ガイダンス
/// </summary>
/// <param name="threadId"></param>
/// <param name="pFormat"></param>
/// <param name="pArgList"></param>
void COutput::Guidance(const std::thread::id threadId, const char* pFormat, va_list pArgList)
{
	auto pInstance = GetInstance();
	if (pInstance && pInstance->Enabled())
	{
		pInstance->AddRequest(OutputMode::OUTPUT_NORMAL, 0, 0, 0, threadId, pFormat, pArgList);
	}
}
/// <summary>
/// トレース
/// </summary>
/// <param name="pFileName"></param>
/// <param name="nLineNumber"></param>
/// <param name="pFuncName"></param>
/// <param name="threadId"></param>
/// <param name="pFormat"></param>
/// <param name="pArgList"></param>
void COutput::Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList)
{
	auto pInstance = GetInstance();
	if (pInstance && pInstance->Enabled())
	{
		pInstance->AddRequest(OutputMode::OUTPUT_TRACE, pFileName, nLineNumber, pFuncName, threadId, pFormat, pArgList);
	}
}
/// <summary>
/// エラー手続
/// </summary>
/// <param name="pFileName"></param>
/// <param name="nLineNumber"></param>
/// <param name="pFuncName"></param>
/// <param name="threadId"></param>
/// <param name="pFormat"></param>
/// <param name="pArgList"></param>
void COutput::ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const std::thread::id threadId, const char* pFormat, va_list pArgList)
{
	auto pInstance = GetInstance();
	if (pInstance && pInstance->Enabled())
	{
		pInstance->AddRequest(OutputMode::OUTPUT_ERROR, pFileName, nLineNumber, pFuncName, threadId, pFormat, pArgList);
	}
}


// ====================================================================
// static instance
// ====================================================================

/// <summary>
/// 単一インスタンス（実体）
/// </summary>
//static std::unique_ptr<COutput> m_pOutput(new COutput());
std::unique_ptr<COutput> COutput::m_pInstance{};

/// <summary>
/// インスタンス取得
/// </summary>
/// <returns></returns>
COutput* COutput::GetInstance()
{
	if (!m_pInstance)
		m_pInstance.reset(new COutput());
	return m_pInstance.get();
}
