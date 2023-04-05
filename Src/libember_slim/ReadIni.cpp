/// <summary>
/// 設定ファイル操作
/// </summary>

#include "Utilities.h"
#include <cassert>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <memory>
#ifdef _MSC_VER
#define strtok_r strtok_s
#else
#include <sys/time.h>
#include <unistd.h>
#endif


/// <summary>
/// utilities 名前空間
/// </summary>
namespace utilities
{
	/// <summary>
	/// ファイル内容ベクタで find_if を使用したキー判断
	/// </summary>
	struct isKey
	{
		std::string key = "";
		isKey(std::string const& value) : key(value) {}

		bool operator()(std::string const& value)
		{
			// 文字列先頭にキーが単語として出現するか
			auto keySize = key.size();
			auto valueSize = value.size();
			return (0 < keySize)
				&& (0 < valueSize)
				&& (value.find(key) == 0)
				&& ((keySize == valueSize)
				 || ((keySize < valueSize)
				  && ((value[keySize] == ' ') || (value[keySize] == '='))));
		}
	};

	/// <summary>
	/// ファイルから読み込み
	/// </summary>
	/// <param name="path">ファイルフルパス</param>
	/// <param name="lines">行単位ファイル内文字列</param>
	/// <returns>
	/// 0 以上 : 有効行数,  
	/// -1 : 指定ファイル不正
	/// </returns>
	int CommReadIniFile(std::string path, std::vector<std::string>& lines)
	{
		lines.clear();
		int len = -1;

		try
		{
			if (path.empty())
			{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "empty path.\n");
				return len;
#else
				throw new std::invalid_argument("[le]empty path");
#endif
			}

			std::ifstream ifs(path);
			if (ifs.fail())
			{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed open file, path = %s\n", path.c_str());
				return len;
#else
				throw new std::invalid_argument(("[le]failed open ifstream, path = " + path).c_str());
#endif
			}

			std::string buf;
			while (getline(ifs, buf))
			{
				// 行端はトリミング
				if (!buf.empty())
				{
					buf = Trim(buf);
				}
				// 空行は無視
				if (!buf.empty())
				{
					char topchr = buf[0];

					// セクション、キー有効行のみ返却対象とする
					// 先頭文字が '0' 未満であればコメント該当として対象外とする
					bool isComment = (topchr < '0');
					if (!isComment)
					{
						bool valid = false;
						if (topchr == '[')
						{
							auto lpos = buf.find(']');
							if ((lpos != std::string::npos) && (lpos > 1))
							{
								if (buf.size() > (lpos + 1))
									buf = buf.substr(0, lpos + 1);
								valid = true;
							}
						}
						else
						{
							valid = true;
						}

						if (valid)
						{
							lines.push_back(buf);
							if (++len >= INT32_MAX)
								break;
						}
					}
				}
			}

			// 2行未満しかデータがなかった場合、データなしとみなす
			if ((len < 2) || (lines.size() < 2))
			{
				if (!lines.empty())
					lines.clear();
				len = 0;
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			if (!lines.empty())
			{
				lines.clear();
				len = -1;
			}
		}

		return len;
	}

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
	int CommGetIniFileData(const std::vector<std::string> lines, std::string section, std::string key, std::string& value)
	{
		value.clear();
		int res = -1;

		// セクションは空白許容
		// キーのみコード依存
		assert(!key.empty());

		// 2行未満の場合データ行なしとみなす
		if (lines.empty() || (lines.size() < 2))
		{
			return res;
		}

		try
		{
			// セクション無しに対応(無条件に該当セクションとする)
			std::string strSection = "";
			ptrdiff_t secidx = -1, nextsecidx = -1;
			std::string keyValue = "";

			// キーの検索範囲を絞る
			// セクション無しの場合は全キー
			auto bitr = lines.cbegin();
			auto eitr = lines.cend();
			if (!section.empty())
			{
				// 検索開始位置を初期化しておく
				bitr = lines.cend();

				// 該当セクションを探す
				strSection = "[" + section + "]";
				auto secitr = find(lines.cbegin(), lines.cend(), strSection);
				if ((secitr != lines.cend()) && ((secitr + 1) != lines.cend()))
				{
					secidx = distance(lines.cbegin(), secitr);

					// 次のセクションを探す
					auto nextsecitr = find_if(secitr + 1,
						lines.cend(),
						[](std::string const& tmp) { return(!tmp.empty() && (tmp[0] == '[')); });
					if (nextsecitr != lines.cend())
						nextsecidx = distance(lines.cbegin(), nextsecitr);

					// セクション下にキー相当行がある場合のみキー検索をする
					if (((secidx >= 0)
					  && ((size_t)secidx < (lines.size() - 1)))	// ←この時点でどちらも0以上
					 && ((nextsecidx < 0)
					  || (nextsecidx > (secidx + 1))))
					{
						// 検索範囲はセクション下のみ
						bitr = secitr + 1;
						eitr = nextsecitr;
						//if (nextsecidx > 1)
						//	eitr--;
					}
				}
			}
			else
			{
				// 検索対象は先頭から末尾まで
			}

			// 検索対象がある場合のみキー検索をする
			if (bitr != lines.cend())
			{
				// 指定 key を探す
				// eitr でなければ、key が単語として出現している
				auto keyitr = find_if(bitr, eitr, isKey(key));
				// '=' より右辺が対象
				size_t epos = (keyitr != eitr) ? (*keyitr).find('=') : std::string::npos;
				//string tmp = (epos != string::npos) ? Trim((*keyitr).substr(epos)) : "";
				std::string tmp = ((epos != std::string::npos) && ((epos + 1) < (*keyitr).length())) ? Trim((*keyitr).substr(epos + 1)) : "";
				if (!tmp.empty())
				{
					// 1文字より長ければ採用条件を確認する
					if (tmp.size() > 1)
					{
						// 先頭に囲み用文字がある場合、同文字が出現するか行末までが抽出対象
						char blkchrs[] = { '\'', '\"', '`' };
						size_t blkchrslen = sizeof(blkchrs) / sizeof(*blkchrs);
						bool exists = std::find(blkchrs, blkchrs + blkchrslen, tmp[0]) != blkchrs + blkchrslen;
						if (exists)
						{
							// 囲みが成立するか
							auto endblkpos = tmp.find(tmp[0], 1);
							if (endblkpos != std::string::npos)
							{
								// 囲み間に文字列がある
								if (endblkpos > 1)
									tmp = tmp.substr(1, endblkpos - 1);
								// ない
								else
									tmp.clear();
							}
							// 囲みが成立しなければそのまま採用
							else
							{
								;
							}
						}
					}
					// 1文字ならそのまま採用
					else
					{
						;
					}
				}
				if (!tmp.empty())
					keyValue = tmp;
			}

			// 入力が認められた場合のみ返却値として設定
			if (!keyValue.empty())
				value = keyValue;
			res = 0;
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}

		return res;
	}
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
	int CommGetIniFileData(std::string path, std::string section, std::string key, std::string& value)
	{
		// ファイル読み込み
		std::vector<std::string> lines;
		int res = CommReadIniFile(path, lines);
		if (res < 0)
		{
			// CommReadIniFile での取得失敗なら先にエラー出力済
			return res;
		}

		// 解析処理
		return CommGetIniFileData(lines, section, key, value);
	}
}
