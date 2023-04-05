#pragma once

#include "DeviceContents.h"
#include <chrono>


// ====================================================================

/// <summary>操作識別</summary>
enum class ActionId : uint8_t
{
	/// <summary>なし</summary>
	ACTION_NONE = 0,

	/// <summary>操作通知</summary>
	ACTION_DONE,
	/// <summary>アラーム</summary>
	ACTION_ALARM,
	/// <summary>エラー通知</summary>
	ACTION_ERROR,

	/// <summary>操作識別</summary>
	ACTION_COUNT,
	/// <summary>不定</summary>
	ACTION_UNKNOWN = INT8_MAX,
};

/// <summary>
/// 各ボタン状態
/// </summary>
/// <remarks>
/// デバイス情報で扱わない、操作や状態 ※インヒビットはデバイス情報に反映する必要あり
/// [0]グループ [1]ページ [2]インヒビット [3]色 [4]状態 [5]ユーザ選択 [6]..[7]予備
/// </remarks>
class ButtonStatus
{
public:

	/// <summary>コンストラクタ</summary>
	ButtonStatus()
	{
		Initialize();
	}

	void Initialize()
	{
		m_nButtonIndex = -1;
		m_nGroup = -1;
		m_nPage = -1;
		m_bInhibit = 0;
		m_nLedColor = 0;

		if (!m_sDisplay.empty())
			m_sDisplay.clear();
#ifdef _OMIT_OLED_DETAIL
		m_nOLedColor = 0;
#else
		m_cOLedStatusUpperHead = '0';
		m_nOLedColorUpperHead = 0;
		m_cOLedStatusUpperTail = '0';
		m_nOLedColorUpperTail = 0;
		m_cOLedStatusLowerHead = '0';
		m_nOLedColorLowerHead = 0;
		m_cOLedStatusLowerTail = '0';
		m_nOLedColorLowerTail = 0;
#endif
		m_bOLedArrow = 0;
		m_nOLedImage = 0;

		m_eFunctionId = FunctionId::FUNC_NONE;
		m_nConnSignal = -1;
		m_nConnCount = 0;
		m_bCanUseXpt = false;

		m_cMode = '0';
		m_bPushed = 0;
		m_bLongPushed = 0;
		m_bSelected = 0;
		m_bActionError = 0;

		m_tPassingTime = std::chrono::system_clock::time_point{};
	}
	void Initialize(char group, char page)
	{
		Initialize();
		m_nGroup = group;
		m_nPage = page;
	}

	/// <summary>ボタンインデックス</summary>
	char m_nButtonIndex;
	/// <summary>グループ</summary>
	char m_nGroup;
	/// <summary>ページ</summary>
	char m_nPage;
	/// <summary>インヒビット</summary>
	char m_bInhibit;
	/// <summary>LED色</summary>
	char m_nLedColor;

	/// <summary>表示文字列</summary>
	std::string m_sDisplay;
#ifdef _OMIT_OLED_DETAIL
	/// <summary>OLED色</summary>
	char m_nOLedColor;
#else
	/// <summary>OLED 状態(1段目 表, 0:通常, 1:反転</summary>
	char m_cOLedStatusUpperHead;
	/// <summary>OLED 色(1段目 表, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorUpperHead;
	/// <summary>OLED 状態(1段目 裏, 0:通常, 1:反転</summary>
	char m_cOLedStatusUpperTail;
	/// <summary>OLED 色(1段目 裏, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorUpperTail;
	/// <summary>OLED 状態(2段目 表, 0:通常, 1:反転</summary>
	char m_cOLedStatusLowerHead;
	/// <summary>OLED 色(2段目 表, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorLowerHead;
	/// <summary>OLED 状態(2段目 裏, 0:通常, 1:反転</summary>
	char m_cOLedStatusLowerTail;
	/// <summary>OLED 色(2段目 裏, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorLowerTail;
#endif
	/// <summary>OLED 矢印表示有無(文字表示時のみ)</summary>
	char m_bOLedArrow;
	/// <summary>OLED 表示画像(当面未対応／対応後、指定時は文字列表示無効、フルパスorリソース番号指定)</summary>
	char m_nOLedImage;

