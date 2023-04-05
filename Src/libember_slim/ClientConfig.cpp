#include "ClientConfig.h"
#include "Output.h"
#include <cassert>

using namespace utilities;

#undef min
#undef max


/// <summary>
/// コンストラクタ
/// </summary>
CClientConfig::CClientConfig() :
	//本来はInitialize()でdefault.csvから初期値の更新を行うがとりあえず現状は代入値で起動する
	m_sPath(INI_FILE),
	m_sDefaultDeviceContentsPath(DEV_CONTS_DEF_PATH),
	m_sStartupDeviceContentsPath(getAppDataPath() + DEV_CONTS_STARTUP_PATH),
	m_sInhibitValuesPath(getAppDataPath() + DEV_CONTS_INHIBIT_PATH),

	m_sOutputLogDirectory(getAppDataPath() + OUTPUT_LOG_DIRECTORY),
	m_sLogFilePathFormat(""),
	m_bOutputTrace(false),
	m_bLogFileEnabled(false),
	m_bMultibyteFormat(false),
	m_bDebugOmitSetLEDStatus(false),
	m_bDebugOmitSetOLEDStatus(false),

	m_nSocketReconnectDelay(SOCKET_RECONN_DELAY_DEF),
	m_nMainThreadDelay(THREAD_DELAY_DEF),
	m_nEmberThreadDelay(THREAD_DELAY_DEF),

	//m_sHwifIpAddr("127.0.0.1"),
	//m_nHwifPort(PROTOPORT_HWIF),
	m_sHwifIpAddr("192.168.1.237"),
	m_nHwifPort(53278),
	m_bHwifEnabled(true),

	//m_sNmosEmberIpAddr("127.0.0.1"),
	//m_nNmosEmberPort(PROTOPORT_NMOS_EMBER),
	//m_bNmosEmberEnabled(false),
	m_sNmosEmberIpAddr("192.168.0.130"),
	m_nNmosEmberPort(50005),
	m_bNmosEmberEnabled(true),
	m_bNmosEmberUseMatrixLabels(true),

	m_sMvEmberIpAddr("127.0.0.1"),
	m_nMvEmberPort(PROTOPORT_MV_EMBER),
	m_bMvEmberEnabled(false),
	m_bMvEmberUseMatrixLabels(true),

	m_sFileImportBackupDirectory(""),
	m_sFileImportDirectory(""),
	m_sFileImportTargetFile(""),
	m_sFileImportDefaultViewFile(""),
	m_sFileImportViewFile(""),
	m_tpFileImportCatchTime(),
	m_bFileImportEnabled(false),
	m_bMuteAll(false),
	m_bInitialized(false)
{
	// メンバ初期化
	//Initialize();
}
/// <summary>
/// デストラクタ
/// </summary>
CClientConfig::~CClientConfig()
{
	if (!m_vConfLines.empty())
	{
		m_vConfLines.clear();
	}

	if (m_pInstance)
	{
		delete m_pInstance;
		m_pInstance = nullptr;
	}
}

/// <summary>
/// 標準エラー出力
/// </summary>
/// <param name="nLineNumber"></param>
/// <param name="pFuncName"></param>
/// <param name="sValue"></param>
/// <remarks>
/// COutput::Trace/ErrorHandler では CClientInfo の内容を参照しており
/// CClientInfo のコンストラクタ終了まで使用ができない
/// CClientInfo の参照がない静的出力関数を使用
/// </remarks>
static void _InnerErrorHandler(int nLineNumber, const char* pFuncName, std::string sValue)
{
	if (sValue.empty())
		return;

	std::string sPrefix = "";
	COutput::CreateTracePrefix(__FILE__, nLineNumber, pFuncName, sPrefix, true);
	COutput::Output(sPrefix + sValue, true);
}

