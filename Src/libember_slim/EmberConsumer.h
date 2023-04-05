#pragma once

#include "SocketEx.h"
#include "ClientConfig.h"
#include "APIFormat.h"
#include "ember_consumer.h"
#include <string>
#include <mutex>
#include <queue>
#include <thread>
#include <time.h>
#include <iostream>


// ====================================================================

/// <summary>
/// MatrixLabels
/// マトリックス情報クラス
/// </summary>
class MatrixLabels
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	MatrixLabels();
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="pContent">受信したマトリックス情報</param>
	/// <remarks>
	/// Worker からのみ呼び出される想定
	/// </remarks>
	MatrixLabels(Element* pRoot, const EmberContent* pContent);

	/// <summary>初期化済</summary>
	/// <returns></returns>
	bool Initialized() { return m_bInitialized; }
	/// <summary>Matrix ノードパス（文字列）</summary>
	std::string MatrixNodePath() { return m_sMatrixNodePath; }
	/// <summary>データ有無効</summary>
	/// <returns></returns>
	bool Valid() { return Initialized() && (m_nTargetsLabelNodePathLength > 0) && (m_nSourcesLabelNodePathLength > 0); }
	/// <summary>参照可否</summary>
	/// <returns></returns>
	bool Enabled() { return Initialized() && !m_mpTargetLabels.empty() && !m_mpSourceLabels.empty(); }

	/// <summary>Target ラベル取得</summary>
	/// <param name="number"></param>
	/// <param name="sLabel"></param>
	/// <returns></returns>
	bool GetTargetLabel(const berint number, std::string& sLabel) { return GetLabel(m_mpTargetLabels, number, sLabel); }
	/// <summary>Source ラベル取得</summary>
	/// <param name="number"></param>
	/// <param name="sLabel"></param>
	/// <returns></returns>
	bool GetSourceLabel(const berint number, std::string& sLabel) { return GetLabel(m_mpSourceLabels, number, sLabel); }
	/// <summary>ラベル設定</summary>
	/// <param name="pContent"></param>
	/// <returns></returns>
	/// <remarks>
	/// パラメータ取得時のラベル文字列更新
	/// </remarks>
	bool SetLabel(const EmberContent* pContent);

private:
	/// <summary>初期化</summary>
	/// <param name="matrix"></param>
	void Initialize();
	/// <summary>初期化</summary>
	/// <param name="pContent"></param>
	void Initialize(Element* pRoot, const EmberContent* pContent);

	/// <summary>ラベル取得</summary>
	/// <param name="map">参照マップ</param>
	/// <param name="number"></param>
	/// <param name="sLabel"></param>
	/// <returns></returns>
	bool GetLabel(const std::unordered_map<berint, std::string>& map, const berint number, std::string& sLabel);

	/// <summary>Matrix ノードパス</summary>
	/// <remarks>固定長で所持</remarks>
	berint m_aMatrixNodePath[GLOW_MAX_TREE_DEPTH];
	/// <summary>Matrix Targets Label ノードパス長</summary>
	int m_nMatrixNodePathLength;
	/// <summary>Matrix ノードパス（文字列）</summary>
	/// <remarks>ラベル参照時照合用</remarks>
	std::string m_sMatrixNodePath;

	/// <summary>Matrix Targets Label ノードパス</summary>
	/// <remarks>固定長で所持</remarks>
	berint m_aTargetsLabelNodePath[GLOW_MAX_TREE_DEPTH];
	/// <summary>Matrix Targets Label ノードパス長</summary>
	int m_nTargetsLabelNodePathLength;
	/// <summary>Target ラベル</summary>
	/// <remarks>
	/// 識別の出現順が保証されていないので
	/// vector ではなく map に識別と文字列を対で保持する
	/// </remarks>
	std::unordered_map<berint, std::string> m_mpTargetLabels;

	/// <summary>Matrix Sources Label ノードパス</summary>
	/// <remarks>固定長で所持</remarks>
	berint m_aSourcesLabelNodePath[GLOW_MAX_TREE_DEPTH];
	/// <summary>Matrix Sources Label ノードパス長</summary>
	int m_nSourcesLabelNodePathLength;
	/// <summary>Source ラベル</summary>
	/// <remarks>
	/// 識別の出現順が保証されていないので
	/// vector ではなく map に識別と文字列を対で保持する
	/// </remarks>
	std::unordered_map<berint, std::string>	m_mpSourceLabels;

	/// <summary>初期化済</summary>
	bool m_bInitialized;
};


