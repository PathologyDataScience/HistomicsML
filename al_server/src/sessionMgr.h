//
//	Copyright (c) 2014-2016, Emory University
//	All rights reserved.
//
//	Redistribution and use in source and binary forms, with or without modification, are
//	permitted provided that the following conditions are met:
//
//	1. Redistributions of source code must retain the above copyright notice, this list of
//	conditions and the following disclaimer.
//
//	2. Redistributions in binary form must reproduce the above copyright notice, this list
// 	of conditions and the following disclaimer in the documentation and/or other materials
//	provided with the distribution.
//
//	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
//	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
//	SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
//	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
//	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//	CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
//	WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
//	DAMAGE.
//
//
#if !defined(SRC_SESSIONMGR_H_)
#define SRC_SESSIONMGR_H_

#include <vector>

#include "sessionClient.h"


using namespace std;



struct Session {
	string	UID;
	string  sessionType;
	SessionClient *client;
	time_t	touchTime;
	int	socket;
};



class SessionMgr {

public:

		SessionMgr(string dataPath = "./", string outPath = "./", string heatmapPath = "./",
					int sessionTimeout = 30);
		~SessionMgr(void);

	bool	HandleRequest(const int sock, string data);

protected:

	int			m_sessionTimeout;
	string		m_dataPath;
	string		m_outPath;
	string		m_heatmapPath;

	vector<Session*> m_sessions;

	void	ParseCommand(const int sock, string data);
	bool	CreateSession(string uid, string data, const char *cmd, int sock);
	bool	EndSession(string uid, string data, int sock);
	bool	CmdSession(string uid, string data, int sock);

	bool 	Status(string uid, string data, int sock);
};



#endif /* SRC_SESSIONMGR_H_ */