/// <summary>
/// 古いログファイル削除
/// </summary>
/// <returns></returns>
/// <remarks>
/// もし古いログファイルがあれば
/// 当日および前日までで直近1つのみ残して削除
/// </remarks>
bool CClientConfig::DeleteOlderLogFiles()
{
	bool res = false;
	try
	{
		if (m_sLogFilePathFormat.empty())
			return res;

		// 日付文字列を作成
		std::string sTodaysPath = DateTimeFormat(std::chrono::system_clock::now(), m_sLogFilePathFormat);
		std::filesystem::path pTodaysPath = std::filesystem::path(sTodaysPath);
		std::filesystem::path pDir = pTodaysPath.parent_path();
		std::string ext = pTodaysPath.extension().string();
		std::string newerName = pTodaysPath.stem().string();
		std::string preName = "";
		std::vector<std::string> vPath{};
		if (!PathExists(pDir.string()) || (newerName.length() <= 8))
			return res;

		std::string sPrefix = newerName.substr(0, sPrefix.length() - 8);
		for (const auto& x : std::filesystem::directory_iterator(pDir))
		{
			auto sName = x.path().stem().string();
			if (sName.find(sPrefix) == std::string::npos)
				continue;
			if (sName == newerName)
				continue;
			if (preName.empty())
			{
				preName = sName;
				continue;
			}
			if (preName == sName)	// 発生し得ない、無視
				continue;

			if (preName < sName)
			{
				vPath.push_back(preName);
				preName = sName;
			}
			else
				vPath.push_back(sName);
		}
		while (!vPath.empty())
		{
			auto itr = vPath.begin();
			auto sPath = *itr;
			vPath.erase(itr);
			
			std::error_code errCode;
			std::filesystem::path pPath = pDir;
			pPath.append(sPath + ext);
			std::filesystem::remove(pPath, errCode);

			if (!res)
				res = true;
		}
	}

	catch (const std::exception ex)
	{
		// CClientConfig のコンストラクタ終了前段階で COutput の Trace/ErrorHandler を使用しない
		//ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
	}

	return res;
}

