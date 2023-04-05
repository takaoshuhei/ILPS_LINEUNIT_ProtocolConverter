#ifdef __cplusplus
extern "C" {
#endif

/*
   libember_slim sample

    Copyright (C) 2012-2016 Lawo GmbH (http://www.lawo.com).
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#include "ember_consumer.h"

#include "emberplus.h"
#include "emberinternal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifndef WIN32
#include <unistd.h>


#define Sleep(msec) usleep(msec * 1000)

#ifndef SIZE_MAX
#define SIZE_MAX SSIZE_MAX
#endif
#define _stricmp(val1, val2) strcasecmp(val1, val2)
#define _strdup(pStr) strdup(pStr)
#define _memicmp(val1, val2, length) memcmp(val1, val2, length)

#define strerror_s(_buffer, _bufferSize, _errorNumber) strerror_r(_errorNumber, _buffer, _bufferSize)
#define sprintf_s(_buffre, _bufferSize, format ...)
#endif


// ====================================================================
//
// utils from wrapper
//
// ====================================================================

extern void __Guidance(const char* pFormat, ...);
extern void __Trace(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...);
extern void __ErrorHandler(const char* pFileName, int nLineNumber, const char* pFuncName, const char* pFormat, ...);
extern bool __CharToLongLong(const char* pSrc, long long* pDest);
extern bool __CharToDouble(const char* pSrc, double* pDest);

extern bool __CharToDouble(const char* pSrc, double* pDest);
extern bool __CharToDouble(const char* pSrc, double* pDest);
extern bool __CharToDouble(const char* pSrc, double* pDest);

extern bool __GetQuitConsumerRequest(short socketId);
extern EmberContent* __GetConsumerRequest(short socketId);
extern void __NotifyReceivedConsumerResult(short socketId, EmberContent* pResult);
extern void __EmberCommandConverter(EmberContent* pResult);


// ====================================================================
//
// utils
//
// ====================================================================

#if defined WIN32
static long dump_exception(const char* pFileName, int nLineNumber, const char* pFuncName, PEXCEPTION_POINTERS pExc, int filter_expression)
{
    PEXCEPTION_RECORD rec = pExc->ExceptionRecord;
    __ErrorHandler(pFileName, nLineNumber, pFuncName, 
                   "code:%x flag:%x addr:%p params:%d\n",
                                    rec->ExceptionCode, rec->ExceptionFlags, rec->ExceptionAddress, rec->NumberParameters);
    for (DWORD i = 0; i < rec->NumberParameters; i++)
        /***
        __OUTPUT_LOG(LOGLEVEL_ERR, pFileName, nLineNumber, pFuncName,
            L"param[%d]:%x", i, rec->ExceptionInformation[i]);
        ***/
        __ErrorHandler(pFileName, nLineNumber, pFuncName, "  param[%d]:%lx\n", i, rec->ExceptionInformation[i]);

    return filter_expression;
}
#endif
#if false
static long WINAPI onUnhandledException(PEXCEPTION_POINTERS pExc)
{
    //return dump_exception(__FILEW__, __LINE__, __FUNCTIONW__, pExc, EXCEPTION_CONTINUE_SEARCH);
    return dump_exception(__FILE__, __LINE__, __FUNCTION__, pExc, EXCEPTION_CONTINUE_SEARCH);
}
#endif

static void onThrowError(int error, pcstr pMessage)
{
    __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "called @ ber, error %d: '%s'\n", error, pMessage);
}

static void onFailAssertion(pcstr pFileName, int lineNumber)
{
    __ErrorHandler(pFileName, lineNumber, __FUNCTION__, "called @ ber.\n");
}

static volatile int allocCount = 0;
static void* allocMemoryImpl(size_t size)
{
    if (!size)
        return NULL;

    void* pMemory = malloc(size);
    //if(sizeof(void *) == 8)
    //    printf("allocate %lu bytes: %llX\n", size, (unsigned long long)pMemory);
    //else
    //    printf("allocate %lu bytes: %lX\n", size, (unsigned long)pMemory);

    allocCount++;
    return pMemory;
}
static void freeMemoryImpl(void* pMemory)
{
    if (!pMemory)
        return;
    //if(sizeof(void *) == 8)
    //    printf("free: %llX\n", (unsigned long long)pMemory);
    //else
    //    printf("free: %lX\n", (unsigned long)pMemory);

    allocCount--;
    free(pMemory);
}
/// <summary></summary>
/// <returns></returns>
/// <remarks>
/// サンプルコードは以下の記述であったが
/// 内部で malloc を使用する strdup 利用では
/// allocCount をインクリメントしない
/// #define stringDup(pStr) \
///     ((pStr != NULL) ? _strdup(pStr) : NULL)
/// </remarks>
static pstr stringDup(pcstr pStr)
{
    pstr value = NULL;
    if (pStr != NULL)
    {
        value = _strdup(pStr);
    }
    allocCount++;
    return value;
}
/// <summary>
/// （デバッグ用途）16進表現文字列
/// </summary>
/// <param name="p"></param>
/// <param name="len"></param>
/// <returns></returns>
static char* hex2string(byte* p, int len)
{
    if (len <= 0)
        return NULL;
    // stdio.output デフォルトは1024でそれ以上は例外となる
    // 長い送信パケットの場合、ログとして冗長、100バイト分くらいで十分か
    const int lim = 100;
    bool omit = false;
    char* ret = NULL;
    int bufflen = 0;
    if (len > lim)
    {
        omit = true;
        len = lim;
    }
    bufflen = len * 2 + 10;
    ret = newarr(char, bufflen);
    memset(ret, 0, bufflen);

    for (int i = 0; i < len; i++, p++)
    {
        int wlen = (int)strlen(ret);
        if (bufflen <= wlen)
            break;
        sprintf_s(&ret[i * 2], bufflen - wlen, "%02x", (unsigned)*p);
    }
    if (omit)
    {
        for (int i = 0; i < 3; i++)
            ret[lim * 2 + i] = '.';
    }
    return ret;
}

static bool initializedEmberContents = false;
/// <summary>libember_slim初期処理</summary>
/// <remarks>
/// runConsumer 呼び出し以前に
/// newobj/newarr/allocMemory/freeMemory 使用したい場合に呼び出す
/// </remarks>
DLLAPI void initEmberContents()
{
    if (!initializedEmberContents)
    {
        ember_init(onThrowError, onFailAssertion, allocMemoryImpl, freeMemoryImpl);
        initializedEmberContents = true;
    }
}


// ====================================================================
//
// utils for socket
//
// ====================================================================

static void initSockets()
{
#if defined WIN32
    //$$ MSVCRT specific
    WSADATA wsaData;

    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
}

static void shutdownSockets()
{
#if defined WIN32
    //$$ MSVCRT specific
    WSACleanup();
#endif
}


// ====================================================================
//
// for sending/received contents
//
// ====================================================================

/// <summary>
/// パス一致確認
/// </summary>
/// <param name="pPath1"></param>
/// <param name="pPath2"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
static bool isSamePath(const berint* pPath1, const berint* pPath2, int pathLength)
{
    if (!pPath1 && !pPath2 && !pathLength)
        return true;

    return _memicmp(pPath1, pPath2, pathLength * sizeof(berint)) == 0;
}

/// <summary>
/// ノードパス複製
/// </summary>
/// <param name="ppDest"></param>
/// <param name="pSrc"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
static int cloneEmberPath(berint** ppDest, const berint* pSrc, int pathLength)
{
    *ppDest = NULL;
    if ((pSrc == NULL) || (pathLength <= 0))
        return 0;

    berint* _pDest = NULL;
#if defined WIN32
    __try
#endif
    {
        _pDest = newarr(berint, pathLength);
        memcpy(_pDest, pSrc, pathLength * sizeof(berint));
    }
#if defined WIN32
    __except (dump_exception(__FILE__, __LINE__, __FUNCTION__, GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER))
    {
        if (_pDest)
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(_pDest);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            _pDest = NULL;
        }
    }
