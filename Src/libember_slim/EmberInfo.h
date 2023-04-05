#pragma once

#include <string>

class NMosEmberInfo {
public:
	std::string m_sNmosEmberIpAddr;
	unsigned short m_nNmosEmberPort;
	bool m_bNmosEmberEnabled;
	bool m_bNmosEmberUseMatrixLabels;
};
class MvEmberInfo {
public:
	std::string m_sMvEmberIpAddr;
	unsigned short m_nMvEmberPort;
	bool m_bMvEmberEnabled;
	bool m_bMvEmberUseMatrixLabels;
};
class MuteInfo {
public:
	bool m_bMuteAll;
};