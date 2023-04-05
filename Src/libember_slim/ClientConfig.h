#pragma once

#include "SocketEx.h"
#include "Utilities.h"


// ====================================================================
// TCP/IP 接続用識別
// ====================================================================
/// <summary>Client用ソケット識別</summary>
enum class ClientSocketId : uint8_t
{
	/// <summary>ハードウェアI/F</summary>
	SOCK_HWIF = 0,

	/// <summary>NMOS-Ember プロバイダ</summary>
	SOCK_NMOS_EMBER,

	/// <summary>MV-Ember プロバイダ</summary>
	SOCK_MV_EMBER,


	/// <summary>ソケット数</summary>
	SOCK_COUNT,
	/// <summary>不定</summary>
	SOCK_UNKNOWN = INT8_MAX,
};

/// <summary>ソケット識別判断：定義ID</summary>
/// <param name="value"></param>
/// <returns></returns>
inline bool IsClientSocketId(int value) { return utilities::IsRange(value, 0, (int)ClientSocketId::SOCK_COUNT - 1); }
/// <summary>ソケット識別判断：定義ID</summary>
/// <param name="value"></param>
/// <returns></returns>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
inline bool IsDefined(ClientSocketId value) { return IsClientSocketId((int)value); }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
/// <summary>ソケット識別判断：ハードウェアI/F</summary>
/// <param name="value"></param>
/// <returns></returns>
inline bool IsHwif(ClientSocketId value) { return (value == ClientSocketId::SOCK_HWIF); }
/// <summary>ソケット識別判断：NMOS-Ember プロバイダ</summary>
/// <param name="value"></param>
/// <returns></returns>
inline bool IsNmosEmber(ClientSocketId value) { return (value == ClientSocketId::SOCK_NMOS_EMBER); }
/// <summary>ソケット識別判断：MV-Ember プロバイダ</summary>
/// <param name="value"></param>
/// <returns></returns>
inline bool IsMvEmber(ClientSocketId value) { return (value == ClientSocketId::SOCK_MV_EMBER); }
/// <summary>ソケット識別判断：Ember プロバイダ</summary>
/// <param name="value"></param>
/// <returns></returns>
inline bool IsEmberId(ClientSocketId value) { return IsNmosEmber(value) || IsMvEmber(value); }


/// <summary>ポート番号デフォルト：ハードウェアI/F</summary>
#define PROTOPORT_HWIF			5193	//PROTOPORT
/// <summary>ポート番号デフォルト：NMOS-Ember プロバイダ</summary>
#define PROTOPORT_NMOS_EMBER	57001
/// <summary>ポート番号デフォルト：MV-Ember プロバイダ</summary>
#define PROTOPORT_MV_EMBER		57002

/// <summary>ソケット再接続時ディレイデフォルト</summary>
#define SOCKET_RECONN_DELAY_DEF	5000
/// <summary>ソケット再接続時ディレイ最小値</summary>
#define SOCKET_RECONN_DELAY_MIN	1000
/// <summary>ソケット再接続時ディレイ最大値</summary>
#define SOCKET_RECONN_DELAY_MAX	10000

/// <summary>スレッドディレイデフォルト</summary>
#define THREAD_DELAY_DEF		20
/// <summary>スレッドディレイ最小値</summary>
#define THREAD_DELAY_MIN		5
/// <summary>スレッドディレイ最大値</summary>
#define THREAD_DELAY_MAX		10000


// ====================================================================
// 設定ファイル用識別
// ====================================================================
/// <summary>Client用設定ファイル</summary>
#define INI_FILE				"./conf.ini"
/// <summary>Client用設定ファイルセクション：共用</summary>
#define INI_SEC_COMMON			"Common"
/// <summary>Client用設定ファイルセクション：ハードウェアI/F</summary>
#define INI_SEC_HWIF			"HwIf"
/// <summary>Client用設定ファイルセクション：NMOS-Ember プロバイダ</summary>
#define INI_SEC_NMOS_EMBER		"NmosEmber"
/// <summary>Client用設定ファイルセクション：MV-Ember プロバイダ</summary>
#define INI_SEC_MV_EMBER		"MvEmber"
/// <summary>Client用設定ファイルセクション：ファイルインポート</summary>
#define INI_SEC_FILE_IMPORT		"FileImport"

/// <summary>Client用設定ファイルキー：ログファイル出力有無効</summary>
#define INI_KEY_LOG_FILE		"OutputLogFile"
/// <summary>Client用設定ファイルキー：TRACE出力有無効</summary>
#define INI_KEY_TRACE		"OutputTrace"
/// <summary>Client用設定ファイルキー：ファイル書式マルチバイト指定</summary>
#define INI_KEY_MB_FORMAT		"MultibyteFormat"