#endif

    if (_pDest)
        *ppDest = _pDest;
    return pathLength;
}
/// <summary>
/// ノードパス設定
/// </summary>
/// <param name="content"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
static int setEmberContentPath(EmberContent* content, const berint* pPath, int pathLength)
{
    if (!content)
        return 0;

    if (content->pPath)
    {
#if defined WIN32
        __try
#endif
        {
            freeMemory(content->pPath);
        }
#if defined WIN32
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
        content->pPath = NULL;
    }
    content->pathLength = 0;

    berint* _pPath = NULL;
    int _pathLength = cloneEmberPath(&_pPath, pPath, pathLength);
    if ((_pPath != NULL) && (_pathLength == pathLength))
    {
        content->pPath = _pPath;
        content->pathLength = _pathLength;
    }

    return _pathLength;
}
/// <summary>
/// Ember 要素設定
/// </summary>
/// <param name="pDest"></param>
/// <param name="pSrc"></param>
/// <param name="length"></param>
/// <returns></returns>
static size_t setEmberContentData(voidptr pDest, const voidptr pSrc, size_t length)
{
    if (!pDest)
        return 0;

    size_t _length = 0ull;
    if (pSrc && (length > 0))
    {
#if defined WIN32
        __try
#endif
        {
            memcpy(pDest, pSrc, length);
            _length = length;
        }
#if defined WIN32
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
    }

    if ((!pSrc || !_length) && length)
    {
#if defined WIN32
        __try
#endif
        {
            memset(pDest, 0, length);
        }
#if defined WIN32
        __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
    }

    return _length;
}

/// <summary>
/// Ember 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
static EmberContent* createEmberContent(RequestId* pId, const berint* pPath, int pathLength)
{
    EmberContent* pContent = NULL;
#if defined WIN32
    __try
#endif
    {
        pContent = newobj(EmberContent);
        bzero_item(*pContent);
        pContent->requestId.group = pContent->requestId.page = pContent->requestId.buttonIndex = -1;

        // 要求元設定
        if (pId)
            memcpy(&pContent->requestId, pId, sizeof(RequestId));
        // パス内容退避
        setEmberContentPath(pContent, pPath, pathLength);
    }
#if defined WIN32
    __except (dump_exception(__FILE__, __LINE__, __FUNCTION__, GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER))
    {
        if (pContent)
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
    }
#endif

    return pContent;
}

/// <summary>
/// Ember コマンド 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="type"></param>
/// <returns></returns>
static EmberContent* createEmberCommandContent(RequestId* pId, const berint* pPath, int pathLength, GlowCommandType type)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        pContent->type = GlowType_Command;
        pContent->command.number = type;
    }

    return pContent;
}
/// <summary>
/// Ember GetDirectory 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
DLLAPI EmberContent* createEmberGetDirectoryContent(RequestId* pId, const berint* pPath, int pathLength)
{
    EmberContent* pContent = createEmberCommandContent(pId, pPath, pathLength, GlowCommandType_GetDirectory);

    return pContent;
}
/// <summary>
/// Ember Invoke 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pInvocation"></param>
/// <returns></returns>
EmberContent* createEmberInvokeContent(RequestId* pId, const berint* pPath, int pathLength, GlowInvocation* pInvocation)
{
    EmberContent* pContent = createEmberCommandContent(pId, pPath, pathLength, GlowCommandType_Invoke);
    if (pContent && pInvocation)
    {
        memcpy(&pContent->command.options.invocation, pInvocation, sizeof(GlowInvocation));

        // argument の複製
        pContent->command.options.invocation.pArguments = NULL;
        if ((pInvocation->pArguments != NULL) && (pInvocation->argumentsLength > 0))
        {
            GlowValue* pArguments = newarr(GlowValue, pInvocation->argumentsLength);
            for (int i = 0; i < pInvocation->argumentsLength; ++i)
            {
                glowValue_copyFrom(&pArguments[i], &pInvocation->pArguments[i]);
            }
            pContent->command.options.invocation.pArguments = pArguments;
        }
        // id設定
        pContent->command.options.invocation.invocationId = pId->id;
    }

    return pContent;
}

/// <summary>
/// Ember ノード 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="fields"></param>
/// <returns></returns>
static EmberContent* createEmberNodeContent(RequestId* pId, const berint* pPath, int pathLength, const GlowNode* pValue, GlowFieldFlags fields)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->node, (voidptr)pValue, sizeof(GlowNode)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            pContent->type = GlowType_Node;
            pContent->fields = fields;
        }
    }

    return pContent;
}
/// <summary>
/// Ember パラメータ 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="fields"></param>
/// <returns></returns>
static EmberContent* createEmberParameterContent(RequestId* pId, const berint* pPath, int pathLength, const GlowParameter* pValue, GlowFieldFlags fields)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->parameter, (voidptr)pValue, sizeof(GlowParameter)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            // ポインタデータは複製を設定し直す
            if (pValue)
            {
                if (pContent->parameter.value.flag == GlowParameterType_String)
                {
                    pContent->parameter.value.choice.pString = stringDup(pValue->value.choice.pString);
                }
                else if (pContent->parameter.value.flag == GlowParameterType_Octets)
                {
                    pContent->parameter.value.choice.octets.pOctets = stringDup(pValue->value.choice.octets.pOctets);
                }
            }

            pContent->type = GlowType_Parameter;
            pContent->fields = fields;
        }
    }

    return pContent;
}
/// <summary>
/// Ember パラメータ取得 送信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="fields"></param>
/// <returns></returns>
EmberContent* createEmberGetParameterContent(RequestId* pId, const berint* pPath, int pathLength)
{
    // GetDirectory を設定して取得に使用
    return createEmberGetDirectoryContent(pId, pPath, pathLength);
}
/// <summary>
/// Ember パラメータ設定 送信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="fields"></param>
/// <returns></returns>
EmberContent* createEmberSetParameterContent(RequestId* pId, const berint* pPath, int pathLength, const GlowParameter* pValue)
{
    EmberContent* pContent = createEmberParameterContent(pId, pPath, pathLength, pValue, GlowFieldFlag_Value);
    if (pContent)
    {
        pContent->type = GlowType_QualifiedParameter;
    }

    return pContent;
}

/// <summary>
/// Ember マトリックス 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <returns></returns>
static EmberContent* createEmberMatrixContent(RequestId* pId, const berint* pPath, int pathLength, const GlowMatrix* pValue)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->matrix, (voidptr)pValue, sizeof(GlowMatrix)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            pContent->type = GlowType_Matrix;
        }
    }

    return pContent;
}
/// <summary>
/// Ember シグナル 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="isTarget"></param>
/// <returns></returns>
static EmberContent* createEmberSignalContent(RequestId* pId, const berint* pPath, int pathLength, const GlowSignal* pValue, bool isTarget)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->signal, (voidptr)pValue, sizeof(GlowSignal)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            pContent->type = isTarget ? GlowType_Target : GlowType_Source;
        }
    }

    return pContent;
}
/// <summary>
/// Ember マトリックス接続 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <returns></returns>
EmberContent* createEmberConnectionContent(RequestId* pId, const berint* pPath, int pathLength, const GlowConnection* pValue)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->connection, (voidptr)pValue, sizeof(GlowConnection)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            // ポインタデータは複製を設定し直す
            if (pValue && pValue->pSources)
            {
                if (pValue->sourcesLength > 0)
                {
                    pContent->connection.pSources = newarr(berint, pValue->sourcesLength);
                    memcpy(pContent->connection.pSources, pValue->pSources, sizeof(berint) * pValue->sourcesLength);
                }
                else
                    pContent->connection.pSources = 0;
            }

            pContent->type = GlowType_Connection;
        }
    }

    return pContent;
}

/// <summary>
/// Ember ファンクション 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <returns></returns>
static EmberContent* createEmberFunctionContent(RequestId* pId, const berint* pPath, int pathLength, const GlowFunction* pValue)
{
    EmberContent* pContent = createEmberContent(pId, pPath, pathLength);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->function, (voidptr)pValue, sizeof(GlowFunction)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            pContent->type = GlowType_Function;
        }
    }

    return pContent;
}
/// <summary>
/// Ember ファンクション結果 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <returns></returns>
static EmberContent* createEmberInvocationResultContent(RequestId* pId, const GlowInvocationResult* pValue)
{
    EmberContent* pContent = createEmberContent(pId, 0, 0);
    if (pContent)
    {
        if (!setEmberContentData((voidptr)&pContent->invocationResult, (voidptr)pValue, sizeof(GlowInvocationResult)))
        {
#if defined WIN32
            __try
#endif
            {
                freeMemory(pContent);
            }
#if defined WIN32
            __except (EXCEPTION_EXECUTE_HANDLER) {}
#endif
            pContent = NULL;
        }
        else
        {
            // argument の複製
            if ((pValue->pResult != NULL) && (pValue->resultLength > 0))
            {
                GlowValue* pResult = newarr(GlowValue, pValue->resultLength);
                for (int i = 0; i < pValue->resultLength; ++i)
                {
                    glowValue_copyFrom(&pResult[i], &pValue->pResult[i]);
                }
                pContent->invocationResult.pResult = pResult;
            }

            pContent->type = GlowType_InvocationResult;
        }
    }

    return pContent;
}


