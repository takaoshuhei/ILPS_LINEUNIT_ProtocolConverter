#pragma once

#include "APIFormat.h"
#include "EmberConsumer.h"
#include "Utilities.h"

#define _OMIT_OLED_DETAIL

class NMosEmberInfo;
class MvEmberInfo;
class MuteInfo;
// ====================================================================

/// <summary>CSV カラム識別</summary>
enum class CsvColumnId : uint8_t
{
	/// <summary>ボタン（デバイス識別）</summary>
	COLUMN_BUTTON_ID = 0,
	/// <summary>グループ</summary>
	COLUMN_GROUP,
	/// <summary>ページ</summary>
	COLUMN_PAGE,
	/// <summary>ファンクション</summary>
	COLUMN_FUNCTION,
	/// <summary>表示文字列</summary>
	COLUMN_DISPLAY,
	/// <summary>引数-1</summary>
	COLUMN_ARG1,
	/// <summary>引数-2</summary>
	COLUMN_ARG2,
	/// <summary>LED デフォルト色</summary>
	COLUMN_LED_COLOR,

#ifdef _OMIT_OLED_DETAIL
	/// <summary>OLED デフォルト色</summary>
	COLUMN_OLED_COLOR,
#else
	/// <summary>OLED デフォルト状態(1段目 表, 0:通常, 1:反転)</summary>
	COLUMN_OLED_STATUS_UPPER_HEAD,
	/// <summary>OLED デフォルト色(1段目 表, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	COLUMN_OLED_COLOR_UPPER_HEAD,
	/// <summary>OLED デフォルト状態(1段目 裏, 0:通常, 1:反転)</summary>
	COLUMN_OLED_STATUS_UPPER_TAIL,
	/// <summary>OLED デフォルト色(1段目 裏, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	COLUMN_OLED_COLOR_UPPER_TAIL,
	/// <summary>OLED デフォルト状態(2段目 表, 0:通常, 1:反転)</summary>
	COLUMN_OLED_STATUS_LOWER_HEAD,
	/// <summary>OLED デフォルト色(2段目 表, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	COLUMN_OLED_COLOR_LOWER_HEAD,
	/// <summary>OLED デフォルト状態(2段目 裏, 0:通常, 1:反転)</summary>
	COLUMN_OLED_STATUS_LOWER_TAIL,
	/// <summary>OLED デフォルト色(2段目 裏, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	COLUMN_OLED_COLOR_LOWER_TAIL,
#endif
	/// <summary>OLED 矢印表示有無(文字表示時のみ)</summary>
	COLUMN_OLED_ARROW,
	/// <summary>OLED 表示画像(当面未対応／対応後、指定時は文字列表示無効、フルパスorリソース番号指定)</summary>
	COLUMN_OLED_IMAGE,

	/// <summary>インヒビットフラグ</summary>
	COLUMN_INHIBIT,
	/// <summary>コメント</summary>
	COLUMN_COMMENT,

	/// <summary>カラム数</summary>
	COLUMN_COUNT,
	/// <summary>不定</summary>
	COLUMN_UNKNOWN = INT8_MAX,

	//
	// for attribute
	//
	/// <summary>ファンクション</summary>
	COLUMN_ATTR_ID = 3,
	/// <summary>条件-1</summary>
	COLUMN_ATTR_VALUE1 = 4,
	/// <summary>条件-2</summary>
	COLUMN_ATTR_VALUE2 = 5,
	/// <summary>条件-3</summary>
	COLUMN_ATTR_VALUE3 = 6,
	/// <summary>コメント</summary>
	COLUMN_ATTR_COMMENT = 7,

};

/// <summary>機能識別</summary>
enum class FunctionId : uint8_t
{
	/// <summary>なし</summary>
	FUNC_NONE = 0,

