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
#include <fstream>
#include <vector>
#include <cstdlib>

#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <jansson.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 

#include <jansson.h>
#include <libconfig.h++>

#include "sessionMgr.h"
#include "learner.h"
#include "logger.h"

#include "base_config.h"




using namespace std;
using namespace libconfig;



EvtLogger	*gLogger = NULL;

#define RX_BUFFER_SIZE		(1024 * 1024)
static char *gBuffer = NULL;


//
// Change the process's session and direct all output to
// a log file.
//
bool Daemonize(void)
{
	bool	result = true;
	pid_t	pid, sid;

	// Fork child process
	pid = fork();
	if( pid < 0 ) {
		// fork error
		result = false;
	}

	if( result && pid > 0 ) {
		// Kill parent process
		exit(EXIT_SUCCESS);
	}

	// Change umask
	if( result ) {
		umask(0);
		sid = setsid();
		if( sid < 0 ) {
			result = false;
		}
	}

	// Change to root directory
	if( result ) {
		if( chdir("/") < 0 ) {
			result = false;
		}
	}

	// Close stdio
	if( result ) {
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
	}
	return result;
}





// Create the needed objects
//
SessionMgr* Initialize(string dataPath, string outPath, string heatmapPath, int sessionTimeout)
{
	bool	result = true;
	SessionMgr	*mgr = NULL;

	mgr = new SessionMgr(dataPath, outPath, heatmapPath, sessionTimeout);
	if( mgr == NULL ) {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to create session manager object");
	}
	return mgr;
}





bool HandleRequest(const int fd, SessionMgr *mgr)
{
	bool	result = true;
	int		bytesRx;

	//
	// TODO - Validate the JSON message to determine if it was received
	// 		  in its entirety.
	//

	bytesRx = recv(fd, gBuffer, RX_BUFFER_SIZE, 0);

	//
	//	TODO - Add check for buffer overflow
	//

	// Make sure we read all the data by looking for the end of the JSON
	// object
	//
	if( gBuffer[bytesRx - 1] != '}' ) {
		bytesRx += recv(fd, &gBuffer[bytesRx], RX_BUFFER_SIZE - bytesRx, 0);
	}


	if( bytesRx > 0 ) {
		gBuffer[bytesRx] = 0;
		result = mgr->HandleRequest(fd, gBuffer);
	} else {
		gLogger->LogMsg(EvtLogger::Evt_ERROR, "Invalid request");
	}
	return result;
}






bool ReadConfig(string& dataPath, string& outPath, short& port, string& interface,
				string& heatmapPath, int& sessionTimeout)
{
	bool	result = true;
	Config	config;
	int		readPort;

	// Load & parse config file
	try{
		config.readFile("/etc/al_server/al_server.conf");
	} catch( const FileIOException &ioex ) {
		result = false;;
	}
	
	// Data path
	try {
		 dataPath = (const char*)config.lookup("data_path");
	} catch( const SettingNotFoundException &nfEx ) {
		result = false;
	}

	// Out path
	try {
		 outPath = (const char*)config.lookup("out_path");
	} catch( const SettingNotFoundException &nfEx ) {
		result = false;
	}

	// Heatmap path
	try {
		 heatmapPath = (const char*)config.lookup("heatmap_path");
	} catch( const SettingNotFoundException &nfEx ) {
		result = false;
	}

	// Server port
	try {
		config.lookupValue("port", readPort);
	} catch(const SettingNotFoundException &nfEx) {
		result = false;
	}
	port = (short)readPort;
	
	// interface address
	try { 
		interface = (const char*)config.lookup("interface"); 
	} catch( const SettingNotFoundException &nfEx ) {
		interface = "127.0.0.1";
	}
	
	try {
		config.lookupValue("session_timeout", sessionTimeout);
		sessionTimeout *= 60;	// In minutes in the conf file, need seconds internal
	} catch( const SettingNotFoundException &nfEx ) {
		// Default to 30 minutes if not in config file.
		sessionTimeout = 30 * 60 ;
	}

	return result;
}








int main(int argc, char *argv[])
{
	int status = 0;
	SessionMgr *sessionMgr = NULL;
	short 	port;
	string 	dataPath, outPath, interface, heatmapPath;
	int		sessionTimeout;

	gLogger = new EvtLogger();
	if( gLogger == NULL ) {
		status = -1;
	} else {
		gLogger->Open("/var/log/al_server.log");
	}

	if( status == 0 ) {
		gLogger->LogMsg(EvtLogger::Evt_INFO, "al_server started, Ver %02d.%02d", AL_SERVER_VERSION_MAJOR, AL_SERVER_VERSION_MINOR);

		
		if( !ReadConfig(dataPath, outPath, port, interface, heatmapPath, sessionTimeout) ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to read configuration file");
			status = -1;
		} else {
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Listening on interface: %s, port: %d", interface.c_str(), port);
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Dataset path: %s", dataPath.c_str());
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Output path: %s", outPath.c_str());
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Heatmap path: %s", heatmapPath.c_str());
			gLogger->LogMsg(EvtLogger::Evt_INFO, "Session timeout: %d", sessionTimeout);
		}
	}

	if( status == 0 ) {
		Daemonize();
	}

	if( status == 0 ) {
		gBuffer = (char*)malloc(RX_BUFFER_SIZE);
		if( gBuffer == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to allocate RX buffer");;
			status = -1;
		}
	}

	if( status == 0 ) {

		sessionMgr = Initialize(dataPath, outPath, heatmapPath, sessionTimeout);
		if( sessionMgr == NULL ) {
			gLogger->LogMsg(EvtLogger::Evt_ERROR, "Unable to initialize server");
			status = -1;
		}

		if( status == 0 ) {

			int listenFD = 0, connFD = 0;
			struct sockaddr_in serv_addr, test;

			// Setup socket
			listenFD = socket(AF_INET, SOCK_STREAM, 0);
			memset(&serv_addr, 0, sizeof(struct sockaddr_in));

			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = inet_addr(interface.c_str());
			serv_addr.sin_port = htons(port);

			// Need to specify global namespace when using C++11, as std::bind was
			// added and is not the networking bind.
			//
			::bind(listenFD, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in));
			listen(listenFD, 10);

			struct sockaddr_in	peer;
			int	len = sizeof(peer);

			// Event loop...
			// TODO - Add signal handler to set a flag to exit this loop on shutodwn
			while( 1 ) {

				// Get a connection
				connFD = accept(listenFD, (struct sockaddr*)&peer, (socklen_t*)&len);

				gLogger->LogMsg(EvtLogger::Evt_INFO, "Request from %s", inet_ntoa(peer.sin_addr));

				if( HandleRequest(connFD, sessionMgr) == false ) {
					gLogger->LogMsg(EvtLogger::Evt_ERROR, "Request failed");
				}
				sleep(1);
			}
		}

		if( gBuffer )
			free(gBuffer);
		if( sessionMgr )
			delete sessionMgr;
	}
	if( gLogger )
		delete gLogger;

	return status;
}