/// <summary>
/// メンバ初期化
/// </summary>
void CClientConfig::Initialize()
{
	assert(!m_sPath.empty());
	assert(!m_sDefaultDeviceContentsPath.empty() && PathExists(m_sDefaultDeviceContentsPath));
	assert(!m_sStartupDeviceContentsPath.empty());
	if (!PathExists(m_sStartupDeviceContentsPath))
	{
		FileCopy(m_sStartupDeviceContentsPath, m_sDefaultDeviceContentsPath);
	}

	try
	{
		// ファイル内容を先に取得しておく
		m_vConfLines.clear();
		int len = CommReadIniFile(m_sPath, m_vConfLines);

		// ファイル内容が取得できた場合のみデータ取得する
		// 取得失敗時はデフォルト値ままとなる
		if (len > 0)
		{
			// ファイル内データ取得
			// データ変換は関数戻りがエラーでないかつ文字列ありの場合のみ実施
			std::string tmp = "";
			bool ena = false;

			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, INI_KEY_TRACE, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
				m_bLogFileEnabled = ena;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, INI_KEY_LOG_FILE, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
				m_bLogFileEnabled = ena;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, INI_KEY_MB_FORMAT, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
				m_bMultibyteFormat = ena;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, "DebugOmitSetLEDStatus", tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
				m_bDebugOmitSetLEDStatus = ena;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, "DebugOmitSetOLEDStatus", tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
				m_bDebugOmitSetOLEDStatus = ena;
			}

			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, INI_KEY_RECONN_DELAY, tmp) == 0) && !tmp.empty())
			{
				int num = 0;
				if (ToNumber(tmp, num) && (m_nSocketReconnectDelay != num) && IsRange(num, SOCKET_RECONN_DELAY_MIN, SOCKET_RECONN_DELAY_MAX))
					m_nSocketReconnectDelay = (unsigned)num;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, INI_KEY_MTHREAD_DELAY, tmp) == 0) && !tmp.empty())
			{
				int num = 0;
				if (ToNumber(tmp, num) && (m_nMainThreadDelay != num) && IsRange(num, THREAD_DELAY_MIN, THREAD_DELAY_MAX))
					m_nMainThreadDelay = (unsigned)num;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_COMMON, INI_KEY_ETHREAD_DELAY, tmp) == 0) && !tmp.empty())
			{
				int num = 0;
				if (ToNumber(tmp, num) && (m_nEmberThreadDelay != num) && IsRange(num, THREAD_DELAY_MIN, THREAD_DELAY_MAX))
					m_nEmberThreadDelay = (unsigned)num;
			}

			if ((CommGetIniFileData(m_vConfLines, INI_SEC_HWIF, INI_KEY_IPADDR, tmp) == 0) && !tmp.empty())
			{
				if (IsIPv4(tmp))
					//m_sHwifIpAddr = tmp;
				/***/
				{
					m_sHwifIpAddr.clear();
					m_sHwifIpAddr = tmp;
				}
				/***/
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_HWIF, INI_KEY_PORT, tmp) == 0) && !tmp.empty())
			{
				unsigned short num = 0;
				if (ToNumber(tmp, num) && (num != 0) && (m_nHwifPort != num))
					m_nHwifPort = num;
			}
			// enabled は他条件も考慮して設定
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_HWIF, INI_KEY_ENABLE, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
			}
			m_bHwifEnabled = IsIPv4(m_sHwifIpAddr) && (m_nHwifPort != 0) && ena;

			if ((CommGetIniFileData(m_vConfLines, INI_SEC_NMOS_EMBER, INI_KEY_IPADDR, tmp) == 0) && !tmp.empty())
			{
				if (IsIPv4(tmp))
					m_sNmosEmberIpAddr = tmp;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_NMOS_EMBER, INI_KEY_PORT, tmp) == 0) && !tmp.empty())
			{
				unsigned short num = 0;
				if (ToNumber(tmp, num) && (num != 0) && (m_nNmosEmberPort != num))
					m_nNmosEmberPort = num;
			}
			// enabled は他条件も考慮して設定
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_NMOS_EMBER, INI_KEY_ENABLE, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
			}
			m_bNmosEmberEnabled = IsIPv4(m_sNmosEmberIpAddr) && (m_nNmosEmberPort != 0) && ena;
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_NMOS_EMBER, INI_KEY_MATRIX_LABELS, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				m_bNmosEmberUseMatrixLabels = ToBool(tmp, ena) ? ena : true;
			}

			if ((CommGetIniFileData(m_vConfLines, INI_SEC_MV_EMBER, INI_KEY_IPADDR, tmp) == 0) && !tmp.empty())
			{
				if (IsIPv4(tmp))
					m_sMvEmberIpAddr = tmp;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_MV_EMBER, INI_KEY_PORT, tmp) == 0) && !tmp.empty())
			{
				unsigned short num = 0;
				if (ToNumber(tmp, num) && (num != 0) && (m_nMvEmberPort != num))
					m_nMvEmberPort = num;
			}
			// enabled は他条件も考慮して設定
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_MV_EMBER, INI_KEY_ENABLE, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				ToBool(tmp, ena);
			}
			m_bMvEmberEnabled = IsIPv4(m_sMvEmberIpAddr) && (m_nMvEmberPort != 0) && ena;
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_MV_EMBER, INI_KEY_MATRIX_LABELS, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				m_bMvEmberUseMatrixLabels = ToBool(tmp, ena) ? ena : true;
			}

			if ((CommGetIniFileData(m_vConfLines, INI_SEC_FILE_IMPORT, INI_KEY_DIRECTORY, tmp) == 0) && !tmp.empty())
			{
				if (m_sFileImportDirectory != tmp)
					m_sFileImportDirectory = tmp;
			}
			if ((CommGetIniFileData(m_vConfLines, INI_SEC_FILE_IMPORT, INI_KEY_ENABLE, tmp) == 0) && !tmp.empty())
			{
				ena = false;
				if (ToBool(tmp, ena))
					m_bFileImportEnabled = ena;
			}
		}
	}
	catch (const std::exception ex)
	{
		// CClientConfig のコンストラクタ終了前段階で COutput の Trace/ErrorHandler を使用しない
		//ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
	}

	// ログファイル出力有無に拘らずパス情報は生成する
	std::string ExecutableFilePath = "";
	std::filesystem::path pBase = std::filesystem::path(ExecutableFilePath);
	try
	{
#if defined WIN32
		std::filesystem::path pDir = std::filesystem::temp_directory_path();
		pDir.append("for-a");
		pDir.append("fru-core");
		pDir.append(m_sOutputLogDirectory);
		m_sLogFilePathFormat = pDir.append(pBase.stem().string()).string() + "%Y%m%d" + OUTPUT_LOG_EXTENSION;
#else
		std::filesystem::path pDir = pBase.parent_path().append(m_sOutputLogDirectory);
		m_sLogFilePathFormat = pDir.append(pBase.stem().string()).string() + "%Y%m%d" + OUTPUT_LOG_EXTENSION;
#endif

		// もし古いログファイルがあれば
		// 当日および前日までで直近1つのみ残して削除
		DeleteOlderLogFiles();
	}
	catch (const std::exception ex)
	{
		// CClientConfig のコンストラクタ終了前段階で COutput の Trace/ErrorHandler を使用しない
		//ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
	}

	// ファイルインポート用パスの調整
	try
	{
		// バックアップディレクトリ名
		std::string sBackupDef = getAppDataPath()+FILE_IMPORT_BACKUP_DIR;
		assert(!sBackupDef.empty());
		m_sFileImportBackupDirectory = pBase.parent_path().append(sBackupDef).string();
		std::filesystem::path pBackupDir(m_sFileImportBackupDirectory);
		// ディレクトリがなければ生成
		if (!std::filesystem::exists(pBackupDir))
		{
			try
			{
				std::filesystem::create_directories(pBackupDir);
			}
			catch (const std::exception ex)
			{
				_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
			}
		}

		// インポートディレクトリ名指定が空なら合成して作成
		std::string sImportDef = FILE_IMPORT_DIRECTORY;
		assert(!sImportDef.empty());
		if (m_sFileImportDirectory.empty())
			m_sFileImportDirectory = pBase.parent_path().append(sImportDef).string();
		std::filesystem::path pImportDir(m_sFileImportDirectory);
		// ディレクトリがなければ生成
		if (!std::filesystem::exists(pImportDir))
		{
			try
			{
				std::filesystem::create_directories(pImportDir);
			}
			catch (const std::exception ex)
			{
				_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
				pImportDir = std::filesystem::path();
			}
		}
		if (!pImportDir.empty() && std::filesystem::exists(pImportDir) && std::filesystem::is_directory(pImportDir))
		{
			std::filesystem::permissions(pImportDir, std::filesystem::perms::all);

			// 機能使用有無に拘らずデフォルト表示用ファイルは配置
			std::filesystem::path pStartupFilePath(m_sStartupDeviceContentsPath);
			m_sFileImportDefaultViewFile = std::filesystem::path(pImportDir).append(pStartupFilePath.filename().string()).string();
			FileCopy(m_sFileImportDefaultViewFile, m_sStartupDeviceContentsPath);
		}
		else
		{
			// 使用を諦める
			m_sFileImportDefaultViewFile = "";
			m_bFileImportEnabled = false;
		}

		// 参照ファイル名
		m_sFileImportTargetFile = std::filesystem::path(pImportDir).append(FILE_IMPORT_TARGET_FILE).string();
		m_sFileImportViewFile = std::filesystem::path(pImportDir).append(FILE_IMPORT_VIEW_FILE).string();
	}
	catch (const std::exception ex)
	{
		// CClientConfig のコンストラクタ終了前段階で COutput の Trace/ErrorHandler を使用しない
		//ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
	}
	m_tpFileImportCatchTime = std::chrono::system_clock::now();

	m_bInitialized = true;
}