	/// <summary>LockLocal</summary>
	FUNC_LOCKLOCAL,
	/// <summary>LockAll</summary>
	FUNC_LOCKALL,
	/// <summary>LockOther</summary>
	FUNC_LOCKOTHER,
	/// <summary>PageUp</summary>
	FUNC_PAGE_UP,
	/// <summary>PageDown</summary>
	FUNC_PAGE_DOWN,
	/// <summary>PageJump</summary>
	FUNC_PAGE_JUMP,
	/// <summary>ControlPage</summary>
	FUNC_PAGE_CONTROL,
	/// <summary>Src</summary>
	FUNC_SRC,
	/// <summary>Dest</summary>
	FUNC_DEST,
	/// <summary>Take</summary>
	FUNC_TAKE,
	/// <summary>Salvo</summary>
	/// <remarks>予約</remarks>
	FUNC_SALVO,
	/// <summary>EmberFunc(Invoke)</summary>
	FUNC_EMBER_FUNC,
	/// <summary>EmberValue(Parameter)</summary>
	FUNC_EMBER_VALUE,
	/// <summary>Inhibit</summary>
	FUNC_INHIBIT,

	/// <summary>機能識別数</summary>
	FUNC_COUNT,
	/// <summary>不定</summary>
	FUNC_UNKNOWN = INT8_MAX,
};

/// <summary>
/// 機能識別マップ
/// </summary>
extern const std::unordered_map<FunctionId, std::string> FunctionMap;
/// <summary>
/// 機能識別デフォルト文字列マップ
/// </summary>
extern const std::unordered_map<FunctionId, std::string> FunctionDefaultStringMap;

/// <summary>
/// 有効機能識別
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
extern bool IsValidFunctionId(FunctionId id);
/// <summary>
/// ページ操作対象機能
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
extern bool IsPageControlRequest(FunctionId id);
/// <summary>
/// Ember 対象機能
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
extern bool IsEmberRequest(FunctionId id);
/// <summary>
/// XPT 操作対象機能
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
extern bool IsXptRequest(FunctionId id);


// ====================================================================

/// <summary>1ボタン辺り指定可能 XPT 接続端子数最小値</summary>
#define CONN_COUNT_MIN	(1)
/// <summary>1ボタン辺り指定可能 XPT 接続端子数最大値</summary>
#define CONN_COUNT_MAX	(4)

/// <summary>動作属性識別：TAKE ボタン有無効</summary>
#define ATTR_TAKE_BUTTON_ENABLE	"TakeButtonEnable"

#define	ATTR_MVEMBER1	"MvEmber1"
#define	ATTR_MVEMBER2	"MvEmber2"
#define	ATTR_NMOSEMBER	"NMOSEmber"
#define	ATTR_MUTE		"Mute"

// ====================================================================

/// <summary>
/// ボタン（デバイス）設定要素
/// </summary>
/// <remarks>
/// ファイル内各行のグループ／ページカラム内容で抽出したもの
/// ∴内包定義各行のグループ／ページカラム内容は同一
/// </remarks>
class DeviceContent
{
public:
	/// <summary>コンストラクタ</summary>
	DeviceContent()
	{
		m_nButtonId = 0;
		m_nGroup = 0;
		m_nPage = 0;

		m_eFunctionId = FunctionId::FUNC_NONE;
		m_sDisplay = std::string();
		m_sArg1 = std::string();
		m_sArg2 = std::string();

		m_nLedColor = 0;

#ifdef _OMIT_OLED_DETAIL
		m_nOLedColor = 0;
#else
		m_nOLedStatusUpperHead = 0;
		m_nOLedColorUpperHead = 0;
		m_nOLedStatusUpperTail = 0;
		m_nOLedColorUpperTail = 0;
		m_nOLedStatusLowerHead = 0;
		m_nOLedColorLowerHead = 0;
		m_nOLedStatusLowerTail = 0;
		m_nOLedColorLowerTail = 0;
#endif
		m_bOLedArrow = false;
		m_nOLedImage = 0;

		m_bInhibit = true;
	}

	/// <summary>接続端子先頭シグナル</summary>
	int GetConnSignal()
	{
		int value = -1;
		if (((m_eFunctionId == FunctionId::FUNC_DEST)
		  || (m_eFunctionId == FunctionId::FUNC_SRC))
		 && utilities::ToNumber(m_sArg1, value))
			;
		else if (value != -1)
			value = -1;

		return value;
	}
	/// <summary>接続端子数</summary>
	/// <returns></returns>
	unsigned short GetConnCount()
	{
		unsigned short value = 0;
		if (((m_eFunctionId == FunctionId::FUNC_DEST)
		  || (m_eFunctionId == FunctionId::FUNC_SRC))
		 && utilities::ToNumber(m_sArg2, value)
		 && utilities::IsRange(value, (unsigned short)CONN_COUNT_MIN, (unsigned short)CONN_COUNT_MAX))
			;
		else if (value != 0)
			value = 0;

		return value;
	}

