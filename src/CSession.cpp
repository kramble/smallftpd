// Revision : $Id: CSession.cpp,v 1.1.1.1 2004/04/05 21:26:54 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
//   The CSession object is used to manage dialogs between the server and
// *one* connected client
//   CSession objects are created by the main CFtpd object
//

#include <windows.h>

#include "resource.h"
#include "const.h"
#include "CUser.h"
#include "CFtpd.h"
#include "CSession.h"


#include <stdio.h>

#include "main.h"


char *months[] = {
	{"Jan"},
	{"Feb"},
	{"Mar"},
	{"Apr"},
	{"May"},
	{"Jun"},
	{"Jul"},
	{"Aug"},
	{"Sep"},
	{"Oct"},
	{"Nov"},
	{"Dec"}
};



CSession::CSession(SOCKET s, CFtpd *ftp) {
	m_MainSock = s;
	m_Ftpd = ftp;
	m_DataSock = 0;
	m_PasvSock = 0;
	m_PasvSock2 = 0;

    m_Exit = false;
	m_Rest = 0;


	m_Mode = ACTIVE_MODE;

	m_PasvPort = -1;
	m_DataPort = -1;

	m_ShouldStopSession = false;
	m_ShouldStopTransfer = false;
	
	m_IsAboutToBeClosed = false;
	m_IsClosed = false;


	memset(m_vCurrentDir, 0, BUFFER_SIZE);
	memset(m_rCurrentDir, 0, BUFFER_SIZE);
	memset(m_RenameFrom, 0, BUFFER_SIZE);
	
	memset(m_LastCmd, 0, BUFFER_SIZE);
	memset(lastParam, 0, BUFFER_SIZE);

	lstrcpy(m_vCurrentDir, "");

	// lstrcpy(m_LogFile, DEBUG_FILE);
	// lstrcpy(m_TxLogFile, ACTIVITY_FILE);
	
	loggingUser = -1;
	loggedUser = -1;

    m_ActionThreadRunning = false;

	GetLocalTime(&m_PasvThreadStartTime);
	m_PasvThreadRunning = false;

	GetLocalTime(&m_LastCommandTime);
	m_TransferInProgress = false;


	// m_SessionIndex = SendMessage(GetDlgItem(m_Ftpd->getHwnd(), IDC_CONNECTIONS), LB_ADDSTRING, 0, (LPARAM) "Connecting...");
	m_SessionIndex = m_Ftpd->getNbConnections();
	log(LOG_DEBUG, "DEBUG - SessionIndex should be %d\r\n", m_SessionIndex);
	
    m_SessionIndex = SendMessage(GetDlgItem(m_Ftpd->getHwnd(), IDC_CONNECTIONS), LB_INSERTSTRING, m_SessionIndex, (LPARAM) "User connecting...");
    log(LOG_DEBUG, "DEBUG - SessionIndex is %d\r\n", m_SessionIndex);

	log(LOG_DEBUG, "DEBUG - Session object #%d created\r\n", m_SessionIndex);

}

CSession::~CSession() {

    // int t = m_SessionIndex;

    log(LOG_DEBUG, "DEBUG - Session object S%d destroyed\r\n", m_SessionIndex);
    
	// free((void*)m_vCurrentDir);
	// free((void*)m_rCurrentDir);
	// free((void*)m_rRootDir);
	

}


