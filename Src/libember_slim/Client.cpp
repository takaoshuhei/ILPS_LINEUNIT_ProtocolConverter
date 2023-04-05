#include "Client.h"
#include "APIFormat.h"
#include "ClientConfig.h"
#include "DeviceAction.h"
#include "Utilities.h"
#include "EmberConsumer.h"
#include "EmberInfo.h"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <mutex>
#include <process.h>

#define BUFFERSIZE 512
#define PROTOPORT 5193 // Default port number

using namespace utilities;

std::string ExecutableFilePath = ""; //クライアントorサーバーで起動するかの判別文字列なので多分不要
int ProcessId = 0;
int testcnt = 0;

int before_send_val = 0;
bool longPushFlag = false;

SOCKET ActiveClientSock = 0;

void ClearWinSock() {
#if defined WIN32
	WSACleanup();
#endif
}

/// <summary>LINE UNIT初期化</summary>
/// <returns></returns>
void UnitInitialize(SOCKET sock)
{
	//パレット色の初期設定
	const char* SendPalette1Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x00\x00\x00\x03";
	const char* SendPalette2Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x01\x00\x05\x03";
	const char* SendPalette3Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x02\x04\x05\x03";
	const char* SendPalette4Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x03\x01\x05\x03";
	const char* SendPalette5Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x04\x00\x01\x03";
	const char* SendPalette6Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x05\x04\x01\x03";
	const char* SendPalette7Data = "\x46\x4F\x52\x41\x00\x06\x08\x00\x06\x01\x01\x03";
	send(sock, SendPalette1Data, 12, 0);
	send(sock, SendPalette2Data, 12, 0);
	send(sock, SendPalette3Data, 12, 0);
	send(sock, SendPalette4Data, 12, 0);
	send(sock, SendPalette5Data, 12, 0);
	send(sock, SendPalette6Data, 12, 0);
	send(sock, SendPalette7Data, 12, 0);

	//点灯状態をクリア
	//PGM列制御
	for (int i = 1; i < 26; i++)
	{
		char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 0 };
		send(sock, cmd, sizeof(cmd), 0);
	}
	//PST列制御
	for (int i = 46; i < 71; i++)
	{
		char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 0 };
		send(sock, cmd, sizeof(cmd), 0);
	}
	//XPT列制御
	for (int i = 91; i < 116; i++)
	{
		char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 0 };
		send(sock, cmd, sizeof(cmd), 0);
	}
	for (int i = 136; i < 161; i++)
	{
		char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 0 };
		send(sock, cmd, sizeof(cmd), 0);
	}

}