	/// <summary>TAKE 有効</summary>
	/// <returns></returns>
	bool IsValidTake()
	{
		return (m_eFunctionId == FunctionId::FUNC_TAKE)
			&& (m_sArg1.size() >= 4)
			&& (m_sArg1[0] == '/');
	}
	/// <summary>DEST 有効</summary>
	/// <returns></returns>
	bool IsValidDest()
	{
		return (m_eFunctionId == FunctionId::FUNC_DEST)
			&& (GetConnCount() > 0)
			&& (GetConnSignal() >= 0);
	}
	/// <summary>SRC 有効</summary>
	/// <returns></returns>
	bool IsValidSource()
	{
		return (m_eFunctionId == FunctionId::FUNC_SRC)
			&& (GetConnCount() > 0)
			&& (GetConnSignal() >= 0);
	}
	/// <summary>XPT 操作要素是非</summary>
	/// <returns></returns>
	bool IsXptContent() { return IsValidTake() || IsValidDest() || IsValidSource(); }

	/// <summary>操作対象グループ</summary>
	/// <returns></returns>
	std::vector<char>* ControlGroups()
	{
		if (!IsPageControlRequest(m_eFunctionId))
			return nullptr;

		std::vector<char>* pGroups = new std::vector<char>();
		std::string sGroups = m_sArg1;
		if (!sGroups.empty())
		{
			// 引数-1にパイプ区切でグループ番号が設定されている想定
			const char delimiter = '|';
			auto offset = std::string::size_type(0);
			bool isLast = false;
			while (!isLast)
			{
				try
				{
					std::string sPage = "";
					auto pos = sGroups.find(delimiter, offset);

					if (pos == std::string::npos)
					{
						sPage = sGroups.substr(offset);
						isLast = true;
					}
					else
					{
						sPage = sGroups.substr(offset, pos - offset);
						offset = pos + 1;
					}

					int nGroup = 0;
					if (!sPage.empty() && utilities::ToNumber(sPage, nGroup) && utilities::IsRange(nGroup, -1, CHAR_MAX))
					{
						// -1==全グループ対象
						if (nGroup == -1)
						{
							pGroups->clear();
							break;
						}

						// 0==ページ替対象外グループ
						if (nGroup == 0)
							continue;

						// 既存グループなら何もしない
						if (!pGroups->empty())
						{
							auto itr = std::find_if(pGroups->begin(), pGroups->end(),
													[nGroup](char number) { return (int)number == nGroup; });
							if (itr != pGroups->end())
								continue;
						}

						// グループ追加
						pGroups->push_back((char)nGroup);
					}
				}
#if true
				catch (...) { break; }
#else
				catch (const std::exception ex)
				{
					utilities::ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
					break;
				}
#endif
			}
		}

		if (!pGroups->empty())
		{
			// 単純昇順ソートしておく
			std::sort(pGroups->begin(), pGroups->end());
		}
		else
		{
			//delete pGroups;
			//pGroups = nullptr;

			// 空なら全対象とする
			pGroups->push_back(-1);
		}

		return pGroups;
	}
	/// <summary>操作対象ページ</summary>
	/// <returns></returns>
	char ControlPage()
	{
		int nPage = -1;
		if (m_eFunctionId != FunctionId::FUNC_PAGE_JUMP)
			return nPage;

		std::string sPage = m_sArg2;
		if (!sPage.empty())
		{
			// 引数-2にページ番号が設定されている想定
			if (!sPage.empty() && utilities::ToNumber(sPage, nPage) && utilities::IsRange(nPage, -1, CHAR_MAX))
				;
			else if (nPage != -1)
				nPage = -1;
		}

		return (char)nPage;
	}

	/// <summary>ボタン（デバイス識別）</summary>
	int m_nButtonId;
	/// <summary>グループ</summary>
	int m_nGroup;
	/// <summary>ページ</summary>
	int m_nPage;

