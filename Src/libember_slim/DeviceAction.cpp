// TCPClient.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "DeviceAction.h"
#include "DeviceContents.h"
#include "Utilities.h"
#include <cassert>
#include <iterator>
#include <sstream>

using namespace utilities;


// ====================================================================

/// <summary>長押し判定時間(秒)</summary>
static const int64_t m_nLongPushDistSeconds = 3;

/// <summary>Lock All</summary>
/// <remarks>次期対応予定</remarks>
static bool m_bLockAll = false;
/// <summary>Lock All</summary>
bool IsLockAll() { return m_bLockAll; }
/// <summary>Lock Other</summary>
/// <remarks>次期対応予定</remarks>
static bool m_bLockOther = false;
/// <summary>Lock Other</summary>
bool IsLockOther() { return m_bLockOther; }
/// <summary>Lock Local</summary>
static bool m_bLockLocal = false;
/// <summary>Lock Local</summary>
bool IsLockLocal() { return m_bLockLocal; }


// ====================================================================

/// <summary>
/// 各ボタン状態
/// </summary>
/// <remarks>
/// デバイス情報で扱わない、操作や状態 ※インヒビットはデバイス情報に反映する必要あり
/// </remarks>
static ButtonStatus m_aButtonStatus[DEVCNT_MAX]{};
/// <summary>
/// ボタン状態初期化
/// </summary>
void ClearButtonStatus()
{
	for (int i = 0; i < DEVCNT_MAX; ++i)
		m_aButtonStatus[i].Initialize();
}

/// <summary>
/// ボタンインデックス妥当
/// </summary>
/// <param name="index"></param>
/// <returns></returns>
static bool IsValidButtonIndex(const int index)
{
	int cnt = GetButtonCount();
	return (cnt > 0) && IsRange(index, 0, cnt - 1);
}
/// <summary>
/// LEDボタンインデックス妥当
/// </summary>
/// <param name="index"></param>
/// <returns></returns>
static bool IsValidLEDButtonIndex(const int index)
{
	int cnt = GetLEDButtonCount();
	return (cnt > 0) && IsRange(index, 0, cnt - 1);
}


// ====================================================================

/// <summary>
/// XPT(DEST/SRC)設定可否
/// </summary>
/// <remarks>
/// false 時 TAKE ボタン操作で true に切り替え
///  true 時 TAKE ボタン操作で DEST/SRC 設定あれば   false に切り替え かつ マトリックス操作要求
///                            DEST/SRC 設定なければ false に切り替え
///          DEST/SRC/TAKE 以外のボタン操作で false に切り替え
/// </remarks>
static bool m_bCanSetXpt = false;
/// <summary>最終選択XPT</summary>
static XptValue m_sXptValue {};
/// <summary>XPT 設定実績</summary>
/// <remarks>
/// VECTOR で用意したが今用件では最終1組のみ有効とする
/// </remarks>
static std::vector<XptValue*> m_vXpts{};
/// <summary>選択 Dest 接続 Src</summary>
static std::vector<int> m_vDestConnSrcs{};
/// <summary>XPT マトリックスパス</summary>
static std::string m_sMatrixPath{};
/// <summary>XPT 設定用データ初期化</summary>
static void ClearXptValue()
{
	while (!m_vXpts.empty())
	{
		auto itr = m_vXpts.begin();
		if (!(*itr))
			delete (*itr);
		m_vXpts.erase(itr);
	}
	m_vDestConnSrcs.clear();
	if (!m_sMatrixPath.empty())
		m_sMatrixPath.clear();
	m_sXptValue.Initialize();
	m_bCanSetXpt = false;
}
/// <summary>XPT 設定用データ初期化</summary>
/// <param name="path"></param>
static void ClearXptValue(std::string path)
{
	ClearXptValue();
	if (!path.empty())
	{
		m_sMatrixPath = path;
		m_bCanSetXpt = true;
	}
}
/// <summary>XPT 設定用ソースデータ初期化</summary>
static void ClearXptSourcesValue()
{
	// 一旦すべて初期化しDest側は初期化前のデータを再設定する
	std::string sPath = m_sMatrixPath;
	XptValue value = m_sXptValue;
	ClearXptValue(sPath);
	m_sXptValue.m_nDestConnSignal = value.m_nDestConnSignal;
	m_sXptValue.m_nDestConnCount = value.m_nDestConnCount;
}

/// <summary>XPT マトリックスパス取得</summary>
/// <returns></returns>
std::string GetMatrixPath() { return m_sMatrixPath; }
/// <summary>選択 Dext 接続 Src 取得</summary>
/// <param name="vDestConnSrcs"></param>
/// <returns></returns>
int GetDestConnSrcs(std::vector<int>& vDestConnSrcs)
{
	vDestConnSrcs.clear();
	if (!m_vDestConnSrcs.empty())
	{
		std::copy(m_vDestConnSrcs.cbegin(), m_vDestConnSrcs.cend(), std::back_inserter(vDestConnSrcs));
	}

	return (int)vDestConnSrcs.size();
}

/// <summary>XPT(DEST/SRC)設定可否</summary>
bool CanSetXpt() { return m_bCanSetXpt; }
/// <summary>XPT 設定実績取得</summary>
/// <param name="value"></param>
/// <returns></returns>
int GetXptSignals(XptValue& value)
{
	value.Initialize();
	int cnt = 0;

	// ポインタは参照のみで解放しない
	XptValue* buf = nullptr;
	if (m_bCanSetXpt)
	{
		if (!m_vXpts.empty()
		 && ((buf = *m_vXpts.begin()) != nullptr))
		{
			memcpy(&value, buf, sizeof(XptValue));
			cnt = 1;
		}
		else
		{
			// 設定実績がない場合最終押下を返す
			memcpy(&value, &m_sXptValue, sizeof(XptValue));
		}
	}
	return cnt;
}


// ====================================================================

/// <summary>基本ステータス</summary>
struct structBASICSTATUS_ANS m_BasicStatus{};
/// <summary>基本ステータス初期化</summary>
void ClearBasicStatus()
{
	memset(&m_BasicStatus, 0, sizeof(struct structBASICSTATUS_ANS));
}
/// <summary>基本ステータス設定</summary>
/// <param name="ans">基本ステータス</param>
/// <returns>true</returns>
bool SetBasicStatus(const struct structBASICSTATUS_ANS& ans)
{
	memcpy(&m_BasicStatus, &ans, sizeof(struct structBASICSTATUS_ANS));
	return true;
}

/// <summary>ステータス</summary>
struct structSTATUS_ANS m_LastStatus{};
/// <summary>ステータス初期化</summary>
void ClearLastStatus()
{
	memset(&m_LastStatus, 0, sizeof(struct structSTATUS_ANS));
}
/// <summary>ステータス設定</summary>
/// <param name="ans">ステータス</param>
/// <returns>true</returns>
bool SetLastStatus(const struct structSTATUS_ANS& ans)
{
	memcpy(&m_LastStatus, &ans, sizeof(struct structSTATUS_ANS));
	return true;
}

/// <summary>デバイス情報</summary>
static CDeviceContents* m_pDeviceContents = nullptr;
/// <summary>デバイス情報有効</summary>
/// <returns></returns>
static bool IsValidDeviceContents()
{
	return m_pDeviceContents && m_pDeviceContents->Enabled();
}
/// <summary>
/// デバイス情報破棄
/// </summary>
static void DisposeDeviceContents()
{
	if (m_pDeviceContents)
	{
		delete m_pDeviceContents;
		m_pDeviceContents = nullptr;
	}
}