// ====================================================================
//
// Linked List
//
// ====================================================================

void ptrList_init(PtrList *pThis)
{
    bzero_item(*pThis);
}

void ptrList_addLast(PtrList *pThis, voidptr value)
{
    PtrListNode *pNode = newobj(PtrListNode);
    pNode->value = value;
    pNode->pNext = NULL;

    if(pThis->pLast == NULL)
    {
        pThis->pHead = pNode;
        pThis->pLast = pNode;
    }
    else
    {
        pThis->pLast->pNext = pNode;
        pThis->pLast = pNode;
    }

    pThis->count++;
}

void ptrList_free(PtrList *pThis)
{
    PtrListNode *pNode = pThis->pHead;
    PtrListNode *pPrev;

    while(pNode != NULL)
    {
        pPrev = pNode;
        pNode = pNode->pNext;

        freeMemory(pPrev);
    }

    bzero_item(*pThis);
}


// ====================================================================
//
// Model
//
// ====================================================================

static void element_init(Element *pThis, Element *pParent, GlowElementType type, berint number)
{
    bzero_item(*pThis);

    pThis->pParent = pParent;
    pThis->type = type;
    pThis->number = number;

    ptrList_init(&pThis->children);

    if(pParent != NULL)
        ptrList_addLast(&pParent->children, pThis);

    if(pThis->type == GlowElementType_Node)
        pThis->glow.node.isOnline = true;
}

static void element_free(Element *pThis)
{
    Element *pChild;
    Target *pTarget;
    PtrListNode *pNode;

    for(pNode = pThis->children.pHead; pNode != NULL; pNode = pNode->pNext)
    {
        pChild = (Element *)pNode->value;
        element_free(pChild);
        freeMemory(pChild);
    }

    ptrList_free(&pThis->children);

    if(pThis->type == GlowElementType_Parameter)
    {
        glowValue_free(&pThis->glow.parameter.value);

        if(pThis->glow.parameter.pEnumeration != NULL)
            freeMemory((void *)pThis->glow.parameter.pEnumeration);
        if(pThis->glow.parameter.pFormula != NULL)
            freeMemory((void *)pThis->glow.parameter.pFormula);
        if(pThis->glow.parameter.pFormat != NULL)
            freeMemory((void *)pThis->glow.parameter.pFormat);
    }
    else if(pThis->type == GlowElementType_Matrix)
    {
        glowMatrix_free(&pThis->glow.matrix.matrix);

        for(pNode = pThis->glow.matrix.targets.pHead; pNode != NULL; pNode = pNode->pNext)
        {
            pTarget = (Target *)pNode->value;

            if(pTarget->pConnectedSources != NULL)
                freeMemory(pTarget->pConnectedSources);

            freeMemory(pTarget);
        }

        for(pNode = pThis->glow.matrix.sources.pHead; pNode != NULL; pNode = pNode->pNext)
        {
            if(pNode->value != NULL)
                freeMemory(pNode->value);
        }

        ptrList_free(&pThis->glow.matrix.targets);
        ptrList_free(&pThis->glow.matrix.sources);
    }
    else if(pThis->type == GlowElementType_Function)
    {
        glowFunction_free(&pThis->glow.function);
    }
    else if(pThis->type == GlowElementType_Node)
    {
        glowNode_free(&pThis->glow.node);
    }

    bzero_item(*pThis);
}

static pcstr element_getIdentifier(const Element *pThis)
{
    switch(pThis->type)
    {
        case GlowElementType_Node: return pThis->glow.node.pIdentifier;
        case GlowElementType_Parameter: return pThis->glow.parameter.pIdentifier;
        case GlowElementType_Matrix: return pThis->glow.matrix.matrix.pIdentifier;
        case GlowElementType_Function: return pThis->glow.function.pIdentifier;
    }

    return NULL;
}

/*static*/ berint* element_getPath(const Element* pThis, berint* pBuffer, int* pCount)
{
    const Element *pElement;
    berint *pPosition = &pBuffer[*pCount];
    int count = 0;

    for(pElement = pThis; pElement->pParent != NULL; pElement = pElement->pParent)
    {
        pPosition--;

        if(pPosition < pBuffer)
            return NULL;

        *pPosition = pElement->number;
        count++;
    }

    *pCount = count;
    return pPosition;
}

static pcstr element_getIdentifierPath(const Element *pThis, pstr pBuffer, int bufferSize)
{
    const Element *pElement;
    pstr pPosition = &pBuffer[bufferSize - 1];
    int length;
    pcstr pIdentifier;

    *pPosition = 0;

    for(pElement = pThis; pElement->pParent != NULL; pElement = pElement->pParent)
    {
        if(*pPosition != 0)
            *--pPosition = '/';

        pIdentifier = element_getIdentifier(pElement);
        length = (int)strlen(pIdentifier);
        pPosition -= length;

        if(pPosition < pBuffer)
            return NULL;

        memcpy(pPosition, pIdentifier, length);
    }

    return pPosition;
}

static Element *element_findChild(const Element *pThis, berint number)
{
    Element *pChild;
    PtrListNode *pNode;

    for(pNode = pThis->children.pHead; pNode != NULL; pNode = pNode->pNext)
    {
        pChild = (Element *)pNode->value;

        if(pChild->number == number)
            return pChild;
    }

    return NULL;
}

static Element *element_findChildByIdentifier(const Element *pThis, pcstr pIdentifier)
{
    Element *pChild;
    pcstr pIdent;
    PtrListNode *pNode;

    for(pNode = pThis->children.pHead; pNode != NULL; pNode = pNode->pNext)
    {
        pChild = (Element *)pNode->value;
        pIdent = element_getIdentifier(pChild);

        if(_stricmp(pIdent, pIdentifier) == 0)
            return pChild;
    }

    return NULL;
}

/*static*/ Element* element_findDescendant(const Element* pThis, const berint* pPath, int pathLength, Element** ppParent)
{
    int index;
    Element *pElement = (Element *)pThis;
    Element *pParent = NULL;

    if ((pPath == NULL) || (pathLength <= 0))
        return NULL;

    for(index = 0; index < pathLength; index++)
    {
        if(pElement != NULL)
            pParent = pElement;

        pElement = element_findChild(pElement, pPath[index]);

        if(pElement == NULL)
        {
            if(index < pathLength - 1)
                pParent = NULL;

            break;
        }
    }

    if(ppParent != NULL)
        *ppParent = pParent;

    return pElement;
}

static GlowParameterType element_getParameterType(const Element *pThis)
{
    if(pThis->type == GlowElementType_Parameter)
    {
        if(pThis->paramFields & GlowFieldFlag_Enumeration)
            return GlowParameterType_Enum;

        if(pThis->paramFields & GlowFieldFlag_Value)
            return pThis->glow.parameter.value.flag;

        if(pThis->paramFields & GlowFieldFlag_Type)
            return pThis->glow.parameter.type;
    }

    return (GlowParameterType)0;
}

static Target *element_findOrCreateTarget(Element *pThis, berint number)
{
    Target *pTarget;
    PtrListNode *pNode;

    if(pThis->type == GlowElementType_Matrix)
    {
        for(pNode = pThis->glow.matrix.targets.pHead; pNode != NULL; pNode = pNode->pNext)
        {
            pTarget = (Target *)pNode->value;

            if(pTarget->number == number)
                return pTarget;
        }

        pTarget = newobj(Target);
        bzero_item(*pTarget);
        pTarget->number = number;

        ptrList_addLast(&pThis->glow.matrix.targets, pTarget);
        return pTarget;
    }

    return NULL;
}