	/// <summary>機能識別</summary>
	FunctionId m_eFunctionId;
	/// <summary>表示文字列</summary>
	std::string m_sDisplay;
	/// <summary>引数-1</summary>
	std::string m_sArg1;
	/// <summary>引数-2</summary>
	std::string m_sArg2;

	/// <summary>LED ボタンデフォルト色</summary>
	/// <remarks>
	/// BUTTONLED_COLOR 範疇
	/// </remarks>
	uint8_t m_nLedColor;

#ifdef _OMIT_OLED_DETAIL
	/// <summary>OLED ボタンデフォルト色</summary>
	/// <remarks>
	/// OLED_COLOR 範疇
	/// </remarks>
	uint8_t m_nOLedColor;
#else
	/// <summary>OLED デフォルト状態(1段目 表, 0:通常, 1:反転)</summary>
	uint8_t m_nOLedStatusUpperHead;
	/// <summary>OLED デフォルト色(1段目 表, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorUpperHead;
	/// <summary>OLED デフォルト状態(1段目 裏, 0:通常, 1:反転)</summary>
	uint8_t m_nOLedStatusUpperTail;
	/// <summary>OLED デフォルト色(1段目 裏, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorUpperTail;
	/// <summary>OLED デフォルト状態(2段目 表, 0:通常, 1:反転)</summary>
	uint8_t m_nOLedStatusLowerHead;
	/// <summary>OLED デフォルト色(2段目 表, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorLowerHead;
	/// <summary>OLED デフォルト状態(2段目 裏, 0:通常, 1:反転)</summary>
	uint8_t m_nOLedStatusLowerTail;
	/// <summary>OLED デフォルト色(2段目 裏, 3桁RGB 000-377, R=0-3,G/B=0-7)</summary>
	int m_nOLedColorLowerTail;
#endif
	/// <summary>OLED 矢印表示有無(文字表示時のみ)</summary>
	bool m_bOLedArrow;
	/// <summary>OLED 表示画像(当面未対応／対応後、指定時は文字列表示無効、フルパスorリソース番号指定)</summary>
	uint8_t m_nOLedImage;

	/// <summary>インヒビットフラグ</summary>
	bool m_bInhibit;
};

/// <summary>
/// グループ／ページ括りCSVファイル定義内容
/// </summary>
class GroupPage
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	GroupPage();
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="group"></param>
	/// <param name="page"></param>
	/// <param name="lines"></param>
	GroupPage(int group, int page, std::vector<std::vector<std::string>> lines);
	/// <summary>
	/// デストラクタ
	/// </summary>
	~GroupPage();
	/// <summary>
	/// ボタン（デバイス）設定要素取得
	/// </summary>
	/// <param name="columns"></param>
	/// <returns></returns>
	DeviceContent* GetContent(const std::vector<std::string>& columns);
	/// <summary>
	/// ボタン（デバイス）設定要素取得
	/// </summary>
	/// <param name="contents"></param>
	/// <param name="buttonId"></param>
	/// <returns></returns>
	static DeviceContent* GetContent(const std::vector<DeviceContent*>& contents, int buttonId);
	/// <summary>
	/// ボタン（デバイス）設定要素取得
	/// </summary>
	/// <param name="buttonId"></param>
	/// <returns></returns>
	DeviceContent* GetContent(int buttonId);
	/// <summary>
	/// グループ／ページ内ボタン（デバイス）設定要素取得
	/// </summary>
	/// <param name="contents"></param>
	/// <returns></returns>
	int GetContents(std::vector<DeviceContent*>& contents);

	/// <summary>グループ</summary>
	/// <remarks>0:PageUp/Down 依存なし、1-10</remarks>
	int m_nGroup;
	/// <summary>ページ</summary>
	/// <remarks>0-99</remarks>
	int m_nPage;
	/// <summary>定義行</summary>
	/// <remarks>
	/// ファイル内各行のグループ／ページカラム内容で抽出したもの
	/// ∴内包定義各行のグループ／ページカラム内容は同一
	/// </remarks>
	std::vector<std::vector<std::string>> m_vLines;
};


// ====================================================================

/// <summary>グループ最大</summary>
/// <remarks>
/// 0:ページ替なし固定、1-GROUP_MAX
/// </remarks>
#define GROUP_MAX	(10)
/// <summary>ページ最大</summary>
/// <remarks>
/// 0-PAGE_MAX
/// </remarks>
#define PAGE_MAX	(99)


