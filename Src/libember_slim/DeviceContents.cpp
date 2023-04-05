#include "DeviceContents.h"
#include "EmberInfo.h"
#include "Utilities.h"
#include <climits>
#include <cassert>
#include <cstring>
#include <sstream>
#include <iterator>

using namespace utilities;


// ====================================================================

/// <summary>
/// 機能識別マップ
/// </summary>
const std::unordered_map<FunctionId, std::string> FunctionMap =
{
	{ FunctionId::FUNC_LOCKLOCAL,		"LockLocal" },
	{ FunctionId::FUNC_LOCKALL,			"LockAll" },
	{ FunctionId::FUNC_LOCKOTHER,		"LockOther" },
	{ FunctionId::FUNC_PAGE_UP,			"PageUp" },
	{ FunctionId::FUNC_PAGE_DOWN,		"PageDown" },
	{ FunctionId::FUNC_PAGE_JUMP,		"PageJump" },
	{ FunctionId::FUNC_PAGE_CONTROL,	"ControlPage" },
	{ FunctionId::FUNC_SRC,				"Src" },
	{ FunctionId::FUNC_DEST,			"Dest" },
	{ FunctionId::FUNC_TAKE,			"Take" },
	{ FunctionId::FUNC_SALVO,			"Salvo" },
	{ FunctionId::FUNC_EMBER_FUNC,		"EmberFunc" },
	{ FunctionId::FUNC_EMBER_VALUE,		"EmberValue" },
	{ FunctionId::FUNC_INHIBIT,			"Inhibit" },
};
/// <summary>
/// 機能識別デフォルト文字列マップ
/// </summary>
const std::unordered_map<FunctionId, std::string> FunctionDefaultStringMap =
{
	{ FunctionId::FUNC_LOCKLOCAL,		"Lock   Local" },
	{ FunctionId::FUNC_LOCKALL,			"Lock   All" },
	{ FunctionId::FUNC_LOCKOTHER,		"Lock   Other" },
	{ FunctionId::FUNC_PAGE_UP,			"Page   Up" },
	{ FunctionId::FUNC_PAGE_DOWN,		"Page   Down" },
	{ FunctionId::FUNC_PAGE_JUMP,		"Page   Jump" },
	{ FunctionId::FUNC_PAGE_CONTROL,	"Page   Control" },
	{ FunctionId::FUNC_SRC,				"Src" },
	{ FunctionId::FUNC_DEST,			"Dest" },
	{ FunctionId::FUNC_TAKE,			"Take" },
	{ FunctionId::FUNC_SALVO,			"Salvo" },
	{ FunctionId::FUNC_EMBER_FUNC,		"Ember  Func" },
	{ FunctionId::FUNC_EMBER_VALUE,		"Ember  Value" },
	{ FunctionId::FUNC_INHIBIT,			"Inhibit" },
};


/// <summary>
/// 有効機能識別判断
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
/// <remarks>
/// 未搭載機能も省いておき、実装時に有効にする
/// </remarks>
bool IsValidFunctionId(FunctionId id)
{
	return (id == FunctionId::FUNC_LOCKLOCAL)
		//|| (id == FunctionId::FUNC_LOCKALL)
		//|| (id == FunctionId::FUNC_LOCKOTHER)
		|| (id == FunctionId::FUNC_PAGE_UP)
		|| (id == FunctionId::FUNC_PAGE_DOWN)
		|| (id == FunctionId::FUNC_PAGE_JUMP)
		|| (id == FunctionId::FUNC_PAGE_CONTROL)
		|| (id == FunctionId::FUNC_SRC)
		|| (id == FunctionId::FUNC_DEST)
		|| (id == FunctionId::FUNC_TAKE)
		//|| (id == FunctionId::FUNC_SALVO)
		|| (id == FunctionId::FUNC_EMBER_FUNC)
		|| (id == FunctionId::FUNC_EMBER_VALUE)
		|| (id == FunctionId::FUNC_INHIBIT)
		;
}
/// <summary>
/// ページ操作対象機能
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool IsPageControlRequest(FunctionId id)
{
	return (id == FunctionId::FUNC_PAGE_UP)
		|| (id == FunctionId::FUNC_PAGE_DOWN)
		|| (id == FunctionId::FUNC_PAGE_JUMP)
		|| (id == FunctionId::FUNC_PAGE_CONTROL);
}
/// <summary>
/// Ember 対象機能
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool IsEmberRequest(FunctionId id)
{
	return (id == FunctionId::FUNC_LOCKALL)
		|| (id == FunctionId::FUNC_LOCKOTHER)
		|| (id == FunctionId::FUNC_TAKE)
		|| (id == FunctionId::FUNC_SALVO)
		|| (id == FunctionId::FUNC_EMBER_FUNC)
		|| (id == FunctionId::FUNC_EMBER_VALUE);
}
/// <summary>
/// XPT 操作対象機能
/// </summary>
/// <param name="id"></param>
/// <returns></returns>
bool IsXptRequest(FunctionId id)
{
	return (id == FunctionId::FUNC_SRC)
		|| (id == FunctionId::FUNC_DEST)
		|| (id == FunctionId::FUNC_TAKE);
}