/// <summary>
/// インヒビット状態書き出し
/// </summary>
/// <param name="value"></param>
/// <returns></returns>
bool CClientConfig::OutputInhibitValues(std::string value)
{
	assert(!m_sInhibitValuesPath.empty());
	std::error_code errcode{};
	std::filesystem::path path(m_sInhibitValuesPath);
	try
	{
		// 既存のファイルを削除
		if (std::filesystem::exists(path))
		{
			FileRemove(path, errcode);
		}

		// 出力要求がない
		if (value.empty())
			return false;

		// 書き出し
		std::ofstream ofs{};
		ofs.open(path);
		ofs.write(value.c_str(), value.size());
		ofs.close();
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return std::filesystem::exists(path);
}

/// <summary>デバイス情報リロード要求有無</summary>
/// <returns></returns>
bool CClientConfig::DeviceDataReloadRequest()
{
	bool res = false;
	if (!FileImportEnabled())
		return res;

	bool cpres = false;
	std::error_code errCode{};
	std::filesystem::path pTemp{};
	try
	{
		// 対象ファイルの有無確認
		std::filesystem::path pTarget(m_sFileImportTargetFile);
		if (!PathExists(pTarget))
			return res;
		// サイズ確認
		if (std::filesystem::file_size(pTarget) == 0)
			return res;
		// 認識時間控え
		auto now = std::chrono::system_clock::now();
		// 前回確認時に削除できなかったものなら再度削除を試行する
		//auto ftime = std::filesystem::last_write_time(pTarget);
		//if (ftime.time_since_epoch() < m_tpFileImportCatchTime.time_since_epoch())
		std::filesystem::path pStartup(m_sStartupDeviceContentsPath);
		std::filesystem::path pShare(m_sFileImportDefaultViewFile);
		auto targetTime = std::filesystem::last_write_time(pTarget);
		auto shareTime = std::filesystem::last_write_time(pShare);
		if (targetTime <= shareTime)
		{
			try
			{
				FileRemove(pTarget, errCode);
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			}
			if (PathExists(pTarget))
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pTarget.string().c_str());
			}
			return res;
		}

		// テンポラリファイル名を用意
		pTemp = pTarget.parent_path().append(pTarget.stem().string() + DateTimeFormat(now, "%H%M%S") + pTarget.extension().string());
		if (!pTemp.empty())
		{
			// ファイルがあれば削除
			if (PathExists(pTemp))
			{
				try
				{
					FileRemove(pTemp, errCode);
				}
				catch (const std::exception ex)
				{
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
				}
				if (PathExists(pTemp))
				{
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pTemp.string().c_str());
					// 処理は継続
				}
			}
		}

		// 一旦テンポラリに移す
		try
		{
			cpres = FileCopy(pTemp, pTarget);
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
		Trace(__FILE__, __LINE__, __FUNCTION__, "copy file, result = %d, %s -> %s\n", cpres, pTarget.string().c_str(), pTemp.string().c_str());
		// コピーに成功
		if (cpres && PathExists(pTemp))
		{
			// コピーできたなら即削除
			try
			{
				FileRemove(pTarget, errCode);
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			}
			if (PathExists(pTarget))
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pTarget.string().c_str());
				// 処理は継続
			}
		}
		// コピーに失敗する場合直接参照
		else
		{
			pTemp = pTarget;
			if (!PathExists(pTemp))
			{
				// 取り込み不可
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed import file, path = %s\n", pTarget.string().c_str());
				return res;
			}
		}

		// 削除できていれば次回はファイルがあれば新規と判断できるが
		// 補助として時間を控えておく
		m_tpFileImportCatchTime = now;

		// 現行 startup のバックアップ
		std::filesystem::path pBackupDir = std::filesystem::path(m_sFileImportBackupDirectory);
		std::filesystem::path pBk0(std::filesystem::path(pBackupDir).append(pStartup.filename().string()));
		if (!m_sFileImportBackupDirectory.empty())
		{
			if (PathExists(pBackupDir))
			{
				// フォルダ内にファイルがある場合は1世代のみ残す
				if (PathExists(pBk0))
				{
					std::filesystem::path pBk1(std::filesystem::path(pBackupDir).append(pStartup.stem().string() + ".bak"));
					if (PathExists(pBk1))
					{
						// 削除試行
						try
						{
							FileRemove(pBk1, errCode);
						}
						catch (const std::exception ex)
						{
							ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
						}
						if (PathExists(pBk1))
						{
							ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pBk1.string().c_str());
							// 処理は継続
						}
					}
					// コピー試行
					cpres = false;
					try
					{
						cpres = FileCopy(pBk1, pBk0);
					}
					catch (const std::exception ex)
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
					}
					Trace(__FILE__, __LINE__, __FUNCTION__, "copy file, result = %d, %s -> %s\n", cpres, pBk0.string().c_str(), pBk1.string().c_str());

					// 削除試行
					try
					{
						FileRemove(pBk0, errCode);
					}
					catch (const std::exception ex)
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
					}
					if (PathExists(pBk0))
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pBk0.string().c_str());
						// 処理は継続
					}
				}
			}
			else
			{
				try
				{
					std::filesystem::create_directories(pBackupDir);
				}
				catch (const std::exception ex)
				{
					_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
				}
			}

			// バックアップディレクトリがあれば startup をコピー
			if (PathExists(pBackupDir))
			{
				cpres = false;
				try
				{
					cpres = FileCopy(pBk0, pStartup);
				}
				catch (const std::exception ex)
				{
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
				}
				Trace(__FILE__, __LINE__, __FUNCTION__, "copy file, result = %d, %s -> %s\n", cpres, pStartup.string().c_str(), pBk0.string().c_str());
			}
		}

		// client() のメインループ内からしか当該関数も startup.csv も参照されない
		// ここでファイルを直接差し替える
		// 削除試行
		try
		{
			FileRemove(pStartup, errCode);

			std::filesystem::path pInhibit(m_sInhibitValuesPath);
			if (std::filesystem::exists(pInhibit))
				FileRemove(pInhibit, errCode);
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
		if (PathExists(pStartup))
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pStartup.string().c_str());
			// 処理は継続
		}

		// startup.csv の有無で振る舞いを変える
		if (PathExists(pStartup))
		{
			// コピー試行
			cpres = false;
			try
			{
				cpres = FileCopy(pStartup, pTemp);
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			}
			Trace(__FILE__, __LINE__, __FUNCTION__, "copy file, result = %d, %s -> %s\n", cpres, pTemp.string().c_str(), pStartup.string().c_str());
		}
		if (!PathExists(pStartup))
		{
			// 移動試行
			cpres = false;
			try
			{
				FileMove(pStartup, pTemp, errCode);
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			}
			Trace(__FILE__, __LINE__, __FUNCTION__, "move file, result = %d, %s -> %s\n", cpres, pTemp.string().c_str(), pStartup.string().c_str());

			// ここまでして startup.csv がなくなってしまった場合
			if (!PathExists(pStartup))
			{
				// バックアップが残っているならコピー
				if (PathExists(pBk0))
				{
					// コピー試行
					cpres = false;
					try
					{
						cpres = FileCopy(pStartup, pBk0);
					}
					catch (const std::exception ex)
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
					}
					Trace(__FILE__, __LINE__, __FUNCTION__, "copy file, result = %d, %s -> %s\n", cpres, pBk0.string().c_str(), pStartup.string().c_str());
				}
			}
		}

		// フォルダ内の startup も更新する
		if (PathExists(pStartup))
		{
			// 削除試行
			try
			{
				FileRemove(pShare, errCode);
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			}
			if (PathExists(pShare))
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pShare.string().c_str());
				// 処理は継続
			}

			// コピー試行
			cpres = false;
			try
			{
				cpres = FileCopy(pShare, pStartup);
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			}
			Trace(__FILE__, __LINE__, __FUNCTION__, "copy file, result = %d, %s -> %s\n", cpres, pStartup.string().c_str(), pShare.string().c_str());
		}

		res = PathExists(pStartup);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	// テンポラリが残っているなら削除
	try
	{
		if (PathExists(pTemp))
		{
			FileRemove(pTemp, errCode);
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
	if (PathExists(pTemp))
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed remove file, path = %s\n", pTemp.string().c_str());
		// 処理は継続
	}

	// true 時のみトレース
	if (res)
	{
		Trace(__FILE__, __LINE__, __FUNCTION__, "exit, result = 1.\n");
	}
	return res;
}

/// <summary>
/// インスタンス取得
/// </summary>
/// <returns></returns>
CClientConfig* CClientConfig::GetInstance()
{
	if (m_pInstance == nullptr)
		m_pInstance = new CClientConfig();
	return m_pInstance;
}

#if defined WIN32
#include <shlobj.h>
#endif
std::string CClientConfig::getAppDataPath()
{
#if defined WIN32
	char out[MAX_PATH];
	if (SHGetSpecialFolderPathA(NULL, out, CSIDL_APPDATA, 0)) {

		std::string v= std::string(out) +"\\FOR-A\\FRU\\";
		try {
			std::filesystem::create_directories(v);
		}
		catch (const std::exception ex)
		{
			_InnerErrorHandler(__LINE__, __FUNCTION__, "exception : " + std::string(ex.what()) + "\n");
		}
		return v;
	}
	else {
		return "";
	}
#else
	return "";
#endif
}

/// <summary>
/// 単一インスタンス（実体）
/// </summary>
CClientConfig* CClientConfig::m_pInstance = nullptr;