//
// This function will :
//  + close m_PasvSock2, m_DataSock (sockets used for transfers)
//  + close m_PasvThread if it's running, and close m_PasvSock
//  + close m_MainSock, which will close the ClientThread.
//                    
bool CSession::close() {

    if (m_IsAboutToBeClosed) {
        // Session has already been closed
        return false;
    }

    m_IsAboutToBeClosed = true;

    if (m_SessionIndex < 0) {
        return false;
    }

    log(LOG_DEBUG, "DEBUG - Closing session S%d - CSession::Close()\r\n", m_SessionIndex);

	m_ShouldStopSession = true;

	// Stop Passive thread if it's running
	if (m_PasvThreadRunning) {
		
        if (m_PasvSock != 0 && closesocket(m_PasvSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_PasvSock - error %d - CSession::Close()\r\n", WSAGetLastError()); 
		}
		m_PasvSock = 0;
		
		if  (WaitForSingleObject(m_PasvThread, SMALL_TIMEOUT) == WAIT_TIMEOUT) {
			// m_PasvThread could not be stopped. (why ???)
			// Kill it.
			TerminateThread(m_PasvThread, 0);
		}
	}

	// close Pasv2 sock
	if ( m_PasvSock2 != 0 && closesocket(m_PasvSock2) != 0) {
        log(LOG_DEBUG, "ERROR - could not close m_PasvSock2 - error %d - CSession::Close()\r\n", WSAGetLastError()); 
    }
    m_PasvSock2 = 0;
    
	// close dataSock
	if ( m_DataSock != 0 && closesocket(m_DataSock) != 0) {
        log(LOG_DEBUG, "ERROR - could not close m_DataSock - error %d - CSession::Close()\r\n", WSAGetLastError()); 
    }
    m_DataSock = 0;

	// Close mainSock : should let thread finish automatically
	if ( m_MainSock != 0 &&  closesocket(m_MainSock) != 0) {
    	log(LOG_DEBUG, "ERROR - could not close m_MainSock - error %d - CSession::Close()\r\n", WSAGetLastError()); 
    }
    m_MainSock = 0;

    
    // wait for transfers in progress - sometimes they take time to stop
    while (m_TransferInProgress) {
        log(LOG_DEBUG, "TEMP - waiting for transfer to be stopped - CSession::close()\r\n"); 
        Sleep(250);
    }

    log(LOG_DEBUG, "DEBUG - session was closed - CSession::close()\r\n"); 

    m_IsClosed = true;

	return true;
}


bool CSession::sendToClient(char* msg) {
	char a[BUFFER_SIZE];

	wsprintf(a, "INFO - Server_S%d> %s", m_SessionIndex, msg);
	log(LOG_DEBUG, a);
	send(m_MainSock, msg, strlen(msg), 0);
	return true;
}

bool CSession::dialog() {
    // Read data from client
    char* acTotalBuffer;
	char acReadBuffer[BUFFER_SIZE];

    DWORD nThreadID;

    int nReadBytes = -1;
	int nTotalBytes = 0;

	acTotalBuffer = (char*)malloc(BUFFER_SIZE);
	acTotalBuffer[0] = 0;

	// log(LOG_DEBUG, "DEBUG Starting dialog with client.\r\n");
	
	// Start Main Dialog Loop
    while (!m_ShouldStopSession) {
        if (m_MainSock == INVALID_SOCKET || m_MainSock == 0) break;
        nReadBytes = recv(m_MainSock, acReadBuffer, BUFFER_SIZE, 0);
        if (nReadBytes > 0) {
			acReadBuffer[nReadBytes-1]=0;
			nTotalBytes += nReadBytes;

			if (nTotalBytes>=BUFFER_SIZE) {
				send(m_MainSock, ERR_OVERFLOW, strlen(ERR_OVERFLOW),0);

				
				// TODO : WHY DOESN'T IT WORK ???
				//free((void*)acTotalBuffer);
				nTotalBytes = 0;
				acReadBuffer[0]=0;
				acTotalBuffer[0]=0;
                continue;
			}
			
			// acTotalBuffer = (char*)realloc((void*)acTotalBuffer, sizeof(acTotalBuffer)+sizeof(acReadBuffer));
			int utu = sizeof(acTotalBuffer) + sizeof(acReadBuffer);
			acTotalBuffer = (char*)realloc((void*)acTotalBuffer, nTotalBytes);
			acTotalBuffer = lstrcat(acTotalBuffer, acReadBuffer);
			//log(acTotalBuffer);

			//check if acReadBuffer contains a \r before parsing the command
			for (int i=0; i<nReadBytes;i++) {
				if (acReadBuffer[i]==13) {
					// server received a '\r'

                    acTotalBuffer[lstrlen(acTotalBuffer)-1] = 0;
					
					// get ABOR and QUIT commands
	                if (m_ActionThreadRunning) {
					    // An action thread is already running !
    				    // If command is ABOR, then kill this running action thread
	       				if (inStr(m_LastCmd, "ABOR") > -1 || inStr(m_LastCmd, "QUIT") > -1) {
                            parse();
                        }

                        // wait for old action thread to stop
                        if (WaitForSingleObject(m_ActionThread, 2*SMALL_TIMEOUT) == WAIT_TIMEOUT) {
                            TerminateThread(m_ActionThread, 0);
                        }

                    }

					// copy buffer to m_LastCmd
					lstrcpy(m_LastCmd, acTotalBuffer);
					
					// log(LOG_DEBUG, "DEBUG - m_LastCmd = %s - CSession::dialog()\r\n", m_LastCmd); 
					
                    // WaitForSingleObject(m_ActionThread, MEDIUM_TIMEOUT);
                    
                    // if (m_ActionThreadRunning) {
                        // sendToClient(TRANSFER_IN_PROGRESS);
                    // } else if (lstrlen(m_LastCmd) > 1){
                        m_ActionThread = CreateThread(0, 0, ActionThread, (void*)this, 0, &nThreadID);
                    // }

	                // re-initialize buffers
                    nTotalBytes = 0;
                    acReadBuffer[0]=0;
					acTotalBuffer[0]=0;
					break;
                }
			}
        }
		if (m_Exit) {
    		log(LOG_DEBUG, "DEBUG - m_Exit=true - CSession::dialog()\r\n"); 
            break;
        }
    }

    log(LOG_DEBUG, "DEBUG - connection with session S%d is closed - CSession::dialog()\r\n", m_SessionIndex);

	close();
	
	free((void*)acTotalBuffer);
    return true;
}


bool CSession::parse() {

	char *first;
	int rez;
	unsigned int i;
	bool ret;

    // Set the time of last command
	GetLocalTime(&m_LastCommandTime);

    log(LOG_DEBUG, "INFO - Client_S%d> %s \r\n", m_SessionIndex, m_LastCmd);

	ret = true;
	
	first = (char*) malloc(sizeof(m_LastCmd));
	lstrcpy(first, m_LastCmd);
	// first = strdup(m_LastCmd);

	// make the command UPPER CASE
	for (i = 0; (i < lstrlen(first)) && (first[i] != 13); i++) {
		if (first[i]<=122 && first[i]>=97) {
			first[i]-=32;
		}

		if (first[i] == ' ') break;
	}
	first[i] = 0;
	

	int j=0;
	i++;
	while (i<strlen(m_LastCmd) && m_LastCmd[i] != 13 && m_LastCmd[i] != 10) {
		lastParam[j] = m_LastCmd[i];
		j++;
		i++;
	}

	if (lastParam[j] != 0) lastParam[j+1]=0;

	char a[BUFFER_SIZE];
	if (loggedUser > -1 && loggedUser < m_Ftpd->getNbUsers()) {
 		wsprintf(a, "#%d - %s - %s (%s)", m_SessionIndex, m_Ftpd->getUser(loggedUser)->login, m_LastCmd, m_vCurrentDir);
	} else {
        wsprintf(a, "#%d - %s", m_SessionIndex, m_LastCmd);	
	}
	
	// if (strcmp(first, "PASS")==0) {
	//     wsprintf(a, "#%d - PASS xxxxx", m_SessionIndex);	
	// }

	int p = strlen(a);
	if (a[p-1] == '\n') a[p-1] = 0;
	if (a[p-1] == '\r') a[p-1] = 0;
	updateListMsg(a);

	if (loggedUser<0) {
		rez = 2;
		if (strcmp(first, "USER")==0) rez = user();
		else if (strcmp(first, "PASS")==0) rez = pass();
	}
	else {
		if (strcmp(first, "ABOR")==0 || strcmp(first, "ÿABOR")==0) rez = abor();
		else if (strcmp(first, "CDUP")==0) rez = cdup();
		else if (strcmp(first, "CWD")==0) rez = cwd();
		else if (strcmp(first, "DELE")==0) rez = dele();
		else if (strcmp(first, "LIST")==0) rez = list(false);
		else if (strcmp(first, "NLST")==0) rez = list(true);
		else if (strcmp(first, "MKD")==0 || strcmp(first, "XMKD")==0) rez = mkd();
		else if (strcmp(first, "PASV")==0) rez = pasv();
		else if (strcmp(first, "PORT")==0) rez = port();
		else if (strcmp(first, "PWD")==0) rez = pwd();
		else if (strcmp(first, "QUIT")==0) rez = quit();
		else if (strcmp(first, "REST")==0) rez = rest();
		else if (strcmp(first, "RETR")==0) rez = retr();
		else if (strcmp(first, "RMD")==0) rez = rmd();
		else if (strcmp(first, "RNFR")==0) rez = rnfr();
		else if (strcmp(first, "RNTO")==0) rez = rnto();
		else if (strcmp(first, "SIZE")==0) rez = size();
		else if (strcmp(first, "STOR")==0) rez = stor();
		else if (strcmp(first, "SYST")==0) rez = syst();
		else if (strcmp(first, "TYPE")==0) rez = type();
		else rez = 0;
	}

	switch (rez) {
	case -10:
		sendToClient(PASV_ERROR);
		ret = true;
		break;
	case -2:
		sendToClient(FILE_RIGHTS);
		ret = true;
		break;
	case -1:
		ret = false;
		break;
	case 0:
		// log(LOG_DEBUG, NOT_UNDERSTOOD);
		sendToClient(NOT_UNDERSTOOD);
		ret = true;
		break;
	case 1:
		ret = true;
		break;
	case 2:
		sendToClient(NOT_LOGGED_IN);
		ret = false;
		break;
	case 3:
		sendToClient(BAD_SEQUENCE);
		ret = true;
		break;
	case 4:
		sendToClient(TOO_MANY_USERS);
		ret = false;
		break;
	}
	
	memset(m_LastCmd, 0, BUFFER_SIZE);
	memset(lastParam, 0, BUFFER_SIZE);

	free(first);
	
	m_Exit = !ret;
	return ret;
}


int CSession::changeDirectory(char *newDir) {

	// unsigned int i;
	// char *tmp;		// Virtual File System   ('/qsdf/qsdf')
	// char *path;		// Real File System		 ('C:\tmp\')


	// tmp = (char*)malloc(strlen(m_vCurrentDir)+2+strlen(newDir));

	// tmp[0] = '\0';

	// Make an absolute path
	if (newDir[0] == '/') {
		lstrcpy(m_vCurrentDir, newDir);
	}
	else {
		// deal with relative path
		if (m_vCurrentDir[strlen(m_vCurrentDir)-1] != '/') {
		      lstrcat(m_vCurrentDir, "/");
		}
		lstrcat(m_vCurrentDir, newDir);
	}

	m_Ftpd->getUser(loggedUser)->setRealPath(m_vCurrentDir, m_rCurrentDir, false);


	/*
	// delete all "/.." 
	if (strlen(tmp) > 2) {
	for (i=0;i<(strlen(tmp)-2);i++) {
		if (tmp[i]=='/' && tmp[i+1]=='.' && tmp[i+2]=='.') {
			i += 2;	
		}
		else {
			dir[i] = tmp[i];
		}
	}
	dir[i] = tmp[i];
	dir[i+1] = tmp[i+1];
	dir[i+2] = tmp[i+2];
	}
	
	lstrcpy(tmp, dir);


	lstrcpy(dir, tmp);

	// Now we have an absolute path
	// Replace the '/' with '\'
	for (i=0;i<(int)strlen(dir);i++) {
		if (dir[i]=='/') dir[i]='\\';
	}
    
	
	path = (char*)malloc(strlen(m_rRootDir)+2+strlen(dir));
	path = lstrcpy(path, m_rRootDir);

	//delete the last '\' from path
	if (dir[0]=='\\') path[strlen(path)-1]=0;

	// Append dir to path to get the final path
	lstrcat(path, dir);
	if (path[strlen(path)-1]!='\\') lstrcat(path, "\\");
	


	if (strlen(path)>=strlen(m_rRootDir)) {
		lstrcpy(m_vCurrentDir, tmp);
		lstrcpy(m_rCurrentDir, path);
	}

	free((void*)tmp);
	free((void*)dir);
	free((void*)path);

	*/
	
	return 1;

}


//
// And now... All the FTP commands !
//

//
//  CWD
//
int CSession::cwd() {

	char l_oldDir[BUFFER_SIZE];
	lstrcpy(l_oldDir, m_vCurrentDir);

    log(LOG_DEBUG, "oldDir : %s\r\n", l_oldDir);
    log(LOG_DEBUG, "m_vCurrentDir : %s\r\n", m_vCurrentDir);

	if (lastParam[0] == '/') {
		lstrcpy(m_vCurrentDir, lastParam);
	}
	else {
		// deal with relative path
		if (m_vCurrentDir[strlen(m_vCurrentDir)-1] != '/') {
            lstrcat(m_vCurrentDir, "/");
        }
		lstrcat(m_vCurrentDir, lastParam);
    }

    log(LOG_DEBUG, "oldDir : %s\r\n", l_oldDir);
    log(LOG_DEBUG, "m_vCurrentDir : %s\r\n", m_vCurrentDir);
    log(LOG_DEBUG, "m_rCurrentDir : %s\r\n", m_rCurrentDir);

	if (m_Ftpd->getUser(loggedUser)->setRealPath(m_vCurrentDir, m_rCurrentDir, false) ) {
        sendToClient(CWD_SUCCESS);
	}
	else {
		lstrcpy(m_vCurrentDir, l_oldDir);
		sendToClient(CWD_FAILED);
	}

    log(LOG_DEBUG, "oldDir : %s\r\n", l_oldDir);
    log(LOG_DEBUG, "m_vCurrentDir : %s\r\n", m_vCurrentDir);
    log(LOG_DEBUG, "m_rCurrentDir : %s\r\n", m_rCurrentDir);
	// free((void*)newDir);
	return 1;
}

//
//  CDUP
//
int CSession::cdup() {

	int i;

	// delete the trailing slash, if any
	i=strlen(m_vCurrentDir)-1;
	if (m_vCurrentDir[i] == '/') m_vCurrentDir[i] = 0;

	// remove last directory
	for (i=lstrlen(m_vCurrentDir);i>-1;i--) {
		if (m_vCurrentDir[i] == '/') {
			m_vCurrentDir[i] = 0;
			break;
		}
		
	}

	m_Ftpd->getUser(loggedUser)->setRealPath(m_vCurrentDir, m_rCurrentDir, false);


	char a[BUFFER_SIZE];
	wsprintf(a, "%s%s%s", CDUP_1, m_vCurrentDir, CDUP_2);
	sendToClient(a);

	return 1;
}


//
//  PASS
//
int CSession::pass() {

	CUser* u;
	if (loggingUser>=0 && (lstrcmp(m_Ftpd->getUser(loggingUser)->passwd, lastParam)==0/*| strcmp(m_Ftp->users[loggingUser]->passwd, "")==0*/)) {

		// If too many users are connected
		int a = m_Ftpd->getNbConnections();
		int b = m_Ftpd->getMaxConnections();
		int c = m_Ftpd->getNbSessions();
		
        if (m_Ftpd->getNbConnections() >= m_Ftpd->getMaxConnections()) {
            return 4;
        }

		// If current user has reached his max connections
		int userNbConn  = m_Ftpd->getUserNbSessions(loggingUser);
		int userMaxConn = m_Ftpd->getUserMaxConnections(loggingUser);
		if ( userNbConn >= userMaxConn) {
            return 4;
        }

		
		loggedUser = loggingUser;
		// lstrcpy(m_rRootDir, m_Ftpd->getUser(loggedUser)->rootDir);
		
		lstrcpy(m_vCurrentDir, "/");
		u = m_Ftpd->getUser(loggedUser);
		u->setRealPath(m_vCurrentDir, m_rCurrentDir, false);

		// send(m_MainSock, LOG_SUCCESS, lstrlen(LOG_SUCCESS),0);
		sendToClient(LOG_SUCCESS);
		
        updateTipText();
		
		// updateListMsg(m_Ftpd->getUser(loggedUser)->login);

		return 1;
	}

	return 2;
}


//
//  PWD
//
int CSession::pwd() {
	char *resp;
	int i = lstrlen(PWD_1)+lstrlen(PWD_2)+lstrlen(m_vCurrentDir)+2;

	resp = (char*)malloc(i);
	lstrcpy(resp, PWD_1);
	lstrcat(resp, m_vCurrentDir);
	lstrcat(resp, PWD_2);

	sendToClient(resp);
	free((void*)resp);

	return 1;
}

//
//  QUIT
//
int CSession::quit() {

    m_ShouldStopSession = true;

    if (m_TransferInProgress) {
        // Try to stop transfer
        m_ShouldStopTransfer = true;

        // See if running action thread can stop by itself
        if (WaitForSingleObject(m_ActionThread, 2*SMALL_TIMEOUT) == WAIT_TIMEOUT) {
            TerminateThread(m_ActionThread, 0);
        }
    }

	sendToClient(LOGOUT);
	return -1;
}


//
//  REST
//
int CSession::rest() {

	sscanf(lastParam, "%d", &m_Rest);
	char *a;

	a = (char*)malloc(lstrlen(REST_1)+lstrlen(lastParam)+lstrlen(REST_2));
	wsprintf(a, "%s%s%s", REST_1, lastParam, REST_2);
	sendToClient(a);

	free((void*)a);
	return 1;
}


//
//  SYST
//
int CSession::syst() {
	sendToClient(SYSTEM_MSG);
	return 1;
}


//
//  USER
//
int CSession::user() {

	loggingUser = m_Ftpd->findUser(lastParam);
	sendToClient(MSG_REQ_PWD);
	return 1;
}


//
//  ABOR
//
int CSession::abor() {
	
	if (m_ActionThreadRunning) {
        if (m_TransferInProgress) {
            // Try to stop transfer
            m_ShouldStopTransfer = true;

            // See if running action thread can stop by itself
            if (WaitForSingleObject(m_ActionThread, 2*SMALL_TIMEOUT) == WAIT_TIMEOUT) {
                TerminateThread(m_ActionThread, 0);
            }
/*
            if (m_Mode == ACTIVE_MODE) {
	            closesocket(m_DataSock);
	            m_DataSock = 0;
            } else if (m_Mode == PASSIVE_MODE) {
                closesocket(m_PasvSock);
                m_PasvSock = 0;	       
            }
*/
	    }
/*	    
	    // Small delay to let transfer action thread finish
	    Sleep(1000);
	    
	    // Close Action Thread
        TerminateThread(m_ActionThread, 0);
        WaitForSingleObject(m_ActionThread, SMALL_TIMEOUT);
        m_ActionThreadRunning = false;
        log(LOG_DEBUG, "DEBUG - action thread was terminated due to ABOR command- CSession::abor()\r\n");
*/
	   
	}
	
	
	sendToClient(ABOR_SUCCESS);
	return 1;
}


//
//  TYPE
//
int CSession::type() {
	// TODO : change transfer TYPE
	char *a;
	
	a = (char*)malloc(lstrlen(TYPE_SET_1)+lstrlen(lastParam)+lstrlen(TYPE_SET_2));
	wsprintf(a, "%s%s%s", TYPE_SET_1, lastParam, TYPE_SET_2);
	sendToClient(a);

	free((void*)a);
	return 1;
}

//
//  PORT
//
int CSession::port() {
	SOCKADDR_IN lSockAddr;
	int port_a;
	int port_b;
	int ip_a, ip_b, ip_c, ip_d;
	char tmp[BUFFER_SIZE];

	// get specified IP
	sscanf(m_LastCmd, "%*s %d,%d,%d,%d,%d,%d", &ip_a, &ip_b, &ip_c, &ip_d, &port_a, &port_b);
	
	// Set transfer mode to ACTIVE
	m_Mode = ACTIVE_MODE;

	// Set data port
	m_DataPort = 256*port_a + port_b;

	// Set port_ip
	// make a temp buffer "a.b.c.d"
	wsprintf(tmp, "%d.%d.%d.%d", ip_a, ip_b, ip_c, ip_d);

	if (m_DataSock != 0) {
        if (m_DataSock != 0 && closesocket(m_DataSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_DataSock - error %d\r\n", WSAGetLastError()); 
		}
		m_DataSock = 0;
	}

	// Open the specified DATA port
	lSockAddr.sin_family = AF_INET;
	lSockAddr.sin_addr.s_addr = inet_addr(tmp);
	lSockAddr.sin_port = htons(m_DataPort);

    // Create the socket
	m_DataSock = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(m_DataSock, (LPSOCKADDR)&lSockAddr, sizeof(struct sockaddr)) == SOCKET_ERROR) {
		log(LOG_DEBUG, "ERROR - %s - winsock error %d - CSession::port()\r\n", LOG_ERROR_SOCKET, GetLastError());
		m_DataSock = 0;
	};

	sendToClient(PORT_SUCCESS);
	m_Rest = 0;
	return 1;
}

//
//  PASV
//
int CSession::pasv() {
	SOCKADDR_IN lSockAddr;
	unsigned long	ThreadAddr;
	int nRet;

	// Set transfer Mode to PASSIVE
	m_Mode = PASSIVE_MODE;

    // close m_PasvSock if it's in use
	if (m_PasvSock !=0) {
        if (m_PasvSock != 0 && closesocket(m_PasvSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_PasvSock - error %d - CSession::pasv()\r\n", WSAGetLastError()); 
		}
		m_PasvSock = 0;
	}	

	// If a PasvThread is still running, kill it
	if (m_PasvThreadRunning) {
/*
        log(LOG_DEBUG, "DEBUG - a passive thread is already running - CSession::pasv()\r\n");
        if (m_PasvSock != 0 && closesocket(m_PasvSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_PasvSock - error %d - CSession::pasv()\r\n", WSAGetLastError()); 
		}
        m_PasvSock = 0;
*/
        if (WaitForSingleObject(m_PasvThread, SMALL_TIMEOUT) == WAIT_TIMEOUT) {
            TerminateThread(m_PasvThread, 0);
        }
	}


	// killThread(m_PasvThread);

	hostent *h;
	char ip[20];
	int i;

	if (m_Ftpd->usePasvUrl()) {
		// Resolve passive url specified by user
		log(LOG_DEBUG, "DEBUG - resolving passive url - CSession::pasv()\r\n");
		h = gethostbyname(m_Ftpd->getPasvUrl());
		if (h == 0) {
			log(LOG_DEBUG, "DEBUG - unable to resolve hostname '%s' - CSession::pasv()\r\n", m_Ftpd->getPasvUrl());
			return -1;
		}
	}
	else
	{
		// Find local ip
		char buf[BUFFER_SIZE];
		gethostname(buf, BUFFER_SIZE);
		h = gethostbyname(buf);
	}

	if (h->h_addrtype != AF_INET) {
        log(LOG_DEBUG, "ERROR - not an IP address was returned - CSession::pasv()\r\n");
    }
    
	lstrcpy(ip, inet_ntoa(*(LPIN_ADDR)h->h_addr ));

	for (i=0; i<lstrlen(ip); i++) {
		if (ip[i]=='.') ip[i] = ',';
	}
	
	// Get free port to listen to
	m_PasvPort = m_Ftpd->getPasvPort();
	if (m_PasvPort < 0) {
        return -10;
    }

	log(LOG_DEBUG, "DEBUG - Listening to passive port %d - CSession::pasv()\r\n", m_PasvPort);

	// Listen to the PASV port
    // Create the SOCKET
    m_PasvSock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_PasvSock == INVALID_SOCKET) {
		log(LOG_DEBUG, "DEBUG - %s - CSession::pasv()\r\n", LOG_ERROR_SOCKET);
		return -10;
	}

    // Define the SOCKET address
    lSockAddr.sin_family = PF_INET;
    lSockAddr.sin_port = htons(m_PasvPort);
    lSockAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the SOCKET
    nRet = bind(m_PasvSock,(SOCKADDR *)&lSockAddr, sizeof(SOCKADDR_IN));
	if (nRet == SOCKET_ERROR) {
		// log(LOG_DEBUG, "PASV : %s\r\n", ERROR_BIND);
		log(LOG_DEBUG, "DEBUG - %s - CSession::pasv()\r\n", LOG_ERROR_BIND);
		return -10;
	}

	// Listen for an incoming connection
    nRet = listen(m_PasvSock, SOMAXCONN);
	if (nRet == SOCKET_ERROR) {
		log(LOG_DEBUG, "DEBUG - %s - CSession::pasv()\r\n", LOG_ERROR_LISTEN);
		return -10;
	}

	
	// Start of the PasvThread, to accept connections on socket
	GetLocalTime(&m_PasvThreadStartTime);
	m_PasvThread = CreateThread(0, 0, PasvThread, this, 0, &ThreadAddr);
	if (m_PasvThread == NULL) {
		log(LOG_DEBUG, "DEBUG - %s - CSession::pasv()\r\n", LOG_ERROR_CREATE_THREAD);
		return -10;
	}

    // Send reply to client
	char msg[BUFFER_SIZE];
	wsprintf(msg, PASV_SUCCESS, ip, (int)(m_PasvPort/256), (int)(m_PasvPort-256*(int)(m_PasvPort/256)));
	sendToClient(msg);

	m_Rest = 0;
	return 1;
}

//
//  LIST
//
int CSession::list(bool nlst) {

	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "L")) {
        return -2;
    }


    WIN32_FIND_DATA findData;
    HANDLE fileHandle;
	
	int i;

    char size[15];
    char date[30];
    char day[3];
    char hour[3];
    char min[3];
    SYSTEMTIME st;
    long hasMore = 0;

    int index = 0;
    char* rSubDirs;
	

    sendToClient(PORT_OPENED);

	char buf[3*BUFFER_SIZE];
	char *content;
	char *path;

	content = (char*)malloc(1);
	rSubDirs = (char*)malloc(1);
    
	content[0] = '\0';
	rSubDirs[0] = '\0';
	buf[0] = '\0';

	// log(LOG_DEBUG, "TEMP - calling setRealPath (%s) - CSession::list()\r\n", m_vCurrentDir);
	m_Ftpd->getUser(loggedUser)->setRealPath(m_vCurrentDir, m_rCurrentDir, false);

	path = (char*)malloc(sizeof(m_rCurrentDir)+5);
	
	lstrcpy(path, m_rCurrentDir);
	lstrcat(path, "*.*");

    fileHandle = FindFirstFile(path, &findData);
    
	free((void*)path);

	if (lstrlen(m_rCurrentDir)>0) {
        while (true) {
            
            if (fileHandle == INVALID_HANDLE_VALUE) {
                log(LOG_DEBUG, "ERROR - INVALID_HANDLE_FILE ! - CSession::list()\r\n");
                break;
            }
            

            int memNeeded = 0;
            if (nlst) {
                memNeeded = lstrlen(content)+lstrlen(findData.cFileName)+4;
            } else {
                memNeeded = lstrlen(content)+25+15+30+lstrlen(findData.cFileName)+20;
            }
            // log(LOG_DEBUG, "TEMP - current : strlen=%d bytes \r\n", lstrlen(content));
            // log(LOG_DEBUG, "TEMP - trying to reallocate %d bytes for LIST\r\n", memNeeded);

            content = (char*)realloc((void*)content,memNeeded);

            if(content == NULL) {
                log(LOG_DEBUG, "ERROR - !! unable to allocate memory (%d bytes) for LIST !!!! !!!\r\n", memNeeded);
            }

            // log(LOG_DEBUG, "TEMP - %d bytes allocated for char *content - CSession::list()\r\n", memNeeded);
            // log(LOG_DEBUG, "TEMP - new : strlen=%d bytes \r\n", lstrlen(content));
            // log(LOG_DEBUG, "TEMP - new : strlen=%d bytes \r\n", lstrlen(content));
		
		    if (!nlst) {
                if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
                
                    lstrcat(content, "d");
                    // concat new subDir in rSubDirs
                    rSubDirs = (char*) realloc(rSubDirs, index + lstrlen(findData.cFileName) + 1);
                    memcpy(rSubDirs+index, findData.cFileName, lstrlen(findData.cFileName)+1);
                    // char verif[BUFFER_SIZE];
                    // lstrcpy(verif, rSubDirs+index);
                } else {
                    lstrcat(content, "-");
                }
		
                lstrcat(content, "rw-rw-rw- 1 user group ");
            
                // Get the size of the file
                wsprintf(size, "%u ", (findData.nFileSizeHigh * MAXDWORD) + findData.nFileSizeLow);
		
                for (i=0;i<15-lstrlen(size);i++) {
                    lstrcat(content, " ");
                }
                lstrcat(content, size);

                // Get last modified date
                FileTimeToSystemTime(&(findData.ftLastWriteTime), &st);
                if (st.wDay<10) {
                  wsprintf((char*)&day, "0%i", st.wDay);
                } else {
                  wsprintf((char*)&day, "%i", st.wDay);
                }
    		    if (st.wMonth<10) {
                  wsprintf((char*)&hour, "0%i", st.wMonth);
                } else {
                  wsprintf((char*)&hour, "%i", st.wMonth);
                }
    		    if (st.wMinute<10) {
                    wsprintf((char*)&min, "0%i", st.wMinute);
                } else {
                    wsprintf((char*)&min, "%i", st.wMinute);
                }

                wsprintf((char*)&date, "%s %s %s:%s ", months[st.wMonth-1], day, hour, min);
                lstrcat(content, (char*)&date);
            }
            
            lstrcat(content, findData.cFileName);
            
            if (nlst) {
                lstrcat(content, "\r\n");
            } else {
                lstrcat(content, "\r\n");
            }
            
            if (!FindNextFile(fileHandle, &findData)) {
                break;
            }
        }
    } else {
        log(LOG_DEBUG, "ERROR - m_rCurrentDir is empty ! - CSession::list()\r\n", path);
    }

	memcpy(rSubDirs+index, "\0", 1);

    FindClose(fileHandle);

	int nbSubDirs = m_Ftpd->getUser(loggedUser)->getVirtualSubDirs(m_vCurrentDir, rSubDirs, buf);
	
	free((void*)rSubDirs);

	if (!nlst && nbSubDirs>0) {
		// content = (char*)realloc(content,lstrlen(content)+lstrlen(buf)+3);
        // log(LOG_DEBUG, "TEMP - SubDirectories : reallocating memory for content : %d + %d + 10 bytes - CSession::list()\r\n", sizeof(content), sizeof(buf));
		content = (char*)realloc(content,lstrlen(content)+lstrlen(buf)+10);
        lstrcat(content, buf);
		// free((void*)buf);
	}

	lstrcat(content, "\r\n");
	
	// log(LOG_DEBUG, "DEBUG - LIST CONTENT : %s", content);

	if (m_Mode == ACTIVE_MODE) {
		// Use DATA Port to send data
		// log(LOG_DEBUG, "TEMP - sending listing to active port - CSession::list()\r\n");
		if (send(m_DataSock, content, lstrlen(content), 0) == SOCKET_ERROR) {
            log(LOG_DEBUG, "ERROR - error %d occured while sending list - CSession::list()\r\n", WSAGetLastError());
		}

        // close DATA socket
        if (m_DataSock != 0 && closesocket(m_DataSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_DataSock - error %d - CSession::list()\r\n", WSAGetLastError()); 
        }
        m_DataSock = 0;

	} else if (m_Mode == PASSIVE_MODE) {
		// Use PASV sock to send data

		// Wait for m_PasvSock to be initialized (by m_PassiveThread)
		// todo : something else
		while (m_PasvSock2 == 0) {
			Sleep(250);
		}

        // log(LOG_DEBUG, "TEMP - sending listing to passive port - CSession::list()\r\n");
        
		if (send(m_PasvSock2, content, lstrlen(content), 0) == SOCKET_ERROR) {
            log(LOG_DEBUG, "ERROR - error %d occured while sending list - CSession::list()\r\n", WSAGetLastError());
        }

        if (m_PasvSock2 != 0 && closesocket(m_PasvSock2) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_PasvSock2 - error %d - CSession::list()\r\n", WSAGetLastError()); 
        }
        m_PasvSock2 = 0;

	}

    free((void*)content);

    log(LOG_DEBUG, "DEBUG - list sent - CSession::list()\r\n");

    // crashes
    // to do : free this memory
	// free((void*)content);

    // log(LOG_DEBUG, "TEMP - free((void*)content); OK  - CSession::list()\r\n");

	sendToClient(LIST_SUCCESS);

	return 1;
}