// ====================================================================



// ====================================================================

/// <summary>
/// コンストラクタ
/// </summary>
GroupPage::GroupPage()
{
	m_nGroup = -1;
	m_nPage = -1;

	m_vLines.clear();
}
/// <summary>
/// コンストラクタ
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <param name="lines"></param>
GroupPage::GroupPage(int group, int page, std::vector<std::vector<std::string>> lines)
{
	m_nGroup = group;
	m_nPage = page;

	m_vLines.swap(lines);
}
/// <summary>
/// デストラクタ
/// </summary>
GroupPage::~GroupPage()
{
	if (!m_vLines.empty())
		m_vLines.clear();
}
/// <summary>
/// ボタン（デバイス）設定要素取得
/// </summary>
/// <param name="columns"></param>
/// <returns></returns>
DeviceContent* GroupPage::GetContent(const std::vector<std::string>& columns)
{
	std::string tmpStr = "";
	int tmpNum;
	bool tmpBool;
	FunctionId tmpFId = FunctionId::FUNC_NONE;

	DeviceContent* pCont = new DeviceContent();

	pCont->m_nGroup = m_nGroup;
	pCont->m_nPage = m_nPage;

	tmpNum = -1;
	//CommGetColumnValue()
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_BUTTON_ID, tmpStr)
		&& ToNumber(tmpStr, tmpNum))
		pCont->m_nButtonId = tmpNum;
	else
		pCont->m_nButtonId = 0;

	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_FUNCTION, tmpStr)
		&& ToKey(FunctionMap, tmpStr, tmpFId))
		pCont->m_eFunctionId = tmpFId;
	else
		pCont->m_eFunctionId = FunctionId::FUNC_NONE;

	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_ARG1, tmpStr))
		pCont->m_sArg1 = tmpStr;
	else
		pCont->m_sArg1.clear();
	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_ARG2, tmpStr))
		pCont->m_sArg2 = tmpStr;
	else
		pCont->m_sArg2.clear();

	// 表示文字列は引数文字列取得後に処理する
	// 設定がない場合、デフォルトの文字列を充てる
	tmpStr.clear();
	CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_DISPLAY, tmpStr);
	if (tmpStr.empty() && IsValidFunctionId(pCont->m_eFunctionId))
	{
		try
		{
			tmpStr = FunctionDefaultStringMap.at(pCont->m_eFunctionId);
		}
		catch (...) { tmpStr.clear(); }

		// DEST/SRC 時は後ろにシグナルを付加するようにする
		if ((pCont->m_eFunctionId == FunctionId::FUNC_SRC)
		 || (pCont->m_eFunctionId == FunctionId::FUNC_DEST))
		{
			// map で定義しているので empty なら assert でも
			if (tmpStr.empty())
				tmpStr = (pCont->m_eFunctionId == FunctionId::FUNC_SRC) ? "SRC" : "DEST";

			// 端子数設定がある場合のみ2行にして文字列を追加する
			tmpNum = -1;
			if (ToNumber(pCont->m_sArg2, tmpNum) && (tmpNum > 0))
			{
				int cnt = tmpNum;
				// 先頭シグナル
				int top = 0;
				tmpNum = -1;
				if (ToNumber(pCont->m_sArg1, tmpNum))
					top = tmpNum;

				tmpStr.resize(7, ' ');
				std::string sSig = "";
				if (cnt > 1)
					sSig = Format("%d%c%d", top, (cnt > 2) ? '-' : ',', top + cnt - 1);
				else
					sSig = Format("%d", top);

				tmpStr += sSig;
			}
		}
	}
	pCont->m_sDisplay = tmpStr;

	tmpNum = -1;
	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_LED_COLOR, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& ((tmpNum == 0) || IsRange(tmpNum, 10, 15)))
		pCont->m_nLedColor = (uint8_t)tmpNum;
	else
		pCont->m_nLedColor = 0;