/// <summary>LINE UNIT->Ember+プロトコルへの変換</summary>
/// <returns></returns>
void LineUnitCommand(SOCKET sock, char* recvBuffer)
{
	unsigned char* u_recvBuffer = (unsigned char*)recvBuffer;

	GlowValue* pValue = nullptr;
	GlowParameter* pParameter = nullptr;
	RequestId requestId = { 0 };
	berint* pPath = nullptr;

	//コマンド番号以降のデータ長をバイトカウントから取得
	int ByteCount = (u_recvBuffer[4] << 8) + u_recvBuffer[5];

	//コマンド番号と内容を抽出
	switch (u_recvBuffer[6])
	{
	case 0:
		//スイッチコマンド
		if (ByteCount == 3)
		{
			//状態要求コマンド
		}
		else if (ByteCount == 4)
		{
			//設定コマンド
			//スイッチ番号
			int num = (u_recvBuffer[7] << 8) + u_recvBuffer[8];
			//スイッチ状態
			int status = u_recvBuffer[9];

			//ILPS側送信処理
			std::string Value_btn = "";
			std::string Value_tally = "";
			std::string Path_btn = "";
			std::string Path_tally = "";
			char bright;

			if (status == 0)
			{
				if (num > 0 && num <= 25)
				{
					for (int i = 1; i < 26; i++)
					{
						char cmd[9] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x03, 0x00, (i >> 8) & 0xff, i & 0xff};
						send(sock, cmd, sizeof(cmd), 0);
					}

					longPushFlag = true;
				}
				else if (num == 210 || num == 211)
				{
					//CUT or AUTOトランジション
					//ボタンから離したタイミングでトランジション実行
					if (num == 210) Path_btn = "/root/suite/s-1/switcher/n-1/abTrans/n-#/trans/cut";
					else Path_btn = "/root/suite/s-1/switcher/n-1/abTrans/n-#/trans/auto";
					Value_btn = "0";

					//CUT,AUTOボタンは微灯
					char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, recvBuffer[7], recvBuffer[8], 6 };
					send(sock, cmd, sizeof(cmd), 0);

					int len = (int)m_pNmosEmberConsumer->GetNodePath(Path_btn, &pPath);
					GlowInvocation* pInvocation = newobj(GlowInvocation);
					bzero_item(*pInvocation);
					Call_handleInput(m_pNmosEmberConsumer->CreateInvokeRequest(&requestId, pPath, len, *pInvocation));

					glowInvocation_free(pInvocation);
				}
			}
			else
			{
				if (status == 2 && !longPushFlag)
					break;

				if (num > 0 && num <= 25)
				{
					//PGM列制御
					Path_btn = "/root/suite/s-1/switcher/n-1/abTrans/n-#/sw/t/btn";
					Path_tally = "/root/suite/s-1/switcher/n-1/abTrans/n-#/sw/t/tally";
					Value_btn = std::to_string(num);
					Value_tally = "0";
					int len_btn = (int)m_pNmosEmberConsumer->GetNodePath(Path_btn, &pPath);

					pValue = m_pNmosEmberConsumer->CreateGlowValue(GlowParameterType::GlowParameterType_Integer, Value_btn);
					if (pValue)
					{
						pParameter = newobj(GlowParameter);
						bzero_item(*pParameter);
						glowValue_copyFrom(&pParameter->value, pValue);
						Call_handleInput(m_pNmosEmberConsumer->CreateSetParameterRequest(&requestId, pPath, len_btn, *pParameter));

						glowParameter_free(pParameter);
						pParameter = nullptr;
						freeMemory(pValue);
						pValue = nullptr;
					}
					longPushFlag = false;
				}
				else if (num > 45 && num <= 70)
				{
					//PST列制御
					Path_btn = "/root/suite/s-1/switcher/n-1/abTrans/n-#/sw/b/btn";
					Path_tally = "/root/suite/s-1/switcher/n-1/abTrans/n-#/sw/b/tally";
					Value_btn = std::to_string(num - 45);
					Value_tally = "1";
					int len_btn = (int)m_pNmosEmberConsumer->GetNodePath(Path_btn, &pPath);

					pValue = m_pNmosEmberConsumer->CreateGlowValue(GlowParameterType::GlowParameterType_Integer, Value_btn);
					if (pValue)
					{
						pParameter = newobj(GlowParameter);
						bzero_item(*pParameter);
						glowValue_copyFrom(&pParameter->value, pValue);
						Call_handleInput(m_pNmosEmberConsumer->CreateSetParameterRequest(&requestId, pPath, len_btn, *pParameter));

						glowParameter_free(pParameter);
						pParameter = nullptr;
						freeMemory(pValue);
						pValue = nullptr;
					}
				}
				else if ((num > 90 && num <= 115) || (num > 135 && num <= 160))
				{
					//XPT
					Path_btn = "/root/suite/s-1/switcher/n-1/xptSw/n-#/sw/btn";
					Path_tally = "/root/suite/s-1/switcher/n-1/xptSw/n-#/sw/tally";
					if (num > 90 && num <= 115) Value_btn = std::to_string(num - 90);
					else Value_btn = std::to_string(num - (135 + 25));
					Value_tally = "0";

					int len_btn = (int)m_pNmosEmberConsumer->GetNodePath(Path_btn, &pPath);

					pValue = m_pNmosEmberConsumer->CreateGlowValue(GlowParameterType::GlowParameterType_Integer, Value_btn);
					if (pValue)
					{
						pParameter = newobj(GlowParameter);
						bzero_item(*pParameter);
						glowValue_copyFrom(&pParameter->value, pValue);
						Call_handleInput(m_pNmosEmberConsumer->CreateSetParameterRequest(&requestId, pPath, len_btn, *pParameter));

						glowParameter_free(pParameter);
						pParameter = nullptr;
						freeMemory(pValue);
						pValue = nullptr;
					}
				}
				else if (num == 210 || num == 211)
				{
					//CUT or AUTOトランジション
					//押下時はボタンの点灯のみ実行
					bright = 3;
					char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, recvBuffer[7], recvBuffer[8], bright };
					send(sock, cmd, sizeof(cmd), 0);
				}

			}

		}
		break;

	case 1:
		//スイッチLEDコマンド
		break;
	case 3:
		//フェーダーコマンド
		//LINE UNIT側からは要求コマンドは飛んでこないので設定のみ
		if (ByteCount == 5)
		{
			//設定コマンド
			GlowParameterType type = GlowParameterType::GlowParameterType_Real;
			unsigned int fader_val = ((unsigned char)recvBuffer[9] << 8) + (unsigned char)recvBuffer[10];

			Trace(__FILE__, __LINE__, __FUNCTION__, "fader_val = %d\n", fader_val);

			//LINE UNIT(0～65535) -> ILPS(0～100)
			int send_val = ((double)fader_val / 65535) * 100;

			Trace(__FILE__, __LINE__, __FUNCTION__, "send_val = %d\n", send_val);

			std::string Path = "/root/suite/s-1/switcher/n-1/scene/n-#/nextTrans/fader";
			std::string Val;
			Val = std::to_string(send_val);

			if (before_send_val != send_val)
			{
				int len_btn = (int)m_pNmosEmberConsumer->GetNodePath(Path, &pPath);
				pValue = m_pNmosEmberConsumer->CreateGlowValue(type, Val);
				if (pValue)
				{
					pParameter = newobj(GlowParameter);
					bzero_item(*pParameter);
					glowValue_copyFrom(&pParameter->value, pValue);
					Call_handleInput(m_pNmosEmberConsumer->CreateSetParameterRequest(&requestId, pPath, len_btn, *pParameter));
				}
				before_send_val = send_val;

				glowParameter_free(pParameter);
				pParameter = nullptr;
				freeMemory(pValue);
				pValue = nullptr;
			}

		}
		break;

	default:
		break;
	}
}