static Source *element_findSource(const Element *pThis, berint number)
{
    Source *pSource;
    PtrListNode *pNode;

    if(pThis->type == GlowElementType_Matrix)
    {
        for(pNode = pThis->glow.matrix.sources.pHead; pNode != NULL; pNode = pNode->pNext)
        {
            pSource = (Source *)pNode->value;

            if(pSource->number == number)
                return pSource;
        }
    }

    return NULL;
}

/****/
static int validityPathLength = 0;
static Element* element_setNode(const GlowNode* pNode, GlowFieldFlags fields, const berint* pPath, int pathLength, Element* pRootTop, int* pDuplicateRequest)
{
    Element* pElement;
    Element* pParent;
    int nDuplicateRequest = 0;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, &pParent);

    if (pParent != NULL)
    {
        if (pElement == NULL)
        {
            if (validityPathLength < pathLength)
                validityPathLength = pathLength;

            pElement = newobj(Element);
            element_init(pElement, pParent, GlowElementType_Node, pPath[pathLength - 1]);

            if (fields & GlowFieldFlag_Identifier)
                pElement->glow.node.pIdentifier = stringDup(pNode->pIdentifier);
        }
        else
            nDuplicateRequest = 1;

        if (fields & GlowFieldFlag_Description)
            pElement->glow.node.pDescription = stringDup(pNode->pDescription);

        if (fields & GlowFieldFlag_IsOnline)
            pElement->glow.node.isOnline = pNode->isOnline;

        if (fields & GlowFieldFlag_IsRoot)
            pElement->glow.node.isRoot = pNode->isRoot;

        if (fields & GlowFieldFlag_SchemaIdentifier)
            pElement->glow.node.pSchemaIdentifiers = stringDup(pNode->pSchemaIdentifiers);
    }

    if (pDuplicateRequest)
        *pDuplicateRequest = nDuplicateRequest;

    return pElement;
}

static Element* element_setParameter(const GlowParameter* pParameter, GlowFieldFlags fields, const berint* pPath, int pathLength, Element* pRootTop)
{
    Element* pElement;
    Element* pParent;
    GlowParameter* pLocalParam;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, &pParent);

    if (pParent != NULL)
    {
        if (pElement == NULL)
        {
            pElement = newobj(Element);
            element_init(pElement, pParent, GlowElementType_Parameter, pPath[pathLength - 1]);
        }

        pLocalParam = &pElement->glow.parameter;

        if ((fields & GlowFieldFlag_Identifier) == GlowFieldFlag_Identifier)
            pLocalParam->pIdentifier = stringDup(pParameter->pIdentifier);
        if (fields & GlowFieldFlag_Description)
            pLocalParam->pDescription = stringDup(pParameter->pDescription);
        if (fields & GlowFieldFlag_Value)
        {
            glowValue_free(&pLocalParam->value);
            glowValue_copyTo(&pParameter->value, &pLocalParam->value);
        }
        if (fields & GlowFieldFlag_Minimum)
            memcpy(&pLocalParam->minimum, &pParameter->minimum, sizeof(GlowMinMax));
        if (fields & GlowFieldFlag_Maximum)
            memcpy(&pLocalParam->maximum, &pParameter->maximum, sizeof(GlowMinMax));
        if (fields & GlowFieldFlag_Access)
            pLocalParam->access = pParameter->access;
        if (fields & GlowFieldFlag_Factor)
            pLocalParam->factor = pParameter->factor;
        if (fields & GlowFieldFlag_IsOnline)
            pLocalParam->isOnline = pParameter->isOnline;
        if (fields & GlowFieldFlag_Step)
            pLocalParam->step = pParameter->step;
        if (fields & GlowFieldFlag_Type)
            pLocalParam->type = pParameter->type;
        if (fields & GlowFieldFlag_StreamIdentifier)
            pLocalParam->streamIdentifier = pParameter->streamIdentifier;
        if (fields & GlowFieldFlag_StreamDescriptor)
            memcpy(&pLocalParam->streamDescriptor, &pParameter->streamDescriptor, sizeof(GlowStreamDescription));
        if (fields & GlowFieldFlag_SchemaIdentifier)
            pLocalParam->pSchemaIdentifiers = stringDup(pParameter->pSchemaIdentifiers);

        pElement->paramFields = (GlowFieldFlags)(pElement->paramFields | fields);
    }

    return pElement;
}

static Element* element_setMatrix(const GlowMatrix* pMatrix, const berint* pPath, int pathLength, Element* pRootTop)
{
    Element* pElement;
    Element* pParent;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, &pParent);

    if (pParent != NULL)
    {
        if (pElement == NULL)
        {
            pElement = newobj(Element);
            element_init(pElement, pParent, GlowElementType_Matrix, pPath[pathLength - 1]);
        }

        memcpy(&pElement->glow.matrix.matrix, pMatrix, sizeof(*pMatrix));
        pElement->glow.matrix.matrix.pIdentifier = stringDup(pMatrix->pIdentifier);
        pElement->glow.matrix.matrix.pDescription = stringDup(pMatrix->pDescription);
        pElement->glow.matrix.matrix.pSchemaIdentifiers = stringDup(pMatrix->pSchemaIdentifiers);
        if ((pMatrix->pLabels != NULL) && (pMatrix->labelsLength > 0))
        {
            pElement->glow.matrix.matrix.pLabels = newarr(GlowLabel, pMatrix->labelsLength);
            //pElement->glow.matrix.matrix.labelsLength = pMatrix->labelsLength;    // 先の memcpy で設定済
            //memcpy(pElement->glow.matrix.matrix.pLabels, pMatrix->pLabels, sizeof(GlowLabel) * pMatrix->labelsLength);
            for (int i = 0; i < pElement->glow.matrix.matrix.labelsLength; ++i)
            {
                memcpy(pElement->glow.matrix.matrix.pLabels[i].basePath, pMatrix->pLabels[i].basePath, sizeof(pMatrix->pLabels[i].basePath));
                pElement->glow.matrix.matrix.pLabels[i].basePathLength = pMatrix->pLabels[i].basePathLength;
                pElement->glow.matrix.matrix.pLabels[i].pDescription = stringDup(pMatrix->pLabels[i].pDescription);
            }
        }
    }

    return pElement;
}

static Element* element_setTarget(const GlowSignal* pSignal, const berint* pPath, int pathLength, Element* pRootTop)
{
    Element* pElement;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, NULL);

    if (pElement != NULL
        && pElement->type == GlowElementType_Matrix)
        element_findOrCreateTarget(pElement, pSignal->number);

    return pElement;
}

static Element* element_setSource(const GlowSignal* pSignal, const berint* pPath, int pathLength, Element* pRootTop)
{
    Element* pElement;
    Source* pSource;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, NULL);

    if (pElement != NULL
        && pElement->type == GlowElementType_Matrix)
    {
        pSource = newobj(Source);
        bzero_item(*pSource);
        pSource->number = pSignal->number;

        ptrList_addLast(&pElement->glow.matrix.sources, pSource);
    }

    return pElement;
}

static Element* element_setConnection(const GlowConnection* pConnection, const berint* pPath, int pathLength, Element* pRootTop)
{
    Element* pElement;
    Target* pTarget;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, NULL);

    if (pElement != NULL
        && pElement->type == GlowElementType_Matrix)
    {
        pTarget = element_findOrCreateTarget(pElement, pConnection->target);

        if (pTarget != NULL)
        {
            if (pTarget->pConnectedSources != NULL)
            {
                freeMemory(pTarget->pConnectedSources);
                pTarget->pConnectedSources = NULL;
            }
            if (pConnection->sourcesLength > 0)
            {
                pTarget->pConnectedSources = newarr(berint, pConnection->sourcesLength);
                memcpy(pTarget->pConnectedSources, pConnection->pSources, pConnection->sourcesLength * sizeof(berint));
                pTarget->connectedSourcesCount = pConnection->sourcesLength;
            }
        }
    }

    return pElement;
}