/// <summary>グループ／ページデバイス情報</summary>
static std::vector<DeviceContent*>* m_pGroupPageContents[GROUP_MAX + 1][PAGE_MAX + 1] = { 0 };
/// <summary>
/// グループ妥当
/// </summary>
/// <param name="group"></param>
/// <returns></returns>
static bool IsValidGroup(const char group)
{
	return IsRange<char>(group, 0, GROUP_MAX);
}
/// <summary>
/// ページ妥当
/// </summary>
/// <param name="page"></param>
/// <returns></returns>
static bool IsValidPage(const char page)
{
	return IsRange<char>(page, 0, PAGE_MAX);
}
/// <summary>
/// グループ／ページ妥当
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <returns></returns>
static bool IsValidGroupPage(const char group, const char page)
{
	return IsValidGroup(group) && IsValidPage(page);
}
/// <summary>
/// グループ／ページデバイス情報破棄
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <returns></returns>
static void DisposeGroupPageContent(const char group, const char page)
{
	if (!IsValidDeviceContents() || !IsValidGroupPage(group, page))
		return;

	// 既存
	if (m_pGroupPageContents[group][page])
	{
		delete m_pGroupPageContents[group][page];
		m_pGroupPageContents[group][page] = nullptr;
	}
}
/// <summary>
/// グループ／ページデバイス情報破棄
/// </summary>
static void DisposeGroupPageContents()
{
	for (int g = 0; g <= GROUP_MAX; ++g)
		for (int p = 0; p <= PAGE_MAX; ++p)
			DisposeGroupPageContent(g, p);
}

/// <summary>XPT ラベル設定</summary>
/// <param name="content"></param>
/// <returns></returns>
/// <remarks>
/// マトリックス側にラベル情報を持っている場合優先して表示に使用する
/// </remarks>
static bool SetXptLabel(DeviceContent& content)
{
	bool bUpdate = false;
	std::string sMatrixPath = m_pDeviceContents->TakeButtonEnable()
							? m_pDeviceContents->InnerMatrixPath()
							: m_pDeviceContents->MatrixPath();
	if (sMatrixPath.empty()
	 || !m_pNmosEmberConsumer->EnableMatrixLabels()
	 || ((content.m_eFunctionId != FunctionId::FUNC_DEST)
	  && (content.m_eFunctionId != FunctionId::FUNC_SRC)))
		return bUpdate;

	std::string sLabel = "";
	berint nSignalNumber = (berint)content.GetConnSignal();
	if (content.m_eFunctionId == FunctionId::FUNC_DEST)
	{
		if (m_pNmosEmberConsumer->GetTargetLabel(sMatrixPath, nSignalNumber, sLabel)
		 && !sLabel.empty())
		{
			content.m_sDisplay = sLabel;
			bUpdate = true;
		}
	}
	else// if (content.m_eFunctionId == FunctionId::FUNC_SRC)
	{
		if (m_pNmosEmberConsumer->GetSourceLabel(sMatrixPath, nSignalNumber, sLabel)
			&& !sLabel.empty())
		{
			content.m_sDisplay = sLabel;
			bUpdate = true;
		}
	}

	return bUpdate;
}
/// <summary>XPT ラベル設定</summary>
/// <param name="status"></param>
/// <returns></returns>
/// <remarks>
/// マトリックス側にラベル情報を持っている場合優先して表示に使用する
/// </remarks>
static bool SetXptLabel(ButtonStatus& status)
{
	bool bUpdate = false;
	std::string sMatrixPath = m_pDeviceContents->TakeButtonEnable()
							? m_pDeviceContents->InnerMatrixPath()
							: m_pDeviceContents->MatrixPath();
	if (sMatrixPath.empty()
	 || !m_pNmosEmberConsumer->EnableMatrixLabels()
	 || ((status.m_eFunctionId != FunctionId::FUNC_DEST)
	  && (status.m_eFunctionId != FunctionId::FUNC_SRC)))
		return bUpdate;

	std::string sLabel = "";
	berint nSignalNumber = (berint)status.m_nConnSignal;
	if (status.m_eFunctionId == FunctionId::FUNC_DEST)
	{
		if (m_pNmosEmberConsumer->GetTargetLabel(sMatrixPath, nSignalNumber, sLabel)
		 && !sLabel.empty())
		{
			status.m_sDisplay = sLabel;
			bUpdate = true;
		}
	}
	else// if (status.m_eFunctionId == FunctionId::FUNC_SRC)
	{
		if (m_pNmosEmberConsumer->GetSourceLabel(sMatrixPath, nSignalNumber, sLabel)
			&& !sLabel.empty())
		{
			status.m_sDisplay = sLabel;
			bUpdate = true;
		}
	}

	return bUpdate;
}

/// <summary>
/// グループ／ページデバイス情報取得
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <returns></returns>
static std::vector<DeviceContent*>* LoadGroupPageContents(const char group, const char page)
{
	if (!IsValidDeviceContents() || !IsValidGroupPage(group, page))
		return nullptr;

	// 既存
	std::vector<DeviceContent*> conts{};
	if (m_pGroupPageContents[group][page])
		return m_pGroupPageContents[group][page];

	if (m_pDeviceContents->GetGroupPageContents(group, page, conts))
	{
		auto pConts = new std::vector<DeviceContent*>();
		pConts->swap(conts);
		m_pGroupPageContents[group][page] = pConts;
	}
	return m_pGroupPageContents[group][page];
}

/// <summary>直近マトリックス操作通知回数</summary>
static int m_nLastMatrixNotifyCount = 0;
/// <summary>マトリックス更新有無</summary>
/// <returns></returns>
bool IsUpdateMatrix()
{
	bool res = false;

	int nLastMatrixNotifyCount = m_pNmosEmberConsumer->MatrixNoticeCount();
	if (m_nLastMatrixNotifyCount != nLastMatrixNotifyCount)
	{
		m_nLastMatrixNotifyCount = nLastMatrixNotifyCount;

		// シグナルを控え直す
		m_pNmosEmberConsumer->GetSignalValues(m_sXptValue.m_nDestConnSignal, m_sXptValue.m_nDestConnCount, m_vDestConnSrcs);

		// 現行配置のみ情報更新
		if (m_pNmosEmberConsumer->EnableMatrixLabels())
		{
			int cnt = 0;
			for (int i = 0; i < DEVCNT_MAX; ++i)
			{
				if ((m_aButtonStatus[i].m_eFunctionId != FunctionId::FUNC_DEST)
				 && (m_aButtonStatus[i].m_eFunctionId != FunctionId::FUNC_SRC))
					continue;
				if (SetXptLabel(m_aButtonStatus[i]))
					++cnt;
			}
		}

		res = true;
	}

	return res;
}