#ifdef _OMIT_OLED_DETAIL
	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_COLOR, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& (IsRange(tmpNum, 0, 7) || IsRange(tmpNum, 100, 107)))
		pCont->m_nOLedColor = (uint8_t)tmpNum;
	else
		pCont->m_nOLedColor = 0;
#else
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_STATUS_UPPER_HEAD, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, 1))
		pCont->m_nOLedStatusUpperHead = (uint8_t)tmpNum;
	else
		pCont->m_nOLedStatusUpperHead = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_COLOR_UPPER_HEAD, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, (int)377)
		&& IsRange((tmpNum % 10), 0, 7)
		&& IsRange(((tmpNum / 10) % 10), 0, 7))
		pCont->m_nOLedColorUpperHead = tmpNum;
	else
		pCont->m_nOLedColorUpperHead = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_STATUS_UPPER_TAIL, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, 1))
		pCont->m_nOLedStatusUpperTail = (uint8_t)tmpNum;
	else
		pCont->m_nOLedStatusUpperTail = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_COLOR_UPPER_TAIL, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, (int)377)
		&& IsRange((tmpNum % 10), 0, 7)
		&& IsRange(((tmpNum / 10) % 10), 0, 7))
		pCont->m_nOLedColorUpperTail = tmpNum;
	else
		pCont->m_nOLedColorUpperTail = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_STATUS_LOWER_HEAD, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, 1))
		pCont->m_nOLedStatusLowerHead = (uint8_t)tmpNum;
	else
		pCont->m_nOLedStatusLowerHead = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_COLOR_LOWER_HEAD, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, (int)377)
		&& IsRange((tmpNum % 10), 0, 7)
		&& IsRange(((tmpNum / 10) % 10), 0, 7))
		pCont->m_nOLedColorLowerHead = tmpNum;
	else
		pCont->m_nOLedColorLowerHead = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_STATUS_LOWER_TAIL, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, 1))
		pCont->m_nOLedStatusLowerTail = (uint8_t)tmpNum;
	else
		pCont->m_nOLedStatusLowerTail = 0;
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_COLOR_LOWER_TAIL, tmpStr)
		&& ToNumber(tmpStr, tmpNum)
		&& IsRange(tmpNum, 0, (int)377)
		&& IsRange((tmpNum % 10), 0, 7)
		&& IsRange(((tmpNum / 10) % 10), 0, 7))
		pCont->m_nOLedColorLowerTail = tmpNum;
	else
		pCont->m_nOLedColorLowerTail = 0;