/// <summary>Ember+->LINE UNITプロトコルへの変換</summary>
/// <returns></returns>
extern "C" void __EmberCommandConverter(EmberContent* pResult)
{
	Trace(__FILE__, __LINE__, __FUNCTION__, "Command Create Start.\n");
	Trace(__FILE__, __LINE__, __FUNCTION__, "length = %d.\n", pResult->pathLength);

	if (pResult && (pResult->pathLength > 0))
	{
		pstr pathName = convertPath2String(m_pNmosEmberConsumer->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
		Trace(__FILE__, __LINE__, __FUNCTION__, "Path = %s.\n", pathName);
		if (pathName)
		{
			//ひとまずstringにキャスト
			std::string strPath = pathName;

			//フィルター毎に処理分岐
			if (strPath.rfind("abTrans") != std::string::npos)
			{
				if (strPath.rfind("/sw/t/btn") != std::string::npos)
				{
					//PGM列制御
					if (pResult->parameter.value.flag == GlowParameterType::GlowParameterType_Integer)
					{
						//念のためint型かチェックしてから送信
						int btn_num = (int)(pResult->parameter.value.choice.integer);
						char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (btn_num >> 8) & 0xff, btn_num & 0xff, 1 };
						m_pNmosEmberConsumer->end = clock();
						m_pNmosEmberConsumer->ProcessTimeDisp();
						send(ActiveClientSock, cmd, sizeof(cmd), 0);
						for (int i = 1; i < 26; i++)
						{
							if (btn_num != i)
							{
								char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 4 };
								send(ActiveClientSock, cmd, sizeof(cmd), 0);
							}
						}
					}
				}
				else if (strPath.rfind("/sw/b/btn") != std::string::npos)
				{
					//PST列制御
					if (pResult->parameter.value.flag == GlowParameterType::GlowParameterType_Integer)
					{
						//念のためint型かチェックしてから送信
						int btn_num = (int)(pResult->parameter.value.choice.integer) + 45;
						char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (btn_num >> 8) & 0xff, btn_num & 0xff, 2 };
						send(ActiveClientSock, cmd, sizeof(cmd), 0);
						for (int i = 46; i < 71; i++)
						{
							if (btn_num != i)
							{
								char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 5 };
								send(ActiveClientSock, cmd, sizeof(cmd), 0);
							}
						}
					}
				}
			}
			else if (strPath.rfind("xptSw") != std::string::npos)
			{
				if (strPath.rfind("/sw/btn") != std::string::npos)
				{
					//XPT制御
					if (pResult->parameter.value.flag == GlowParameterType::GlowParameterType_Integer)
					{
						//念のためint型かチェックしてから送信
						int btn_num = 0;
						if (pResult->parameter.value.choice.integer < 26)
							btn_num = (int)(pResult->parameter.value.choice.integer) + 90;
						else
							btn_num = (int)(pResult->parameter.value.choice.integer) + 135;

						char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (btn_num >> 8) & 0xff, btn_num & 0xff, 3 };
						send(ActiveClientSock, cmd, sizeof(cmd), 0);
						for (int i = 91; i < 116; i++)
						{
							if (btn_num != i)
							{
								char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 6 };
								send(ActiveClientSock, cmd, sizeof(cmd), 0);
							}
						}
						for (int i = 136; i < 161; i++)
						{
							if (btn_num != i)
							{
								char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x01, (i >> 8) & 0xff, i & 0xff, 6 };
								send(ActiveClientSock, cmd, sizeof(cmd), 0);
							}
						}
					}
				}
			}
			else if (strPath.rfind("scene") != std::string::npos)
			{
				if (strPath.rfind("/nextTrans/fader") != std::string::npos)
				{
					//フェーダー制御
					if (pResult->parameter.value.flag == GlowParameterType::GlowParameterType_Real)
					{
						//念のためint型かチェックしてから送信
						int ember_val = (int)pResult->parameter.value.choice.real;

						//ILPS(0～100) -> LINE UNIT(0～65535)
						int fader_val = (double)ember_val * 65535 / 100;

						//フェーダー状態設定
						char cmd[11] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x05, 0x03, 0x00, 0x00, (fader_val >> 8) & 0xff, fader_val & 0xff };
						send(ActiveClientSock, cmd, sizeof(cmd), 0);

						//フェーダー横LED点灯設定
						//ILPS(0～100) -> LED_MAX(30)
						int led_num_max = (double)ember_val / 100 * 30;
						for (int i = 30; i > 0; i--)
						{
							int led_num = 246 - i + 1;
							if (i <= led_num_max)
							{
								char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x02, (led_num >> 8) & 0xff, led_num & 0xff, 0x02 };
								send(ActiveClientSock, cmd, sizeof(cmd), 0);
							}
							else
							{
								char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x04, 0x02, (led_num >> 8) & 0xff, led_num & 0xff, 0x00 };
								send(ActiveClientSock, cmd, sizeof(cmd), 0);
							}
						}
					}
				}
			}

			freeMemory(pResult);
			freeMemory(pathName);
		}
	}
}


