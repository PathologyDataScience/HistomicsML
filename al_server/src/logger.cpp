//
//	Copyright (c) 2014-2015, Emory University
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
#include <iostream>
#include <ctime>
#include <cstdarg>
#include <cstdio>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "logger.h"


using namespace std;


#define LOGFILE_PATH_LENGTH		1024





EvtLogger::EvtLogger(void) :
m_fqfn(""),
m_curFileDay(-1),
m_curFileDayOfYear(-1),
m_isOpen(false)
{
}






EvtLogger::~EvtLogger(void)
{
	if( m_isOpen )
		m_logFile.close();

	pthread_mutex_destroy(&m_fileMtx);
}




bool EvtLogger::Open(string logFile)
{
	bool	result = true;


	// Init file access mutex
	if( pthread_mutex_init(&m_fileMtx, NULL) == 0 ) {

		// Check for existing file first
		struct stat buffer;
		time_t	now = time(0);
		tm	*ltime = localtime(&now);

		m_curFileDayOfYear = ltime->tm_yday;
		m_curFileDay = ltime->tm_wday;

		if( stat(logFile.c_str(), &buffer) == 0 ) {
			// Check last modification day, rename if not today
			//
			ltime = localtime(&buffer.st_mtime);
			int  modDayOfYear = ltime->tm_yday, modDay = ltime->tm_wday;

			if( modDayOfYear != m_curFileDayOfYear ) {
				char newName[LOGFILE_PATH_LENGTH];
				// Rename the file with the days since sunday appended
				// to the end.
				//
				snprintf(newName, LOGFILE_PATH_LENGTH, "%s.%d", logFile.c_str(), modDay);
				rename(logFile.c_str(), newName);
			}
		}
		m_logFile.open(logFile.c_str(), std::ofstream::out | std::ofstream::app);
		m_isOpen = m_logFile.is_open();
		m_fqfn = logFile;
	}
	return result;
}




bool EvtLogger::LogMsg(LogType type, const char* msg, ...)
{
	bool	result = true;

	if( m_isOpen ) {
		char	buffer[1024];
		va_list	args;

		va_start(args, msg);
		vsnprintf(buffer, 1024, msg, args);
		va_end(args);

		time_t	now;

		if( m_logFile.is_open() ) {

			pthread_mutex_lock(&m_fileMtx);
			now = time(0);

			tm	*ltm = localtime(&now);
			char	timeBuff[100];
			string	typeName;

			switch( type ) {
			case Evt_ERROR:
				typeName = "[ERROR]";
				break;
			case Evt_WARN:
				typeName = "[WARN ]";
				break;
			case Evt_INFO:
				typeName = "[INFO ]";
				break;
			default:
				typeName = "[?????]";
				break;
			}

			// Archive the file for each day
			if( ltm->tm_yday != m_curFileDayOfYear ) {
				Archive();
			}

			snprintf(timeBuff, 100, "%02d-%02d-%d %02d:%02d:%02d",
					ltm->tm_mon + 1, ltm->tm_mday, 1900 + ltm->tm_year,
					ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
			m_logFile << typeName << " " << timeBuff << " - " << buffer << endl;
			result = true;

			pthread_mutex_unlock(&m_fileMtx);
		}

	}
	return result;
}





double EvtLogger::WallTime(void)
{
    double          now_time;
    struct timeval  etstart;
    struct timezone tzp;

    if (gettimeofday(&etstart, &tzp) == -1)
        perror("Error: calling gettimeofday() not successful.\n");

    now_time = ((double)etstart.tv_sec) +              /* in seconds */
               ((double)etstart.tv_usec) / 1000000.0;  /* in microseconds */
    return now_time;
}







void EvtLogger::Archive(void)
{
	time_t		now = time(0);
	tm			*ltime = localtime(&now);

	m_logFile.close();
	char	newName[LOGFILE_PATH_LENGTH];

	snprintf(newName, LOGFILE_PATH_LENGTH, "%s.%d", m_fqfn.c_str(), m_curFileDay);
	rename(m_fqfn.c_str(), newName);

	m_curFileDay = ltime->tm_wday;
	m_curFileDayOfYear = ltime->tm_yday;

	m_logFile.open(m_fqfn.c_str(), std::ofstream::out | std::ofstream::trunc);
}

