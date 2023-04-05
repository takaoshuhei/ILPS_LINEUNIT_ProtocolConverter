/// <summary>
/// CSVファイル操作
/// </summary>

#include "ClientConfig.h"
#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <filesystem>
#include <iterator>
#include <locale>
#include <codecvt>
#ifndef WIN32
#include "IConvWrapper.h"
#endif

/// <summary>
/// utilities 名前空間
/// </summary>
namespace utilities
{
	
	static const std::vector<char> invalidChars = { '\t' };
	static const std::vector<char> lineTerms = { '\r', '\n' };
	static const std::vector<char> quotes = { '\'', '\"', '`' };

	static char csvDelimiter = ',';
	static int csvColumnMax = 16;
	static const int csvLineMax = UINT16_MAX;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
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
	/// <remarks>
	/// ファイル有効範囲
	/// - NULL が出現した場合、以降データは破棄する
	/// - 行末文字（下記参照）を除く空白文字(0x20)未満の文字は無視、タブ '\t' も同条件
	/// - ファイル先頭のみ BOM 除去を考慮する
	/// 
	/// 行判断
	/// - 0x0a-0x0d('\n','\v','\f','\r') は出現順に拘らず行末とみなす
	/// - 引用符を除いた '0' 未満の文字が行頭に2つ並んでいた場合はコメント行とみなす
	/// 
	/// 行内カラム判断
	/// - デリミタは原則 ','
	/// - トリミングはタブ('\t')を除き原則実施しない
	/// - 引用符による囲み判断は行わない
	/// </remarks>
	int CommReadCsvFile(std::string path, std::vector<std::vector<std::string>>& lines, bool useMultibyte)
	{
		CClientConfig* _ClientConfig = CClientConfig::GetInstance();
		assert(_ClientConfig != nullptr);
		bool bMultibyteFormat = _ClientConfig->MuitibyteFormat() || useMultibyte;

		lines.clear();

		std::vector<char> rawColumn;
		std::vector<std::vector<char>> rawSplitLine;
		std::vector<std::vector<std::vector<char>>> rawLines;

		try
		{
			if (path.empty())
			{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "empty path.\n");
				return 0;
#else
				throw new std::invalid_argument("[le]empty path");
#endif
			}
			if (!PathExists(path))
			{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "file is not exists, path = %s.\n", path.c_str());
				return 0;
#else
				throw new std::invalid_argument("[le]path not exists");
#endif
			}
			auto filesize = std::filesystem::file_size(std::filesystem::path(path));
			if (filesize == 0)
			{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "empty size file, path = %s.\n", path.c_str());
				return 0;
#else
				throw new std::invalid_argument("[le]empty file");
#endif
			}

			std::ifstream ifs(path);
#ifdef WIN32
			std::wifstream wifs(path);
			if (bMultibyteFormat)
#endif
			{
				if (ifs.fail())
				{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed open file, path = %s.\n", path.c_str());
					return 0;
#else
					throw new std::invalid_argument(("[le]failed open ifstream, path = " + path).c_str());
#endif
				}
			}
#ifdef WIN32
			else
			{
				if (wifs.fail())
				{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed open file, path = %s.\n", path.c_str());
					return 0;
#else
					throw new std::invalid_argument(("[le]failed open ifstream, path = " + path).c_str());
#endif
				}
			}
#endif

			rawColumn.clear();
			rawSplitLine.clear();
			rawLines.clear();

			// for _line
			char preComment = 0;
			bool commentLine = false;
			// for _column
			uint16_t topSpaceCnt = 0;
			bool eof = false;	// for binary null data

			// for utf-8 with bom
			// メモリストリーム代わりに文字列ストリーム使用
			std::stringstream ss{};
			if (!bMultibyteFormat)
			{
				Trace(__FILE__, __LINE__, __FUNCTION__, "start decode..\n");
#ifdef WIN32
				{
					std::wstring wstr{};
					try
					{
						wifs.imbue(std::locale(std::locale(""), new std::codecvt_utf8_utf16<wchar_t, 0x10ffff, std::consume_header>()));
						if (wifs)
						{
							std::wstringstream _ss{};
							_ss << wifs.rdbuf();
							wstr = _ss.str();
						}
					}
					catch (...) {}
					if (!wstr.empty())
					{
						if (CreateMultibyteStream(wstr, ss) <= 0)
						{
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
							ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "failed convert utf-8 to sjis, path = %s.\n", path.c_str());
							return 0;
#else
							throw new std::invalid_argument(("[le]failed convert utf-8 to sjis, path = " + path).c_str());
#endif
						}
					}
				}