DLLAPI int main()
{
	SOCKET socketHandle = 0;
	SOCKET clientHandle = 0;
	struct sockaddr_in sad { 0 };
	struct sockaddr_in cad { 0 };
	fd_set fdset = { 0 };
	int fdsReady = 0;
	bool connected = false;
	bool firstSended = false;
	struct timeval timeout = { 0, 16 * 1000 }; // 16 milliseconds timeout for select()

	char str[1] = "";
	int cnt = 0;
	char recvBuffer[4096];
	int flag = 0;
	char cmd[10] = { 0x46, 0x4f, 0x52, 0x41, 0x00, 0x03, 0x03, 0x00, 0x00 };

	char HeaderChar[4] = { '\x46', '\x4F', '\x52', '\x41' };

	//プロセスID取得
#if defined WIN32
	ProcessId = _getpid();
#else
	ProcessId = getpid();
#endif

	//winsockスタートアップ実行
#if defined WIN32
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "error at WSASturtup\n");
		return 0;
	}
#endif

	//クライアント情報取得
	CClientConfig* _ClientConfig = CClientConfig::GetInstance();
	assert(_ClientConfig != nullptr);

	Trace(__FILE__, __LINE__, __FUNCTION__, "**************************************************************\n");
	Trace(__FILE__, __LINE__, __FUNCTION__, "       ExecutableFilePath : %s, processId = %d\n", ExecutableFilePath.c_str(), ProcessId);
	Trace(__FILE__, __LINE__, __FUNCTION__, "     ClientConfigFilePath : %s\n", _ClientConfig->Path().c_str());
	Trace(__FILE__, __LINE__, __FUNCTION__, " DefaultDeviceContntsPath : %s\n", _ClientConfig->DefaultDeviceContentsPath().c_str());
	Trace(__FILE__, __LINE__, __FUNCTION__, "StartupDeviceContentsPath : %s\n", _ClientConfig->StartupDeviceContentsPath().c_str());
	Trace(__FILE__, __LINE__, __FUNCTION__, "           LogFileEnabled : %d\n", _ClientConfig->LogFileEnabled());
	Trace(__FILE__, __LINE__, __FUNCTION__, "     SocketReconnectDelay : %d\n", _ClientConfig->SocketReconnectDelay());
	Trace(__FILE__, __LINE__, __FUNCTION__, "          MainThreadDelay : %d\n", _ClientConfig->MainThreadDelay());
	Trace(__FILE__, __LINE__, __FUNCTION__, "         EmberThreadDelay : %d\n", _ClientConfig->EmberThreadDelay());
	Trace(__FILE__, __LINE__, __FUNCTION__, "               HwifIpAddr : %s\n", _ClientConfig->HwifIpAddr().c_str());
	Trace(__FILE__, __LINE__, __FUNCTION__, "                 HwifPort : %d\n", _ClientConfig->HwifPort());
	Trace(__FILE__, __LINE__, __FUNCTION__, "              HwifEnabled : %d\n", _ClientConfig->HwifEnabled());
	Trace(__FILE__, __LINE__, __FUNCTION__, "          NmosEmberIpAddr : %s\n", _ClientConfig->NmosEmberIpAddr().c_str());
	Trace(__FILE__, __LINE__, __FUNCTION__, "            NmosEmberPort : %d\n", _ClientConfig->NmosEmberPort());
	Trace(__FILE__, __LINE__, __FUNCTION__, "         NmosEmberEnabled : %d\n", _ClientConfig->NmosEmberEnabled());
	Trace(__FILE__, __LINE__, __FUNCTION__, "            MvEmberIpAddr : %s\n", _ClientConfig->MvEmberIpAddr().c_str());
	Trace(__FILE__, __LINE__, __FUNCTION__, "              MvEmberPort : %d\n", _ClientConfig->MvEmberPort());
	Trace(__FILE__, __LINE__, __FUNCTION__, "           MvEmberEnabled : %d\n", _ClientConfig->MvEmberEnabled());

	Trace(__FILE__, __LINE__, __FUNCTION__, "start.\n");

	// Ember コンシューマ機能の準備
	m_pNmosEmberConsumer = _ClientConfig->NmosEmberEnabled() ? new CNmosEmberConsumer() : nullptr;

	SOCKET sock = m_pNmosEmberConsumer->m_sRemoteContent.hSocket;

	auto reconnDelay = std::chrono::milliseconds(_ClientConfig->SocketReconnectDelay());
	auto emptyDelay = std::chrono::milliseconds(_ClientConfig->MainThreadDelay());

	for (;;)
	{
		//main roop 3355OU側の処理
		// 非接続ならサーバー生成
		if (!connected)
		{
			// ハンドルがなければ再生成
			if (socketHandle == 0)
			{
				// ソケット生成失敗時は動作停止
				if (!_ClientConfig->CreateHwifSocket(socketHandle, sad)
					|| (socketHandle == 0))
				{
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "HWIF socket creation failed.\n");
					return 0;
				}
			}

			//sad.sin_family = AF_INET;
			//sad.sin_port = htons(53278);
			sad.sin_addr.s_addr = INADDR_ANY;

			if (bind(socketHandle, (struct sockaddr*)&sad, sizeof(sad)) < 0)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "socket can not bind.\n");
				return 0;
			}
			if (listen(socketHandle, 5) < 0) {
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "socket listen failed.\n");
				return 0;
			}

			int cadlen = sizeof(cad);
			if ((clientHandle = accept(socketHandle, (struct sockaddr*)&cad, (socklen_t*)&cadlen)) < 0)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "socket accepted failed.\n");
				return 0;
			}

			// 接続したら初期設定要求
			ActiveClientSock = clientHandle;
			connected = true;
			firstSended = false;
			continue;
		}

		if (!firstSended)
		{
			//LINE UNITの初期設定
			UnitInitialize(clientHandle);
			Trace(__FILE__, __LINE__, __FUNCTION__, "complete first send.\n");
			firstSended = true;
		}

		if (cnt == 1)
		{
			send(clientHandle, cmd, sizeof(cmd), 0);
			cnt = 0;
		}
		else
		{
			cnt++;
		}

		// 非受信時、待機せずにループを回す
		FD_ZERO(&fdset);
		FD_SET(clientHandle, &fdset);
		fdsReady = select((int)(clientHandle + 1), &fdset, NULL, NULL, &timeout);
		if (fdsReady == 1) // socket is ready to read
		{
			if (FD_ISSET(clientHandle, &fdset))
			{
				//LINE UNIT側コマンド受信動作
				int recvLength = recv(clientHandle, (char*)&recvBuffer, sizeof(recvBuffer), 0);

				if (recvLength > 0)
				{
					//ヘッダーを識別
					for (int i = 0; i < strlen(HeaderChar); i++)
					{
						if (recvBuffer[i] != HeaderChar[i])
						{
							//ヘッダーが異なれば次のループを開始
							continue;
						}
					}
					//コマンド番号以降のデータ長をバイトカウントから取得
					Trace(__FILE__, __LINE__, __FUNCTION__, "Data received..\n");
					m_pNmosEmberConsumer->start = clock();
					LineUnitCommand(clientHandle, recvBuffer);
				}
				else
				{
					//切断された時
					Trace(__FILE__, __LINE__, __FUNCTION__, "recv == 0, LINE UNIT lost connection.\n");

					// ソケットハンドルを捨てる
					try
					{
						closesocket(clientHandle);
					}
					catch (...) {}
					clientHandle = 0;

					// 待機後再接続
					connected = false;
					firstSended = false;
					std::this_thread::sleep_for(reconnDelay);
					continue;
				}
			}
		}
		else if (fdsReady < 0)
		{
			//切断された時
			Trace(__FILE__, __LINE__, __FUNCTION__, "fdsReady < 0, lost connection.\n");

			// ソケットハンドルを捨てる
			try
			{
				closesocket(socketHandle);
			}
			catch (...) {}
			socketHandle = 0;

			// 待機後再接続
			connected = false;
			firstSended = false;
			std::this_thread::sleep_for(reconnDelay);
			continue;
		}

		std::this_thread::sleep_for(emptyDelay);
	}
	Trace(__FILE__, __LINE__, __FUNCTION__, "exit main roop.\n");

	if (m_pNmosEmberConsumer)
	{
		m_pNmosEmberConsumer->CancelRequest();
	}
	if (socketHandle != 0)
	{
		try
		{
			closesocket(socketHandle);
		}
		catch (...) {}
	}

	//winsock終了処理
	ClearWinSock();

	return (0);
}