static void cloneTupleItemDescription(GlowTupleItemDescription* pDest, const GlowTupleItemDescription* pSource)
{
    memcpy(pDest, pSource, sizeof(*pSource));
    pDest->pName = stringDup(pSource->pName);
}
static Element* element_setFunction(const GlowFunction* pFunction, const berint* pPath, int pathLength, Element* pRootTop)
{
    Element* pElement;
    Element* pParent;
    int index;

    pElement = element_findDescendant(pRootTop, pPath, pathLength, &pParent);

    if (pParent != NULL)
    {
        if (pElement == NULL)
        {
            pElement = newobj(Element);
            element_init(pElement, pParent, GlowElementType_Function, pPath[pathLength - 1]);
        }

        memcpy(&pElement->glow.function, pFunction, sizeof(*pFunction));
        pElement->glow.function.pIdentifier = stringDup(pFunction->pIdentifier);
        pElement->glow.function.pDescription = stringDup(pFunction->pDescription);

        // clone arguments
        if (pFunction->pArguments != NULL)
        {
            pElement->glow.function.pArguments = newarr(GlowTupleItemDescription, pFunction->argumentsLength);

            for (index = 0; index < pFunction->argumentsLength; index++)
                cloneTupleItemDescription(&pElement->glow.function.pArguments[index], &pFunction->pArguments[index]);
        }

        // clone result
        if (pFunction->pResult != NULL)
        {
            pElement->glow.function.pResult = newarr(GlowTupleItemDescription, pFunction->resultLength);

            for (index = 0; index < pFunction->resultLength; index++)
                cloneTupleItemDescription(&pElement->glow.function.pResult[index], &pFunction->pResult[index]);
        }
    }

    return pElement;
}


/// <summary>
/// 文字列化パスデリミタ
/// </summary>
static char PathDelimiter = '/';

/// <summary>
/// 文字列パス分割
/// </summary>
/// <param name="dst"></param>
/// <param name="src"></param>
/// <param name="delim"></param>
/// <returns></returns>
static size_t split(char* dst[], char* src, char delim)
{
    int count = 0;

    while (true)
    {
        while (*src == delim)
        {
            src++;
        }

        if (*src == '\0') break;

        dst[count++] = src;

        while (*src && (*src != delim))
        {
            src++;
        }

        if (*src == '\0') break;

        *src++ = '\0';
    }

    return count;
}
/// <summary>
/// 文字列パス→パス
/// </summary>
/// <param name="pRoot"></param>
/// <param name="pathValue"></param>
/// <param name="ppPath"></param>
/// <returns></returns>
size_t convertString2Path(Element* pRoot, const pstr pathValue, berint** ppPath)
{
//#pragma warning(push)
//#pragma warning(disable:4267)
    size_t len = strlen(pathValue);
    Element* pCursor = pRoot;

    // そもそも先頭が未取得
    if (!pRoot)
    {
        __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "empty tree.\n");
        *ppPath = NULL;
        return 0;
    }
    // 未入力orデリミタ始まりでないなら不定とする
    if (len <= 0)
    {
        __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "empty path.\n");
        *ppPath = NULL;
        return 0;
    }
    if (pathValue[0] != PathDelimiter)
    {
        __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "invalid path top char.\n");
        *ppPath = NULL;
        return 0;
    }

    berint* pPath = newarr(berint, GLOW_MAX_TREE_DEPTH);
    memset((byte*)pPath, 0, sizeof(berint) * GLOW_MAX_TREE_DEPTH);
    *ppPath = pPath;

    // デリミタだけなら先頭のみという扱いで返す
    if ((len == 1) && (pathValue[0] == PathDelimiter))
        return 1;

    // 分割 strtok不使用
    pstr dst[GLOW_MAX_TREE_DEPTH] = { 0 };
    pstr _pathValue = newarr(char, len + 1);
    memcpy(_pathValue, pathValue, len);
    _pathValue[len] = 0;
    size_t count = split(dst, _pathValue, PathDelimiter);

    int chkdepth = -1;
    for (int pathIndex = 0; (pPath != NULL) && (pathIndex < count) && (dst[pathIndex] != NULL); ++pathIndex)
    {
        pCursor = element_findChildByIdentifier(pCursor, dst[pathIndex]);
        if (pCursor)
        {
            pPath[pathIndex] = pCursor->number;
        }
        else
        {
            // 指定された子がいない
            freeMemory(pPath);
            pPath = NULL;
            *ppPath = NULL;

            chkdepth = pathIndex;
            break;
        }
    }

    if (pPath)
    {
#if false   // どうにも嫌われる
        int bufsize = 512;
        pstr tmp = newarr(char, bufsize);
        memset(tmp, 0, bufsize);
        int numsize = 16;
        pstr sub = newarr(char, numsize);
        memset(sub, 0, numsize);
        for (int i = 0, pos = 0; i < count; ++i)
        {
            if (i > 0)
                tmp[pos++] = '.';
            sprintf_s(sub, numsize, "%d\0", pPath[i]);
            memcpy(&tmp[pos], sub, strlen(sub));
            pos += strlen(sub);
            if ((bufsize - 10) <= pos)
                break;
        }
        __Trace(__FILE__, __LINE__, __FUNCTION__, "path %s is %s\n", pathValue, tmp);
        freeMemory(tmp);
        freeMemory(sub);
#endif
    }
    else
    {
        __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "unknown path %s, lost depth = %d, topNode children count = %d\n", pathValue, chkdepth, pRoot->children.count);
        count = 0;
    }

    if (_pathValue)
    {
        freeMemory(_pathValue);
        _pathValue = NULL;
    }

    return count;
//#pragma warning(pop)
}
/// <summary>
/// パス→文字列パス
/// </summary>
/// <param name="pRoot"></param>
/// <param name="pPath"></param>
/// <param name="pPathLength"></param>
/// <returns></returns>
pstr convertPath2String(Element* pRoot, const berint* pPath, int pathLength)
{
//#pragma warning(push)
//#pragma warning(disable:4267)
    int pathIndex;
    int nodeId;
    Element* pCursor = pRoot;

    // 指定なし
    if (pathLength <= 0)
        return NULL;

    // 先頭のみ
    if (pathLength <= 1)
    {
        pstr pd = newarr(char, 2);
        pd[0] = PathDelimiter;
        pd[1] = 0;
        return pd;
    }

    pstr* ids = NULL;
    pstr value = NULL;

#if defined WIN32
    __try
#endif
    {
#if defined WIN32
        __try
#endif
        {
            // 一時枠確保
            ids = newarr(pstr, pathLength);
            bzero_item(*ids);
            int _pathLength = 0;
            size_t totalLength = 0;
            for (pathIndex = 0; pathIndex < pathLength; pathIndex++)
            {
                nodeId = pPath[pathIndex];
                pCursor = element_findChild(pCursor, nodeId);
                if (pCursor)
                {
                    if ((pCursor->type == GlowElementType_Node)
                     || (pCursor->type == GlowElementType_Parameter)
                     || (pCursor->type == GlowElementType_Matrix)
                     || (pCursor->type == GlowElementType_Function))
                    {
                        ids[pathIndex] = pCursor->glow.nodeId.pIdentifier;
                        totalLength += strlen(ids[pathIndex]);
                        ++_pathLength;
                    }
                    else
                        break;
                }
                else
                {
                    break;
                }
            }

            //
            if (_pathLength > 0)
            {
                // 必要なデータ長分メモリ確保
                totalLength = totalLength + _pathLength + 1;
                value = newarr(char, totalLength);
                memset(value, 0, totalLength * sizeof(char));
                size_t pos = 0, len = 0;
                for (int i = 0; (i < _pathLength) && ((len = strlen(ids[i])) > 0) && ((pos + 1 + len) <= totalLength); ++i)
                {
                    value[pos++] = PathDelimiter;
                    memcpy(&value[pos], ids[i], len);
                    pos += len;
                }
            }
        }
#if defined WIN32
        __except (dump_exception(__FILE__, __LINE__, __FUNCTION__, GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER)) {}
#endif
    }
#if defined WIN32
    __finally
#endif
    {
        // 一時枠解放
        if (ids)
            freeMemory(ids);
    }

    return value;
//#pragma warning(pop)
}


// ====================================================================
//
// glow handlers
//
// ====================================================================
/*
#pragma pack(1)
typedef struct tagSession
{
    RemoteContent remoteContent;
    EmberContent* pRequest;

    Element root;
#if false
    Element* pCursor;
    berint cursorPathBuffer[GLOW_MAX_TREE_DEPTH];
    berint* pCursorPath;
    int cursorPathLength;
#endif
} Session;
#pragma pack()
*/