#else
				{
					// BOM があれば位置をずらす
					if (filesize > 3)
					{
						constexpr unsigned char bom[] = { 0xEF, 0xBB, 0xBF };						
						unsigned char pre[3]{};
						for (auto& i : pre) i = ifs.get();
						if (!std::equal(std::begin(pre), std::end(pre), bom))
							ifs.seekg(0);
					}

					// streambuf に文字列部分のみ受けかえ
					// 元 stream の rdbuf() はどのタイミングで呼び出してもアドレスは変わらないが
					// 先の get()/seekg() による位置移動有無で出力側への反映内容が変わる
					std::stringstream _ss{};
					_ss << ifs.rdbuf();
					// 文字列化してデータ参照
					std::string _str = _ss.str();
					const char* pBuff = _str.data();
					auto dlen = strlen(pBuff);
					if (dlen > 0)
					{
						// コード変換
						std::string sres = IConvWrapper::ConvertU8ToSJis<std::string>(pBuff, dlen);
						if (!sres.empty())
						{
							// 読み取り用文字列ストリームに設定
							ss << sres;
							/***
							// for debug
							{
								std::filesystem::path pSJisFile = std::filesystem::path(path).parent_path().append("startup_sjis" + DateTimeFormat(std::chrono::system_clock::now(), "%Y%m%d%H%M%S") + ".csv");
								std::ofstream ofs(pSJisFile);
								//ofs << ss.rdbuf();	// 出力ファイル内容は sres 指定と同じになるが参照位置がずれて下記 while での分割ができない
								ofs << sres;
								ofs.close();
							}
							***/
						}
						else
						{
							// データがあるのに変換に失敗する場合
							// 元々 sjis だったとしてデータを扱う
							ifs.seekg(0);
							ss << ifs.rdbuf();
						}
					}
				}
#endif
				Trace(__FILE__, __LINE__, __FUNCTION__, "exit decode.\n");
			}
			else
			{
				ss << ifs.rdbuf();
			}

			while (!ss.eof() && !eof)
			{
				unsigned char ch;
				ss.read((char*)&ch, sizeof(char));

				// null
				eof = (ch == 0);
				// 行末
				bool lineTerm = (std::find(lineTerms.cbegin(), lineTerms.cend(), ch) != lineTerms.cend());
				// デリミタ
				bool delimiter = (ch == csvDelimiter);
				// 無効文字
				bool invalidChar = !eof && !lineTerm && !delimiter && (ch < 0x20);
				// 引用符
				bool quote = (std::find(quotes.cbegin(), quotes.cend(), ch) != quotes.cend());

				// コメント候補
				if (!commentLine && rawSplitLine.empty())
				{
					if (rawColumn.empty() && !eof && !lineTerm && !delimiter && !quote && (ch < '0'))
						preComment = ch;
					if (!rawColumn.empty() && preComment)
					{
						commentLine = (preComment == ch);
						preComment = 0;

						if (commentLine)
						{
							rawColumn.clear();
						}
					}
				}

				// この時点でコメント行、無効文字なら次の文字へ進む
				if ((commentLine || invalidChar) && !(eof || lineTerm))
					continue;

				// 以下カラム内判断

				// 先頭空白判定（引用符出現時用）
				if (ch == ' ')
				{
					if (rawColumn.size() == topSpaceCnt)
						++topSpaceCnt;
				}

				// 末尾判定
				if (eof || lineTerm || delimiter)
				{
					// 改行のみなら無視
					if ((eof || lineTerm) && rawColumn.empty() && rawSplitLine.empty())
					{
						commentLine = false;
					}
					else
					{
						// カラムを閉じる
						rawSplitLine.push_back(std::vector<char>(rawColumn));
						rawColumn.clear();

						// 行末出現なら行を閉じる
						if (eof || lineTerm)
						{
							rawLines.push_back(std::vector<std::vector<char>>(rawSplitLine));
							rawSplitLine.clear();

							// 
							if (!eof && rawSplitLine.size() >= csvLineMax)
								eof = true;
						}
					}
				}
				else
					// 末尾に追加
					rawColumn.push_back(ch);

				if (eof)
					break;
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			if (!rawLines.empty())
			{
				rawLines.clear();
			}
		}

		// 
		for (const auto& _line : rawLines)
		{
			std::vector<std::string> line;
			line.clear();
			for (const auto& _column : _line) 
			{
				size_t len = _column.size();
				std::string value = "";
				if (len > 0)
				{
					value = std::string(_column.cbegin(), _column.cend());

					if ((len >= 2)
					 && (_column.front() == _column.back())
					 && (std::find(quotes.cbegin(), quotes.cend(), _column.front()) != quotes.cend()))
					{
						if (len > 2)
							value = value.substr(1, len - 2);
					}
				}
				line.push_back(value);

				if (line.size() >= csvColumnMax)
					break;
			}
			lines.push_back(line);
		}
		rawColumn.clear();
		rawSplitLine.clear();
		rawLines.clear();

		if (lines.size() > csvLineMax)
			lines.resize(csvLineMax);
		return (int)lines.size();
	}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	/// <summary>
	/// 指定カラム位置取得
	/// </summary>
	/// <param name="columns"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	int CommGetColumnPosition(const std::vector<std::string> columns, std::string value)
	{
		int position = -1;

		auto itr = std::find(columns.cbegin(), columns.cend(), value);
		if (itr != columns.cend())
		{
			auto _pos = std::distance(columns.cbegin(), itr);
			if (_pos <= csvColumnMax)
				position = (int)_pos;
		}

		return position;
	}
	/// <summary>
	/// 指定位置カラム内容取得
	/// </summary>
	/// <param name="columns"></param>
	/// <param name="position"></param>
	/// <param name="value"></param>
	/// <returns>
	/// true/false
	/// </returns>
	bool CommGetColumnValue(const std::vector<std::string> columns, int position, std::string& value)
	{
		value.clear();
		bool res = false;

		if ((0 <= position) && ((unsigned)position < columns.size()))
		{
			value = columns[position];
			res = true;
		}

		return res;
	}
}