//
//  RETR
//
int CSession::retr() {

	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "R")) {
        return -2;
    }

	char *filePath;
	char msg[BUFFER_SIZE];
	char buf[TX_BUFFER_SIZE];
	HANDLE h;
	FILE *f;
	int i;
	int l = 0;
	
	int percent = 0;
	
	char userLogin[BUFFER_SIZE];
    char size1[BUFFER_SIZE];
    char size2[BUFFER_SIZE];
    
    char txRate[BUFFER_SIZE];

	memset(buf, 0, TX_BUFFER_SIZE);

	filePath = (char*)malloc(lstrlen(m_rCurrentDir)+lstrlen(lastParam)+2);
	lstrcpy(filePath, m_rCurrentDir);
	lstrcat(filePath, lastParam);

    lstrcpy(userLogin, m_Ftpd->getUser(loggedUser)->login);

    // Create file handle to check if the file exists,
    // and to retrieve its size
	h = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
	    log(LOG_DEBUG, "ERROR - File %s does not exist ! - CSession::retr()", filePath);
	    sendToClient(FILE_AXX_2);
	    return 1;
	}
	// DWORD low, high;
	int low;
    low = GetFileSize(h, NULL);
    CloseHandle(h);
    formatSize(low, size2);

	
	if (m_Mode != ACTIVE_MODE && m_Mode != PASSIVE_MODE) {
        log(LOG_DEBUG, "ERROR - unknown mode : neither active nor passsive - CSession::retr()");
        return 0;
    }

	// If in PASSIVE MODE, wait until passive Socket is initialized
	// todo : change that
	while(m_Mode == 1 && m_PasvSock2 == 0) {
		Sleep(250);
	}

	sendToClient(PORT_OPENED);
	m_TransferInProgress = true;
	m_ShouldStopTransfer = false;
	

    // open the file for reading	
	f = fopen(filePath, "rb");
 	
	log(LOG_TRANSFER, "Download started by %s [%s] : %s\r\n", userLogin, inet_ntoa(m_ClientAddr), filePath);
	free((void*)filePath);
	fseek(f, m_Rest, 0);
	
	if (f == 0) {
        return 0;
    }

    i = 1;
	while (!m_ShouldStopTransfer && !m_ShouldStopSession) {

	    // read data from file
        i = fread(buf, sizeof(char), TX_BUFFER_SIZE, f);
        if (i == 0) {
             // file completed
             break;
        }
 		
		// send data to socket
		if (m_Mode == 0) {
			if (send(m_DataSock, buf, i, 0) == SOCKET_ERROR) {
                log(LOG_DEBUG, "ERROR - error %d occured while sending data - CSession::retr()\r\n", WSAGetLastError());
			}
		}
		else {
			if (send(m_PasvSock2, buf, i, 0) == SOCKET_ERROR) {
    			log(LOG_DEBUG, "ERROR - error %d occured while sending data - CSession::retr()\r\n", WSAGetLastError());
			}
		}


		// estimate transfer rate
        getTransferRate(i, getTimeAge(m_LastCommandTime), txRate);

        // update m_LastCommandTime
        GetLocalTime(&m_LastCommandTime);
		
        // update size1 string
        formatSize(ftell(f), size1);
        
        if (low <=0 ) {
                percent = 0;
                low = 0;
        } else {
                percent = (int) (100 * (double)(ftell(f)) / (double)low);
        }

        log(LOG_DEBUG, "TEMP - position=%d - taille totale=%d - percent=%d - %d bytes read\r\n", ftell(f), low, percent, i);

		wsprintf(msg, "#%d - %s - downloading \"%s\" - %s / %s ( %d%% - %s )", m_SessionIndex, userLogin, lastParam, size1, size2, percent, txRate);
		updateListMsg(msg);

		
		// Debugging ONLY : slow down transfer
		// Sleep(2000);
		
	}
	log(LOG_DEBUG, "TEMP - closed file - CSession::retr()\r\n"); 
	fclose(f);

	
	
    // Close data transfer socket
	if (m_Mode == ACTIVE_MODE) {

        if (m_DataSock != 0 && closesocket(m_DataSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_DataSock - error %d - CSession::retr()\r\n", WSAGetLastError()); 
        }
        m_DataSock = 0;
        log(LOG_DEBUG, "TEMP - closed m_DataSock - CSession::retr()\r\n"); 
	}
	else {
        if (m_PasvSock2 != 0 && closesocket(m_PasvSock2) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_PasvSock2 - error %d - CSession::retr()\r\n", WSAGetLastError()); 
        }
        m_PasvSock2 = 0;
        log(LOG_DEBUG, "TEMP - closed m_PasvSock2 - CSession::retr()\r\n"); 
	}
	
	if (m_ShouldStopTransfer) {
        sendToClient(TRANSFER_ABORTED);
    } else {
    	sendToClient(PORT_CLOSE);
	}

	wsprintf(msg, "#%d - user %s - download completed - \"%s\"", m_SessionIndex, userLogin, lastParam, size1, size2, ftell(f)*100/low);
	updateListMsg(msg);

	log(LOG_TRANSFER, "Download completed by %s : %s\r\n", userLogin, inet_ntoa(m_ClientAddr), filePath);

    // update lastCommandTime to avoid inactivity timeout
	GetLocalTime(&m_LastCommandTime);
	m_TransferInProgress = false;

	return 1;
}