#endif

	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_ARROW, tmpStr)
		&& ToBool(tmpStr, tmpBool))
		pCont->m_bOLedArrow = tmpBool;
	else
		pCont->m_bOLedArrow = false;

	tmpStr.clear();
	if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_OLED_IMAGE, tmpStr)
		&& ToNumber(tmpStr, tmpNum))
		pCont->m_nOLedImage = (uint8_t)tmpNum;
	else
		pCont->m_nOLedImage = 0;

	tmpStr.clear();
	if ((pCont->m_eFunctionId != FunctionId::FUNC_INHIBIT)
		&& CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_INHIBIT, tmpStr)
		&& ToBool(tmpStr, tmpBool))
		pCont->m_bInhibit = tmpBool;
	else
		pCont->m_bInhibit = false;

	return pCont;
}
/// <summary>
/// ボタン（デバイス）設定要素取得
/// </summary>
/// <param name="contents"></param>
/// <param name="buttonId"></param>
/// <returns></returns>
DeviceContent* GroupPage::GetContent(const std::vector<DeviceContent*>& contents, int buttonId)
{
	DeviceContent* pCont = nullptr;
	if (contents.empty())
		return pCont;

	auto itr = std::find_if(contents.cbegin(),
		contents.cend(),
		[buttonId](DeviceContent* content) { return content && (content->m_nButtonId == buttonId); });
	if (itr != contents.cend())
		pCont = *itr;

	return pCont;
}
/// <summary>
/// ボタン（デバイス）設定要素取得
/// </summary>
/// <param name="buttonId"></param>
/// <returns></returns>
DeviceContent* GroupPage::GetContent(int buttonId)
{
	DeviceContent* pCont = nullptr;
	if (m_vLines.empty())
		return pCont;

	// グループ／ページで分割する
	// ラベルがあるなら出現位置に併せて変更すべき
	int ip = (int)CsvColumnId::COLUMN_BUTTON_ID;
	std::string sId = Format("%d", buttonId);
	auto itr = std::find_if(m_vLines.cbegin(),
		m_vLines.cend(),
		[ip, sId](std::vector<std::string> columns)
	{
		bool res = false;
		std::string value;
		if (CommGetColumnValue(columns, ip, value))
		{
			res = (value == sId);
		}
		return res;
	});
	if (itr == m_vLines.cend())
		return pCont;
	return GetContent(*itr);
}
/// <summary>
/// グループ／ページ内ボタン（デバイス）設定要素取得
/// </summary>
/// <param name="contents"></param>
/// <returns></returns>
int GroupPage::GetContents(std::vector<DeviceContent*>& contents)
{
	contents.clear();
	if (m_vLines.empty())
		return 0;

	std::vector<DeviceContent*> conts;
	conts.clear();
	for (const auto& cont : m_vLines)
	{
		conts.push_back(GetContent(cont));
		if (conts.size() >= INT_MAX)
			break;
	}
	if (!conts.empty())
		contents.swap(conts);

	return (int)contents.size();
}


// ====================================================================

/// <summary>
/// コンストラクタ
/// </summary>
CDeviceContents::CDeviceContents()
{
	m_pNMosEmberInfo = nullptr;
	m_pMvEmberInfo1 = nullptr;
	m_pMuteAll = nullptr;
	// メンバ初期化
	Initialize(true);
}
/// <summary>
/// コンストラクタ
/// </summary>
CDeviceContents::CDeviceContents(std::string path)
{
	m_pNMosEmberInfo = nullptr;
	m_pMvEmberInfo1 = nullptr;
	m_pMuteAll = nullptr;
	// メンバ初期化
	Initialize(path);
}
/// <summary>
/// デストラクタ
/// </summary>
CDeviceContents::~CDeviceContents()
{
	if (!m_vGroupPages.empty())
	{
		m_vGroupPages.clear();
	}
	if (m_pNMosEmberInfo) {
		delete m_pNMosEmberInfo;
	}
	if (m_pMvEmberInfo1) {
		delete m_pMvEmberInfo1;
	}
	if (m_pMuteAll) {
		delete m_pMuteAll;
	}
}