static Session* pActiveSession = NULL;
static void setActiveSession(Session* pSession)
{
    pActiveSession = pSession;
}
/// <summary>ツリー先頭取得</summary></summary>
/// <returns></returns>
Element* getRootTop()
{
    Element* pRootTop = NULL;
    if (pActiveSession != NULL)
    {
        pRootTop = &pActiveSession->root;
    }
    return pRootTop;
}
/// <summary>ツリー先頭取得有無</summary>
/// <returns></returns>
bool hasEmberTree()
{
    bool ena = false;
    Element* pRootTop = getRootTop();
    if (pRootTop != NULL)
    {
        ena = pRootTop->children.count > 0;
    }

    return ena;
}


static bool getQuitConsumerRequest(const Session* pSession)
{
    short id = -1;
    if (pSession)
        id = pSession->remoteContent.id;
    return __GetQuitConsumerRequest(id);
}
static EmberContent* getConsumerRequest(const Session* pSession)
{
    short id = -1;
    if (pSession)
        id = pSession->remoteContent.id;
    return __GetConsumerRequest(id);
}
static void notifyReceivedConsumerResult(const Session* pSession, EmberContent* pResult)
{
    short id = -1;
    if (pSession)
        id = pSession->remoteContent.id;
    __NotifyReceivedConsumerResult(id, pResult);
}


/// <summary>
/// ノード受信
/// </summary>
/// <param name="pNode"></param>
/// <param name="fields"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onNode(const GlowNode *pNode, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    int nDuplicateRequest = false;
    Element* pElement = element_setNode(pNode, fields, pPath, pathLength, &pSession->root, &nDuplicateRequest);
    if (!pElement)
        return;

    RequestId* pId = NULL;
    if (pSession->pRequest
     && (pSession->pRequest->type == GlowType_Command)
     && (pSession->pRequest->command.number == GlowCommandType_GetDirectory))
    {
        pId = &pSession->pRequest->requestId;
        pSession->pRequest = NULL;
    }
    // ツリーに反映したインスタンスから通知データを生成する
    //EmberContent* pResult = createEmberNodeContent(pId, pPath, pathLength, pNode, fields);
    EmberContent* pResult = createEmberNodeContent(pId, pPath, pathLength, &pElement->glow.node, fields);
    if (pResult)
    {
        // 上位ラッパへ通知
        pResult->duplicateRequests = nDuplicateRequest;
        notifyReceivedConsumerResult(pSession, pResult);
    }
}
EmberContent* m_pResult = NULL;
/// <summary>
/// パラメータ受信
/// </summary>
/// <param name="pParameter"></param>
/// <param name="fields"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onParameter(const GlowParameter *pParameter, GlowFieldFlags fields, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    Element* pElement = element_setParameter(pParameter, fields, pPath, pathLength, &pSession->root);
    if (!pElement)
        return;

    RequestId* pId = NULL;
    if (pSession->pRequest
     && (pSession->pRequest->type == GlowType_Parameter)
     && (pSession->pRequest->pathLength == pSession->pRequest->pathLength)
     && isSamePath(pSession->pRequest->pPath, pPath, pathLength))
    {
        pId = &pSession->pRequest->requestId;
        pSession->pRequest = NULL;
    }
    // ツリーに反映したインスタンスから通知データを生成する
    //EmberContent* pResult = createEmberParameterContent(pId, pPath, pathLength, pParameter, fields);
    EmberContent* pResult = createEmberParameterContent(pId, pPath, pathLength, &pElement->glow.parameter, fields);
    if (pResult)
    {
        // 上位ラッパへ通知
        //notifyReceivedConsumerResult(pSession, pResult);
		__EmberCommandConverter(pResult);
    }
}

/// <summary>
/// マトリックス受信
/// </summary>
/// <param name="pMatrix"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onMatrix(const GlowMatrix *pMatrix, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    Element* pElement = element_setMatrix(pMatrix, pPath, pathLength, &pSession->root);
    if (!pElement)
        return;

    RequestId* pId = NULL;
#if false
    if (pSession->pRequest)
        pId = &pSession->pRequest->requestId;
#endif
    // ツリーに反映したインスタンスから通知データを生成する
    //EmberContent* pResult = createEmberMatrixContent(pId, pPath, pathLength, pMatrix);
    EmberContent* pResult = createEmberMatrixContent(pId, pPath, pathLength, &pElement->glow.matrix.matrix);
    if (pResult)
    {
        // 上位ラッパへ通知
        notifyReceivedConsumerResult(pSession, pResult);
    }
}
/// <summary>
/// マトリックス Target 受信
/// </summary>
/// <param name="pSignal"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onTarget(const GlowSignal *pSignal, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    Element* pElement = element_setTarget(pSignal, pPath, pathLength, &pSession->root);

    RequestId* pId = NULL;
#if false
    if (pSession->pRequest)
        pId = &pSession->pRequest->requestId;
#endif
    EmberContent* pResult = createEmberSignalContent(pId, pPath, pathLength, pSignal, true);
    if (pResult)
    {
        // 上位ラッパへ通知
        notifyReceivedConsumerResult(pSession, pResult);
    }
}
/// <summary>
/// マトリックス Source 受信
/// </summary>
/// <param name="pSignal"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onSource(const GlowSignal *pSignal, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    Element* pElement = element_setSource(pSignal, pPath, pathLength, &pSession->root);

    RequestId* pId = NULL;
#if false
    if (pSession->pRequest)
        pId = &pSession->pRequest->requestId;
#endif
    EmberContent* pResult = createEmberSignalContent(pId, pPath, pathLength, pSignal, false);
    if (pResult)
    {
        // 上位ラッパへ通知
        notifyReceivedConsumerResult(pSession, pResult);
    }
}
/// <summary>
/// マトリックス 接続 受信
/// </summary>
/// <param name="pConnection"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onConnection(const GlowConnection *pConnection, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    Element* pElement = element_setConnection(pConnection, pPath, pathLength, &pSession->root);

    RequestId* pId = NULL;
    if (pSession->pRequest)
        pId = &pSession->pRequest->requestId;
    // pConnection は onConnection 呼び出し元で削除されてしまうので、コピーしたインスタンスから通知データを生成する
    //EmberContent* pResult = createEmberConnectionContent(pId, pPath, pathLength, pConnection);
    GlowConnection* _pConnection = newobj(GlowConnection);
    bzero_item(*_pConnection);
    memcpy(_pConnection, pConnection, sizeof(GlowConnection));
    EmberContent* pResult = createEmberConnectionContent(pId, pPath, pathLength, _pConnection);
    if (pResult)
    {
        // 上位ラッパへ通知
        notifyReceivedConsumerResult(pSession, pResult);
    }
}

/// <summary>
/// ファンクション 受信
/// </summary>
/// <param name="pFunction"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="state"></param>
static void onFunction(const GlowFunction *pFunction, const berint *pPath, int pathLength, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    // ツリーへ反映
    Element* pElement = element_setFunction(pFunction, pPath, pathLength, &pSession->root);
    if (!pElement)
        return;

    RequestId* pId = NULL;
#if false
    if (pSession->pRequest)
        pId = &pSession->pRequest->requestId;
#endif
    // ツリーに反映したインスタンスから通知データを生成する
    //EmberContent* pResult = createEmberFunctionContent(pId, pPath, pathLength, pFunction);
    EmberContent* pResult = createEmberFunctionContent(pId, pPath, pathLength, &pElement->glow.function);
    if (pResult)
    {
        // 上位ラッパへ通知
        notifyReceivedConsumerResult(pSession, pResult);
    }
}
/// <summary>
/// ファンクション 結果受信
/// </summary>
/// <param name="pInvocationResult"></param>
/// <param name="state"></param>
static void onInvocationResult(const GlowInvocationResult *pInvocationResult, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;
    RequestId* pId = NULL;
    if (pSession->pRequest
     && (pSession->pRequest->type == GlowType_Command)
     && (pSession->pRequest->command.number == GlowCommandType_Invoke)
     && (pSession->pRequest->command.options.invocation.invocationId == pInvocationResult->invocationId))
    {
        pId = &pSession->pRequest->requestId;
        pSession->pRequest = NULL;
    }
    EmberContent* pResult = createEmberInvocationResultContent(pId, pInvocationResult);
    if (pResult)
    {
        // 上位ラッパへ通知
        notifyReceivedConsumerResult(pSession, pResult);
    }
}

