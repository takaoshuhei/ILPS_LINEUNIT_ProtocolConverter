#include "SocketEx.h"
#include "Utilities.h"


/// <summary>
/// utilities 名前空間
/// </summary>
namespace utilities
{
    // ====================================================================
    //
    // ====================================================================

    /// <summary>
    /// ソケット情報生成
    /// </summary>
    /// <param name="id">Client 用ソケット識別</param>
    /// <param name="socketHandle">ソケットハンドル</param>
    /// <param name="sad">接続先情報</param>
    /// <returns></returns>
    bool CreateSocketHandle(std::string ipaddr, unsigned short port, SOCKET& socketHandle, struct sockaddr_in& sad)
    {
        socketHandle = 0;
        memset(&sad, 0, sizeof(struct sockaddr_in));
        bool res = false;

        try
        {
            if (!IsIPv4(ipaddr) || (port == 0))
            {
#if true    // throw を使い呼出元へ戻す行為で強制終了してしまう事象がみられた
                ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "invalid args, ipaddr = %s, port = %d.\n", ipaddr.c_str(), port);
                if (socketHandle != 0)
                    socketHandle = 0;
                return res;
#else
                throw new std::invalid_argument("[le]invalid args");
#endif
            }

            SOCKET sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

            if (sock > 0)
            {
                sad.sin_family = AF_INET;
                inet_pton(sad.sin_family, ipaddr.c_str(), &sad.sin_addr);
                sad.sin_port = htons(port);
                socketHandle = sock;
            }
            else
            {
                ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "socket creation failed.\n");
                //ClearWinSock();
                return res;
            }

            res = (socketHandle != 0);
        }
        catch (const std::exception ex)
        {
            ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
            if (socketHandle != 0)
                socketHandle = 0;
        }

        return res;
    }


    // ====================================================================
}