/// <summary>
/// メンバ初期化
/// </summary>
void CDeviceContents::Initialize(bool setflg)
{
	m_bInitialized = false;
	if (!m_sPath.empty())
		m_sPath.clear();
	if (!m_vGroupPages.empty())
		m_vGroupPages.clear();
	m_bHasTakeButtonEnable = false;
	m_bTakeButtonEnable = true;
	if (!m_sMatrixPath.empty())
		m_sMatrixPath.clear();
	if (!m_sInnerMatrixPath.empty())
		m_sInnerMatrixPath.clear();
	m_nTakeCount = false;
	m_nDestCount = false;
	m_nSourceCount = false;

	m_bInitialized = true;
}
/// <summary>
/// メンバ初期化
/// </summary>
/// <param name="path"></param>
void CDeviceContents::Initialize(std::string path)
{
	Initialize(false);
	std::vector<GroupPage*> gps;
	gps.clear();

	try
	{
		// ファイル内容が取得できた場合のみデータ取得する
		int cntTake = 0, cntDest = 0, cntSource = 0;
		do
		{
			// ファイル内容を取得
			m_sPath = path;

			std::vector<std::vector<std::string>> lines{};
			int len = CommReadCsvFile(m_sPath, lines, false);
			if (len <= 0)
			{
				lines.clear();
				break;
			}

			bool extinh = false;
			std::vector<std::vector<std::string>> inhs{};
			try
			{
				//int len = CommReadCsvFile(CClientConfig::GetInstance()->InhibitValuesPath(), inhs, true);
				extinh = !inhs.empty();
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());

				if (!inhs.empty())
					inhs.clear();
			}

			// グループ／ページで分割する
			// ラベルがあるなら出現位置に併せて変更すべき
			int gp = (int)CsvColumnId::COLUMN_GROUP;
			int pp = (int)CsvColumnId::COLUMN_PAGE;
			int bp = (int)CsvColumnId::COLUMN_BUTTON_ID;

			auto itr = lines.begin();
			while (itr != lines.end())
			{
				std::vector<std::string> columns = *itr;

				std::string gs, ps, bs;
				int gn, pn, bn;
				CommGetColumnValue(columns, gp, gs);
				CommGetColumnValue(columns, pp, ps);
				CommGetColumnValue(columns, bp, bs);

				// 先に動作属性情報かを確認する
				if (ToNumber(gs, gn) && ToNumber(ps, pn) && ToNumber(bs, bn)
				 && (gn == -1) && (pn == -1) && (bn == -1))
				{
					// 識別を確認する
					std::string sAttrId{};
					std::string sAttrVal1{};
					std::string sAttrVal2{};
					std::string sAttrVal3{};
					CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_ATTR_ID, sAttrId);
					CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_ATTR_VALUE1, sAttrVal1);
					CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_ATTR_VALUE2, sAttrVal2);
					CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_ATTR_VALUE3, sAttrVal3);
					if (sAttrId == ATTR_TAKE_BUTTON_ENABLE)
					{
						// 先行出現優先
						if (!m_bHasTakeButtonEnable)
						{
							// ButtonEnable のデフォルトは true
							bool ena = false;
							m_bTakeButtonEnable = ToBool(sAttrVal1, ena) ? ena : true;
							// 無効の場合のみパスを取る
							if (!m_bTakeButtonEnable)
								m_sMatrixPath = sAttrVal2;

							m_bHasTakeButtonEnable = true;
						}
					}
					else if (sAttrId == ATTR_MUTE) {
						bool mute = false;
						m_pMuteAll = new MuteInfo();
						m_pMuteAll->m_bMuteAll = ToBool(sAttrVal1, mute) ? mute : false;
					}
					else if (sAttrId == ATTR_MVEMBER1) {
						// sAttrVal1 192.168.183.100:5000:T:F
						std::vector<std::string> ad = split(sAttrVal1, ':');
						if (ad.size() >= 4) {
							unsigned portno; ToNumber(ad[1], portno);
							bool enable = false, matrixlabel=true;
							m_pMvEmberInfo1= new MvEmberInfo();
							m_pMvEmberInfo1->m_sMvEmberIpAddr=ad[0];
							m_pMvEmberInfo1->m_nMvEmberPort = portno;
							m_pMvEmberInfo1->m_bMvEmberEnabled = ToBool(ad[2], enable) ? enable : true;
							m_pMvEmberInfo1->m_bMvEmberUseMatrixLabels = ToBool(ad[3], matrixlabel) ? matrixlabel : true;
						}
					}
					else if (sAttrId == ATTR_NMOSEMBER) {
						// sAttrVal1 192.168.183.100:5000:1:1
						std::vector<std::string> ad = split(sAttrVal1, ':');
						if (ad.size() >= 4) {
							unsigned portno; ToNumber(ad[1], portno);
							bool enable = false, matrixlabel = true;
							m_pNMosEmberInfo = new NMosEmberInfo();
							m_pNMosEmberInfo->m_sNmosEmberIpAddr = ad[0];
							m_pNMosEmberInfo->m_nNmosEmberPort = portno;
							m_pNMosEmberInfo->m_bNmosEmberEnabled = ToBool(ad[2], enable) ? enable : true;
							m_pNMosEmberInfo->m_bNmosEmberUseMatrixLabels = ToBool(ad[3], matrixlabel) ? matrixlabel : true;
						}
					}
					// 転載したので消す
					itr = lines.erase(itr);
					continue;
				}

				if (ToNumber(gs, gn) && ToNumber(ps, pn)
				 && IsRange(gn, 0, GROUP_MAX) && IsRange(pn, 0, PAGE_MAX))
				{
					// インヒビットのデータがあるなら
					// csv 内容を上書きする
					if (extinh)
					{
						auto _itr = std::find_if(inhs.cbegin(), inhs.cend(),
												 [gn, pn, bn](std::vector<std::string> _columns)
						{
							std::string _gs, _ps, _bs;
							int _gn, _pn, _bn;
							CommGetColumnValue(_columns, (int)CsvColumnId::COLUMN_GROUP, _gs);
							CommGetColumnValue(_columns, (int)CsvColumnId::COLUMN_PAGE, _ps);
							CommGetColumnValue(_columns, (int)CsvColumnId::COLUMN_BUTTON_ID, _bs);

							// 作業中の行内3カラムが一致するかを確認する
							return (ToNumber(_gs, _gn) && ToNumber(_ps, _pn) && ToNumber(_bs, _bn)
								&& (_gn == gn) && (_pn == pn) && (_bn == bn));
						});
						// 行末尾がinhibit
						bool _binh = false;
						bool binh = (_itr != inhs.cend()) && (3 < (*_itr).size()) && (ToBool((*_itr).back(), _binh) && _binh);
						columns[(int)CsvColumnId::COLUMN_INHIBIT] = std::to_string((int)binh);
					}

					// グループページが既存か確認する
					GroupPage* targetgp = GetGroupPage(gps, gn, pn);
					if (!targetgp)
					{
						// なければ追加
						gps.push_back(new GroupPage(gn, pn, std::vector<std::vector<std::string>>()));
						// 追加したインスタンス
						targetgp = gps.back();
					}
					// グループページに転載
					targetgp->m_vLines.push_back(columns);

					// マトリックス設定有無確認
					{
						std::unique_ptr<DeviceContent> lineContent;
						lineContent.reset(targetgp->GetContent(columns));
						if (lineContent)
						{
							switch (lineContent->m_eFunctionId)
							{
							case FunctionId::FUNC_TAKE:
								// パスの控えがない場合のみパスを取る
								// （扱うマトリックスは1つだけという仕様による
								// 　／ツリー側では複数マトリックスを保持している可能性があり判断できない）
								if (m_sInnerMatrixPath.empty())
									m_sInnerMatrixPath = lineContent->m_sArg1;
								++cntTake;
								break;
							case FunctionId::FUNC_DEST:
								++cntDest;
								break;
							case FunctionId::FUNC_SRC:
								++cntSource;
								break;
							}
						}
						lineContent.reset();
					}
				}

				// 転載したので消す
				itr = lines.erase(itr);
			}

			lines.clear();
		} while (false);

		if (!gps.empty())
		{
			m_vGroupPages.swap(gps);
			m_nTakeCount = cntTake;
			m_nDestCount = cntDest;
			m_nSourceCount = cntSource;
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());

		if (!gps.empty())
		{
			auto itr = gps.begin();
			while (itr != gps.end())
			{
				if (*itr)
					delete *itr;
				itr = gps.erase(itr);
			}
		}
	}

	m_bInitialized = true;
}