	/// <summary>機能識別</summary>
	FunctionId m_eFunctionId;
	/// <summary>XPT 先頭シグナル（DEST/SRC時のみ）</summary>
	int m_nConnSignal;
	/// <summary>XPT 接続端子数（DEST/SRC時のみ）</summary>
	int m_nConnCount;
	/// <summary>XPT 操作可否（TAKE/DEST/SRC時のみ）</summary>
	bool m_bCanUseXpt;

	/// <summary>状態</summary>
	char m_cMode;
	/// <summary>押</summary>
	char m_bPushed;
	/// <summary>長押</summary>
	char m_bLongPushed;
	/// <summary>ユーザ選択</summary>
	char m_bSelected;
	/// <summary>ユーザ選択</summary>
	char m_bActionError;

	/// <summary>長押判断時刻</summary>
	std::chrono::system_clock::time_point m_tPassingTime;
};

/// <summary>インヒビット設定値</summary>
/// <remarks>
/// 設定開始～終了までの間にページ替発生の可能性がある
/// </remarks>
class InhibitValue
{
public:

	/// <summary>コンストラクタ</summary>
	InhibitValue()
	{
		Initialize();
	}
	/// <summary>コンストラクタ</summary>
	/// <param name="status"></param>
	InhibitValue(const ButtonStatus& status)
	{
		Initialize(status);
	}

	/// <summary>初期化</summary>
	void Initialize()
	{
		m_nButtonIndex = -1;
		m_nGroup = -1;
		m_nPage = -1;
		m_bInhibit = false;
	}
	/// <summary>初期化</summary>
	/// <param name="status"></param>
	void Initialize(const ButtonStatus& status)
	{
		m_nButtonIndex = status.m_nButtonIndex;
		m_nGroup = status.m_nGroup;
		m_nPage = status.m_nPage;
		m_bInhibit = status.m_bInhibit ? 0 : 1;
	}
	/// <summary>データ一致</summary>
	/// <param name="status"></param>
	/// <returns></returns>
	bool IsSame(const ButtonStatus& status)
	{
		return ((m_nButtonIndex == status.m_nButtonIndex)
			&& (m_nGroup == status.m_nGroup)
			&& (m_nGroup == status.m_nPage));
	}

	/// <summary>ボタンインデックス</summary>
	char m_nButtonIndex;
	/// <summary>グループ</summary>
	char m_nGroup;
	/// <summary>ページ</summary>
	char m_nPage;

	/// <summary>更新インヒビット</summary>
	/// <remarks>
	/// 更新要求値
	/// ∴DeviceContents保持の内容とは逆となるはず
	/// </remarks>
	char m_bInhibit;
};

/// <summary>XPT 設定値</summary>
class XptValue
{
public:
	/// <summary>コンストラクタ</summary>
	XptValue()
	{
		Initialize();
	}