/// <summary>Client用設定ファイルキー：ソケット再接続時ディレイ</summary>
#define INI_KEY_RECONN_DELAY	"SocketReconnectDelay"
/// <summary>Client用設定ファイルキー：メインスレッドディレイ</summary>
#define INI_KEY_MTHREAD_DELAY	"MainThreadDelay"
/// <summary>Client用設定ファイルキー：Ember監視スレッドディレイ</summary>
#define INI_KEY_ETHREAD_DELAY	"EmberThreadDelay"

/// <summary>Client用設定ファイルキー：IPアドレス（ホスト）</summary>
#define INI_KEY_IPADDR			"IpAddr"
/// <summary>Client用設定ファイルキー：ポート番号</summary>
#define INI_KEY_PORT			"Port"
/// <summary>Client用設定ファイルキー：TCP/IP接続有無効</summary>
#define INI_KEY_ENABLE			"Enable"

/// <summary>Client用設定ファイルキー：マトリックスラベル使用有無</summary>
#define INI_KEY_MATRIX_LABELS	"UseMatrixLabels"

/// <summary>Client用設定ファイルキー：ディレクトリ</summary>
#define INI_KEY_DIRECTORY		"Directory"


// ====================================================================
// データファイル用識別
// ====================================================================
/// <summary>デバイス情報ファイル：デフォルト</summary>
#define DEV_CONTS_DEF_PATH		"./default.csv"
/// <summary>デバイス情報ファイル：起動時</summary>
#define DEV_CONTS_STARTUP_PATH	"./startup.csv"
/// <summary>デバイスインヒビット状態ファイル</summary>
#define DEV_CONTS_INHIBIT_PATH	"./inhibits.dat"

/// <summary>ログファイル出力ディレクトリ名</summary>
#define OUTPUT_LOG_DIRECTORY	"log"
/// <summary>ログファイル拡張子</summary>
#define OUTPUT_LOG_EXTENSION	".log"

/// <summary>ファイルインポート時バックアップディレクトリ</summary>
#define FILE_IMPORT_BACKUP_DIR	"backup"
/// <summary>ファイルインポート監視ディレクトリ名デフォルト</summary>
#define FILE_IMPORT_DIRECTORY	"webtemp"
/// <summary>インポートファイル名</summary>
#define FILE_IMPORT_TARGET_FILE	"importfile.csv"
/// <summary>閲覧ファイル名</summary>
#define FILE_IMPORT_VIEW_FILE	"userfile.csv"

// ====================================================================

