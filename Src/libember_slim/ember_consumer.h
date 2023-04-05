#ifndef EMBER_CONSUMER_H
#define EMBER_CONSUMER_H

#ifdef DLL_EXPORT
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI __declspec(dllimport)
#endif // DLL_EXPORT


#ifdef __cplusplus
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
extern "C" {
#endif

#include "SocketEx.h"
#include "glow.h"
#include "emberinternal.h"
#include <limits.h>


// ====================================================================
// 
// ====================================================================

/// <summary>コンシューマ機能離脱要求</summary>
/// <remarks>
/// GlowType の適用外値
/// </remarks>
#define QUIT_REQUEST_CONSUMER	0xFFFF

#pragma pack(1)

typedef struct STarget
{
	berint number;
	berint* pConnectedSources;
	int connectedSourcesCount;
} Target;

typedef struct SSource
{
	berint number;
} Source;

typedef struct SPtrListNode
{
	voidptr value;
	struct SPtrListNode* pNext;
} PtrListNode;

typedef struct SPtrList
{
	PtrListNode* pHead;
	PtrListNode* pLast;
	int count;
} PtrList;

typedef struct tagGlowNodeId
{
	pstr pIdentifier;
	pstr pDescription;
} GlowNodeId;

typedef struct SElement
{
	berint number;
	GlowElementType type;
	GlowFieldFlags paramFields;

	union
	{
		GlowNodeId nodeId;
		GlowNode node;
		GlowParameter parameter;

		struct
		{
			GlowMatrix matrix;
			PtrList targets;
			PtrList sources;
		} matrix;

		GlowFunction function;
	} glow;

	PtrList children;
	struct SElement* pParent;
} Element;

typedef struct tagEmberStringValue
{
	int length;
	pstr pString;
} EmberStringValue;
typedef struct tagRequestId
{
	/// <summary>識別</summary>
	int id;
	/// <summary>ボタン（デバイス識別）</summary>
	int buttonIndex;
	/// <summary>グループ</summary>
	int group;
	/// <summary>ページ</summary>
	int page;
} RequestId;
/// <summary>
/// 対プロバイダ要求／返却データ
/// </summary>
typedef struct tagEmberContent
{
	/// <summary>要求識別</summary>
	RequestId requestId;

	/// <summary>種別</summary>
	GlowType type;
	/// <summary>フィールドフラグ</summary>
	/// <remarks>Node,Parameter でのみ使用</remarks>
	GlowFieldFlags fields;
	/// <summary>ノードパス</summary>
	berint* pPath;
	/// <summary>ノード長</summary>
	int pathLength;

	/// <summary>重複要求通知</summary>
	int duplicateRequests;

	/// <summary>内容</summary>
	union
	{
		GlowCommand command;

		GlowNodeId nodeId;
		GlowNode node;
		GlowParameter parameter;
		GlowFunction function;
		GlowInvocationResult invocationResult;

		GlowMatrix matrix;
		GlowSignal signal;
		GlowConnection connection;

		EmberStringValue stringValue;
	};
} EmberContent;

typedef struct tagRemoteContent
{
	short id;
	SOCKET hSocket;
	struct sockaddr_in remoteAddr;

	dword reconnectDelay;
	dword threadDelay;

	Element* pTopNode;
} RemoteContent;

typedef struct tagSession
{
	RemoteContent remoteContent;
	EmberContent* pRequest;

	Element root;
} Session;

#pragma pack()

// ====================================================================
// 
// ====================================================================

/// <summary>libember_slim初期処理</summary>
/// <remarks>
/// runConsumer 呼び出し以前に
/// newobj/newarr/allocMemory/freeMemory 使用したい場合に呼び出す
/// </remarks>
DLLAPI extern void initEmberContents();

extern berint* element_getPath(const Element* pThis, berint* pBuffer, int* pCount);

/// <summary>ツリー先頭取得</summary></summary>
/// <returns></returns>
extern Element* getRootTop();
/// <summary>ツリー先頭取得有無</summary>
/// <returns></returns>
extern bool hasEmberTree();
/// <summary>ツリー内エレメント取得</summary>
/// <param name="pThis"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="ppParent"></param>
/// <returns></returns>
extern Element* element_findDescendant(const Element* pThis, const berint* pPath, int pathLength, Element** ppParent);

/// <summary>
/// Ember GetDirectory 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
DLLAPI extern EmberContent* createEmberGetDirectoryContent(RequestId* pId, const berint* pPath, int pathLength);
/// <summary>
/// Ember Invoke 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pInvocation"></param>
/// <returns></returns>
extern EmberContent* createEmberInvokeContent(RequestId* pId, const berint* pPath, int pathLength, GlowInvocation* pInvocation);
/// <summary>
/// Ember パラメータ取得 送信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="fields"></param>
/// <returns></returns>
extern EmberContent* createEmberGetParameterContent(RequestId* pId, const berint* pPath, int pathLength);
/// <summary>
/// Ember パラメータ設定 送信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <param name="fields"></param>
/// <returns></returns>
extern EmberContent* createEmberSetParameterContent(RequestId* pId, const berint* pPath, int pathLength, const GlowParameter* pValue);
/// <summary>
/// Ember マトリックス接続 送受信要素生成
/// </summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <param name="pValue"></param>
/// <returns></returns>
extern EmberContent* createEmberConnectionContent(RequestId* pId, const berint* pPath, int pathLength, const GlowConnection* pValue);

/// <summary>
/// 文字列パス→パス
/// </summary>
/// <param name="pRoot"></param>
/// <param name="pathValue"></param>
/// <param name="ppPath"></param>
/// <returns></returns>
extern size_t convertString2Path(Element* pRoot, const pstr pathValue, berint** ppPath);
/// <summary>
/// パス→文字列パス
/// </summary>
/// <param name="pRoot"></param>
/// <param name="pPath"></param>
/// <param name="pPathLength"></param>
/// <returns></returns>
extern pstr convertPath2String(Element* pRoot, const berint* pPath, int pathLength);


/// <summary>コンシューマ機能</summary>
/// <param name="pRemoteContent"></param>
extern void runConsumer(RemoteContent* pRemoteContent);

extern bool Call_handleInput(EmberContent* pRequest);


#ifdef __cplusplus
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
#endif  // EMBER_CONSUMER_H