//
//  RNFR
//
int CSession::rnfr() {

	char buf[BUFFER_SIZE];

	// check permission
	// if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "W")) return -2;

	
	if (lastParam[0]=='/') {
		// absolute path
		// check permission in specified path
		if (!m_Ftpd->getUser(loggedUser)->hasPermission(lastParam, "W")) return -2;
		m_Ftpd->getUser(loggedUser)->setRealPath(lastParam, m_RenameFrom, false);
	}
	else {
		//relative path
		lstrcpy(buf, m_vCurrentDir);
		lstrcat(buf, "/");
		lstrcat(buf, lastParam);
		
		if (!m_Ftpd->getUser(loggedUser)->hasPermission(buf, "W")) return -2;
		m_Ftpd->getUser(loggedUser)->setRealPath(buf, m_RenameFrom, true);

	}

	// Remove trailing \   
	// TODO : make sthing better
	m_RenameFrom[lstrlen(m_RenameFrom)-1] = 0;


	// TODO : check if file exists  (+ check if it's a directory ?)

	sendToClient(RNFR_SUCCESS);
	return 1;
}

//
//  RNTO
//
int CSession::rnto() {

	
	// check write permission in target dir
	// if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "W")) return -2;

	char to[BUFFER_SIZE];
	char buf[BUFFER_SIZE];
	bool success = true;

	if (lastParam[0] == '/') {
		// Absolute path
		if (!m_Ftpd->getUser(loggedUser)->hasPermission(lastParam, "W")) return -2;
		success = m_Ftpd->getUser(loggedUser)->setRealPath(lastParam, to, true);

	}
	else {
		// Relative path
		lstrcpy(buf, m_vCurrentDir);
		if (buf[lstrlen(buf)] != '/') {
			lstrcat(buf, "/");
		}
		lstrcat(buf, lastParam);
		
		if (!m_Ftpd->getUser(loggedUser)->hasPermission(buf, "W")) return -2;
		success = m_Ftpd->getUser(loggedUser)->setRealPath(buf, to, true);

	}

	// Remove trailing \
	// TODO : find sthing better !
	to[strlen(to)-1] = 0;

	// TODO : check if everything worked fine
	if (lstrlen(m_RenameFrom)>0 && success) {
		if (MoveFile(m_RenameFrom, to) != 0) {
			sendToClient(RNTO_SUCCESS);
		}
		else {
			sendToClient(RNTO_FAILED);
		}

	}
	else {
		sendToClient(RNTO_FAILED);
	}

	// free((void*)from);
	// free((void*)to);
	return 1;

}