/// <summary>
/// グループ／ページファイル定義内容
/// </summary>
/// <param name="vGroupPage"></param>
/// <param name="group"></param>
/// <param name="page"></param>
/// <returns></returns>
GroupPage* CDeviceContents::GetGroupPage(std::vector<GroupPage*>& vGroupPage, const int group, const int page)
{
	GroupPage* pGroupPage = nullptr;

	// グループページが既存か確認する
	auto itr = std::find_if(vGroupPage.begin(),
							vGroupPage.end(),
							[group, page](GroupPage* gp)
							{
								return gp && gp->m_nGroup == group && gp->m_nPage == page; }
							);
	if (itr != vGroupPage.end())
	{
		// 既存のインスタンス
		pGroupPage = *itr;
	}

	return pGroupPage;
}

/// <summary>
/// グループ／ページ内ボタン（デバイス）設定要素
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <returns></returns>
int CDeviceContents::GetGroupPageContents(const int group, const int page, std::vector<DeviceContent*>& contents)
{
	contents.clear();
	GroupPage* pGroupPage = GetGroupPage(m_vGroupPages, group, page);
	if (!pGroupPage)
		return 0;

	std::vector<DeviceContent*> conts;
	conts.clear();
	int len = pGroupPage->GetContents(conts);
	if (len > 0)
		contents.swap(conts);

	return len;
}