// ====================================================================

/// <summary>
/// CEmberConsumer
/// Ember コンシューマクラス
/// </summary>
/// <remarks>
/// 継承使用想定
/// </remarks>
class CEmberConsumer
{
public:
	/// <summary>ソケット識別</summary>
	/// <returns></returns>
	ClientSocketId SocketId() { return (ClientSocketId)m_sRemoteContent.id; }

	/// <summary>Client クラスインスタンス参照可否</summary>
	/// <returns></returns>
	bool IsValidClientConfig() { return m_pClientConfig != nullptr; }
	/// <summary>初期化済</summary>
	/// <returns></returns>
	bool Initialized() { return m_bInitialized; }
	/// <summary>有無効</summary>
	/// <returns></returns>
	bool Enabled() { return Initialized() && IsValidClientConfig() && IsEmberId(SocketId()) && m_pClientConfig->Enabled(SocketId()); }
	/// <summary>要求可否</summary>
	/// <returns></returns>
	bool CanRequest() { return Enabled() && m_bHasEmberRoot; }

	/// <summary></summary>
	/// <returns></returns>
	bool CancelRequest();
	/// <summary>コンシューマ離脱要求取得</summary>
		/// <returns></returns>
	bool IsCancelRequest();

	/// <summary>コンシューマ操作要求追加</summary>
	/// <param name="nDestConnSignal"></param>
	/// <param name="vSrcs"></param>
	/// <returns></returns>
	int GetSignalValue(int nDestConnSignal, std::vector<int>& vSrcs);
	/// <summary>コンシューマ操作要求追加</summary>
	/// <param name="nDestConnSignal"></param>
	/// <param name="nSignalCount"></param>
	/// <param name="vSrcs"></param>
	/// <returns></returns>
	int GetSignalValues(int nDestConnSignal, int nSignalCount, std::vector<int>& vSrcs);

	/// <summary>ディレクトリ取得要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <returns></returns>
	EmberContent* CreateGetDirectoryRequest(RequestId* pId, std::string sPath);
	/// <summary>パラメータ取得要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <returns></returns>
	EmberContent* CreateGetParameterRequest(RequestId* pId, std::string sPath);
	/// <summary>パラメータ設定要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	EmberContent* CreateSetParameterRequest(RequestId* pId, std::string sPath, GlowParameter& value);
	/// <summary>マトリックス接続要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	EmberContent* CreateConnectionRequest(RequestId* pId, std::string sPath, GlowConnection& value);
	/// <summary>ファンクション実行要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	EmberContent* CreateInvokeRequest(RequestId* pId, std::string sPath, GlowInvocation& value);

	/// <summary>コンシューマ操作要求追加</summary>
	/// <param name="pRequest"></param>
	/// <returns></returns>
	int AddConsumerRequest(EmberContent* pRequest);
	/// <summary>コンシューマ操作要求追加</summary>
	/// <param name="pId"></param>
	/// <param name="sPath"></param>
	/// <param name="sValue"></param>
	/// <returns></returns>
	int AddConsumerRequest(RequestId* pId, std::string sPath, std::string sValue, GlowParameterType Type = GlowParameterType_Integer);
	/// <summary>コンシューマ操作要求追加</summary>
	/// <param name="pId"></param>
	/// <param name="sPath"></param>
	/// <param name="nDestTopSignal"></param>
	/// <param name="nDestConnCount"></param>
	/// <param name="nSrcTopSignal"></param>
	/// <param name="nSrcConnCount"></param>
	/// <returns></returns>
	int AddConsumerRequest(RequestId* pId, std::string sPath, int nDestTopSignal, int nDestConnCount, int nSrcTopSignal, int nSrcConnCount);

	/// <summary>コンシューマ受信通知</summary>
	/// <summary>コンシューマ操作要求取得</summary>
	/// <returns></returns>
	EmberContent* GetConsumerRequest();
	/// <summary>コンシューマ受信通知</summary>
	/// <param name="pResult"></param>
	void NotifyReceivedConsumerResult(EmberContent* pResult);

	/// <summary>Client処理用のコンシューマ操作結果格納</summary>
/// <param name="pResult"></param>
	void CEmberConsumer::AddSendMessage(EmberContent* pResult);
	/// <summary>Client処理用のコンシューマ操作結果取得</summary>
	/// <returns></returns>
	EmberContent* CEmberConsumer::GetSendMessage();