/// <summary>
/// CClientConfig
/// Client用情報クラス
/// </summary>
/// <remarks>
/// シングルトン使用想定
/// </remarks>
class CClientConfig
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CClientConfig();
	/// <summary>
	/// デストラクタ
	/// </summary>
	~CClientConfig();

	std::string Path() { return m_sPath; };

	std::string DefaultDeviceContentsPath() { return m_sDefaultDeviceContentsPath; }
	std::string StartupDeviceContentsPath() { return m_sStartupDeviceContentsPath; }
	std::string InhibitValuesPath() { return m_sInhibitValuesPath; }

	std::string getAppDataPath();

	bool bOutputTrace() { return m_bOutputTrace; }
	bool LogFileEnabled() { return m_bLogFileEnabled && !m_sLogFilePathFormat.empty(); }
	std::string LogFilePathFormat() { return m_sLogFilePathFormat; }
	bool MuitibyteFormat() { return m_bMultibyteFormat; }
	bool DebugOmitSetLEDStatus() { return m_bDebugOmitSetLEDStatus; }
	bool DebugOmitSetOLEDStatus() { return m_bDebugOmitSetOLEDStatus; }

	unsigned int SocketReconnectDelay() { return m_nSocketReconnectDelay; }
	unsigned int MainThreadDelay() { return m_nMainThreadDelay; }
	unsigned int EmberThreadDelay() { return m_nEmberThreadDelay; }

	std::string HwifIpAddr() { return m_sHwifIpAddr; }
	unsigned short HwifPort() { return m_nHwifPort; }
	bool HwifEnabled() { return m_bHwifEnabled; }

	std::string NmosEmberIpAddr() { return m_sNmosEmberIpAddr; }
	unsigned short NmosEmberPort() { return m_nNmosEmberPort; }
	bool NmosEmberEnabled() { return m_bNmosEmberEnabled; }
	bool NmosEmberUseMatrixLabels() { return m_bNmosEmberUseMatrixLabels; }

	std::string MvEmberIpAddr() { return m_sMvEmberIpAddr; }
	unsigned short MvEmberPort() { return m_nMvEmberPort; }
	bool MvEmberEnabled() { return m_bMvEmberEnabled; }
	bool MvEmberUseMatrixLabels() { return m_bMvEmberUseMatrixLabels; }

	bool Enabled(ClientSocketId id)
	{
		return (id == ClientSocketId::SOCK_MV_EMBER) ? m_bMvEmberEnabled
			 : (id == ClientSocketId::SOCK_NMOS_EMBER) ? m_bNmosEmberEnabled
			 : (id == ClientSocketId::SOCK_HWIF) ? m_bHwifEnabled
			 : false;
	}
	std::string FileInportBackupDirectory() { return m_sFileImportBackupDirectory; }
	std::string FileInportDirectory() { return m_sFileImportDirectory; }
	std::string FileImportTargetFile() { return m_sFileImportTargetFile; }
	std::string FileImportDefaultViewFile() { return m_sFileImportDefaultViewFile; }
	std::string FileImportViewFile() { return m_sFileImportViewFile; }
	std::chrono::system_clock::time_point FileImportCatchTime() { return m_tpFileImportCatchTime; }
	bool FileImportEnabled() { return Initialized() && !m_sFileImportTargetFile.empty() && m_bFileImportEnabled; }
	bool OutputInhibitValues(std::string value);
	bool DeviceDataReloadRequest();

	/// <summary>初期化済</summary>
	bool Initialized() { return m_bInitialized; }

	/// <summary>ハードウェアI/Fソケット生成</summary>
	/// <param name="sock"></param>
	/// <param name="address"></param>
	/// <returns></returns>
	inline bool CreateHwifSocket(SOCKET& sock, struct sockaddr_in& address)
	{ return HwifEnabled() ? utilities::CreateSocketHandle(m_sHwifIpAddr, m_nHwifPort, sock, address) : false; }
	/// <summary>NMOS-Ember プロバイダソケット生成</summary>
	/// <param name="sock"></param>
	/// <param name="address"></param>
	/// <returns></returns>
	inline bool CreateNmosEmberSocket(SOCKET& sock, struct sockaddr_in& address)
	{ return NmosEmberEnabled() ? utilities::CreateSocketHandle(m_sNmosEmberIpAddr, m_nNmosEmberPort, sock, address) : false; }
	/// <summary>MV-Ember プロバイダソケット生成</summary>
	/// <param name="sock"></param>
	/// <param name="address"></param>
	/// <returns></returns>
	inline bool CreateMvEmberSocket(SOCKET& sock, struct sockaddr_in& address)
	{ return MvEmberEnabled() ? utilities::CreateSocketHandle(m_sMvEmberIpAddr, m_nMvEmberPort, sock, address) : false; }

	/// <summary>
	/// インスタンス取得
	/// </summary>
	/// <returns></returns>
	static CClientConfig* GetInstance();


	void UpdateMV(std::string ad, unsigned port, bool enabled, bool matrixlabel) {
		m_sMvEmberIpAddr = ad;
		m_nMvEmberPort = port;
		m_bMvEmberEnabled = enabled;
		m_bMvEmberUseMatrixLabels = matrixlabel;
	}
	void UpdateNMOS(std::string ad, unsigned port, bool enabled, bool matrixlabel) {
		m_sNmosEmberIpAddr = ad;
		m_nNmosEmberPort = port;
		m_bNmosEmberEnabled = enabled;
		m_bNmosEmberUseMatrixLabels = matrixlabel;
	}
	bool m_bMuteAll;
private:
	/// <summary>
	/// 古いログファイル削除
	/// </summary>
	/// <returns></returns>
	/// <remarks>
	/// もし古いログファイルがあれば
	/// 当日および前日までで直近1つのみ残して削除
	/// </remarks>
	bool DeleteOlderLogFiles();

	/// <summary>
	/// メンバ初期化
	/// </summary>
	void Initialize();

	std::string m_sPath;
	std::vector<std::string> m_vConfLines;

	std::string m_sDefaultDeviceContentsPath;
	std::string m_sStartupDeviceContentsPath;
	std::string m_sInhibitValuesPath;

	std::string m_sOutputLogDirectory;
	std::string m_sLogFilePathFormat;
	bool m_bOutputTrace;
	bool m_bLogFileEnabled;
	bool m_bMultibyteFormat;
	bool m_bDebugOmitSetLEDStatus;
	bool m_bDebugOmitSetOLEDStatus;

	unsigned int m_nSocketReconnectDelay;
	unsigned int m_nMainThreadDelay;
	unsigned int m_nEmberThreadDelay;

	std::string m_sHwifIpAddr;
	unsigned short m_nHwifPort;
	bool m_bHwifEnabled;

	std::string m_sNmosEmberIpAddr;
	unsigned short m_nNmosEmberPort;
	bool m_bNmosEmberEnabled;
	bool m_bNmosEmberUseMatrixLabels;

	std::string m_sMvEmberIpAddr;
	unsigned short m_nMvEmberPort;
	bool m_bMvEmberEnabled;
	bool m_bMvEmberUseMatrixLabels;

	std::string m_sFileImportBackupDirectory;
	std::string m_sFileImportDirectory;
	std::string m_sFileImportTargetFile;
	std::string m_sFileImportDefaultViewFile;
	std::string m_sFileImportViewFile;
	std::chrono::system_clock::time_point m_tpFileImportCatchTime;
	bool m_bFileImportEnabled;

	bool m_bInitialized;

	/// <summary>
	/// 単一インスタンス（宣言）
	/// </summary>
	static CClientConfig* m_pInstance;
};