// ====================================================================

/// <summary>
/// CDeviceContents
/// デバイス情報クラス
/// </summary>
class CDeviceContents
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="path"></param>
	CDeviceContents(std::string path);
	/// <summary>
	/// デストラクタ
	/// </summary>
	~CDeviceContents();

	/// <summary>ファイルパス</summary>
	std::string Path() { return m_sPath; };

	/// <summary>使用可否</summary>
	bool Enabled() { return m_bInitialized && !m_vGroupPages.empty(); }

	/// <summary>TAKE ボタン操作可否</summary>
	bool TakeButtonEnable() { return m_bTakeButtonEnable || m_sMatrixPath.empty(); }
	/// <summary>マトリックスパス</summary>
	std::string MatrixPath() { return m_sMatrixPath; }
	/// <summary>ボタン情報内出現マトリックスパス</summary>
	std::string InnerMatrixPath() { return m_sInnerMatrixPath; }
	/// <summary>TAKE 定義有無</summary>
	bool HasTake() { return (m_nTakeCount > 0); }
	/// <summary>DEST 定義有無</summary>
	bool HasDest() { return (m_nDestCount > 0); }
	/// <summary>SRC 定義有無</summary>
	bool HasSource() { return (m_nSourceCount > 0); }
	/// <summary>XPT 操作可否</summary>
	bool CanXptRequest() { return Enabled()
							   && (!TakeButtonEnable() || HasTake())
							   && HasDest()
							   && HasSource(); }

	/// <summary>初期化済</summary>
	bool Initialized() { return m_bInitialized; } 

	/// <summary>
	/// グループ／ページ内ボタン（デバイス）設定要素
	/// </summary>
	/// <param name="group"></param>
	/// <param name="page"></param>
	/// <param name="contents"></param>
	/// <returns></returns>
	int GetGroupPageContents(const int group, const int page, std::vector<DeviceContent*>& contents);
	
	/// <summary>
	/// インヒビット更新
	/// </summary>
	/// <param name="group"></param>
	/// <param name="page"></param>
	/// <param name="buttonId"></param>
	/// <param name="inhibit"></param>
	/// <returns></returns>
	bool UpdateInhibit(const int group, const int page, const int buttonId, const bool inhibit);
	/// <summary>
	/// 書き出し用インヒビット情報
	/// </summary>
	/// <param name="ss"></param>
	int CreateOutputInhibits(std::stringstream& ss);

	NMosEmberInfo* m_pNMosEmberInfo;
	MvEmberInfo* m_pMvEmberInfo1;
	MuteInfo* m_pMuteAll;
private:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CDeviceContents();
	/// <summary>
	/// メンバ初期化
	/// </summary>
	void Initialize(bool setflg);
	/// <summary>
	/// メンバ初期化
	/// </summary>
	/// <param name="path"></param>
	void Initialize(std::string path);

	/// <summary>
	/// グループ／ページ内ボタン（デバイス）設定要素
	/// </summary>
	/// <param name="vGroupPage"></param>
	/// <param name="group"></param>
	/// <param name="page"></param>
	/// <param name="contents"></param>
	/// <returns></returns>
	GroupPage* GetGroupPage(std::vector<GroupPage*>& vGroupPage, const int group, const int page);

	/// <summary>ファイルパス</summary>
	std::string m_sPath;
	/// <summary>グループ／ページ別 CSV ファイル定義内容</summary>
	std::vector<GroupPage*> m_vGroupPages;

	/// <summary>TAKEボタン属性保有</summary>
	bool m_bHasTakeButtonEnable;
	/// <summary>TAKEボタン有無効</summary>
	bool m_bTakeButtonEnable;

	/// <summary>マトリックスパス</summary>
	std::string m_sMatrixPath;
	/// <summary>ボタン情報内出現マトリックスパス</summary>
	std::string m_sInnerMatrixPath;

	/// <summary>TAKE 定義数</summary>
	unsigned short m_nTakeCount;
	/// <summary>DEST 定義数</summary>
	unsigned short m_nDestCount;
	/// <summary>SOURCE 定義数</summary>
	unsigned short m_nSourceCount;

	/// <summary>初期化済</summary>
	bool m_bInitialized;
};