//
//  MKD
//
int CSession::mkd() {

	char *dir;
	char *msg;
	

	// Absolute path
	if (lastParam[0] == '/') {
		// check permission
		if (!m_Ftpd->getUser(loggedUser)->hasPermission(lastParam, "W")) return -2;
		dir = (char*)malloc(BUFFER_SIZE);
		// Force setting real path
		m_Ftpd->getUser(loggedUser)->setRealPath(lastParam, dir, true);
	}
	// Relative path
	else {
	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "W")) return -2;
	
		dir = (char*)malloc(strlen(m_rCurrentDir)+strlen(lastParam)+3);
		lstrcpy(dir, m_rCurrentDir);
		lstrcat(dir, lastParam);
	}

	// NO trailing \ to create a new dir !
	if (dir[strlen(dir)-1] == '\\') {
		dir[strlen(dir)-1] = '\0';
	}

	CreateDirectory(dir, NULL);

	msg = (char*)malloc(strlen(MKD_SUCCESS_1)+strlen(MKD_SUCCESS_2)+strlen(lastParam)+3);
	lstrcpy(msg, MKD_SUCCESS_1);
	lstrcat(msg, lastParam);
	lstrcat(msg, MKD_SUCCESS_2);
	sendToClient(msg);

	free((void*)dir);
	free((void*)msg);
	return 1;
}