	/// <summary>マトリックス操作通知回数</summary>
	/// <returns></returns>
	/// <remarks>
	/// マトリックス情報／接続切替／該当パラメータ受信が対象
	/// </remarks>
	unsigned short MatrixNoticeCount();
	/// <summary>マトリックスラベル使用有無</summary>
	/// <returns></returns>
	bool UseMatrixLabels() { return m_bUseMatrixLabels; }
	/// <summary>マトリックスラベル情報有無効</summary>
	/// <returns></returns>
	bool EnableMatrixLabels() { return UseMatrixLabels() && (MatrixNoticeCount() > 0); }

	/// <summary>Target ラベル取得</summary>
	/// <param name="sEmberPath"></param>
	/// <param name="nSignalNumber"></param>
	/// <param name="sLabel"></param>
	/// <returns></returns>
	bool GetTargetLabel(const std::string sEmberPath, berint nSignalNumber, std::string& sLabel) { return GetLabel(1, sEmberPath, nSignalNumber, sLabel); }
	/// <summary>Source ラベル取得</summary>
	/// <param name="sEmberPath"></param>
	/// <param name="nSignalNumber"></param>
	/// <param name="sLabel"></param>
	/// <returns></returns>
	bool GetSourceLabel(const std::string sEmberPath, berint nSignalNumber, std::string& sLabel) { return GetLabel(2, sEmberPath, nSignalNumber, sLabel); }

	/// <summary></summary>
	static void Worker(CEmberConsumer* instance);
	/// <summary></summary>
	static void Watcher(CEmberConsumer* instance);

	/// <summary>
	/// コンシューマ要求用データ生成
	/// </summary>
	/// <param name="type"></param>
	/// <param name="sValue"></param>
	/// <returns></returns>
	GlowValue* CreateGlowValue(GlowParameterType type, std::string sValue);
	/// <summary></summary>
	/// <param name="path"></param>
	/// <param name="pPath"></param>
	/// <returns></returns>
	size_t GetNodePath(std::string sPath, berint** pPath);
	/// <summary>パラメータ設定要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	EmberContent* CreateSetParameterRequest(RequestId* pId, berint* pPath, int pathLength, GlowParameter& value) { return createEmberSetParameterContent(pId, pPath, pathLength, &value); }
	/// <summary>ファンクション実行要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	EmberContent* CreateInvokeRequest(RequestId* pId, berint* pPath, int pathLength, GlowInvocation& value) { return createEmberInvokeContent(pId, pPath, pathLength, &value); }

	/// <summary>接続情報</summary>
	RemoteContent m_sRemoteContent;
	/// <summary></summary>

	/// <summary></summary>
	bool m_bUpdateDetected;

	clock_t start;
	clock_t end;
	void ProcessTimeDisp();

protected:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CEmberConsumer();
	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <param name="socketId">ソケット識別</param>
	CEmberConsumer(const ClientSocketId socketId);
	/// <summary>
	/// デストラクタ
	/// </summary>
	virtual ~CEmberConsumer();

	/// <summary>
	/// メンバ初期化
	/// </summary>
	void Initialize();
	/// <summary>
	/// メンバ初期化
	/// </summary>
	/// <param name="socketId">ソケット識別</param>
	void Initialize(const ClientSocketId socketId);

	/// <summary>
	/// コンシューマ操作要求識別発行
	/// </summary>
	/// <returns></returns>
	int IssueConsumerRequestId();
	/// <summary>コンシューマ操作結果取得</summary>
	/// <returns></returns>
	EmberContent* GetConsumerResult();
	/// <summary>要求済コンシューマ操作取得</summary>
	/// <returns></returns>
	EmberContent* GetConsumerRequestedContent(int id);

	/// <summary>ディレクトリ取得要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <returns></returns>
	EmberContent* CreateGetDirectoryRequest(RequestId* pId, berint* pPath, int pathLength) { return createEmberGetDirectoryContent(pId, pPath, pathLength); }
	/// <summary>パラメータ取得要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <returns></returns>
	EmberContent* CreateGetParameterRequest(RequestId* pId, berint* pPath, int pathLength) { return createEmberGetParameterContent(pId, pPath, pathLength); }
	/// <summary>マトリックス接続要求生成</summary>
	/// <param name="pId"></param>
	/// <param name="pPath"></param>
	/// <param name="pathLength"></param>
	/// <param name="value"></param>
	/// <returns></returns>
	EmberContent* CreateConnectionRequest(RequestId* pId, berint* pPath, int pathLength, GlowConnection& value) { return createEmberConnectionContent(pId, pPath, pathLength, &value); }