/// <summary>
/// インヒビット設定中
/// </summary>
/// <remarks>
/// false 時 INHIBIT ボタン操作で true に切り替え
///  true 時 INHIBIT ボタン操作で false に切り替え かつ
///                               変更があれば startup.csv 更新
/// </remarks>
static bool m_bInhibitSetting = false;
/// <summary>インヒビット 設定実績</summary>
/// <remarks>
/// 設定開始～終了までの間にページ替発生の可能性があり
/// m_aButtonStatus が持つ表示用の m_bInhibit では管理不十分となるため設置
/// </remarks>
static std::vector<InhibitValue*> m_vInhibits{};
/// <summary>インヒビット 設定用データ初期化/// </summary>
static void ClearInhibitValue()
{
	m_vInhibits.clear();
	m_bInhibitSetting = false;
}
/// <summary>インヒビット 切替設定</summary>
/// <param name="status"></param>
/// <returns></returns>
static int AddOrRemoveInhibitValue(const ButtonStatus& status)
{
	int res = 0;
	if ((status.m_eFunctionId == FunctionId::FUNC_INHIBIT)
	 || !IsValidFunctionId(status.m_eFunctionId)
	 || !IsRange<char>(status.m_nButtonIndex, 0, DEVCNT_MAX - 1))
		return res;

	// 設定実績がある場合は検索
	if (!m_vInhibits.empty())
	{
		try
		{
			// ContainsInhibitValue() も使えるが削除する場合二度手間になる
			auto itr = std::find_if(m_vInhibits.begin(), m_vInhibits.end(),
									[status](InhibitValue* p)
									{ return p && p->IsSame(status); });
			// 該当ボタンがあれば削除
			if (itr != std::end(m_vInhibits))
			{
				m_vInhibits.erase(itr);
				res = -1;
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			res = 0;
		}
	}
	// 削除していないならば追加
	if (res == 0)
	{
		try
		{
			auto pInh = new InhibitValue(status);
			m_vInhibits.push_back(pInh);
			res = 1;
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			res = 0;
		}
	}

	// 変化あり
	if (res != 0)
	{
		// 対象ボタンのインヒビット設定を反転
		char nInh = m_aButtonStatus[status.m_nButtonIndex].m_bInhibit;
		m_aButtonStatus[status.m_nButtonIndex].m_bInhibit = (nInh == 0) ? 1 : 0;
	}

	return res;
}
static int UpdateInhivitValue()
{
	int cnt = 0;
	if (m_vInhibits.empty())
		return cnt;

	// ButtonStatus の元、m_aButtonStatus[bidx] 上のデータは更新済
	// そのまま1件ずつ扱うと効率悪いので、グループページごとに操作する
	for (int g = 0; g <= GROUP_MAX; ++g)
	{
		for (int p = 0; p <= PAGE_MAX; ++p)
		{
			std::vector<InhibitValue*> _vInhibits{};
			std::copy_if(m_vInhibits.cbegin(), m_vInhibits.cend(),
						 std::back_inserter(_vInhibits), [p, g](InhibitValue* pVal)
						 { return pVal->m_nGroup == g && pVal->m_nPage == p; });
			if (_vInhibits.empty())
				continue;

			// 表示用にメモリ展開されているデータ
			DeviceContent* pCont;
			auto pLoadedConts = LoadGroupPageContents(g, p);
			for (auto itr : _vInhibits)
			{
				auto inhval = *itr;
				pCont = GroupPage::GetContent(*pLoadedConts, inhval.m_nButtonIndex + 1);
				if (pCont)
				{
					pCont->m_bInhibit = inhval.m_bInhibit;
					// ファイルから読んだデータも更新してもらう
					m_pDeviceContents->UpdateInhibit(g, p, inhval.m_nButtonIndex + 1, inhval.m_bInhibit);
					++cnt;
				}
			}
		}
	}

	// ファイルに書き出してもらう
	if (cnt > 0)
	{
		std::stringstream ss{};
		m_pDeviceContents->CreateOutputInhibits(ss);
		CClientConfig::GetInstance()->OutputInhibitValues(ss.str());
	}
	else
	{
		CClientConfig::GetInstance()->OutputInhibitValues("");
	}
	return cnt;
}

/// <summary>インヒビット 設定中是非</summary>
bool IsInhibitSetting() { return m_bInhibitSetting; }
/// <summary>インヒビット 設定有無確認</summary>
/// <param name="buttonIndex"></param>
/// <returns></returns>
bool ContainsInhibitValue(const ButtonStatus& status)
{
	bool res = false;
	if (m_vInhibits.empty())
		return res;

	// 設定実績から対象キーを持つ内容を抽出
	try
	{
		auto itr = std::find_if(m_vInhibits.cbegin(), m_vInhibits.cend(),
								[status](InhibitValue* p)
								{ return p && p->IsSame(status); });
		res = (itr != std::cend(m_vInhibits));
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		res = false;
	}

	return res;
}

/// <summary>ページ操作中</summary>
static bool m_bPageControlEnabled = false;
/// <summary>ページ操作中時対象グループ</summary>
static std::vector<char>* m_pControlGroups = nullptr;
/// <summary>ページ操作中</summary>
/// <returns></returns>
bool IsPageControlEnabled() { return m_bPageControlEnabled; }
/// <summary>ページ操作中反転</summary>
/// <returns></returns>
static bool TogglePageControlEnabled()
{
	m_bPageControlEnabled = !m_bPageControlEnabled;
	return m_bPageControlEnabled;
}
/// <summary>ページ操作対象グループ設定</summary>
/// <param name="pGroups"></param>
/// <returns>pGroups</returns>
static std::vector<char>* SetPageControlGroups(std::vector<char>* pGroups)
{
	if (m_pControlGroups)
		delete m_pControlGroups;
	m_pControlGroups = pGroups;
	return m_pControlGroups;
}
/// <summary>ページ操作対象グループ</summary>
/// <returns></returns>
std::vector<char>* GetPageControlGroups()
{
	return (IsPageControlEnabled() && m_pControlGroups && !m_pControlGroups->empty())
		 ? m_pControlGroups
		 : nullptr;
}
/// <summary>ページ操作グループ対象確認</summary>
/// <param name="pGroups"></param>
/// <param name="group"></param>
/// <returns></returns>
static bool ContainsPageControlGroups(std::vector<char>* pGroups, char group)
{
	bool res = false;
	if (pGroups->empty())
		return res;

	// 一致するグループを抽出
	try
	{
		auto itr = std::find_if(pGroups->cbegin(), pGroups->cend(),
								[group](char number) { return number == group; });
		res = (itr != pGroups->cend());
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		res = false;
	}

	return res;
}

/// <summary>選択グループ</summary>
static char m_nCurrentGroup = -1;
/// <summary>選択ページ（全グループ分）</summary>
static char m_nCurrentPage[GROUP_MAX + 1] = { 0 };
/// <summary>選択グループページ初期化</summary>
/// <param name="group"></param>
/// <param name="group"></param>
static bool SetCurrentGroupPage(const char group, const char page)
{
	if (!IsValidGroup(group))
		return false;

	char _page = page;
	if ((page != -1) && !IsValidPage(_page))
		_page = -1;
	m_nCurrentPage[group] = _page;

	return true;
}
/// <summary>選択グループページ初期化</summary>
/// <param name="group"></param>
static void ClearCurrentGroupPage(const char group)
{
	SetCurrentGroupPage(group, -1);
}
/// <summary>選択グループページ初期化</summary>
static void ClearCurrentGroupPage()
{
	for (int g = 0; g <= GROUP_MAX; ++g)
		ClearCurrentGroupPage(g);
	m_nCurrentGroup = -1;
}
/// <summary>選択グループ取得</summary>
char GetCurrentGroup() { return m_nCurrentGroup; }
/// <summary>グループ内選択ページ取得</summary>
/// <param name="group"></param>
/// <returns></returns>
char GetCurrentGroupPage(char group) { return IsValidGroup(group) ? m_nCurrentPage[group] : -1; }
/// <summary>選択グループ内選択ページ取得</summary>
char GetCurrentGroupPage() { return GetCurrentGroupPage(m_nCurrentGroup); }

/// <summary>
/// ボタン情報設定
/// </summary>
/// <param name="status"></param>
/// <param name="content"></param>
/// <returns></returns>
static bool SetButtonStatus(ButtonStatus& status, DeviceContent& content)
{
	int nButtonIndex = content.m_nButtonId - 1;
	if (!IsValidButtonIndex(nButtonIndex))
	{
		status.Initialize();
		return false;
	}
	status.Initialize(content.m_nGroup, content.m_nPage);
	status.m_nButtonIndex = (char)nButtonIndex;

	status.m_bInhibit = content.m_bInhibit ? 1 : 0;
	//status.m_nLedColor = content.m_nLedColor % 10;
	status.m_nLedColor = content.m_nLedColor;

	if (!content.m_sDisplay.empty())
	{
		//status.m_sDisplay = std::string(content.m_sDisplay);
		status.m_sDisplay = content.m_sDisplay;
	}

#ifdef _OMIT_OLED_DETAIL
	//status.m_nOLedColor = content.m_nOLedColor % 100;
	status.m_nOLedColor = content.m_nOLedColor;
#else
	status.m_cOLedStatusUpperHead = '0' + content.m_nOLedStatusUpperHead;
	status.m_nOLedColorUpperHead = content.m_nOLedColorUpperHead;
	status.m_cOLedStatusUpperTail = '0' + content.m_nOLedStatusUpperTail;
	status.m_nOLedColorUpperTail = content.m_nOLedColorUpperTail;
	status.m_cOLedStatusLowerHead = '0' + content.m_nOLedStatusLowerHead;
	status.m_nOLedColorLowerHead = content.m_nOLedColorLowerHead;
	status.m_cOLedStatusLowerTail = '0' + content.m_nOLedStatusLowerTail;
	status.m_nOLedColorLowerTail = content.m_nOLedColorLowerTail;
#endif
	status.m_nOLedImage = content.m_nOLedImage;
	status.m_bOLedArrow = content.m_bOLedArrow ? 1 : 0;

	// XPT 操作に使えるデータかどうかは先に判断しておく
	status.m_eFunctionId = content.m_eFunctionId;
	status.m_nConnSignal = content.GetConnSignal();
	status.m_nConnCount = content.GetConnCount();
	status.m_bCanUseXpt = m_pDeviceContents->CanXptRequest()
					   && (content.IsValidTake()
						|| content.IsValidDest()
						|| content.IsValidSource());
	SetXptLabel(status);
	if ((status.m_eFunctionId == FunctionId::FUNC_TAKE)
	 && !m_pDeviceContents->TakeButtonEnable())
	{
		status.m_bInhibit = true;
	}

	return true;
}

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
int ResetButtonStatus(const char group, const char page, bool initialize)
{
	// 返却はボタン割当数
	int asnCnt = 0;
	// ボタン数
	int btnCnt = GetButtonCount();	// 39D時、041,042() は抜け番
	if (btnCnt <= 0)
		return asnCnt;

	// 設定候補
	auto pConts = LoadGroupPageContents(group, page);
	// LoadGroupContents ではデータがある場合のみ
	// m_pGroupPageContents 該当グループページ箇所にバッファポインタを設定している
	if (!pConts)
		return false;
	// 空ならバッファポインタを残さない
	if (pConts->empty())
	{
		DisposeGroupPageContent(group, page);
		return false;
	}

	// 先に対象の有無を確認、集積する
	std::vector<DeviceContent*> vConts{};
	for (int b = 0; b < btnCnt; ++b)
	{
		// 割当済or異なるグループ
		if ((initialize && (m_aButtonStatus[b].m_eFunctionId != FunctionId::FUNC_NONE))
			|| (!initialize && (m_aButtonStatus[b].m_nGroup != group)))
			continue;

		// グループ内に該当のボタン情報があれば集積
		bool _res = false;
		auto pCont = GroupPage::GetContent(*pConts, b + 1);
		if (pCont)
		{
			vConts.push_back(pCont);
		}
	}

	// 集積結果がある場合のみボタン情報をボタンバッファに転載
	while (!vConts.empty())
	{
		auto itr = vConts.begin();
		auto pCont = *itr;
		vConts.erase(itr);
		if (pCont)
		{
			// この時点で pCont->m_nButtonId > 0 は確定している
			if (SetButtonStatus(m_aButtonStatus[pCont->m_nButtonId - 1], *pCont))
				++asnCnt;
			// pCont は m_pGroupPageContents の内容を指しているので delete の必要なし
		}
	}

	// ボタン情報を更新した場合
	if (asnCnt > 0)
	{
		// カレントページを更新する
		SetCurrentGroupPage(group, page);
	}
	// グループ内でボタン割当が行われなかった場合
	else
	{
		// 以降の参照はないのでバッファポインタを残さない
		DisposeGroupPageContent(group, page);
	}

	return asnCnt;
}
/// <summary>
/// ボタン情報リセット
/// </summary>
/// <param name="setTopPageButtonStatus">先頭ページボタン情報設定</param>
/// <returns></returns>
static int ResetButtonStatus(bool setTopPageButtonStatus)
{
	// ボタン用の保持バッファをすべて初期化
	ClearInhibitValue();
	ClearXptValue();
	ClearButtonStatus();
	ClearCurrentGroupPage();
	DisposeGroupPageContents();

	// 返却はボタン割当数
	int asnCnt = 0;
	if (!setTopPageButtonStatus)
		return asnCnt;

	// デバイス情報が確定していない
	if (!IsValidDeviceContents())
		return asnCnt;
	// ボタン数
	int btnCnt = GetButtonCount();	// 39D時、041,042() は抜け番
	// ボタン割当最大数
	int asnMax = IsButton39D() ? btnCnt - 2 : btnCnt;
	if (!IsRange(btnCnt, 0, DEVCNT_MAX) || !IsRange(asnMax, 1, btnCnt))
		return asnCnt;

	// 定義可能なグループ数分、ボタン情報が埋まるまで回す
	for (char g = 0; (g <= GROUP_MAX) && (asnCnt < asnMax); ++g)
	{
		// 各グループの0ページを取得、転載する
		int _asnCnt = ResetButtonStatus(g, 0, true);

		// 全体の割当数にグループ内割当数を反映
		if (_asnCnt > 0)
			asnCnt += _asnCnt;
	}

	// 割当数
	return asnCnt;
}

/// <summary>
/// ボタン状態取得
/// </summary>
/// <param name="index"></param>
/// <param name="status"></param>
/// <returns></returns>
bool GetButtonStatus(const int index, ButtonStatus& status)
{
	// 各値の初期化はしない
	bool valid = false;

	if (!IsValidButtonIndex(index))
		return false;

	status = m_aButtonStatus[index];
	valid = IsValidGroupPage(status.m_nGroup, status.m_nPage);

	return valid;
}

/// <summary>
/// 操作要求
/// </summary>
/// <param name="buttonIndex"></param>
/// <param name="doneAction"></param>
static void RequestButtonAction(int buttonIndex, ActionId& doneAction)
{
	doneAction = ActionId::ACTION_NONE;
	ButtonStatus status{};
	if (!GetButtonStatus(buttonIndex, status)
	 || !IsValidFunctionId(status.m_eFunctionId))
		return;

	// データ参照
	DeviceContent* pCont = nullptr;
	auto pConts = LoadGroupPageContents(status.m_nGroup, status.m_nPage);
	if (pConts)
	{
		// 該当のボタン情報
		pCont = GroupPage::GetContent(*pConts, buttonIndex + 1);
	}
	if (!pCont)
		return;
	std::vector<FunctionId> vlockBtns = { FunctionId::FUNC_LOCKLOCAL, FunctionId::FUNC_LOCKALL, FunctionId::FUNC_LOCKOTHER };
	if (IsLock()
	 && std::find_if(vlockBtns.cbegin(), vlockBtns.cend(),
					 [btnfid = status.m_eFunctionId](FunctionId fid)
					 { return btnfid == fid; }) == vlockBtns.cend())
		return;

	std::string sFunc = FunctionMap.at(status.m_eFunctionId);
	// インヒビット設定確認が最優先
	if (IsInhibitSetting())
	{
		// INHIBIT ボタン
		if (status.m_eFunctionId == FunctionId::FUNC_INHIBIT)
		{
			// 設定実績がない場合キャンセル
			if (m_vInhibits.empty())
			{
				Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, cencel setting.\n",
														 buttonIndex, sFunc.c_str());

				// 変化ありとする
				doneAction = ActionId::ACTION_DONE;
			}
			// 設定実績あり
			else
			{
				Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, inhibit update request count = %d.\n",
														 buttonIndex, sFunc.c_str(), m_vInhibits.size());
				// データ更新
				UpdateInhivitValue();

				// 変化ありとする
				doneAction = ActionId::ACTION_ALARM;
			}

			// いずれもインヒビット設定用データを初期化
			ClearInhibitValue();
		}
		// INHIBIT 以外の有効ボタン
		else
		{
			// TAKE 無効時のTAKEボタンは切り替え操作不可
			if ((status.m_eFunctionId == FunctionId::FUNC_TAKE)
			 && !m_pDeviceContents->TakeButtonEnable())
				return;

			// 設定実績更新と m_aButtonStatus の m_bInhibit 切替
			int res = AddOrRemoveInhibitValue(status);

			// 状態切替発生時は変化ありとする
			if (res != 0)
			{
				bool bNewInh = m_aButtonStatus[status.m_nButtonIndex].m_bInhibit;
				std::string sBef = bNewInh ? "False" : "True";
				std::string sNew = bNewInh ? "True" : "False";
				if (res > 0)
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, inhibit update request %s -> %s.\n",
															 buttonIndex, sFunc.c_str(), sBef.c_str(), sNew.c_str());
				else
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, canceled inhibit update request.\n",
															 buttonIndex, sFunc.c_str());

				doneAction = ActionId::ACTION_DONE;
			}
			else
				Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, cannot inhibit update request.\n",
														 buttonIndex, sFunc.c_str());
		}

		// 戻る
		return;
	}
	// インヒビット設定開始要求
	else if (status.m_eFunctionId == FunctionId::FUNC_INHIBIT)
	{
		m_bInhibitSetting = true;
		// ページ替要求外で XPT 既存設定があれば初期化
		if (m_bCanSetXpt
		 || m_sXptValue.IsSet()
		 || !m_vXpts.empty())
		{
			Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, cencel xpt control.\n",
													 buttonIndex, sFunc.c_str());
			ClearXptValue();
		}
		Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, setting start.\n",
												 buttonIndex, sFunc.c_str());

		// 変化ありとして戻る
		doneAction = ActionId::ACTION_DONE;
		return;
	}
	// インヒビット設定中ではなくインヒビットがかかっているボタン
	else if (status.m_bInhibit != 0)
	{
		Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, this button is inhibit.\n",
												 buttonIndex, sFunc.c_str());
		return;
	}

	// インヒビットの次、ページ替を確認
	else if (IsPageControlRequest(pCont->m_eFunctionId))
	{
		std::string sGroups = "";
		auto pGroups = pCont->ControlGroups();
		bool bAllGroups = (pGroups && !pGroups->empty())
						? ContainsPageControlGroups(pGroups, -1)
						: true;
		if (bAllGroups)
		{
			if (pGroups)
			{
				delete pGroups;
				pGroups = nullptr;
			}
			sGroups = "-1";
		}
		else
		{
			for (char g : *pGroups)
			{
				if (!sGroups.empty())
					sGroups += ",";
				sGroups += std::to_string((int)g);
			}
		}

		try
		{
			switch (pCont->m_eFunctionId)
			{
			case FunctionId::FUNC_PAGE_UP:
				{
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, groups = %s.\n",
															 buttonIndex, sFunc.c_str(), sGroups.c_str());

					int nGCnt = 0;
					for (char nGroup = 1; nGroup <= GROUP_MAX; ++nGroup)
					{
						// 使用していないグループ
						char nPage = GetCurrentGroupPage(nGroup);
						if (nPage < 0)
							continue;
						// 全対象でない際に対照グループに含まれていない
						if (!bAllGroups && !ContainsPageControlGroups(pGroups, nGroup))
							continue;

						// 次ページ
						char nNewPage = nPage + 1;
						int nAsnCnt = 0;
						if ((nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false)) <= 0)
						{
							// 元が1ページ以降なら先頭に戻す
							if (nPage > 0)
							{
								nNewPage = 0;
								nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false);
							}
						}
						if (nAsnCnt > 0)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d -> %d.\n",
																	 nGroup, nPage, nNewPage);
							++nGCnt;
						}
						else
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d, unmoved.\n",
																	 nGroup, nPage);
							// 1ページ以降から次へも先頭へも移動できないという状態はない想定
						}
					}

					if (nGCnt > 0)
						doneAction = ActionId::ACTION_DONE;
				}
				break;
			case FunctionId::FUNC_PAGE_DOWN:
				{
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, groups = %s.\n",
															 buttonIndex, sFunc.c_str(), sGroups.c_str());

					int nGCnt = 0;
					for (char nGroup = 1; nGroup <= GROUP_MAX; ++nGroup)
					{
						// 使用していないグループ
						char nPage = GetCurrentGroupPage(nGroup);
						if (nPage < 0)
							continue;
						// 全対象でない際に対照グループに含まれていない
						if (!bAllGroups && !ContainsPageControlGroups(pGroups, nGroup))
							continue;

						// 前ページ
						int nAsnCnt = 0;
						char nNewPage = nPage;
						for (nNewPage = (nPage > 0) ? nPage - 1 : PAGE_MAX;
							 ((nPage > 0) && (nNewPage >= 0)) || ((nPage <= 0) && (nNewPage > 0));
							 --nNewPage)
						{
							nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false);
							// for 文で nNewPage を更新しないうちに抜ける
							if (nAsnCnt > 0)
								break;
						}
						if (nAsnCnt > 0)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d -> %d.\n",
																	 nGroup, nPage, nNewPage);
							++nGCnt;
						}
						else
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d, unmoved.\n",
																	 nGroup, nPage);
							// 1ページ以降から前に移動できないという状態はない想定
						}
					}

					if (nGCnt > 0)
						doneAction = ActionId::ACTION_DONE;
				}
				break;
			case FunctionId::FUNC_PAGE_JUMP:
				{
					// 移動先ページ
					char nNewPage = pCont->ControlPage();
					if (nNewPage < 0)
					{
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, groups = %s, failed jump page = %d.\n",
																 buttonIndex, sFunc.c_str(), sGroups.c_str(), nNewPage);
						doneAction = ActionId::ACTION_ERROR;
						return;
					}
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, groups = %s, jump page = %d.\n",
															 buttonIndex, sFunc.c_str(), sGroups.c_str());

					int nGCnt = 0;
					for (char nGroup = 1; nGroup <= GROUP_MAX; ++nGroup)
					{
						// 使用していないグループ
						char nPage = GetCurrentGroupPage(nGroup);
						if (nPage < 0)
							continue;
						// 全対象でない際に対照グループに含まれていない
						if (!bAllGroups && !ContainsPageControlGroups(pGroups, nGroup))
							continue;

						int nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false);
						if (nAsnCnt > 0)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d -> %d.\n",
																	 nGroup, nPage, nNewPage);
							++nGCnt;
						}
						else
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d, unmoved.\n",
																	 nGroup, nPage);
						}
					}

					if (nGCnt > 0)
						doneAction = ActionId::ACTION_DONE;
				}
				break;
			case FunctionId::FUNC_PAGE_CONTROL:
				{
					TogglePageControlEnabled();
					bool ena = IsPageControlEnabled();
					if (ena)
					{
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, start page control, groups = %s.\n",
																 buttonIndex, sFunc.c_str(), sGroups.c_str());
						SetPageControlGroups(pGroups);
					}
					else
					{
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, exit page control.\n",
																 buttonIndex, sFunc.c_str());
						SetPageControlGroups(nullptr);
					}
					doneAction = ActionId::ACTION_DONE;
				}
				break;
			}

		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}

		return;
	}

	// インヒビット、ページ替の次、ボタン操作が複数あって初めて動作可能となる XPT 設定の確認を実施する
	else if (pCont->IsXptContent())	// SRC/DEST/TAKE
	{
		try
		{
			// 設定情報不十分で XPT 操作不可
			if (!status.m_bCanUseXpt)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, but invalid content.\n",
																buttonIndex, sFunc.c_str());
				return;
			}

			// XPT 設定不可時
			if (!m_bCanSetXpt)
			{
				// TAKE ボタン有効時
				if (m_pDeviceContents->TakeButtonEnable())
				{
					// DEST/SRC 押下は無効
					if (pCont->m_eFunctionId != FunctionId::FUNC_TAKE)
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, but TAKE button is unpushed.\n",
							buttonIndex, sFunc.c_str());
						return;
					}

					// 設定値初期化してからフラグを設定可にする
					ClearXptValue(pCont->m_sArg1);
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, DEST/SRC button enabled.\n",
						buttonIndex, sFunc.c_str());
				}
				// TAKE ボタン無効時
				else
				{
					// TAKE/SRC 押下は無効
					if (pCont->m_eFunctionId != FunctionId::FUNC_DEST)
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, invalid sequence.\n",
							buttonIndex, sFunc.c_str());
						return;
					}

					// 設定値初期化してからフラグを設定可にする
					ClearXptValue(m_pDeviceContents->MatrixPath());
					m_sXptValue.Set(status);
					// 操作中にページ替えの可能性があるため
					// ボタンインデックスではなくシグナルを控える
					m_pNmosEmberConsumer->GetSignalValues(m_sXptValue.m_nDestConnSignal, m_sXptValue.m_nDestConnCount, m_vDestConnSrcs);
					Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, top signal = %d.\n",
						buttonIndex, sFunc.c_str(), status.m_nConnSignal);
				}

				// 変化ありとして戻る
				doneAction = ActionId::ACTION_DONE;
				return;
			}
			// XPT 設定可時
			else
			{
				// TAKE ボタン有効時
				if (m_pDeviceContents->TakeButtonEnable())
				{
					// 先に DEST/SRC 押下を確認
					if (pCont->m_eFunctionId != FunctionId::FUNC_TAKE)
					{
						// 操作中にページ替えの可能性があるため
						// ボタンインデックスではなく先頭シグナルを控える
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, top signal = %d.\n",
																 buttonIndex, sFunc.c_str(), status.m_nConnSignal);
						m_sXptValue.Set(status);
						if (pCont->m_eFunctionId == FunctionId::FUNC_DEST)
						{
							// ボタンインデックスではなくシグナルを控える
							m_pNmosEmberConsumer->GetSignalValues(m_sXptValue.m_nDestConnSignal, m_sXptValue.m_nDestConnCount, m_vDestConnSrcs);
						}

						// どちらも設定されたら設定実績に控え直す
						if (m_sXptValue.IsValid())
						{
							// 今要件では1組のみ退避
							while (!m_vXpts.empty())
							{
								auto itr = m_vXpts.begin();
								if (!(*itr))
									delete (*itr);
								m_vXpts.erase(itr);
							}
							m_vXpts.push_back(new XptValue{ m_sXptValue });
							Trace(__FILE__, __LINE__, __FUNCTION__, "stored xpt signal, dest top signal = %d, src top signal = %d.\n",
																	 m_sXptValue.m_nDestConnSignal, m_sXptValue.m_nSrcConnSignal);
						}

						// 変化ありとして戻る
						doneAction = ActionId::ACTION_DONE;
						Trace(__FILE__, __LINE__, __FUNCTION__, "exit(set DEST/SRC value), DoneAction = %d.\n", doneAction);
						return;
					}
					// 設定実績がない場合キャンセル
					if (m_vXpts.empty())
					{
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, cencel xpt control.\n",
																 buttonIndex, sFunc.c_str());
						ClearXptValue();

						// 変化ありとして戻る
						doneAction = ActionId::ACTION_DONE;
						return;
					}

					// ここへ到達分はEmber処理へ回す
					if (pCont->m_eFunctionId == FunctionId::FUNC_SRC)
					{
						XptValue xptValue{};
						GetXptSignals(xptValue);
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, dest top signal = %d, src top signal = %d.\n",
							buttonIndex, sFunc.c_str(), xptValue.m_nDestConnSignal, xptValue.m_nSrcConnSignal);
					}
				}
				// TAKE ボタン無効時
				else
				{
					// TAKE ボタン押下は無効
					if (pCont->m_eFunctionId == FunctionId::FUNC_TAKE)
					{
						ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, invalid sequence.\n",
							buttonIndex, sFunc.c_str());
						return;
					}
					// DEST ボタン押下はやり直し
					if (pCont->m_eFunctionId == FunctionId::FUNC_DEST)
					{
						// 設定値初期化してからフラグを設定可にする
						ClearXptValue(m_pDeviceContents->MatrixPath());
						m_sXptValue.Set(status);
						// 操作中にページ替えの可能性があるため
						// ボタンインデックスではなくシグナルを控える
						m_pNmosEmberConsumer->GetSignalValues(m_sXptValue.m_nDestConnSignal, m_sXptValue.m_nDestConnCount, m_vDestConnSrcs);
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, top signal = %d.\n",
							buttonIndex, sFunc.c_str(), status.m_nConnSignal);

						// 変化ありとして戻る
						doneAction = ActionId::ACTION_DONE;
						return;
					}

					// ここへ到達分はEmber処理へ回す
					if (pCont->m_eFunctionId == FunctionId::FUNC_SRC)
					{
						m_sXptValue.Set(status);
						XptValue xptValue{};
						GetXptSignals(xptValue);
						Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, dest top signal = %d, src top signal = %d.\n",
																	buttonIndex, sFunc.c_str(), xptValue.m_nDestConnSignal, xptValue.m_nSrcConnSignal);
						// どちらも設定されたら設定実績に控え直す
						if (m_sXptValue.IsValid())
						{
							// 今要件では1組のみ退避
							while (!m_vXpts.empty())
							{
								auto itr = m_vXpts.begin();
								if (!(*itr))
									delete (*itr);
								m_vXpts.erase(itr);
							}
							m_vXpts.push_back(new XptValue{ m_sXptValue });
							Trace(__FILE__, __LINE__, __FUNCTION__, "stored xpt signal, dest top signal = %d, src top signal = %d.\n",
								m_sXptValue.m_nDestConnSignal, m_sXptValue.m_nSrcConnSignal);
						}
						else
						{
							// キャンセル
							Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, cencel xpt control.\n",
								buttonIndex, sFunc.c_str());
							ClearXptValue();

							// 変化ありとして戻る
							doneAction = ActionId::ACTION_DONE;
							return;
						}
					}
				}
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());

			// XPT 設定用データ初期化を実施する前に既存設定があるか確認
			if (m_bCanSetXpt
			 || m_sXptValue.IsSet()
			 || !m_vXpts.empty())
				doneAction = ActionId::ACTION_DONE;

			ClearXptValue();

			return;
		}
	}
	// XPT 操作外
	else
	{
		// ページ替要求外で XPT 既存設定があれば初期化
		if (!IsPageControlRequest(status.m_eFunctionId)
		 && (m_bCanSetXpt
		  || m_sXptValue.IsSet()
		  || !m_vXpts.empty()))
		{
			Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s, cencel xpt control.\n",
													 buttonIndex, sFunc.c_str());
			ClearXptValue();
			doneAction = ActionId::ACTION_DONE;
		}

		else
			Trace(__FILE__, __LINE__, __FUNCTION__, "buttonIndex = %d, function = %s.\n",
													 buttonIndex, sFunc.c_str());
	}

	RequestId requestId{ 0 };
	if (IsEmberRequest(pCont->m_eFunctionId))
	{
		if (!m_pNmosEmberConsumer
		 || !m_pNmosEmberConsumer->Enabled()
		 || !m_pNmosEmberConsumer->CanRequest())
			return;

		requestId.group = status.m_nGroup;
		requestId.page = status.m_nPage;
		requestId.buttonIndex = buttonIndex;

		switch (pCont->m_eFunctionId)
		{
		// Connection
		case FunctionId::FUNC_TAKE:
			// 事前に有無効の確認をしている
			{
				XptValue xptValue{};
				GetXptSignals(xptValue);

				int nId = m_pNmosEmberConsumer->AddConsumerRequest(&requestId, pCont->m_sArg1,
																   xptValue.m_nDestConnSignal, xptValue.m_nDestConnCount,
																   xptValue.m_nSrcConnSignal, xptValue.m_nSrcConnCount);
				// 要求が承認されたなら結果に拘らず音は鳴らす
				if (nId > 0)
					doneAction = ActionId::ACTION_ALARM;

				// XPT 設定用データ初期化
				ClearXptValue();
			}
			break;
		// Invoke
		case FunctionId::FUNC_LOCKALL:
		case FunctionId::FUNC_LOCKOTHER:
		case FunctionId::FUNC_EMBER_FUNC:
			if (!pCont->m_sArg1.empty())
			{
				std::string sValue = pCont->m_sArg2;
				int nId = m_pNmosEmberConsumer->AddConsumerRequest(&requestId, pCont->m_sArg1, sValue);
				// 要求が承認されたなら結果に拘らず音は鳴らす
				if (nId > 0)
					doneAction = ActionId::ACTION_ALARM;
			}
			break;

		// Parameter
		case FunctionId::FUNC_EMBER_VALUE:
			if (!pCont->m_sArg1.empty())
			{
				std::string sValue = pCont->m_sArg2;
				// 空であっても文字列パラメータの初期化要求の可能性があるのでそのまま通す
				int nId = m_pNmosEmberConsumer->AddConsumerRequest(&requestId, pCont->m_sArg1, sValue);
				// 要求が承認されたなら結果に拘らず音は鳴らす
				if (nId > 0)
					doneAction = ActionId::ACTION_ALARM;
			}
			break;

		// 対応前
		case FunctionId::FUNC_SALVO: break;
		}
	}

	// TAKE ボタン無効時の XPT切り替え発行
	else if ((pCont->m_eFunctionId == FunctionId::FUNC_SRC)
		  && !m_vXpts.empty()
		  && !m_pDeviceContents->TakeButtonEnable())
	{
		if (!m_pNmosEmberConsumer
		 || !m_pNmosEmberConsumer->Enabled()
		 || !m_pNmosEmberConsumer->CanRequest())
			return;

		XptValue xptValue{};
		GetXptSignals(xptValue);

		int nId = m_pNmosEmberConsumer->AddConsumerRequest(&requestId, m_pDeviceContents->MatrixPath(),
															xptValue.m_nDestConnSignal, xptValue.m_nDestConnCount,
															xptValue.m_nSrcConnSignal, xptValue.m_nSrcConnCount);
		// 要求が承認されたなら結果に拘らず音は鳴らす
		if (nId > 0)
			doneAction = ActionId::ACTION_ALARM;

		// XPT 設定用データをソース側のみ初期化
		ClearXptSourcesValue();
	}

	else
	{
		switch (pCont->m_eFunctionId)
		{
		case FunctionId::FUNC_LOCKLOCAL:
			m_bLockLocal = !m_bLockLocal;
			Trace(__FILE__, __LINE__, __FUNCTION__, "LockLocal %s.\n", IsLock() ? "false -> true" : "true -> false");
			doneAction = ActionId::ACTION_ALARM;
			break;
		}
	}
}
/// <summary>ボタン状態解析</summary>
/// <param name="pStatus"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
static bool _AnalyzeButtonAction(const void* pStatus, ActionId& doneAction)
{
	Guidance("\n");
	Trace(__FILE__, __LINE__, __FUNCTION__, "start.\n");

	doneAction = ActionId::ACTION_NONE;
	bool res = false;
	void* pLastStatus = GetLastButtonStatus();
	int len = (int)GetButtonActionDataLength();
	if (!pStatus || !pLastStatus || (len <= 0))
		return res;

	try
	{
		//
		// 操作ターゲットと値、操作を特定
		//
		/*** model
		struct structBUTTON_40RU {
			uint32_t	len;
			uint32_t	command;
			uint8_t		setup_sw;
			uint8_t		btns[ボタン数];
			uint8_t		endcode;
		};
		***/
		int offset = (int)(2 * sizeof(uint32_t));
		int cnt = (int)(((size_t)(len - offset) - (2 * sizeof(uint8_t))) / sizeof(uint8_t));
		if (cnt <= 0)
		{
			Trace(__FILE__, __LINE__, __FUNCTION__, "exit(invalid length).\n");
			return res;
		}

		char* pTop = (char*)pStatus;
		char sw = pTop[offset];

#if true
		// 押されていないボタン用
		std::vector<int> lastUnPushedButtons;
		lastUnPushedButtons.clear();
		// 押されたままのボタン用
		std::vector<int> lastPushedButtons;
		lastPushedButtons.clear();
		std::vector<int> pushedButtons;
		pushedButtons.clear();
		std::vector<int> changedButtons;
		changedButtons.clear();

		// 変化の確認
		for (int b = 0; b < cnt; ++b)
		{
			int pos = (offset + 1) + b;

			char lsw = ((char*)pLastStatus)[pos];
			// 値の設定されていないデータ対策
			if (lsw == 0)
				lsw = '0';
			if (lsw != '0')
			{
				// 確認用に控えておく
				lastPushedButtons.push_back(b);
			}
			else
			{
				// 確認用に控えておく
				lastUnPushedButtons.push_back(b);
			}

			char bsw = pTop[pos];
			// 値の設定されていないデータ対策
			if (bsw == 0)
				bsw = '0';
			// 押されているボタン
			if (bsw != '0')
			{
				// 確認用に控えておく
				pushedButtons.push_back(b);
			}

			// 変化があったボタン
			if (bsw != lsw)
			{
				changedButtons.push_back(b);

				m_aButtonStatus[b].m_bPushed = (bsw != '0') ? 1 : 0;
				m_aButtonStatus[b].m_bLongPushed = 0;
				m_aButtonStatus[b].m_tPassingTime = m_aButtonStatus[b].m_bPushed
					? std::chrono::system_clock::now() + std::chrono::seconds(m_nLongPushDistSeconds)
					: std::chrono::system_clock::time_point{};
			}
		}

		// 単一ボタンが押された状態から離されたのであれば操作要求
		ActionId eDoneAction = ActionId::ACTION_NONE;
		//if ((changedButtons.size() == 1) && pushedButtons.empty() && (pTop[(offset + 1) + changedButtons[0]] == '0'))
		if ((changedButtons.size() == 1) && (pushedButtons.size() == 1) && (pTop[(offset + 1) + changedButtons[0]] != '0'))
		{
			m_aButtonStatus[changedButtons[0]].m_bActionError = 0;
			RequestButtonAction(changedButtons[0], eDoneAction);
			doneAction = eDoneAction;
			if (doneAction == ActionId::ACTION_ERROR)
				m_aButtonStatus[changedButtons[0]].m_bActionError = 1;
		}
#else
		// 新たに押されたものだけ拾う
		for (int b = 0; b < cnt; ++b)
		{
			int pos = (offset + 1) + b;

			char bsw = pTop[pos];
			if (bsw && bsw != ((char*)pLastStatus)[pos])
			{
				ActionId eDoneAction = ActionId::ACTION_NONE;
				RequestButtonAction(b, eDoneAction);
				if (eDoneAction != ActionId::ACTION_NONE)
					doneAction = eDoneAction;
			}
		}
#endif

		res = true;
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	Trace(__FILE__, __LINE__, __FUNCTION__, "exit.\n");
	return res;
}

/// <summary>ボタン状態(18d)</summary>
static struct structBUTTON_18D m_LastButton18D;
/// <summary>ボタン状態(18d)初期化</summary>
static void ClearLastButton18D()
{
	memset(&m_LastButton18D, 0, sizeof(struct structBUTTON_18D));
}
/// <summary>ボタン状態(18d)設定</summary>
/// <param name="status">ボタン状態(18d)</param>
/// <returns>true</returns>
//bool SetLastButton18D(const struct structBUTTON_18D& status)
static bool SetLastButton(const struct structBUTTON_18D& status)
{
	assert(IsButton18D());
	memcpy(&m_LastButton18D, &status, sizeof(struct structBUTTON_18D));
	return true;
}
/// <summary>ボタン状態解析</summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
bool AnalyzeButtonAction(const struct structBUTTON_18D& status, ActionId& doneAction)
{
	doneAction = ActionId::ACTION_NONE;
	bool res = false;
	assert(IsButton18D());

	try
	{
		if ((res = _AnalyzeButtonAction(&status, doneAction)) == true)
			SetLastButton(status);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return res;
}

/// <summary>ボタン状態(39d)</summary>
static struct structBUTTON_39D m_LastButton39D;
/// <summary>ボタン状態(39d)初期化</summary>
static void ClearLastButton39D()
{
	memset(&m_LastButton39D, 0, sizeof(struct structBUTTON_39D));
}
/// <summary>ボタン状態(39d)設定</summary>
/// <param name="status">ボタン状態(39d)</param>
/// <returns>true</returns>
static bool SetLastButton(const struct structBUTTON_39D& status)
{
	assert(IsButton39D());
	memcpy(&m_LastButton39D, &status, sizeof(struct structBUTTON_39D));
	return true;
}
/// <summary>ボタン状態解析</summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
bool AnalyzeButtonAction(const struct structBUTTON_39D& status, ActionId& doneAction)
{
	doneAction = ActionId::ACTION_NONE;
	bool res = false;
	assert(IsButton39D());

	try
	{
		if ((res = _AnalyzeButtonAction(&status, doneAction)) == true)
			SetLastButton(status);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return res;
}

/// <summary>ボタン状態(40ru)</summary>
static struct structBUTTON_40RU m_LastButton40RU;
/// <summary>ボタン状態(40ru)初期化</summary>
static void ClearLastButton40RU()
{
	memset(&m_LastButton40RU, 0, sizeof(struct structBUTTON_40RU));
}
/// <summary>ボタン状態(40ru)設定</summary>
/// <param name="status">ボタン状態(40ru)</param>
/// <returns>true</returns>
static bool SetLastButton(const struct structBUTTON_40RU& status)
{
	assert(IsButton40RU());
	memcpy(&m_LastButton40RU, &status, sizeof(struct structBUTTON_40RU));
	return true;
}
/// <summary>ボタン状態解析</summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
bool AnalyzeButtonAction(const struct structBUTTON_40RU& status, ActionId& doneAction)
{
	doneAction = ActionId::ACTION_NONE;
	bool res = false;
	assert(IsButton40RU());

	try
	{
		if ((res = _AnalyzeButtonAction(&status, doneAction)) == true)
			SetLastButton(status);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return res;
}

/// <summary>
/// 直近ボタン操作データ長
/// </summary>
/// <returns></returns>
void* GetLastButtonStatus() { return IsButton40RU() ? (void*)&m_LastButton40RU
								   : IsButton39D() ? (void*)&m_LastButton39D
								   : IsButton18D() ? (void*)&m_LastButton18D
								   : nullptr; }

/// <summary>ロータリー状態</summary>
static struct structROTARY m_LastRotary{};
/// <summary>ロータリー状態初期化</summary>
static void ClearLastRotary()
{
	memset(&m_LastRotary, 0, sizeof(struct structROTARY));
}
/// <summary>ロータリー状態設定</summary>
/// <param name="ans">ロータリー状態</param>
/// <returns>true</returns>
static bool SetLastRotary(const struct structROTARY& status)
{
	memcpy(&m_LastRotary, &status, sizeof(struct structROTARY));
	return true;
}

static int m_nRotaryTarget = 0;
static int m_nRotaryValue = 0;
static int GetRotaryTarget() { return m_nRotaryTarget; }
static int GetRotaryValue() { return m_nRotaryValue; }

/// <summary>
/// 符号付き整数取得
/// </summary>
/// <param name="srcptr"></param>
/// <param name="offset"></param>
/// <returns></returns>
static int getSignedDWORD(char* srcptr, int offset)
{
	std::string str(&srcptr[offset], 4);
	if (str[0] == '+') {
		return atol(str.substr(1).c_str());
	}
	else if (str[0] == '-') {
		return atol(str.substr(1).c_str()) * (-1);
	}
	else {
		assert(str.compare("0000") == 0);
		return 0;
	}
}
/// <summary>
/// 操作要求
/// </summary>
/// <param name="buttonIndex"></param>
/// <param name="doneAction"></param>
static void RequestRotaryAction(int speed, bool pushed, ActionId& doneAction)
{
	doneAction = ActionId::ACTION_NONE;
	if (IsLock())
		return;
	Trace(__FILE__, __LINE__, __FUNCTION__, "start, speed = %d, pushed = %d.\n", speed, pushed);

	// renc 制御の操作対象は現状 FUNC_PAGE_CONTROL のみ
	bool ena = IsPageControlEnabled();
	if (ena)
	{
		std::string sFunc = FunctionMap.at(FunctionId::FUNC_PAGE_CONTROL);
		std::string sGroups = "";
		auto pGroups = GetPageControlGroups();
		bool bAllGroups = (pGroups && !pGroups->empty())
						? ContainsPageControlGroups(pGroups, -1)
						: true;
		if (bAllGroups)
		{
			if (pGroups)
			{
				delete pGroups;
				pGroups = nullptr;
			}
			sGroups = "-1";
		}
		else
		{
			for (char g : *pGroups)
			{
				if (!sGroups.empty())
					sGroups += ",";
				sGroups += std::to_string((int)g);
			}
		}
		Trace(__FILE__, __LINE__, __FUNCTION__, "function = %s, groups = %s.\n", sFunc.c_str(), sGroups.c_str());

		try
		{
			// +方向 : up
			if (0 < speed)
			{
				for (int i = 0; i < speed; ++i)
				{
					int nGCnt = 0;
					for (char nGroup = 1; nGroup <= GROUP_MAX; ++nGroup)
					{
						// 使用していないグループ
						char nPage = GetCurrentGroupPage(nGroup);
						if (nPage < 0)
							continue;
						// 全対象でない際に対照グループに含まれていない
						if (!bAllGroups && !ContainsPageControlGroups(pGroups, nGroup))
							continue;

						// 次ページ
						char nNewPage = nPage + 1;
						int nAsnCnt = 0;
						if ((nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false)) <= 0)
						{
							// 元が1ページ以降なら先頭に戻す
							if (nPage > 0)
							{
								nNewPage = 0;
								nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false);
							}
						}
						if (nAsnCnt > 0)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d -> %d.\n",
								nGroup, nPage, nNewPage);
							++nGCnt;
						}
						else
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d, unmoved.\n",
								nGroup, nPage);
							// 1ページ以降から次へも先頭へも移動できないという状態はない想定
						}
					}

					if (nGCnt > 0)
						doneAction = ActionId::ACTION_DONE;
				}
			}
			// -方向 : down
			else if (speed < 0)
			{
				for (int i = 0; speed < i; --i)
				{
					int nGCnt = 0;
					for (char nGroup = 1; nGroup <= GROUP_MAX; ++nGroup)
					{
						// 使用していないグループ
						char nPage = GetCurrentGroupPage(nGroup);
						if (nPage < 0)
							continue;
						// 全対象でない際に対照グループに含まれていない
						if (!bAllGroups && !ContainsPageControlGroups(pGroups, nGroup))
							continue;

						// 前ページ
						int nAsnCnt = 0;
						char nNewPage = nPage;
						for (nNewPage = (nPage > 0) ? nPage - 1 : PAGE_MAX;
							((nPage > 0) && (nNewPage >= 0)) || ((nPage <= 0) && (nNewPage > 0));
							--nNewPage)
						{
							nAsnCnt = ResetButtonStatus(nGroup, nNewPage, false);
							// for 文で nNewPage を更新しないうちに抜ける
							if (nAsnCnt > 0)
								break;
						}
						if (nAsnCnt > 0)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d -> %d.\n",
								nGroup, nPage, nNewPage);
							++nGCnt;
						}
						else
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, "group%d's page %d, unmoved.\n",
								nGroup, nPage);
							// 1ページ以降から前に移動できないという状態はない想定
						}
					}

					if (nGCnt > 0)
						doneAction = ActionId::ACTION_DONE;
				}
			}
			// 停止
			else// if (speed == 0)
			{
				//Trace(__FILE__, __LINE__, __FUNCTION__, "unmove renc.\n");
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
	}
	else
	{
		//Trace(__FILE__, __LINE__, __FUNCTION__, "ignore control.\n");
		return;
	}

	//Trace(__FILE__, __LINE__, __FUNCTION__, "exit, DoneAction = %d.\n", doneAction);
}
/// <summary>
/// ロータリー操作解析
/// </summary>
/// <param name="status"></param>
/// <param name="doneAction"></param>
/// <returns></returns>
bool AnalyzeRotaryAction(const struct structROTARY& status, ActionId& doneAction)
{
	Guidance("\n");
	int speed = getSignedDWORD((char*)&status.value, 0);
	bool pushed = status.pushed == '1';
	//Trace(__FILE__, __LINE__, __FUNCTION__, "start, speed = %d, pushed = %d.\n", speed, pushed);

	doneAction = ActionId::ACTION_NONE;
	bool res = false;

	try
	{
		// 無条件で操作要求に回す
		ActionId eDoneAction = ActionId::ACTION_NONE;
		RequestRotaryAction(speed, pushed, eDoneAction);
		doneAction = eDoneAction;

		// 退避
		SetLastRotary(status);
		res = true;
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	//Trace(__FILE__, __LINE__, __FUNCTION__, "exit, result = %d, DoneAction = %d.\n", res, doneAction);
	return res;
}

/// <summary>装置全ステータス初期化</summary>
/// <param name="path"></param>
void ClearDeviceStatus(std::string path)
{
	//ClearBasicStatus();
	ClearLastStatus();
	ClearLastButton18D();
	ClearLastButton39D();
	ClearLastButton40RU();
	ClearLastRotary();

	ResetButtonStatus(false);
	if (!path.empty())
	{
		// 既存のデバイス情報破棄
		DisposeDeviceContents();
		// デバイス情報取得
		m_pDeviceContents = new CDeviceContents(path);
		if(m_pDeviceContents && m_pDeviceContents->Enabled())
		{
			ResetButtonStatus(true);
		}
	}
}

void getInfoFromDeviceContents(MvEmberInfo **o1, NMosEmberInfo **o3, MuteInfo**o4){
	if (m_pDeviceContents) {
		*o1 = m_pDeviceContents->m_pMvEmberInfo1;
		*o3 = m_pDeviceContents->m_pNMosEmberInfo;
		*o4 = m_pDeviceContents->m_pMuteAll;
	}
	else {
		*o1 = nullptr;
		*o3 = nullptr;
		*o4 = nullptr;
	}
}