//
//  RMD : Remove Directory
//
int CSession::rmd() {
	
	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "W")) return -2;


	char *dir;

	// Absolute path
	if (lastParam[0] == '/') {
		dir = (char*)malloc(BUFFER_SIZE);
		m_Ftpd->getUser(loggedUser)->setRealPath(lastParam, dir, false);
	}
	else {
		dir = (char*)malloc(strlen(m_rCurrentDir)+strlen(lastParam)+3);
		lstrcpy(dir, m_rCurrentDir);
		lstrcat(dir, lastParam);
	}

	// TODO : check rights

	if (RemoveDirectory(dir)==0)
		sendToClient(RMD_FAIL);
	else
		sendToClient(RMD_SUCCESS);

	free((void*)dir);
	return 1;
}

//
//  DELE
//
int CSession::dele() {

	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "W")) return -2;

	char* tmp;

	// TODO : check if file is correct
	// TODO : check rights
	tmp = (char*)malloc(strlen(m_rCurrentDir)+strlen(lastParam)+2);
	
	lstrcpy(tmp, m_rCurrentDir);
	lstrcat(tmp, lastParam);
	DeleteFile(tmp);

	sendToClient(DELE_SUCCESS);

	free((void*)tmp);

	return 1;
}

//
//  SIZE
//
int CSession::size() {

	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "L")) return -2;

	char *tmp;
	char res[BUFFER_SIZE];

	HANDLE hFile;
	WIN32_FIND_DATA findData;

	tmp = (char*)malloc(sizeof(m_rCurrentDir)+sizeof(lastParam));
	wsprintf(tmp, "%s%s", m_rCurrentDir, lastParam);

	hFile = FindFirstFile(tmp, &findData);
	free((void*)tmp);
	if (hFile != INVALID_HANDLE_VALUE) {
		wsprintf(res, "213 %u\r\n", (findData.nFileSizeHigh * MAXDWORD) + findData.nFileSizeLow);
    } else {
		wsprintf(res, FILE_AXX_2);
    }

	FindClose(hFile);

	
	// lSize = GetFileSize((HANDLE)f, NULL);


	sendToClient(res);

	return 1;
}