static void onOtherPackageReceived(const byte *pPackage, int length, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession)
        return;

    SOCKET sock = pSession->remoteContent.hSocket;
    const int bufferSize = 512;
    byte* pBuffer = NULL;
    unsigned int txLength;

    if (length >= 4
     && pPackage[1] == EMBER_MESSAGE_ID
     && pPackage[2] == EMBER_COMMAND_KEEPALIVE_REQUEST)
    {
        pBuffer = newarr(byte, bufferSize);
        txLength = emberFraming_writeKeepAliveResponse(pBuffer, sizeof(pBuffer), pPackage[0]);
        int sendlen = send(sock, (char *)pBuffer, txLength, 0);
        if (sendlen != txLength)
        {
            int eno = errno;
            bzero_item(*pBuffer);
            strerror_s(pBuffer, bufferSize, eno);
            __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "send error, (%d)%s\n", eno, pBuffer);
        }

        freeMemory(pBuffer);
        pBuffer = NULL;
    }
}

void onUnsupportedTltlv(const BerReader *pReader, const berint *pPath, int pathLength, GlowReaderPosition position, voidptr state)
{
    __Trace(__FILE__, __LINE__, __FUNCTION__, "state = 0x%016llx\n", state);
    Session* pSession = (Session*)state;
    if (!pSession || !pReader)
        return;

    pstr value = NULL;
    if ((position == GlowReaderPosition_ParameterContents)
     && (pReader->length > 0))
    {
        if (berTag_equals(&pReader->tag, &glowTags.parameterContents.enumeration))
        {
            value = newarr(char, pReader->length + 1);
            berReader_getString(pReader, value, pReader->length);
            __Trace(__FILE__, __LINE__, __FUNCTION__, "received enumeration tag, value = %s\n", value);
        }
        else if (berTag_equals(&pReader->tag, &glowTags.parameterContents.formula))
        {
            value = newarr(char, pReader->length + 1);
            berReader_getString(pReader, value, pReader->length);
            __Trace(__FILE__, __LINE__, __FUNCTION__, "received formula tag, value = %s\n", value);
        }
        else if (berTag_equals(&pReader->tag, &glowTags.parameterContents.format))
        {
            value = newarr(char, pReader->length + 1);
            berReader_getString(pReader, value, pReader->length);
            __Trace(__FILE__, __LINE__, __FUNCTION__, "received format tag, value = %s\n", value);
        }
    }
    if (value)
        freeMemory(value);
    else
        __Trace(__FILE__, __LINE__, __FUNCTION__, "received unknown value.\n");
}


// ====================================================================
//
// main loop
//
// ====================================================================

#if false
static Element *findElement(pcstr pIdentifier, const Session *pSession)
{
    if(strcmp(pIdentifier, ".") == 0)
    {
        return pSession->pCursor;
    }
    else if(strcmp(pIdentifier, "..") == 0)
    {
        return pSession->pCursor->pParent;
    }

    return element_findChildByIdentifier(pSession->pCursor, pIdentifier);
}
#endif

static bool handleInput(Session* pSession, EmberContent* pRequest)
{
    if ((pSession == NULL) || (pRequest == NULL))
        return false;
    // request->requestType が要求内容
    // GlowType 定義にない QUIT_REQUEST_CONSUMER であった場合、この関数の戻りは true となる
    if (pRequest->type == QUIT_REQUEST_CONSUMER)
        return true;

    //
    // 対象エレメント確認
    //
    Element* pElement = NULL;
    int pathLength = pRequest->pathLength;
#if defined WIN32
    __try
#endif
    {
        if ((pathLength > 0) && (pRequest->pPath != NULL))
        {
            pElement = element_findDescendant(&pSession->root, pRequest->pPath, pathLength, NULL);
        }
        else
        {
            // パス指定がない場合、GetDirectory 要求であれば先頭ノードを対象とするが
            // それ以外の要求は操作できない
            if ((pRequest->type == GlowType_Command)
             && (pRequest->command.number == GlowCommandType_GetDirectory))
            {
                pElement = &pSession->root;
                if (pathLength < 0)
                    pathLength = 0;
            }
        }
    }
#if defined WIN32
    __except (dump_exception(__FILE__, __LINE__, __FUNCTION__, GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER)) {}
#endif
    if (pElement == NULL)
        return false;
    pstr pathName = convertPath2String(&pSession->root, pRequest->pPath, pRequest->pathLength);

    GlowOutput output;
    const int bufferSize = 512;
    byte *pBuffer = NULL;

#if defined WIN32
    __try
#endif
    {
#if defined WIN32
        __try
#endif
        {
            //
            // 要求内容確認
            //
            if ((pRequest->type == GlowType_Command)
                && ((pRequest->command.number == GlowCommandType_GetDirectory)
                    || (pRequest->command.number == GlowCommandType_Subscribe)
                    || (pRequest->command.number == GlowCommandType_Unsubscribe)
                    || ((pRequest->command.number == GlowCommandType_Invoke)
                     && (pElement->type == GlowElementType_Function))))
            {
                pBuffer = newarr(byte, bufferSize);
                glowOutput_init(&output, pBuffer, bufferSize, 0);
                glowOutput_beginPackage(&output, true);
                glow_writeQualifiedCommand(
                    &output,
                    &pRequest->command,
                    pRequest->pPath,
                    pathLength,
                    pElement->type);
                pSession->pRequest = pRequest;

                if (pRequest->command.number == GlowCommandType_GetDirectory)
                    __Trace(__FILE__, __LINE__, __FUNCTION__, "send GetDirectory request\n");
                else if (pRequest->command.number == GlowCommandType_Subscribe)
                    __Trace(__FILE__, __LINE__, __FUNCTION__, "send Subscribe request\n");
                else if (pRequest->command.number == GlowCommandType_Unsubscribe)
                    __Trace(__FILE__, __LINE__, __FUNCTION__, "send Unsubscribe request\n");
                else if (pRequest->command.number == GlowCommandType_Invoke)
                {
                    if (pathName)
                        __Trace(__FILE__, __LINE__, __FUNCTION__, "send Invoke request, invocationId = %d, path = %s\n", pRequest->command.options.invocation.invocationId, pathName);
                    else
                        __Trace(__FILE__, __LINE__, __FUNCTION__, "send Invoke request, invocationId = %d\n", pRequest->command.options.invocation.invocationId);
                }

                int txLength = glowOutput_finishPackage(&output);
                int sendlen = send(pSession->remoteContent.hSocket, (char *)pBuffer, txLength, 0);
                if (sendlen != txLength)
                {
                    int eno = errno;
                    bzero_item(*pBuffer);
                    strerror_s(pBuffer, bufferSize, eno);
                    __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "send error, (%d)%s\n", eno, pBuffer);
                }
                freeMemory(pBuffer);
                pBuffer = NULL;
            }
            else if (((pRequest->type == GlowType_Parameter)
                   || (pRequest->type == GlowType_QualifiedParameter))
                  && (pElement->type == GlowElementType_Parameter))
            {
                pBuffer = newarr(byte, bufferSize);
                glowOutput_init(&output, pBuffer, bufferSize, 0);
                glowOutput_beginPackage(&output, true);
                glow_writeQualifiedParameter(
                    &output,
                    &pRequest->parameter,
                    GlowFieldFlag_Value,
                    pRequest->pPath,
                    pathLength);
                if (pathName)
                    __Trace(__FILE__, __LINE__, __FUNCTION__, "send Set Parameter request %s\n", pathName);
                else
                    __Trace(__FILE__, __LINE__, __FUNCTION__, "send Set Parameter request\n");
                int txLength = glowOutput_finishPackage(&output);
                int sendlen = send(pSession->remoteContent.hSocket, (char*)pBuffer, txLength, 0);
                if (sendlen != txLength)
                {
                    int eno = errno;
                    bzero_item(*pBuffer);
                    strerror_s(pBuffer, bufferSize, eno);
                    __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "send error, (%d)%s\n", eno, pBuffer);
                }
                freeMemory(pBuffer);
                pBuffer = NULL;
            }
            else if ((pRequest->type == GlowType_Connection)
                  && (pElement->type == GlowElementType_Matrix))
            {
                pBuffer = newarr(byte, bufferSize);
                glowOutput_init(&output, pBuffer, bufferSize, 0);
                glowOutput_beginPackage(&output, true);
                glow_writeConnectionsPrefix(&output, pRequest->pPath, pathLength);
                glow_writeConnection(&output, &pRequest->connection);
                glow_writeConnectionsSuffix(&output);
                {
                    int bufflen = 256;
                    pstr pbuff = newarr(char, bufflen);
                    memset(pbuff, 0, bufflen);
                    switch (pRequest->connection.operation)
                    {
                    case GlowConnectionOperation_Disconnect: sprintf_s(pbuff, bufflen, "Disconnect"); break;
                    case GlowConnectionOperation_Connect: sprintf_s(pbuff, bufflen, "Connect"); break;
                    case GlowConnectionOperation_Absolute: sprintf_s(pbuff, bufflen, "Absolute"); break;
                    default: sprintf_s(pbuff, bufflen, "unknown"); break;
                    }
                    pstr pOpe = stringDup(pbuff);
                    memset(pbuff, 0, bufflen);
                    if (pRequest->connection.pSources && (pRequest->connection.sourcesLength > 0))
                    {
                        for (int i = 0, pos = 0; (i < pRequest->connection.sourcesLength) && (pos < bufflen); ++i)
                        {
                            if (i > 0)
                            {
                                sprintf_s(&pbuff[pos], bufflen - pos, ",");
                                pos = (int)strlen(pbuff);
                            }
                            sprintf_s(&pbuff[pos], bufflen - pos, "%d", pRequest->connection.pSources[i]);
                            pos = (int)strlen(pbuff);
                        }
                    }
                    else
                        sprintf_s(pbuff, bufflen, "(empty)");
                    pstr pSrcs = stringDup(pbuff);
                    __Trace(__FILE__, __LINE__, __FUNCTION__, "send Connection request, target = %d, sourcesLength = %d, pSources = %s, operation = %s\n",
                                                                pRequest->connection.target, pRequest->connection.sourcesLength, pSrcs, pOpe);

                    freeMemory(pOpe);
                    freeMemory(pSrcs);
                    freeMemory(pbuff);
                }
                int txLength = glowOutput_finishPackage(&output);
                int sendlen = send(pSession->remoteContent.hSocket, (char *)pBuffer, txLength, 0);
                if (sendlen != txLength)
                {
                    int eno = errno;
                    bzero_item(*pBuffer);
                    strerror_s(pBuffer, bufferSize, eno);
                    __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "send error, (%d)%s\n", eno, pBuffer);
                }
                freeMemory(pBuffer);
                pBuffer = NULL;
            }
        }