	/// <summary>マトリックス操作通知回数加算</summary>
	/// <returns></returns>
	/// <remarks>
	/// マトリックス情報／接続切替／該当パラメータ受信が対象
	// </remarks>
	unsigned short IncrementMatrixNoticeCount();
	/// <summary>マトリックスラベル情報初期化</summary>
	/// <returns></returns>
	bool ClearMatrixLabels();
	/// <summary>マトリックスラベル情報リセット</summary>
	/// <returns></returns>
	bool ResetMatrixLabels();
	/// <summary>マトリックスラベル情報リセット</summary>
	/// <param name="pContent"></param>
	/// <returns></returns>
	bool ResetMatrixLabels(const EmberContent* pContent);
	/// <summary>ラベル取得</summary>
	/// <param name="kind">1 : target, 2 : source</param>
	/// <param name="sEmberPath"></param>
	/// <param name="nSignalNumber"></param>
	/// <param name="sLabel"></param>
	/// <returns></returns>
	bool GetLabel(int kind, std::string sEmberPath, berint nSignalNumber, std::string& sLabel);

	bool m_bHasEmberRoot;

	/// <summary></summary>
	std::unique_ptr <std::thread> m_ptWorker;
	/// <summary></summary>
	std::unique_ptr <std::thread> m_ptWatcher;
	/// <summary></summary>
	std::mutex m_mtxCancelRequest;
	/// <summary></summary>
	bool m_bCancelRequest;

	/// <summary></summary>
	std::mutex m_mtxConsumerRequest;
	/// <summary>コンシューマ操作要求最終識別</summary>
	int m_nLastConsumerRequestId;
	/// <summary>コンシューマ操作要求前</summary>
	std::deque<EmberContent*> m_qPreConsumerRequests;
	/// <summary>コンシューマ操作要求済</summary>
	std::deque<EmberContent*> m_qConsumerRequests;
	/// <summary>コンシューマ操作結果</summary>
	std::deque<EmberContent*> m_qConsumerResults;
	
	std::mutex m_mtxSendMessage;
	/// <summary>送信用メッセージ格納キュー</summary>
	std::deque<EmberContent*> m_qSendMessage;

	/// <summary></summary>
	std::mutex m_mtxMatrixNotice;
	/// <summary>マトリックス操作通知回数</summary>
	unsigned short m_nMatrixNoticeCount;
	/// <summary>最終接続通知マトリックスパス</summary>
	std::string m_sLastNotifyMatrixPath;
	/// <summary>マトリックスラベル情報</summary>
	std::vector<MatrixLabels*> m_vMatrixLabels;
	/// <summary>マトリックスラベル使用有無</summary>
	bool m_bUseMatrixLabels;

	/// <summary>クライアント用情報クラスインスタンス</summary>
	CClientConfig* m_pClientConfig;
	/// <summary>初期化済</summary>
	bool m_bInitialized;
};

/// <summary>
/// CNmosEmberConsumer
/// NMOS-Ember コンシューマクラス
/// </summary>
/// <remarks>
/// 継承使用想定
/// </remarks>
class CNmosEmberConsumer : public CEmberConsumer
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CNmosEmberConsumer() : CEmberConsumer(ClientSocketId::SOCK_NMOS_EMBER) {}
};

/// <summary>
/// CMvEmberConsumer
/// MV-Ember コンシューマクラス
/// </summary>
/// <remarks>
/// 継承使用想定
/// </remarks>
class CMvEmberConsumer : public CEmberConsumer
{
public:
	/// <summary>
	/// コンストラクタ
	/// </summary>
	CMvEmberConsumer() : CEmberConsumer(ClientSocketId::SOCK_MV_EMBER) {}
};

/// <summary>
/// NMOS Ember コンシューマインスタンス（実体）
/// </summary>
extern CNmosEmberConsumer* m_pNmosEmberConsumer;
/// <summary>
/// MV Ember コンシューマインスタンス（実体）
/// </summary>
extern CMvEmberConsumer* m_pMVEmberConsumer;