/// <summary>
/// インヒビット更新
/// </summary>
/// <param name="group"></param>
/// <param name="page"></param>
/// <param name="buttonId"></param>
/// <param name="inhibit"></param>
/// <returns></returns>
bool CDeviceContents::UpdateInhibit(const int group, const int page, const int buttonId, const bool inhibit)
{
	bool res = false;
	GroupPage* pGroupPage = GetGroupPage(m_vGroupPages, group, page);
	if (!pGroupPage || pGroupPage->m_vLines.empty())
		return res;

	int ip = (int)CsvColumnId::COLUMN_BUTTON_ID;
	std::string sId = Format("%d", buttonId);
	auto itr = std::find_if(
		pGroupPage->m_vLines.begin(), pGroupPage->m_vLines.end(),
		[ip, sId](std::vector<std::string> columns)
	{
		bool res = false;
		std::string value;
		if (CommGetColumnValue(columns, ip, value))
		{
			res = (value == sId);
		}
		return res;
	});
	if (itr == pGroupPage->m_vLines.cend())
		return res;
	itr[0][(int)CsvColumnId::COLUMN_INHIBIT] = std::to_string((int)inhibit);

	return true;
}
/// <summary>
/// 書き出し用インヒビット情報
/// </summary>
int CDeviceContents::CreateOutputInhibits(std::stringstream& ss)
{
	ss.clear();
	int cnt = 0;
	for (int g = 0; g <= GROUP_MAX; ++g)
	{
		for (int p = 0; p <= PAGE_MAX; ++p)
		{
			GroupPage* pGroupPage = GetGroupPage(m_vGroupPages, g, p);
			if (!pGroupPage || pGroupPage->m_vLines.empty())
				continue;
#if true	// 全行を出力対象にしないと、true→falseになったものが分からない
			for (std::vector<std::string> columns : pGroupPage->m_vLines)
			{
				std::string val = ((int)CsvColumnId::COLUMN_INHIBIT < columns.size())
								? columns[(int)CsvColumnId::COLUMN_INHIBIT]
								: "0";
				// 頭3カラムをそのまま使用
				auto sline = columns[0] + "," + columns[1] + "," + columns[2] + "," + val + "\n";
				ss.write(sline.c_str(), sline.size());
				++cnt;
			}
#else
			std::vector<std::vector<std::string>> vLines{};
			std::copy_if(pGroupPage->m_vLines.cbegin(), pGroupPage->m_vLines.cend(),
						 std::back_inserter(vLines), [](std::vector<std::string> columns)
			{
				bool res = false;
				bool sw = false;
				std::string value;
				if (CommGetColumnValue(columns, (int)CsvColumnId::COLUMN_INHIBIT, value))
				{
					res = ToBool(value, sw) && sw;
				}
				return res;
			});
			if (vLines.empty())
				continue;

			for (std::vector<std::string> columns : vLines)
			{
				// 頭3カラムをそのまま使用
				auto val = columns[0] + "," + columns[1] + "," + columns[2] + "\n";
				ss.write(val.c_str(), val.size());
				++cnt;
			}
#endif
		}
	}

	return cnt;
}