	/// <summary>初期化</summary>
	void Initialize()
	{
		m_nDestConnSignal = -1;
		m_nDestConnCount = 0;
		m_nSrcConnSignal = -1;
		m_nSrcConnCount = 0;
	}
	/// <summary>設定</summary>
	/// <param name="status"></param>
	void Set(ButtonStatus& status)
	{
		if (status.m_bCanUseXpt)
		{
			if (status.m_eFunctionId == FunctionId::FUNC_DEST)
			{
				m_nDestConnSignal = status.m_nConnSignal;
				m_nDestConnCount = status.m_nConnCount;
			}
			else if (status.m_eFunctionId == FunctionId::FUNC_SRC)
			{
				m_nSrcConnSignal = status.m_nConnSignal;
				m_nSrcConnCount = status.m_nConnCount;
			}
		}
	}
	/// <summary>設定有無</summary>
	/// <returns></returns>
	bool IsSet()
	{
		return (m_nDestConnSignal >= 0) || (m_nDestConnCount > 0)
			|| (m_nSrcConnSignal >= 0) || (m_nSrcConnCount > 0);
	}
	/// <summary>有無効</summary>
	/// <returns></returns>
	bool IsValid()
	{
		return (m_nDestConnSignal >= 0) && (m_nDestConnCount > 0)
			&& (m_nSrcConnSignal >= 0) && (m_nSrcConnCount > 0);
	}
	/// <summary>データ一致</summary>
	/// <param name="status"></param>
	/// <returns></returns>
	bool IsSame(const ButtonStatus& status)
	{
		return ((status.m_eFunctionId == FunctionId::FUNC_DEST)
			&& (m_nDestConnSignal == status.m_nConnSignal)
			&& (m_nDestConnCount == status.m_nConnCount))
			|| ((status.m_eFunctionId == FunctionId::FUNC_SRC)
				&& (m_nSrcConnSignal == status.m_nConnSignal)
				&& (m_nSrcConnCount == status.m_nConnCount));
	}

	/// <summary>DEST 先頭シグナル</summary>
	int m_nDestConnSignal;
	/// <summary>DEST 接続端子数</summary>
	int m_nDestConnCount;
	/// <summary>SRC 先頭シグナル</summary>
	int m_nSrcConnSignal;
	/// <summary>SRC 接続端子数</summary>
	int m_nSrcConnCount;
};


// ====================================================================

/// <summary>Lock All</summary>
extern bool IsLockAll();
/// <summary>Lock Other</summary>
extern bool IsLockOther();
/// <summary>Lock Local</summary>
extern bool IsLockLocal();
/// <summary>Lock Any</summary>
/// <returns></returns>
inline bool IsLock() { return IsLockAll() || IsLockOther() || IsLockLocal(); }


// ====================================================================

/// <summary>
/// ボタン状態初期化
/// </summary>
extern void ClearButtonStatus();


// ====================================================================

/// <summary>マトリックス更新有無</summary>
/// <returns></returns>
extern bool IsUpdateMatrix();


// ====================================================================

/// <summary>インヒビット設定中是非</summary>
extern bool IsInhibitSetting();
/// <summary>インヒビット設定有無確認</summary>
/// <param name="status"></param>
/// <returns></returns>
extern bool ContainsInhibitValue(const ButtonStatus& status);


// ====================================================================

/// <summary>XPT マトリックスパス取得</summary>
/// <returns></returns>
extern std::string GetMatrixPath();
/// <summary>選択 Dext 接続 Src 取得</summary>
/// <param name="vDestConnSrcs"></param>
/// <returns></returns>
extern int GetDestConnSrcs(std::vector<int>& vDestConnSrcs);

/// <summary>XPT(DEST/SRC)設定可否</summary>
extern bool CanSetXpt();
/// <summary>XPT 設定実績取得</summary>
/// <param name="destSignal"></param>
/// <param name="srcSignal"></param>
/// <returns></returns>
extern int GetXptSignals(XptValue& value);


// ====================================================================

/// <summary>基本ステータス</summary>
extern struct structBASICSTATUS_ANS m_BasicStatus;
/// <summary>基本ステータス初期化</summary>
extern void ClearBasicStatus();
/// <summary>基本ステータス設定</summary>
/// <param name="ans">基本ステータス</param>
/// <returns>true</returns>
extern bool SetBasicStatus(const struct structBASICSTATUS_ANS& ans);

/// <summary>機種判定(18d)</summary>
inline bool IsButton18D() { return m_BasicStatus.kishu == (uint8_t)'0'; }
/// <summary>機種判定(39d)</summary>
inline bool IsButton39D() { return m_BasicStatus.kishu == (uint8_t)'1'; }
/// <summary>機種判定(40ru)</summary>
inline bool IsButton40RU() { return m_BasicStatus.kishu == (uint8_t)'2'; }
/// <summary>機種判定(有無効)</summary>
inline bool IsValidDevice() { return IsButton18D() || IsButton39D() || IsButton40RU(); }
/// <summary>
/// ボタン操作データ長
/// </summary>
/// <returns></returns>
inline size_t GetButtonActionDataLength() { return IsButton40RU() ? sizeof(structBUTTON_40RU)
												 : IsButton39D() ? sizeof(structBUTTON_39D)
												 : IsButton18D() ? sizeof(structBUTTON_18D)
												 : 0; }