//
//  STOR
//
 int CSession::stor() {

	// check permission
	if (!m_Ftpd->getUser(loggedUser)->hasPermission(m_vCurrentDir, "W")) {
        return -2;
    }


	int nReadBytes;
	char *filePath;
	char acReadBuffer[TX_BUFFER_SIZE];
	FILE *f;
	bool permissionDenied=false;

	long l = 0;
	char msg[BUFFER_SIZE];

	// check rights
	char name[BUFFER_SIZE];
	char name2[BUFFER_SIZE];
	char pass[BUFFER_SIZE];
	char txRate[BUFFER_SIZE];
	
	// char root[BUFFER_SIZE];
	int  simConn;
	int  timeOut;

	char openMode[5];

    m_ShouldStopTransfer = false;

	m_Ftpd->getUserProperties(loggedUser, name, pass, &simConn, &timeOut);
	lstrcpy(name2, "upload_");
	lstrcat(name2, name);

	// unsigned int i=0;
	// int offset=0;
	// if (m_rCurrentDir[strlen(m_rCurrentDir)-1] == '\\') offset++;
	/*
	while (i<strlen(name2)) {
		// if user is not in a dir called "upload_xxx", permission is denied
		if (name2[strlen(name2)-i-1] != m_rCurrentDir[strlen(m_rCurrentDir)-offset-i-1]) {
			// close the sockets
			if (m_Mode == ACTIVE_MODE) {
				closesocket(m_DataSock);
				m_DataSock = 0;
			}
			else {
				closesocket(m_PasvSock2);
				m_PasvSock2 = 0;
			}
			return -2;
		}
		i++;
	}
	*/

    char userLogin[BUFFER_SIZE];
    lstrcpy(userLogin, m_Ftpd->getUser(loggedUser)->login);

	if (m_Mode != ACTIVE_MODE && m_Mode != PASSIVE_MODE) return 0;

	// If in PASSIVE MODE, wait until passive Socket is initialized
	// TODO : CHANGE THAT !!!!!
	while(m_Mode == PASSIVE_MODE && m_PasvSock2 == 0) {
		Sleep(250);
	}

	filePath = (char*)malloc(strlen(m_rCurrentDir)+strlen(lastParam)+2);
	lstrcpy(filePath, m_rCurrentDir);
	lstrcat(filePath, lastParam);

	if (m_Rest>0) {
		lstrcpy(openMode, "ab+");
    } else {
		lstrcpy(openMode, "wb+");
    }

	f = fopen(filePath, openMode);

	log(LOG_TRANSFER, "UL by %s [%s] : %s\r\n", userLogin, inet_ntoa(m_ClientAddr), filePath);
	free((void*)filePath);

	

	// file could not be created
	if (f == NULL) {
		log(LOG_DEBUG, "ERROR - unable to create file - CSession::stor()\r\n");
		return 1;
	}


	if (fseek(f, m_Rest, 0)!=0) {
		log(LOG_DEBUG, "ERROR - unable to resume transfer - CSession::stor()\r\n");
	}

	sendToClient(PORT_OPENED);
	m_TransferInProgress = true;

	nReadBytes = 1;
	while (nReadBytes > 0 && !m_ShouldStopTransfer && !m_ShouldStopSession) {
        
        if (m_Mode == ACTIVE_MODE) {
			nReadBytes = recv(m_DataSock, acReadBuffer, TX_BUFFER_SIZE, 0);
		}
		else {
			nReadBytes = recv(m_PasvSock2, acReadBuffer, TX_BUFFER_SIZE, 0);
		}
		
        if (nReadBytes <= 0) {
            break;
        }
        
		if (l==0) {
            // estimate transfer rate
            getTransferRate(nReadBytes, getTimeAge(m_LastCommandTime), txRate);

            // update m_LastCommandTime, so that the connexion will not expire
    		GetLocalTime(&m_LastCommandTime);

			wsprintf(msg, "#%d - %s - uploading \"%s\" - %d kB (%s)", m_SessionIndex, m_Ftpd->getUser(loggedUser)->login, lastParam, ftell(f)/1024, txRate);
			updateListMsg(msg);
		}
		else if (l == 10) {
			l=0;
		}
		else {
			l++;
		}
		if (nReadBytes == SOCKET_ERROR) {
			// Client probably closed connection
			DWORD err;
			// char tmp[BUFFER_SIZE];
			// err = GetLastError();
			// wsprintf(tmp, "Winsock error #%i", err);
			// MessageBox(NULL, tmp, "Error - Connection closed",0);
			log(LOG_DEBUG, "ERROR - connection closed, transfer aborted - CSession::stor()\r\n");
			break;
		}

		long pos = ftell(f);
		
		fwrite(acReadBuffer, sizeof(char), nReadBytes, f);
		
		// VERY TEMP !!!
		// Sleep(3000);

	}

	fclose(f);
	
	if (m_Mode == ACTIVE_MODE) {
        if (m_DataSock != 0 && closesocket(m_DataSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_DataSock - error %d - CSession::stor()\r\n", WSAGetLastError()); 
        }
        m_DataSock = 0;
	}
	else {
        if (m_PasvSock != 0 && closesocket(m_PasvSock) != 0) {
            log(LOG_DEBUG, "ERROR - could not close m_PasvSock - error %d - CSession::retr()\r\n", WSAGetLastError()); 
        }
        m_PasvSock = 0;
	}
	
    if (m_ShouldStopTransfer) {
        sendToClient(TRANSFER_ABORTED);
    } else {
    	sendToClient(PORT_CLOSE);
	}

    // update lastCommandTime to avoid inactivity timeout
	GetLocalTime(&m_LastCommandTime);
	m_TransferInProgress = false;
	
	return 1;
}

//
// CleanOut : kill unused connections & threads if delay has expired.
//
bool CSession::hasExpired() {
	long to;

    // log(LOG_DEBUG, "DEBUG - checking session S%d - CSession::CleanOut()\r\n", m_SessionIndex);

	if (loggedUser>-1) {
		to = 1000*m_Ftpd->getUser(loggedUser)->timeOut;
    } else {
		to = BIG_TIMEOUT;
    }
	
	// Passive threads & connections
	if (m_PasvThreadRunning && getTimeAge(m_PasvThreadStartTime) > MEDIUM_TIMEOUT) {

        log(LOG_DEBUG, "DEBUG - killing passive socket for session S%d - CSession::CleanOut()\r\n", m_SessionIndex);
		closePasvSock();
		// killThread(m_PasvThread);
		m_PasvThreadRunning = false;
	}

    /*
    if (m_TransferInProgress) {
        log(LOG_DEBUG, "Transfer in progress.\r\n");
    } else {
        log(LOG_DEBUG, "No transfer in progress.\r\n");
    }
    log(LOG_DEBUG, "main sock : %d\r\n", m_MainSock);
    */
    

	// Client Inactivity
	// if (!m_TransferInProgress && getTimeAge(m_LastCommandTime)>to) {
	if (getTimeAge(m_LastCommandTime)>to) {
    	log(LOG_DEBUG, "DEBUG - client inactivity timeout for session S%d - CSession::CleanOut()\r\n", m_SessionIndex);
		// return 0 to tell m_Ftpd that this session must be closed
		return true;
	}
	return false;
}


bool CSession::updateListMsg(char *msg) {

    // log(LOG_DEBUG, "DEBUG - updating list message for S%d : %s - CSession::updateListMsg()\r\n", m_SessionIndex, msg);
	
	HWND listBox = GetDlgItem(m_Ftpd->getHwnd(), IDC_CONNECTIONS);
	
	if (m_ShouldStopSession || m_SessionIndex < 0) {
	    return false;
    }

	int sel;

	// update GUI
	sel = SendMessage(GetDlgItem(m_Ftpd->getHwnd(), IDC_CONNECTIONS), LB_GETCURSEL, 0, 0);
	
	SendMessage(listBox, WM_SETREDRAW, false, 0);

	SendMessage(listBox, LB_INSERTSTRING, m_SessionIndex, (LPARAM) msg);
    SendMessage(listBox, LB_DELETESTRING, m_SessionIndex+1, 0);
    
    // SendMessage(listBox, LB_DELETESTRING, m_Ftpd->getNbConnections(), 0);
        
	int scrollPos = GetScrollPos(listBox, SB_HORZ);
 
    HDC hdc = GetDC(listBox);
	// get longest text in the list box to update scrollbar
	char buf[BUFFER_SIZE];
	long maxWidth = 0;
	SIZE size;
    int i;
	for (i=0; i <= m_Ftpd->getNbConnections()+1; i++) {
	    SendMessage(listBox, LB_GETTEXT, i, (LPARAM) &buf);
        
        GetTextExtentPoint32( hdc, buf, lstrlen(buf), &size ); 

        if( size.cx > maxWidth ) {
    		maxWidth = size.cx;

            SendMessage( listBox, LB_SETHORIZONTALEXTENT, maxWidth - 200, 0 );
        }

	}

    // update horizontal scrollbar position
    SetScrollPos(listBox, SB_HORZ, scrollPos, true);

    SendMessage(listBox, LB_SETCURSEL, sel, 0);
    SendMessage(listBox, WM_SETREDRAW, true, 0);


	return true;
}

unsigned long __stdcall ActionThread(void* pVoid)
{
    CSession *session = (CSession*)pVoid;

    log(LOG_DEBUG, "DEBUG - action thread started - ActionThread()\r\n");    
    session->m_ActionThreadRunning = true;    

    session->parse();

    session->m_ActionThreadRunning = false;
    log(LOG_DEBUG, "DEBUG - action thread finished - ActionThread()\r\n");
}

