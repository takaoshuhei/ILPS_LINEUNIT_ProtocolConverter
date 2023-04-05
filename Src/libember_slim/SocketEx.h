#pragma once

#if defined WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#define closesocket close
typedef int SOCKET;
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif
#define SOCKET_ERROR (-1)

#ifdef __cplusplus
#include <string>


/// <summary>
/// utilities 名前空間
/// </summary>
namespace utilities
{
    /// <summary>
    /// ソケット情報生成
    /// </summary>
    /// <param name="id">Client 用ソケット識別</param>
    /// <param name="socketHandle">ソケットハンドル</param>
    /// <param name="sad">接続先情報</param>
    /// <returns></returns>
    extern bool CreateSocketHandle(std::string ipaddr, unsigned short port, SOCKET& socketHandle, struct sockaddr_in& sad);
}
#endif