/// <summary>
/// ボタン数
/// </summary>
/// <returns></returns>
inline int GetButtonCount() { return IsButton40RU() ? BTNCNT_40RU
								   : IsButton39D() ? BTNCNT_39D
								   : IsButton18D() ? BTNCNT_18D
								   : 0; }
/// <summary>
/// LEDボタン数
/// </summary>
/// <returns></returns>
inline int GetLEDButtonCount() { return IsButton39D() ? BTNCNT_39D_LED : GetButtonCount(); }
/// <summary>
/// OLED数
/// </summary>
/// <returns></returns>
inline int GetOLEDCount() { return IsButton39D() ? OLEDCNT_39D
								 : IsButton18D() ? OLEDCNT_18D
								 : 0; }
/// <summary>
/// ボタンIDオフセット
/// </summary>
/// <returns></returns>
inline int GetLEDButtonIdOffset() { return IsButton39D() ? LED_OFFSET_39D : 0; }

/// <summary>ステータス</summary>
extern struct structSTATUS_ANS m_LastStatus;
/// <summary>ステータス初期化</summary>
extern void ClearLastStatus();
/// <summary>ステータス設定</summary>
/// <param name="ans">ステータス</param>
/// <returns>true</returns>
extern bool SetLastStatus(const struct structSTATUS_ANS& ans);

/// <summary>ページ操作中</summary>
/// <returns></returns>
extern bool IsPageControlEnabled();
/// <summary>ページ操作対象グループ</summary>
/// <returns></returns>
extern std::vector<char>* GetPageControlGroups();

/// <summary>選択グループ取得</summary>
extern char GetCurrentGroup();
/// <summary>グループ内選択ページ取得</summary>
/// <param name="group"></param>
/// <returns></returns>
extern char GetCurrentGroupPage(char group);
/// <summary>選択グループ内選択ページ取得</summary>
extern char GetCurrentGroupPage();

/// <summary>
/// ボタン情報リセット
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <param name="initialize">
/// false : ページ替用、指定グループで割当済のボタンのみボタン情報を更新する
///  true : 初期化時用、割当済ボタン情報を更新しない
/// </param>
/// <returns></returns>
extern int ResetButtonStatus(const char group, const char page, bool initialize);

/// <summary>
/// ボタン状態取得
/// </summary>
/// <param name="index"></param>
/// <param name="status"></param>
/// <returns></returns>
extern bool GetButtonStatus(const int index, ButtonStatus& status);

/// <summary>ボタン状態解析</summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
extern bool AnalyzeButtonAction(const struct structBUTTON_18D& status, ActionId& doneAction);

/// <summary>ボタン状態解析</summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
extern bool AnalyzeButtonAction(const struct structBUTTON_39D& status, ActionId& doneAction);

/// <summary>ボタン状態解析</summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
extern bool AnalyzeButtonAction(const struct structBUTTON_40RU& status, ActionId& doneAction);

/// <summary>
/// 直近ボタン操作データ長
/// </summary>
/// <returns></returns>
extern void* GetLastButtonStatus();

/// <summary>
/// ロータリー操作解析
/// </summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
extern bool AnalyzeRotaryAction(const struct structROTARY& status, ActionId& doneAction);

/// <summary></summary>
/// <param name="path"></param>
extern void ClearDeviceStatus(std::string path);
/// <summary>装置全ステータス初期化</summary>
//inline void ClearDeviceStatus() { return ClearDeviceStatus(""); }

void getInfoFromDeviceContents(MvEmberInfo** o1, NMosEmberInfo** o3, MuteInfo** o4);