#if defined WIN32
        __except (dump_exception(__FILE__, __LINE__, __FUNCTION__, GetExceptionInformation(), EXCEPTION_EXECUTE_HANDLER)) {}
#endif
    }
#if defined WIN32
    __finally
#endif
    {
		if (pathName != NULL)
			freeMemory(pathName);
		if (pBuffer != NULL)
			freeMemory(pBuffer);
	}

    return false;
}

bool Call_handleInput(EmberContent* pRequest)
{
	bool result = handleInput(pActiveSession, pRequest);

	return result;
}

static bool run(Session *pSession)
{
    static char s_input[256];
    byte buffer[64];
    int read;
    bool isReq = false;
    bool isQuitReq = false;
    bool lostConnection = false;
    /*const*/ struct timeval timeout = {0, 16 * 1000}; // 16 milliseconds timeout for select()
    const int rxBufferSize = 1290; // max size of unescaped package
    fd_set fdset = { 0 };
    int fdsReady;
    GlowReader *pReader = newobj(GlowReader);
    byte *pRxBuffer = newarr(byte, rxBufferSize);
    SOCKET sock = pSession->remoteContent.hSocket;

    glowReader_init(pReader, onNode, onParameter, NULL, NULL, (voidptr)pSession, pRxBuffer, rxBufferSize);
    pReader->base.onMatrix = onMatrix;
    pReader->base.onTarget = onTarget;
    pReader->base.onSource = onSource;
    pReader->base.onConnection = onConnection;
    pReader->base.onFunction = onFunction;
    pReader->base.onInvocationResult = onInvocationResult;
    pReader->onOtherPackageReceived = onOtherPackageReceived;
    pReader->base.onUnsupportedTltlv = onUnsupportedTltlv;

    setActiveSession(pSession);
    while (!(isQuitReq = getQuitConsumerRequest(pSession)) && !lostConnection)
    {
        //
        // 前の要求手続きがない場合のみ
        // 次の要求手続きを展開する
        //
        EmberContent* pRequest = NULL;
        isReq = false;
        //if (!pSession->pRequest)
        if (!pSession->pRequest
         || ((pSession->pRequest->type == GlowType_Command)
          && (pSession->pRequest->command.number == GlowCommandType_GetDirectory)))
        {
            //
            // 要求コマンドの有無を isReq に、
            // コマンド内容を s_input に取る
            //
            isReq = (pRequest = getConsumerRequest(pSession)) != NULL;
        }

        if(isReq)
        {
            isQuitReq = handleInput(pSession, pRequest);
        }
        else
        {
            FD_ZERO(&fdset);
            FD_SET(sock, &fdset);

            fdsReady = select((int)(sock + 1), &fdset, NULL, NULL, &timeout);

            if(fdsReady == 1) // socket is ready to read
            {
                if(FD_ISSET(sock, &fdset))
                {
                    read = recv(sock, (char *)buffer, sizeof(buffer), 0);

                    if (read > 0)
                    {
                        char* strBuf = hex2string(buffer, read);
                        if (strBuf)
                        {
                            __Trace(__FILE__, __LINE__, __FUNCTION__, "received %d bytes, %s\n", read, strBuf);
                            freeMemory(strBuf);
                        }

                        glowReader_readBytes(pReader, buffer, read);

                        // (調査中)
                        // 受信実績あれば送信控えをクリアする
                        // ここでのインスタンス削除はしない（上位キューが手続き）
                        if (pSession->pRequest)
                            pSession->pRequest = NULL;
                    }
                    else
                        //isQuitReq = true;
                        lostConnection = true;
                }
            }
            else if(fdsReady < 0) // connection lost
            {
                //isQuitReq = true;
                lostConnection = true;
            }
        }

        Sleep(pSession->remoteContent.threadDelay);
    }
    setActiveSession(NULL);

    glowReader_free(pReader);
    freeMemory(pRxBuffer);
    freeMemory(pReader);

    return isQuitReq;
}


// ====================================================================
//
// consumer sample entry point
//
// ====================================================================
/// <summary>コンシューマ機能</summary>
/// <param name="pRemoteContent"></param>
void runConsumer(RemoteContent* pRemoteContent)
{
    int result;
    Session session;
    bzero_item(session);
    if (!pRemoteContent)
        return;
    memcpy(&session.remoteContent, pRemoteContent, sizeof(RemoteContent));

    initEmberContents();
    initSockets();

    if (session.remoteContent.hSocket != 0)
    {
        char addr[32] = { 0 };
        inet_ntop(AF_INET, &session.remoteContent.remoteAddr.sin_addr, addr, sizeof(addr));
        int port = ntohs(session.remoteContent.remoteAddr.sin_port);

        // 切断により run メソッドが終了しても上位ラッパの離脱要求があるまで再接続を試行する
        size_t connCount = 0ull;
        bool isQuitReq = false;
        while (!(isQuitReq = getQuitConsumerRequest(&session)))
        {
            __Guidance("\n");
            __Trace(__FILE__, __LINE__, __FUNCTION__, "connecting to %s:%d (%zu)...\n", addr, port, connCount);
            result = connect(session.remoteContent.hSocket, (const struct sockaddr*)&session.remoteContent.remoteAddr, sizeof(struct sockaddr_in));

            if (result != SOCKET_ERROR)
            {
                if (connCount == SIZE_MAX)
                    connCount = 0ull;
                ++connCount;

                element_init(&session.root, NULL, GlowElementType_Node, 0);
                pRemoteContent->pTopNode = &session.root;
                validityPathLength = 0;
                __Trace(__FILE__, __LINE__, __FUNCTION__, "connected provider.\n");

                run(&session);

                element_free(&session.root);
                closesocket(session.remoteContent.hSocket);
            }
            else
            {
                //__ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "connect error.\n"));
                __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "connect error.\n");
            }

            Sleep(session.remoteContent.reconnectDelay);
        }
    }
    else
    {
        __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "address or port error.\n");
    }

    if (allocCount > 0)
    {
        __ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "UNFREED MEMORY DETECTED %d!\n", allocCount);
    }

    shutdownSockets();
}

#ifdef __cplusplus
}
#endif
