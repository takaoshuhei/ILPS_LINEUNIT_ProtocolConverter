#include "ClientConfig.h"
#include "EmberConsumer.h"
#include "Utilities.h"
#include "ember_consumer.h"
#include <cassert>
#include <regex>

using namespace utilities;

#undef min
#undef max


// ====================================================================
/// <summary>
/// コンストラクタ
/// </summary>
MatrixLabels::MatrixLabels()
{
	Initialize();
}
/// <summary>
/// コンストラクタ
/// </summary>
/// <param name="pContent">受信したマトリックス情報</param>
/// <remarks>
/// Worker からのみ呼び出される想定
/// </remarks>
MatrixLabels::MatrixLabels(Element* pRoot, const EmberContent* pContent)
{
	Initialize(pRoot, pContent);
}

/// <summary>初期化</summary>
/// <param name="matrix"></param>
void MatrixLabels::Initialize()
{
	memset(m_aMatrixNodePath, 0, sizeof(m_aMatrixNodePath));
	m_nMatrixNodePathLength = 0;
	m_sMatrixNodePath.clear();

	memset(m_aTargetsLabelNodePath, 0, sizeof(m_aTargetsLabelNodePath));
	m_nTargetsLabelNodePathLength = 0;
	m_mpTargetLabels.clear();

	memset(m_aSourcesLabelNodePath, 0, sizeof(m_aSourcesLabelNodePath));
	m_nSourcesLabelNodePathLength = 0;
	m_mpSourceLabels.clear();

	m_bInitialized = true;
}
/// <summary>初期化</summary>
/// <param name="pContent"></param>
void MatrixLabels::Initialize(Element* pRoot, const EmberContent* pContent)
{
	Initialize();
	if (!pContent
	 || (pContent->type != GlowType_Matrix)
	 || !pContent->pPath
	 || !utilities::IsRange(pContent->pathLength, 0, GLOW_MAX_TREE_DEPTH - 1)
	 || !pContent->matrix.pLabels
	 || !utilities::IsRange(pContent->matrix.labelsLength, 0, GLOW_MAX_TREE_DEPTH - 1)
	 || !utilities::IsRange(pContent->matrix.pLabels[0].basePathLength, 0, GLOW_MAX_TREE_DEPTH - 2)
	 || ((pContent->matrix.labelsLength >= 2)
	  && !utilities::IsRange(pContent->matrix.pLabels[1].basePathLength, 0, GLOW_MAX_TREE_DEPTH - 2)))
		return;
	m_bInitialized = false;

	// マトリックス自身の情報
	auto pMatrix = &pContent->matrix;
	for (int i = 0; i < pContent->pathLength; ++i)
		m_aMatrixNodePath[i] = pContent->pPath[i];
	m_nMatrixNodePathLength = pContent->pathLength;
	pstr pathName = convertPath2String(pRoot, pContent->pPath, pContent->pathLength);
	if (pathName)
		m_sMatrixNodePath = std::string(pathName);
	//else
	//	return;

	// 設定が1つの場合、target/source ノードの1つ上と判断
	try
	{
		if (pMatrix->labelsLength < 2)
		{
			for (int i = 0; i < pMatrix->pLabels[0].basePathLength; ++i)
			{
				m_aTargetsLabelNodePath[i] = pMatrix->pLabels[0].basePath[i];
				m_aSourcesLabelNodePath[i] = pMatrix->pLabels[0].basePath[i];
			}
			m_aTargetsLabelNodePath[pMatrix->pLabels[0].basePathLength] = 1;	// target は末尾が 1
			m_aSourcesLabelNodePath[pMatrix->pLabels[0].basePathLength] = 2;	// source は末尾が 2
			m_nTargetsLabelNodePathLength = m_nSourcesLabelNodePathLength = pMatrix->pLabels[0].basePathLength + 1;
		}
		// 設定が2つの場合、target/source ノードそれぞれと判断
		else
		{
			for (int i = 0; i < pMatrix->pLabels[0].basePathLength; ++i)
				m_aTargetsLabelNodePath[i] = pMatrix->pLabels[0].basePath[i];
			m_nTargetsLabelNodePathLength = pMatrix->pLabels[0].basePathLength;
			for (int i = 0; i < pMatrix->pLabels[1].basePathLength; ++i)
				m_aSourcesLabelNodePath[i] = pMatrix->pLabels[1].basePath[i];
			m_nSourcesLabelNodePathLength = pMatrix->pLabels[1].basePathLength;
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		m_nTargetsLabelNodePathLength = m_nSourcesLabelNodePathLength = 0;
	}
	// 何かしら展開できず不成立
	if ((m_nTargetsLabelNodePathLength <= 0) || (m_nSourcesLabelNodePathLength <= 0))
	{
		Initialize();
		return;
	}

	// 対象エレメント抽出
	// マトリックスとパラメータの受信順によってはこの段階で取れない可能性あり
	try
	{
		auto pTargetsLabelNodeElement = element_findDescendant(pRoot, m_aTargetsLabelNodePath, m_nTargetsLabelNodePathLength, NULL);
		if (pTargetsLabelNodeElement)
		{
			// 子供のパラメータをすべて保持する
			for (auto pNode = pTargetsLabelNodeElement->children.pHead; pNode != NULL; pNode = pNode->pNext)
			{
				auto pChild = (Element*)pNode->value;
				if (!pChild
				 || ((pChild->type != GlowElementType_Node) && (pChild->type != GlowElementType_Parameter)))
					continue;

				berint nNumber = pChild->number;
				std::string sValue = std::string(pChild->glow.nodeId.pIdentifier);

				if ((pChild->type == GlowElementType_Parameter)
				 //&& (pChild->glow.parameter.type == GlowParameterType_String)	// InterBee用プロバイダでは設定なし
				 && (pChild->glow.parameter.value.flag == GlowParameterType_String)
				 && (pChild->glow.parameter.value.choice.pString != nullptr)
				 && (strlen(pChild->glow.parameter.value.choice.pString) > 0))
				{
					sValue = std::string(pChild->glow.parameter.value.choice.pString);
				}
				m_mpTargetLabels.insert(std::pair(nNumber, sValue));
			}
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		m_mpTargetLabels.clear();
	}
	try
	{
		auto pSourcesLabelNodeElement = element_findDescendant(pRoot, m_aSourcesLabelNodePath, m_nSourcesLabelNodePathLength, NULL);
		if (pSourcesLabelNodeElement)
		{
			// 子供のパラメータをすべて保持する
			for (auto pNode = pSourcesLabelNodeElement->children.pHead; pNode != NULL; pNode = pNode->pNext)
			{
				auto pChild = (Element*)pNode->value;
				if (!pChild
				 || ((pChild->type != GlowElementType_Node) && (pChild->type != GlowElementType_Parameter)))
					continue;

				berint nNumber = pChild->number;
				std::string sValue = std::string(pChild->glow.nodeId.pIdentifier);

				if ((pChild->type == GlowElementType_Parameter)
				 //&& (pChild->glow.parameter.type == GlowParameterType_String)	// InterBee用プロバイダでは設定なし
				 && (pChild->glow.parameter.value.flag == GlowParameterType_String)
				 && (pChild->glow.parameter.value.choice.pString != nullptr)
				 && (strlen(pChild->glow.parameter.value.choice.pString) > 0))
				{
					sValue = std::string(pChild->glow.parameter.value.choice.pString);
				}
				m_mpSourceLabels.insert(std::pair(nNumber, sValue));
			}
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		m_mpSourceLabels.clear();
	}

	m_bInitialized = true;
}

/// <summary>ラベル取得</summary>
/// <param name="map">参照マップ</param>
/// <param name="number"></param>
/// <param name="sLabel"></param>
/// <returns></returns>
bool MatrixLabels::GetLabel(const std::unordered_map<berint, std::string>& map, const berint number, std::string& sLabel)
{
	sLabel = "";
	bool res = Enabled();
	if (res)
	{
		auto itr = map.find(number);
		if (itr != map.end())
			sLabel = (*itr).second;
	}
	return res;
}
/// <summary>ラベル設定</summary>
/// <param name="pContent"></param>
/// <returns></returns>
/// <remarks>
/// パラメータ取得時のラベル文字列更新
/// </remarks>
bool MatrixLabels::SetLabel(const EmberContent* pContent)
{
	bool bUpdate = false;
	if (!Valid()
	 || !pContent
	 || !pContent->pPath
	 || !utilities::IsRange(pContent->pathLength, 1, GLOW_MAX_TREE_DEPTH - 1)
	 || ((pContent->type != GlowType_Node)
	  && (pContent->type != GlowType_Parameter)))
		return bUpdate;

	// 既存情報にあれば更新、なければ追加
	try
	{
		// ラベル対象パラメータか確認
		if ((pContent->pathLength == (m_nTargetsLabelNodePathLength + 1))
		 && !memcmp(pContent->pPath, m_aTargetsLabelNodePath, m_nTargetsLabelNodePathLength * sizeof(berint)))
		{
			auto nNumber = pContent->pPath[m_nTargetsLabelNodePathLength];
			auto sValue = std::string(pContent->nodeId.pIdentifier);
			if ((pContent->type == GlowType_Parameter)
			 //&& (pContent->parameter.type == GlowParameterType_String)	// InterBee用プロバイダでは設定なし
			 && (pContent->parameter.value.flag == GlowParameterType_String)
			 && (pContent->parameter.value.choice.pString != nullptr)
			 && (strlen(pContent->parameter.value.choice.pString) > 0))
				sValue = std::string(pContent->parameter.value.choice.pString);
			// 既存情報があれば文字列取得ではなくイテレータを探す
			bool bHold = false;
			if (!m_mpTargetLabels.empty())
			{
				// あれば更新
				auto itr = m_mpTargetLabels.find(nNumber);
				if (itr != m_mpTargetLabels.end())
				{
					bHold = true;

					if ((*itr).second != sValue)
					{
						(*itr).second = sValue;
						bUpdate;
					}
				}
			}
			// なければ追加
			if (!bHold)
			{
				m_mpTargetLabels.insert(std::pair(nNumber, sValue));
				bUpdate = bHold = true;
			}
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
	try
	{
		// ラベル対象パラメータか確認
		if ((pContent->pathLength == (m_nSourcesLabelNodePathLength + 1))
		 && !memcmp(pContent->pPath, m_aSourcesLabelNodePath, m_nSourcesLabelNodePathLength * sizeof(berint)))
		{
			auto nNumber = pContent->pPath[m_nSourcesLabelNodePathLength];
			auto sValue = std::string(pContent->nodeId.pIdentifier);
			if ((pContent->type == GlowType_Parameter)
			 //&& (pContent->parameter.type == GlowParameterType_String)	// InterBee用プロバイダでは設定なし
			 && (pContent->parameter.value.flag == GlowParameterType_String)
			 && (pContent->parameter.value.choice.pString != nullptr)
			 && (strlen(pContent->parameter.value.choice.pString) > 0))
			sValue = std::string(pContent->parameter.value.choice.pString);
			// 既存情報があれば文字列取得ではなくイテレータを探す
			bool bHold = false;
			if (!m_mpSourceLabels.empty())
			{
				// あれば更新
				auto itr = m_mpSourceLabels.find(nNumber);
				if (itr != m_mpSourceLabels.end())
				{
					bHold = true;

					if ((*itr).second != sValue)
					{
						(*itr).second = sValue;
						bUpdate;
					}
				}
			}
			// なければ追加
			if (!bHold)
			{
				m_mpSourceLabels.insert(std::pair(nNumber, sValue));
				bUpdate = bHold = true;
			}
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return bUpdate;
}


// ====================================================================

/// <summary>
/// コンストラクタ
/// </summary>
CEmberConsumer::CEmberConsumer()
{
	Initialize();
}
/// <summary>
/// コンストラクタ
/// </summary>
/// <param name="socketId">ソケット識別</param>
CEmberConsumer::CEmberConsumer(const ClientSocketId socketId) : CEmberConsumer()
{
	assert(IsEmberId(socketId));

	// メンバ初期化
	Initialize(socketId);
}
/// <summary>
/// デストラクタ
/// </summary>
CEmberConsumer::~CEmberConsumer()
{
	if (!IsCancelRequest() && (m_ptWorker || m_ptWatcher))
		CancelRequest();

	if (m_ptWorker)
	{
#if 0
		try
		{
			if (m_ptWorker->joinable())
				m_ptWorker->join();
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
#endif
		try
		{
			if (m_ptWorker->joinable())
				m_ptWorker->detach();
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}

		m_ptWorker.reset();
	}
	if (m_ptWatcher)
	{
#if 0
		try
		{
			if (m_ptWatcher->joinable())
				m_ptWatcher->join();
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
#endif
		try
		{
			if (m_ptWatcher->joinable())
				m_ptWatcher->detach();
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}

		m_ptWatcher.reset();
	}
}

/// <summary>
/// メンバ初期化
/// </summary>
void CEmberConsumer::Initialize()
{
	memset(&m_sRemoteContent, 0, sizeof(RemoteContent));
	m_sRemoteContent.id = (short)((ClientSocketId)-1);
	m_bHasEmberRoot = false;
	m_nLastConsumerRequestId = 0;
	if (!m_qPreConsumerRequests.empty())
		m_qPreConsumerRequests.clear();
	if (!m_qConsumerRequests.empty())
		m_qConsumerRequests.clear();
	if (!m_qConsumerResults.empty())
		m_qConsumerResults.clear();
	if (!m_qSendMessage.empty())
		m_qSendMessage.clear();
	m_nMatrixNoticeCount = 0;
	m_sLastNotifyMatrixPath = "";
	if (!m_vMatrixLabels.empty())
		m_vMatrixLabels.clear();
	m_bUseMatrixLabels = false;
	m_ptWorker.reset();
	m_ptWatcher.reset();
	m_pClientConfig = nullptr;
	m_bCancelRequest = false;
	m_bInitialized = false;
	m_bUpdateDetected = false;
	
	start = end = 0;
}
/// <summary>
/// メンバ初期化
/// </summary>
/// <param name="socketId">ソケット識別</param>
void CEmberConsumer::Initialize(const ClientSocketId socketId)
{
	assert(IsEmberId(socketId));

	try
	{
		m_sRemoteContent.id = (short)socketId;
		m_pClientConfig = CClientConfig::GetInstance();
		if ((m_pClientConfig != nullptr) && (m_pClientConfig->Enabled(SocketId())))
		{
			bool res = (socketId == ClientSocketId::SOCK_MV_EMBER)
				     ? m_pClientConfig->CreateMvEmberSocket(m_sRemoteContent.hSocket, m_sRemoteContent.remoteAddr)
				     : m_pClientConfig->CreateNmosEmberSocket(m_sRemoteContent.hSocket, m_sRemoteContent.remoteAddr);
			if (!res && (m_sRemoteContent.hSocket != 0))
				m_sRemoteContent.hSocket = 0;

			m_sRemoteContent.reconnectDelay = m_pClientConfig->SocketReconnectDelay();
			m_sRemoteContent.threadDelay = m_pClientConfig->EmberThreadDelay();

			m_bUseMatrixLabels = (socketId == ClientSocketId::SOCK_MV_EMBER)
							   ? m_pClientConfig->MvEmberUseMatrixLabels()
							   : m_pClientConfig->NmosEmberUseMatrixLabels();
		}

		m_nMatrixNoticeCount = 0;
		m_sLastNotifyMatrixPath = "";
		m_vMatrixLabels.clear();

		m_bInitialized = true;

		if (Enabled())
		{
			m_ptWorker.reset(new std::thread(Worker, this));
			m_ptWatcher.reset(new std::thread(Watcher, this));
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
}

//デバッグ用時間計測
void CEmberConsumer::ProcessTimeDisp()
{
	std::cout << "duration = " << (double)(end - start) << " msec" << std::endl;
	start = end = 0;
}

///// <summary></summary>
///// <param name="mtx"></param>
///// <returns></returns>
//std::unique_lock<std::mutex> _Lock(std::mutex mtx)
//{
//    return std::unique_lock<std::mutex>(mtx);
//}
#define _Lock(mtx) std::unique_lock<std::mutex>(mtx)
/// <summary></summary>
/// <returns></returns>
bool CEmberConsumer::CancelRequest()
{
	auto lock = _Lock(m_mtxCancelRequest);
	m_bCancelRequest = true;
	return m_bCancelRequest;
}
/// <summary>コンシューマ離脱要求取得</summary>
/// <returns></returns>
bool CEmberConsumer::IsCancelRequest()
{
	auto lock = _Lock(m_mtxCancelRequest);
	return m_bCancelRequest;
}

/// <summary>コンシューマ操作要求追加</summary>
/// <param name="nDestConnSignal"></param>
/// <param name="vSrcs"></param>
/// <returns></returns>
int CEmberConsumer::GetSignalValue(int nDestConnSignal, std::vector<int>& vSrcs)
{
	int cnt = 0;
	vSrcs.clear();
	// 確認するパスは最後に接続情報を受信したマトリックス
	if (m_sLastNotifyMatrixPath.empty() || (nDestConnSignal < 0))
		return cnt;
	berint* pPath = nullptr;
	int len = (int)GetNodePath(m_sLastNotifyMatrixPath, &pPath);
	if (len <= 0)
		return cnt;

	Element* pElement = nullptr;
	GlowConnection* pConnection = nullptr;
	berint* pSrcs = nullptr;

	try
	{
		// 対象エレメント抽出
		pElement = element_findDescendant(m_sRemoteContent.pTopNode, pPath, len, NULL);
		if (!pElement)
			return cnt;
		// マトリックス以外は何もしない
		if (pElement->type != GlowElementType_Matrix)
			return cnt;

		Target* pTarget = nullptr;
		PtrListNode* pNode;
		for (pNode = pElement->glow.matrix.targets.pHead; pNode && !pTarget; pNode = pNode->pNext)
		{
			auto _pTarget = (Target*)(pNode->value);
			if (_pTarget->number == nDestConnSignal)
				pTarget = _pTarget;
		}
		if (pTarget && (pTarget->connectedSourcesCount > 0))
		{
			for (int i = 0; i < pTarget->connectedSourcesCount; ++i)
				vSrcs.push_back(pTarget->pConnectedSources[i]);
		}

		cnt = (int)vSrcs.size();
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		vSrcs.clear();
	}

	return cnt;
}
/// <summary>コンシューマ操作要求追加</summary>
/// <param name="nDestConnSignal"></param>
/// <param name="nSignalCount"></param>
/// <param name="vSrcs"></param>
/// <returns></returns>
int CEmberConsumer::GetSignalValues(int nDestConnSignal, int nSignalCount, std::vector<int>& vSrcs)
{
	vSrcs.clear();
	if (nSignalCount <= 0)
		return 0;

	for (int i = 0; i < nSignalCount; ++i)
	{
		try
		{
			std::vector<int> _vSrcs{};
			int _cnt = GetSignalValue(nDestConnSignal + i, _vSrcs);
			if (_cnt > 0 && !_vSrcs.empty())
			{
				vSrcs.insert(vSrcs.end(), _vSrcs.begin(), _vSrcs.end());
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			vSrcs.clear();
		}
	}

	return (int)vSrcs.size();
}

/// <summary>
/// コンシューマ操作要求識別発行
/// </summary>
/// <returns>1..INT_MAX</returns>
int CEmberConsumer::IssueConsumerRequestId()
{
	if (m_nLastConsumerRequestId == INT_MAX)
		m_nLastConsumerRequestId = 1;
	else
		++m_nLastConsumerRequestId;

	return m_nLastConsumerRequestId;
}
/// <summary>コンシューマ操作要求追加</summary>
/// <param name="pRequest"></param>
/// <returns></returns>
int CEmberConsumer::AddConsumerRequest(EmberContent* pRequest)
{
	int id = 0;
	if (IsCancelRequest() || !pRequest)
		return id;

	auto lock = _Lock(m_mtxConsumerRequest);
	// 識別を付番した上でキューに追加
	id = IssueConsumerRequestId();
	try
	{
		pRequest->requestId.id = id;
		// Invocation の場合付番
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
		if ((pRequest->type == GlowType_Command)
		 && (pRequest->command.number == GlowCommandType_Invoke))
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			pRequest->command.options.invocation.invocationId = id;
		m_qPreConsumerRequests.push_back(pRequest);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	// 追加が正しく実施されたか確認
	bool res = false;
	if (!m_qPreConsumerRequests.empty())
	{
		EmberContent* addCont = m_qPreConsumerRequests.back();
		res = addCont && (addCont->requestId.id == id);
	}

	if (!res && (id != 0))
		id = 0;

	return id;
}
/// <summary>コンシューマ操作要求追加</summary>
/// <param name="pId"></param>
/// <param name="sPath"></param>
/// <param name="sValue"></param>
/// <returns></returns>
int CEmberConsumer::AddConsumerRequest(RequestId* pId, std::string sPath, std::string sValue, GlowParameterType Type)
{
	int id = 0;
	if (!m_sRemoteContent.pTopNode)
		return id;
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	if (len <= 0)
		return id;

	std::string _sValue = sValue;
	Element* pElement = nullptr;
	GlowValue* pValue = nullptr;
	GlowParameter* pParameter = nullptr;
	GlowConnection* pConnection = nullptr;
	GlowInvocation* pInvocation = nullptr;

	try
	{
		// 対象エレメント抽出
		pElement = element_findDescendant(m_sRemoteContent.pTopNode, pPath, len, NULL);
		if (!pElement)
			return id;

		// タイプ別
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
		switch (pElement->type)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		{
		case GlowElementType_Parameter:
		{
			Trace(__FILE__, __LINE__, __FUNCTION__, "type = %d\n", pElement->glow.parameter.value.flag);
			//pValue = CreateGlowValue(pElement->glow.parameter.value.flag, _sValue);
			if (Type == GlowParameterType::GlowParameterType_Real)
				pValue = CreateGlowValue(GlowParameterType::GlowParameterType_Real, _sValue);
			else
				pValue = CreateGlowValue(GlowParameterType::GlowParameterType_Integer, _sValue);
			if (pValue)
			{
				pParameter = newobj(GlowParameter);
				bzero_item(*pParameter);
				glowValue_copyFrom(&pParameter->value, pValue);
				id = AddConsumerRequest(CreateSetParameterRequest(pId, pPath, len, *pParameter));
				glowParameter_free(pParameter);		// CreateSetParameterRequest で EmberContent 上の parameter にコピーしたのでこの時点で用無し
				pParameter = nullptr;

				freeMemory(pValue);
				pValue = nullptr;
			}
		}
		break;

		case GlowElementType_Matrix:
		{
		}
		break;

		case GlowElementType_Function:
		{
			// Value が必要か
			if ((pElement->glow.function.argumentsLength > 0) && (pElement->glow.function.pArguments != nullptr))
			{
				// 1つのみ対応
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
				pValue = CreateGlowValue(pElement->glow.function.pArguments->type, _sValue);
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			}

			GlowInvocation* pInvocation = newobj(GlowInvocation);
			bzero_item(*pInvocation);
			if (pValue)
			{
				pInvocation->pArguments = pValue;	// Invocation 上の pArguments は GlowValue のアドレス
				pInvocation->argumentsLength = 1;
			}
			id = AddConsumerRequest(CreateInvokeRequest(pId, pPath, len, *pInvocation));
			glowInvocation_free(pInvocation);		// CreateSetParameterRequest で EmberContent 上の invocation にコピーしたのでこの時点で用無し
													// この時 pArguments も別インスタンスにコピーしている
#if false
			if (pValue)
			{
				glowValue_free(pValue);				// EmberContent 上の invocation 内 pArguments とは別物
				pValue = nullptr;
			}
#endif
		}
		break;
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	try
	{
		if (pParameter)
			glowParameter_free(pParameter);
		if (pConnection)
			glowConnection_free(pConnection);
		if (pInvocation)
			glowInvocation_free(pInvocation);
#if false
		if (pValue)
			glowValue_free(pValue);
#endif
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return id;
}
/// <summary>コンシューマ操作要求追加</summary>
/// <param name="pId"></param>
/// <param name="sPath"></param>
/// <param name="nDestConnSignal"></param>
/// <param name="nDestConnCount"></param>
/// <param name="nSrcConnSignal"></param>
/// <param name="nSrcConnCount"></param>
/// <returns></returns>
int CEmberConsumer::AddConsumerRequest(RequestId* pId, std::string sPath, int nDestConnSignal, int nDestConnCount, int nSrcConnSignal, int nSrcConnCount)
{
	int id = 0;
	if ((nDestConnSignal < 0) || (nDestConnCount <= 0)
		|| (nSrcConnSignal < 0) || (nSrcConnCount <= 0))
		return id;
	if (!m_sRemoteContent.pTopNode)
		return id;
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	if (len <= 0)
		return id;

	Element* pElement = nullptr;
	GlowConnection* pConnection = nullptr;
	berint* pSrcs = nullptr;

	try
	{
		// 対象エレメント抽出
		pElement = element_findDescendant(m_sRemoteContent.pTopNode, pPath, len, NULL);
		if (!pElement)
			return id;
		// マトリックス以外は何もしない
		if (pElement->type != GlowElementType_Matrix)
			return id;

		// 切断は考慮しない
		pConnection = newobj(GlowConnection);
		bzero_item(*pConnection);
		int nSrcCount = std::max(nSrcConnCount, pElement->glow.matrix.matrix.sourceCount);
		pSrcs = newarr(berint, nSrcCount);
		memset(pSrcs, 0, sizeof(berint) * nSrcCount);
		pConnection->pSources = pSrcs;

		// コネクション情報を生成する
		// 当該用件では OneToN のみ運用と思われるが
		// 定義されている MatrixType を考慮しておく
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
		pConnection->operation = GlowConnectionOperation_Connect;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		switch (pElement->glow.matrix.matrix.type)
		{
		case GlowMatrixType_OneToN:
		case GlowMatrixType_OneToOne:
			{
				int cnt = std::min(nDestConnCount, nSrcConnCount);
				pConnection->sourcesLength = 1;
				for (int i = 0; i < cnt; ++i)
				{
					pConnection->target = (berint)(nDestConnSignal + i);
					pConnection->pSources[0] = (berint)(nSrcConnSignal + i);
					id = AddConsumerRequest(CreateConnectionRequest(pId, pPath, len, *pConnection));
				}
			}
			break;
		case GlowMatrixType_NToN:	
			{
				for (int i = 0; i < nSrcConnCount; ++i)
					pConnection->pSources[i] = (berint)(nSrcConnSignal + i);
				pConnection->sourcesLength = nSrcConnCount;
				for (int i = 0; i < nDestConnCount; ++i)
				{
					pConnection->target = (berint)(nDestConnSignal + i);
					id = AddConsumerRequest(CreateConnectionRequest(pId, pPath, len, *pConnection));
				}
			}
			break;
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	try
	{
		// pSrcs は自身で解放する
		if (pSrcs)
		{
			freeMemory(pSrcs);
			pSrcs = nullptr;
		}
		if (pConnection)
		{
			// pSrcs は解放済
			pConnection->pSources = nullptr;
			glowConnection_free(pConnection);
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return id;
}

/// <summary>コンシューマ操作要求取得</summary>
/// <returns></returns>
EmberContent* CEmberConsumer::GetConsumerRequest()
{
	EmberContent* pRequest = nullptr;
	auto lock = _Lock(m_mtxConsumerRequest);

	if (IsCancelRequest() || m_qPreConsumerRequests.empty())
		return pRequest;

	// 要求前キューの先頭を参照
	try
	{
		pRequest = m_qPreConsumerRequests.front();
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		if (pRequest)
			pRequest = nullptr;
	}
	if (!pRequest)
		return pRequest;

	// 要求キュー末尾に追加
	try
	{
		m_qConsumerRequests.push_back(pRequest);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	// 追加が正しく実施されたか確認
	bool res = false;
	if (!m_qConsumerRequests.empty())
	{
		EmberContent* addCont = m_qConsumerRequests.back();
		res = addCont && (addCont->requestId.id == pRequest->requestId.id);
	}
	// 要求前キューから先頭を除去
	if (res)
		m_qPreConsumerRequests.pop_front();
	else if (pRequest)
		pRequest = nullptr;

	return pRequest;
}
/// <summary>コンシューマ受信通知</summary>
/// <param name="pResult"></param>
void CEmberConsumer::NotifyReceivedConsumerResult(EmberContent* pResult)
{
	if (!pResult)
		return;

	// 無条件でキューに積む
	auto lock = _Lock(m_mtxConsumerRequest);
	// 識別を付番した上でキューに追加
	try
	{
		m_qConsumerResults.push_back(pResult);
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
}
/// <summary>コンシューマ操作結果取得</summary>
/// <returns></returns>
EmberContent* CEmberConsumer::GetConsumerResult()
{
	EmberContent* pResult = nullptr;
	auto lock = _Lock(m_mtxConsumerRequest);

	if (IsCancelRequest() || m_qConsumerResults.empty())
		return pResult;

	// 結果キューの先頭を参照
	try
	{
		pResult = m_qConsumerResults.front();
		if (pResult)
			m_qConsumerResults.pop_front();
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
	if (!pResult)
		return pResult;

	// 要求キューに結果の対象より前の要求が残っていたら取り除く
	while ((pResult->requestId.id > 0) && !m_qConsumerRequests.empty())
	{
		try
		{
			EmberContent* pRequest = m_qConsumerRequests.front();
#if true
			if (!pRequest
			  || (pRequest->requestId.id >= pResult->requestId.id))
				break;
#else
			// 同一idで複数結果となる場合があるので
			// idが一致する要求は残しておく
			if (!pRequest
			 || (pRequest->requestId.id == pResult->requestId.id))
				break;
#endif
			m_qConsumerRequests.pop_front();

			// pRequest は自身で生成したもの、破棄
			freeMemory(pRequest);
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			break;
		}
	}

	return pResult;
}
/// <summary>Client処理用のコンシューマ操作結果格納</summary>
/// <param name="pResult"></param>
void CEmberConsumer::AddSendMessage(EmberContent* pResult)
{
	if (!pResult)
		return;

	// 無条件でキューに積む
	auto lock = _Lock(m_mtxSendMessage);
	// 識別を付番した上でキューに追加
	try
	{
		m_qSendMessage.push_back(pResult);
		//パラメータ更新フラグを通知
		m_bUpdateDetected = true;
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
}
/// <summary>Client処理用のコンシューマ操作結果取得</summary>
/// <returns></returns>
EmberContent* CEmberConsumer::GetSendMessage()
{
	EmberContent* pResult = nullptr;
	auto lock = _Lock(m_mtxSendMessage);

	if (IsCancelRequest() || m_qSendMessage.empty())
		return pResult;

	// 結果キューの先頭を参照
	try
	{
		pResult = m_qSendMessage.front();
		if (pResult)
			m_qSendMessage.pop_front();
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}
	if (!pResult)
		return pResult;

	// 要求キューに結果の対象より前の要求が残っていたら取り除く
	while ((pResult->requestId.id > 0) && !m_qConsumerRequests.empty())
	{
		try
		{
			EmberContent* pRequest = m_qConsumerRequests.front();
#if true
			if (!pRequest
				|| (pRequest->requestId.id >= pResult->requestId.id))
				break;
#else
			// 同一idで複数結果となる場合があるので
			// idが一致する要求は残しておく
			if (!pRequest
				|| (pRequest->requestId.id == pResult->requestId.id))
				break;
#endif
			m_qConsumerRequests.pop_front();

			// pRequest は自身で生成したもの、破棄
			freeMemory(pRequest);
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			break;
		}
	}
	return pResult;
}
/// <summary>要求済コンシューマ操作取得</summary>
/// <param name="id"></param>
/// <returns></returns>
EmberContent* CEmberConsumer::GetConsumerRequestedContent(int id)
{
	EmberContent* pRequest = nullptr;
	auto lock = _Lock(m_mtxConsumerRequest);

	if (IsCancelRequest() || m_qConsumerRequests.empty())
		return pRequest;

	// 要求キューから対象idを持つ内容を抽出
	try
	{
		auto itr = std::find_if(m_qConsumerRequests.cbegin(), m_qConsumerRequests.cend(), [id](EmberContent* p) { return p && (p->requestId.id == id); });
		if (itr != std::cend(m_qConsumerRequests))
			pRequest = *itr;
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		if (pRequest)
			pRequest = nullptr;
	}

	return pRequest;
}

/// <summary>文字列パス→パス</summary>
/// <param name="path"></param>
/// <param name="pPath"></param>
/// <returns></returns>
size_t CEmberConsumer::GetNodePath(std::string sPath, berint** pPath)
{
	size_t len = 0ull;
	*pPath = nullptr;
	if (!m_sRemoteContent.pTopNode)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "top node is empty.\n");
		return len;
	}

	try
	{
		len = convertString2Path(m_sRemoteContent.pTopNode, (pstr)sPath.c_str(), pPath);
		if (pPath && (len > 0))
		{
			berint* p = *pPath;

			std::string tmp{};
			for (int i = 0; i < len; ++i)
			{
				if (i > 0)
					tmp.append(".");

				int num = (int)p[i];
				tmp.append(std::to_string(num));
			}
			Trace(__FILE__, __LINE__, __FUNCTION__, "path %s is %s\n", sPath.c_str(), tmp.c_str());
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		if (*pPath != 0)
		{
			freeMemory(*pPath);
			*pPath = NULL;
		}
		len = 0;
	}

	return len;
}
/// <summary>
/// コンシューマ要求用データ生成
/// </summary>
/// <param name="type"></param>
/// <param name="sValue"></param>
/// <returns></returns>
GlowValue* CEmberConsumer::CreateGlowValue(GlowParameterType type, std::string sValue)
{
	GlowValue* pValue = nullptr;
	bool validType = false;
	try
	{
		pValue = newobj(GlowValue);
		bzero_item(*pValue);
		pValue->flag = type;

		switch (pValue->flag)
		{
		case GlowParameterType_Integer:
		case GlowParameterType_Enum:
		{
			berlong _value = 0;
			if (!utilities::ToNumber(sValue, _value))
				_value = 0;
			pValue->choice.integer = _value;

			validType = true;
		}
		break;
		case GlowParameterType_Real:
		{
			double _value = 0;
			if (!utilities::ToNumber(sValue, _value))
				_value = 0;
			pValue->choice.real = _value;

			validType = true;
		}
		break;
		case GlowParameterType_Boolean:
		{
			bool _value = false;
			if (!utilities::ToBool(sValue, _value))
				_value = false;
			pValue->choice.boolean = _value;

			validType = true;
		}
		break;
		case GlowParameterType_String:
		{
			if (!sValue.empty())
			{
				pValue->choice.pString = stringDup(sValue.c_str());
			}

			validType = true;
		}
		break;
		case GlowParameterType_Octets:
		{
			if (!sValue.empty())
			{
				char* pstr = stringDup(sValue.c_str());
				pValue->choice.octets.pOctets = (byte*)pstr;
				pValue->choice.octets.length = (int)strlen(pstr);
			}

			validType = true;
		}
		break;

		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		if (validType)
			validType = false;
	}

	if (!validType && pValue)
	{
		glowValue_free(pValue);
		pValue = nullptr;
	}

	return pValue;
}

/// <summary>ディレクトリ取得要求生成</summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <returns></returns>
EmberContent* CEmberConsumer::CreateGetDirectoryRequest(RequestId* pId, std::string sPath)
{
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	// 何もない場合トップ扱い
	//if (len <= 0)
	//	return nullptr;
	EmberContent* pCont = CreateGetDirectoryRequest(0, pPath, len);
	freeMemory(pPath);
	return pCont;
}
/// <summary>パラメータ取得要求生成</summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <returns></returns>
EmberContent* CEmberConsumer::CreateGetParameterRequest(RequestId* pId, std::string sPath)
{
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	if (len <= 0)
		return nullptr;
	EmberContent* pCont = CreateGetParameterRequest(pId, pPath, len);
	freeMemory(pPath);
	return pCont;
}
/// <summary>パラメータ設定要求生成</summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="value"></param>
/// <returns></returns>
EmberContent* CEmberConsumer::CreateSetParameterRequest(RequestId* pId, std::string sPath, GlowParameter& value)
{
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	if (len <= 0)
		return nullptr;
	EmberContent* pCont = CreateSetParameterRequest(pId, pPath, len, value);
	freeMemory(pPath);
	return pCont;
}
/// <summary>マトリックス接続要求生成</summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="value"></param>
/// <returns></returns>
EmberContent* CEmberConsumer::CreateConnectionRequest(RequestId* pId, std::string sPath, GlowConnection& value)
{
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	if (len <= 0)
		return nullptr;
	EmberContent* pCont = CreateConnectionRequest(pId, pPath, len, value);
	freeMemory(pPath);
	return pCont;
}
/// <summary>ファンクション実行要求生成</summary>
/// <param name="pId"></param>
/// <param name="pPath"></param>
/// <param name="value"></param>
/// <returns></returns>
EmberContent* CEmberConsumer::CreateInvokeRequest(RequestId* pId, std::string sPath, GlowInvocation& value)
{
	berint* pPath = nullptr;
	int len = (int)GetNodePath(sPath, &pPath);
	if (len <= 0)
		return nullptr;
	EmberContent* pCont = CreateInvokeRequest(pId, pPath, len, value);
	freeMemory(pPath);
	return pCont;
}

/// <summary>マトリックス操作通知回数</summary>
/// <returns></returns>
/// <remarks>
/// マトリックス情報／接続切替／該当パラメータ受信が対象
/// </remarks>
unsigned short CEmberConsumer::MatrixNoticeCount()
{
	auto lock = _Lock(m_mtxMatrixNotice);

	return m_nMatrixNoticeCount;
}
/// <summary>マトリックス操作通知回数加算</summary>
/// <returns></returns>
/// <remarks>
/// マトリックス情報／接続切替／該当パラメータ受信が対象
/// 初期化時 0 以降は 1-INT16_MAX 範囲で移動
/// </remarks>
unsigned short CEmberConsumer::IncrementMatrixNoticeCount()
{
	auto lock = _Lock(m_mtxMatrixNotice);

	// signed 幅で使用する、超過時戻し先は0ではなく1
	++m_nMatrixNoticeCount;
	if (INT16_MAX < m_nMatrixNoticeCount)
		m_nMatrixNoticeCount = 1;
	return m_nMatrixNoticeCount;
}
/// <summary>マトリックスラベル情報初期化</summary>
/// <returns></returns>
bool CEmberConsumer::ClearMatrixLabels()
{
	bool res = false;
	while (!m_vMatrixLabels.empty())
	{
		auto itr = m_vMatrixLabels.begin();
		auto pMatrixLabels = *itr;
		m_vMatrixLabels.erase(itr);

		if (!pMatrixLabels)
			delete pMatrixLabels;

		if (!res)
			res = true;
	}
	return res;
}
/// <summary>マトリックスラベル情報リセット</summary>
/// <returns></returns>
bool CEmberConsumer::ResetMatrixLabels()
{
	auto lock = _Lock(m_mtxMatrixNotice);
	bool res = false;

	try
	{
		ClearMatrixLabels();

		res = true;
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	return res;
}
/// <summary>マトリックスラベル情報リセット</summary>
/// <param name="pContent"></param>
/// <returns></returns>
bool CEmberConsumer::ResetMatrixLabels(const EmberContent* pContent)
{
	bool res = false;
	if (//!UseMatrixLabels() ||		// GetDirectory 再要求が必要になったため、ラベル使用有無に拘わらず処理を通す
		!pContent
	 || !pContent->pPath
	 || (pContent->pathLength <= 0))
		return res;

	// マトリックス受信からラベル収集
	if (pContent->type == GlowType_Matrix)
	{
		auto lock = _Lock(m_mtxMatrixNotice);
		try
		{
			// 生成
			auto pMatrixLabels = new MatrixLabels(m_sRemoteContent.pTopNode, pContent);
			if (!pMatrixLabels->Valid())
				return res;

			// 既存なら取り除く
			bool bFirst = true;
			if (!m_vMatrixLabels.empty())
			{
				auto sPath = pMatrixLabels->MatrixNodePath();
				auto itr = std::find_if(m_vMatrixLabels.begin(), m_vMatrixLabels.end(),
										[sPath](MatrixLabels* _pCont)
										{ return _pCont && _pCont->Enabled() && (_pCont->MatrixNodePath() == sPath); });
				if (itr != m_vMatrixLabels.end())
				{
					auto _pMatrixLabels = *itr;
					m_vMatrixLabels.erase(itr);

					if (_pMatrixLabels)
						delete _pMatrixLabels;

					bFirst = false;
				}
			}
			// 初めてデータを追加する場合、親ノードでの GetDirectory により受信した GlowMatrixで、プロバイダが接続情報を送信しない場合がある
			// 自ノードから GetDirectory を取得し直す
			if (bFirst)
			{
				AddConsumerRequest(CreateGetDirectoryRequest(nullptr, pContent->pPath, pContent->pathLength));
			}

			// 追加
			m_vMatrixLabels.push_back(pMatrixLabels);
			res = true;
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
	}
	// ノード／パラメータ受信からラベル更新
	else if (((pContent->type == GlowType_Node)
		   || (pContent->type == GlowType_Parameter))
		  && !m_vMatrixLabels.empty())
	{
		auto lock = _Lock(m_mtxMatrixNotice);
		try
		{
			// 一致するラベルがある場合、文字列を更新する
			for (auto pMatrixLabels : m_vMatrixLabels)
			{
				if (!pMatrixLabels->Valid())
					continue;

				if (pMatrixLabels->SetLabel(pContent))
				{
					res = true;
					// 別々のマトリックスで同じパラメータを参照している可能性があるので break しない
					//break;
				}
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}
	}

	return res;
}
/// <summary>ラベル取得</summary>
/// <param name="kind">1 : target, 2 : source</param>
/// <param name="sEmberPath"></param>
/// <param name="nSignalNumber"></param>
/// <param name="sLabel"></param>
/// <returns></returns>
bool CEmberConsumer::GetLabel(int kind, std::string sEmberPath, berint nSignalNumber, std::string& sLabel)
{
	sLabel = "";
	bool res = false;
	// 対象のマトリックスがあるか
	if (UseMatrixLabels() && !m_vMatrixLabels.empty())
	{
		auto itr = std::find_if(m_vMatrixLabels.cbegin(), m_vMatrixLabels.cend(),
								[sEmberPath](MatrixLabels* pMatrixLabels)
								{ return pMatrixLabels && (pMatrixLabels->MatrixNodePath() == sEmberPath); });
		if ((itr != m_vMatrixLabels.cend())
		 && (*itr)->Enabled())
		{
			std::string _sLabel = "";
			auto _res = (kind == 1)
					  ? (*itr)->GetTargetLabel(nSignalNumber, _sLabel)
					  : (*itr)->GetSourceLabel(nSignalNumber, _sLabel);
			if (_res && !_sLabel.empty())
			{
				sLabel = _sLabel;
				res = true;
			}
		}
	}

	return res;
}

/// <summary>
/// </summary>
void CEmberConsumer::Worker(CEmberConsumer* instance)
{
	if ((instance == nullptr) || !instance->Enabled())
		return;

	while (!instance->m_bCancelRequest)
	{
		try
		{
			runConsumer(&instance->m_sRemoteContent);
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			break;
		}
	}
}

/// <summary>
/// ノードパス値文字列化（デバッグ用途）
/// </summary>
/// <param name="pPath"></param>
/// <param name="pathLength"></param>
/// <returns></returns>
static std::string GetGlowNodePathNumString(const berint* pPath, int pathLength)
{
	std::string npath = "";
	if (pPath && (pathLength > 0))
	{
		for (int i = 0; i < pathLength; ++i)
		{
			if (npath.size() > 0)
				npath += ".";
			npath += std::to_string(pPath[i]);
		}
	}

	return std::string(npath);
}
/// <summary>
/// GlowValue 内容文字列化（デバッグ用途）
/// </summary>
/// <param name="pValue"></param>
/// <returns></returns>
static std::string GetGlowValueString(GlowValue* pValue)
{
	if (!pValue)
		return std::string("");

	std::string tmp = "GlowType = ";
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
	switch (pValue->flag)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	{
	case GlowParameterType_None: tmp += "None."; break;
	case GlowParameterType_Integer: tmp += Format("Integer, value = %lld", pValue->choice.integer); break;
	case GlowParameterType_Real: tmp += Format("Real, value = %lf", pValue->choice.real); break;
	case GlowParameterType_String:
	{
		tmp += "String, value = ";
		if (pValue->choice.pString)
			tmp += pValue->choice.pString;
		else
			tmp += "(empty)";
	}
	break;
	case GlowParameterType_Boolean:
	{
		tmp += "Boolean, value = ";
		if (pValue->choice.boolean)
			tmp += "true";
		else
			tmp += "false";
	}
	break;
	case GlowParameterType_Trigger: tmp += "Trigger."; break;
	case GlowParameterType_Enum: tmp += Format("Enum, value = %lld", pValue->choice.integer); break;
	case GlowParameterType_Octets:
	{
		tmp += "Octets, ";
		if ((pValue->choice.octets.length > 0) && pValue->choice.octets.pOctets)
		{
			tmp += Format("length = %d, value = ", pValue->choice.octets.length);
			pstr octets = nullptr;
			try
			{
				octets = newarr(char, (size_t)(pValue->choice.octets.length + 1));
				bzero_item(*octets);
				memcpy(octets, pValue->choice.octets.pOctets, pValue->choice.octets.length);
				tmp += octets;
			}
			catch (const std::exception ex)
			{
				ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
				tmp += "(unknown)";
			}
			if (octets)
			{
				freeMemory(octets);
				octets = nullptr;
			}
		}
		else
			tmp += "value = (empty)";
	}
	break;
	default: tmp += "(unknown)"; break;
	}

	return std::string(tmp);
}
static std::string GetEmberContentString(EmberContent* pContent)
{
	if (!pContent)
		return std::string();

	std::string tname = "(empty)";
	std::string tmp = "";
	try
	{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
		switch (pContent->type)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
		{
		case GlowType_Command:
		{
			tname = "Command";
			std::string cname = "(empty)";
			switch ((GlowCommandType)pContent->command.number)
			{
			case GlowCommandType_Subscribe: cname = "Subscribe"; break;
			case GlowCommandType_Unsubscribe: cname = "Unsubscribe"; break;
			case GlowCommandType_GetDirectory: cname = "GetDirectory"; break;
			case GlowCommandType_Invoke:
			{
				cname = Format("Invoke, invocationid = %d, argslength = %d",
					pContent->command.options.invocation.invocationId,
					pContent->command.options.invocation.argumentsLength);
				if ((pContent->command.options.invocation.argumentsLength > 0)
					&& pContent->command.options.invocation.pArguments)
				{
					for (int i = 0; i < pContent->command.options.invocation.argumentsLength; ++i)
					{
						std::string argv = GetGlowValueString(&pContent->command.options.invocation.pArguments[i]);
						cname += Format(", arg%d[%s]", i, argv.c_str());
					}
				}
			}
			break;
			default:
				if ((int)pContent->command.number)
					cname = Format("unknown(%d)", pContent->command.number);
				break;
			}
			tmp = Format(", GlowCommandType = %s", cname.c_str());
		}
		break;
		case GlowType_Parameter:
		{
			tname = "Parameter";
			std::string pid = "(empty)";
			if (pContent->parameter.pIdentifier)
				pid = std::string(pContent->parameter.pIdentifier);
			std::string pval = "(empty)";
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
			if (pContent->fields & GlowFieldFlag_Value)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			{
				pval = GetGlowValueString(&pContent->parameter.value);
			}
			tmp = Format(", Identifier = %s, value[%s]", pid.c_str(), pval.c_str());
		}
		break;
		case GlowType_Node:
		{
			tname = "Node";
			std::string pid = "(empty)";
			if (pContent->node.pIdentifier)
				pid = std::string(pContent->node.pIdentifier);
			tmp = Format(", Identifier = %s", pid.c_str());
		}
		break;
		case GlowType_Matrix:
		{
			tname = "Matrix";
			std::string pid = "(empty)";
			if (pContent->matrix.pIdentifier)
				pid = std::string(pContent->matrix.pIdentifier);

			std::string mtype = "(Unknown)";
			switch (pContent->matrix.type)
			{
			case GlowMatrixType_OneToN: mtype = "OneToN"; break;
			case GlowMatrixType_OneToOne: mtype = "OneToOne"; break;
			case GlowMatrixType_NToN: mtype = "NToN"; break;
			}

			std::string amode = "(Unknown)";
			switch (pContent->matrix.addressingMode)
			{
			case GlowMatrixAddressingMode_Linear: amode = "Linear"; break;
			case GlowMatrixAddressingMode_NonLinear: amode = "NonLinear"; break;
			}

			berint tcnt = pContent->matrix.targetCount;
			berint scnt = pContent->matrix.sourceCount;
			berint connmaxTotal = pContent->matrix.maximumTotalConnects;
			berint connmaxTarget = pContent->matrix.maximumConnectsPerTarget;

			std::string location = "(unknown)";
			std::string ltype = "(unknown)";
			std::string lvalue = "(empty)";
			switch (pContent->matrix.parametersLocation.kind)
			{
			case GlowParametersLocationKind_BasePath:
			{
				ltype = "(BasePath)";
				if ((pContent->matrix.parametersLocation.choice.basePath.ids != nullptr)
				 && (pContent->matrix.parametersLocation.choice.basePath.length > 0))
				{
					lvalue = GetGlowNodePathNumString(pContent->matrix.parametersLocation.choice.basePath.ids,
													  pContent->matrix.parametersLocation.choice.basePath.length);
				}
				location = ltype + lvalue;
			}
			break;
			case GlowParametersLocationKind_Inline:
			{
				ltype = "(Inline)";
				lvalue = Format("%d", pContent->matrix.parametersLocation.choice.inlineId);
				location = ltype + lvalue;
			}
			break;
			}

			int lblen = pContent->matrix.labelsLength;
			std::string labelsValue = "(empty)";
			if ((pContent->matrix.pLabels != nullptr)
			 && (lblen > 0))
			{
				for (int i = 0; i < lblen; ++i)
				{
					std::string _label = "(empty)";
					if (pContent->matrix.pLabels[i].basePathLength > 0)
					{
						_label = GetGlowNodePathNumString(pContent->matrix.pLabels[i].basePath,
														  pContent->matrix.pLabels[i].basePathLength) + " ";
						if (pContent->matrix.pLabels[i].pDescription != nullptr)
							_label.append(pContent->matrix.pLabels[i].pDescription);
					}
					if (i == 0)
						labelsValue = "";
					labelsValue += Format("\n    Label%d = %s", i, _label.c_str());
				}
			}

			tmp = Format(", Identifier = %s, type = %s, addressingMode = %s, targetCount = %d, sourceCount = %d,\n    maximumTotalConnects = %d, maximumConnectsPerTarget = %d, parameterLocation = %s, labelsCount = %d, %s",
							pid.c_str(), mtype.c_str(), amode.c_str(), tcnt, scnt, /*LF*/connmaxTotal, connmaxTarget, location.c_str(), lblen, /*LF*/labelsValue.c_str());
		}
		break;
		case GlowType_Target:
		{
			tname = "Target";
			tmp = Format(", signal = %d", pContent->signal.number);
		}
		break;
		case GlowType_Source:
		{
			tname = "Source";
			tmp = Format(", signal = %d", pContent->signal.number);
		}
		break;
		case GlowType_Connection:
		{
			tname = "Connection";
			berint conntarget = pContent->connection.target;
			int connsourcecnt = pContent->connection.sourcesLength;
			std::string connsources = "(empty)";
			if ((pContent->connection.pSources != nullptr)
			 && (connsourcecnt > 0))
			{
				connsources = "";
				for (int i = 0; i < connsourcecnt; ++i)
				{
					if (!connsources.empty())
						connsources += ",";
					connsources += std::to_string(pContent->connection.pSources[i]);
				}
			}

			std::string connope = "(unknown)";
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
			switch (pContent->connection.operation)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			{
			case GlowConnectionOperation_Absolute:		connope = "Absolute";	break;
			case GlowConnectionOperation_Connect:		connope = "Connect";	break;
			case GlowConnectionOperation_Disconnect:	connope = "Disconnect";	break;
			}

			std::string conndsp = "(unknown)";
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
			switch (pContent->connection.disposition)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			{
			case GlowConnectionDisposition_Tally:conndsp = "Tally"; break;
			case GlowConnectionDisposition_Modified:conndsp = "Modified"; break;
			case GlowConnectionDisposition_Pending:conndsp = "Pending"; break;
			case GlowConnectionDisposition_Locked:conndsp = "Locked"; break;
			}

			tmp = Format(", target = %d, soucesCount = %d, sources = %s, operation = %s, disposition = %s", conntarget, connsourcecnt, connsources.c_str(), connope.c_str(), conndsp.c_str());
		}
		break;
		case GlowType_ElementCollection: tname = "ElementCollection"; break;
		case GlowType_StreamEntry: tname = "StreamEntry"; break;
		case GlowType_StreamCollection: tname = "StreamCollection"; break;
		case GlowType_EnumEntry: tname = "EnumEntry"; break;
		case GlowType_EnumCollection: tname = "EnumCollection"; break;
		case GlowType_QualifiedParameter: tname = "QualifiedParameter"; break;
		case GlowType_QualifiedNode: tname = "QualifiedNode"; break;
		case GlowType_RootElementCollection: tname = "RootElementCollection"; break;
		case GlowType_StreamDescription: tname = "StreamDescription"; break;
		case GlowType_QualifiedMatrix: tname = "QualifiedMatrix"; break;
		case GlowType_Label: tname = "Label"; break;
		case GlowType_Function: tname = "Function"; break;
		case GlowType_QualifiedFunction: tname = "QualifiedFunction"; break;
		case GlowType_TupleItemDescription: tname = "TupleItemDescription"; break;
		case GlowType_Invocation: tname = "Invocation"; break;
		case GlowType_InvocationResult: tname = "InvocationResult"; break;
		case GlowType_Template: tname = "Template"; break;
		case GlowType_QualifiedTemplate: tname = "QualifiedTemplate"; break;
		default:
			if ((int)pContent->type != 0)
				tname = "unknown(" + std::to_string((int)pContent->type) + ")";
			break;
		}
	}
	catch (const std::exception ex)
	{
		ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
	}

	std::string npath = GetGlowNodePathNumString(pContent->pPath, pContent->pathLength);
	return std::string(Format("id = %d, path = %s, GlowType = %s%s", pContent->requestId.id, npath.c_str(), tname.c_str(), tmp.c_str()));
}

/// <summary>
/// </summary>
void CEmberConsumer::Watcher(CEmberConsumer* instance)
{
	if ((instance == nullptr) || !instance->Enabled())
		return;

	// Worker より先に動いた際
	// メモリ用マクロを使用するには明示的な初期化が必須
	initEmberContents();

	auto emptyDelay = std::chrono::milliseconds(instance->m_pClientConfig->EmberThreadDelay());
	bool requestedEmberRoot = false;
	bool firstReceived = false;
	while (!instance->m_bCancelRequest)
	{
		EmberContent* pResult = nullptr;
		try
		{
			// 必須情報取得
			if (!requestedEmberRoot)
			{
				instance->AddConsumerRequest(instance->CreateGetDirectoryRequest(nullptr, nullptr, 0));
				requestedEmberRoot = true;
				firstReceived = false;
				std::this_thread::sleep_for(emptyDelay);
				continue;
			}
			// 1回以上受信があった後にツリーインスタンスの確認ができなくなった場合
			// キャンセル要求以外であれば切断状態になったと判断し、データ取得をやり直す
			if (!(instance->m_bHasEmberRoot = hasEmberTree()))
			{
				if (firstReceived)
				{
					ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "lost ember tree. reget request.");
					requestedEmberRoot = false;
					firstReceived = false;
					std::this_thread::sleep_for(emptyDelay);
					continue;
				}
			}

			// 結果取り出し
			pResult = instance->GetConsumerResult();
			// Clientでの処理用にキューに格納
			//instance->AddClientProcess(pResult);
			if (!pResult)
			{
				std::this_thread::sleep_for(emptyDelay);
				continue;
			}
			firstReceived = true;
			std::string cvalue = GetEmberContentString(pResult);
			Trace(__FILE__, __LINE__, __FUNCTION__, "consumer result %s\n", cvalue.c_str());
			// 結果に対する要求参照
			EmberContent* pRequest = instance->GetConsumerRequestedContent(pResult->requestId.id);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:26812)
#endif
			switch (pResult->type)	// 受信通知時に設定するようにした、pRequest 内 type とは異なる可能性あり（pRequest=Commandの場合等）
#ifdef _MSC_VER
#pragma warning(pop)
#endif
			{
			case GlowType_Node:
			case GlowType_QualifiedNode:
				//if (pRequest && (pRequest->type == GlowType_Command) && (pRequest->command.number == GlowCommandType_GetDirectory))
				{
					if (pResult && (pResult->pathLength > 0))
					{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, " node path : %s, duplicateRequests = %d\n", pathName, pResult->duplicateRequests);
							freeMemory(pathName);
						}

						// ラベル更新（対象かの確認含む）
						if (instance->ResetMatrixLabels(pResult))
						{
							// ラベル更新により通知回数更新
							instance->IncrementMatrixNoticeCount();
						}
					}
					// 任意ノードに対する GetDirectoryRequest で
					// 子ノードを持たない場合に要求と同じリーフノードが返却される場合の制限
					if (!pResult->duplicateRequests)
						// 子供がぶらさがっている可能性あり
						// このノードで GetDirectory 要求
						instance->AddConsumerRequest(instance->CreateGetDirectoryRequest(nullptr, pResult->pPath, pResult->pathLength));
					if (pResult)
						freeMemory(pResult);
				}
				break;
			case GlowType_Parameter:
			case GlowType_QualifiedParameter:
				{
					if (pResult && (pResult->pathLength > 0))
					{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, " parameter path : %s\n", pathName);

							freeMemory(pathName);
						}
						instance->AddSendMessage(pResult);
#if false
						// ラベル更新（対象かの確認含む）
						if (instance->ResetMatrixLabels(pResult))
						{
							// ラベル更新により通知回数更新
							instance->IncrementMatrixNoticeCount();
						}
#endif
						//instance->IncrementMatrixNoticeCount();
					}
				}
				break;
			case GlowType_Connection:
				{
					if (pResult && (pResult->pathLength > 0))
					{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							// 最後に接続情報を出してきたパスを控える
							instance->m_sLastNotifyMatrixPath = std::string(pathName);

							Trace(__FILE__, __LINE__, __FUNCTION__, " connection path : %s\n", pathName);
							freeMemory(pathName);
						}
					}
					// 接続変更により通知回数更新
					if (pResult)
						instance->IncrementMatrixNoticeCount();
					if (pResult)
						freeMemory(pResult);
			}
				break;
			case GlowType_InvocationResult:
				{
				if (pResult)
					freeMemory(pResult);
			}
				break;
			case GlowType_Function:
				{
					if (pResult && (pResult->pathLength > 0))
					{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, " function path : %s\n", pathName);
							freeMemory(pathName);
						}
					}
					if (pResult)
						freeMemory(pResult);
			}
				break;
			case GlowType_Matrix:
				{
					if (pResult && (pResult->pathLength > 0))
					{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, " matrix path : %s\n", pathName);
							freeMemory(pathName);
						}

						// ラベル収集
						if (instance->ResetMatrixLabels(pResult))
						{
							// ラベル収集により通知回数更新
							instance->IncrementMatrixNoticeCount();
						}
					}
					if (pResult)
						freeMemory(pResult);
			}
				break;
			case GlowType_Target:
				{
					if (pResult && (pResult->pathLength > 0))
					{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, " target matrix path : %s\n", pathName);
							freeMemory(pathName);
						}
					}
					if (pResult)
						freeMemory(pResult);
			}
				break;
			case GlowType_Source:
				{
						pstr pathName = convertPath2String(instance->m_sRemoteContent.pTopNode, pResult->pPath, pResult->pathLength);
						if (pathName)
						{
							Trace(__FILE__, __LINE__, __FUNCTION__, " source matrix path : %s\n", pathName);
							freeMemory(pathName);
						}
						if (pResult)
							freeMemory(pResult);
			}
				break;
			/****
			case GlowType_Command: break;
			case GlowType_ElementCollection: break;
			case GlowType_StreamEntry: break;
			case GlowType_StreamCollection: break;
			case GlowType_EnumEntry: break;
			case GlowType_EnumCollection: break;
			case GlowType_RootElementCollection: break;
			case GlowType_StreamDescription: break;
			case GlowType_QualifiedMatrix: break;
			case GlowType_Label: break;
			case GlowType_QualifiedFunction: break;
			case GlowType_TupleItemDescription: break;
			case GlowType_Invocation: break;
			case GlowType_Template: break;
			case GlowType_QualifiedTemplate: break;
			****/
			}
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
			break;
		}

		try
		{
			//if (pResult)
			//	freeMemory(pResult);
		}
		catch (const std::exception ex)
		{
			ErrorHandler(__FILE__, __LINE__, __FUNCTION__, "exception : %s\n", ex.what());
		}

		std::this_thread::sleep_for(emptyDelay);
	}
	printf("Watcher Stop...\n");//todo
}


// ====================================================================
// for consumer request
// ====================================================================

/// <summary>コンシューマ離脱要求取得</summary>
/// <param name="">socketId</param>
/// <return></return>
extern "C" bool __GetQuitConsumerRequest(short socketId)
{
	bool ena = false;
	ClientSocketId _socketId = (ClientSocketId)socketId;

	if ((_socketId == ClientSocketId::SOCK_NMOS_EMBER) && m_pNmosEmberConsumer)
		ena = m_pNmosEmberConsumer->IsCancelRequest();
	else if ((_socketId == ClientSocketId::SOCK_MV_EMBER) && m_pMVEmberConsumer)
		ena = m_pMVEmberConsumer->IsCancelRequest();

	return false;
}
/// <summary>コンシューマ操作要求取得</summary>
/// <param name="">socketId</param>
/// <return></return>
extern "C" EmberContent* __GetConsumerRequest(short socketId)
{
	EmberContent* pRequest = nullptr;
	ClientSocketId _socketId = (ClientSocketId)socketId;

	if ((_socketId == ClientSocketId::SOCK_NMOS_EMBER) && m_pNmosEmberConsumer)
		pRequest = m_pNmosEmberConsumer->GetConsumerRequest();
	else if ((_socketId == ClientSocketId::SOCK_MV_EMBER) && m_pMVEmberConsumer)
		pRequest = m_pMVEmberConsumer->GetConsumerRequest();

	return pRequest;
}
/// <summary>コンシューマ受信通知</summary>
/// <param name="">socketId</param>
/// <param name="pResult"></param>
extern "C" void __NotifyReceivedConsumerResult(short socketId, EmberContent* pResult)
{
	ClientSocketId _socketId = (ClientSocketId)socketId;

	if ((_socketId == ClientSocketId::SOCK_NMOS_EMBER) && m_pNmosEmberConsumer)
		m_pNmosEmberConsumer->NotifyReceivedConsumerResult(pResult);
	else if ((_socketId == ClientSocketId::SOCK_MV_EMBER) && m_pMVEmberConsumer)
		m_pMVEmberConsumer->NotifyReceivedConsumerResult(pResult);
}


// ====================================================================
// static instance
// ====================================================================

/// <summary>
/// NMOS Ember コンシューマインスタンス（実体）
/// </summary>
CNmosEmberConsumer* m_pNmosEmberConsumer = nullptr;
/// <summary>
/// MV Ember コンシューマインスタンス（実体）
/// </summary>
CMvEmberConsumer* m_pMVEmberConsumer = nullptr;
