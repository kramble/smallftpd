// Revision : $Id: CFtpd.cpp,v 1.1.1.1 2004/04/05 21:26:56 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
//   CFtpd is the main Ftpd class, listening to incoming connections and
// creating the CSessions objects to deal with them
// 

#include <windows.h>
#include <winbase.h>

#include "const.h"
#include "resource.h"
#include "CUser.h"
#include "CFtpd.h"
#include "CSession.h"
#include "main.h"
#include "mylib.h"


CFtpd::CFtpd() {
    m_hWnd = NULL;
    m_ListeningPort = DEFAULT_PORT;
    nbSessions = 0;
    m_IsRunning = false;
    nbUsers = 0;

    m_UsePasvUrl = false;
    m_MinPasvPort = DEFAULT_MIN_PASV_PORT;
    m_MaxPasvPort = DEFAULT_MAX_PASV_PORT;
    nbPasvPortsInUse = 0;

    memset((void*)m_PasvUrl, 0, sizeof(m_PasvUrl));

    // Start TimerThread
    unsigned long   ThreadAddr;
    m_TimerThread = CreateThread(NULL, 50, TimerThread, this, 0, &ThreadAddr);
}


CFtpd::~CFtpd() {
    int i;

    if (m_ListeningSock != 0 && closesocket(m_ListeningSock) != 0) {
        log(LOG_DEBUG, "ERROR - could not close m_PasvSock - error %d - CFtpd::~CFtpd()\r\n", WSAGetLastError()); 
    }
    m_ListeningSock = 0;

    for (i=0;i<nbUsers;i++) {
        free((void*)m_Users[i]);
    }

    WSACleanup();

    TerminateThread(m_TimerThread, 0);
    TerminateThread(m_StopServerThread, 0);

    WaitForSingleObject(m_ListeningThread, SMALL_TIMEOUT);
    WaitForSingleObject(m_StopServerThread, SMALL_TIMEOUT);
    // WaitForSingleObject(m_StopServerThread, SMALL_TIMEOUT);

}


//
//  START
//
bool CFtpd::start() {
    SOCKADDR_IN     sockAddr;
    unsigned long   ThreadAddr;
    int nRet;


    m_ShouldStop = false;
    // log(LOG_DEBUG, LOG_STARTING);

    // Create the SOCKET
    // log(DEBUG_FILE, "Creating socket %d\r\n", m_ListeningSock);

    // Start Winsock up
    int nCode;
    WSAData wsaData;
    if ((nCode = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0) {
        log(LOG_DEBUG, "ERROR - WSAStartup() returned error code %d - CFtpd::start()\r\n", nCode);
    }

    m_ListeningSock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_ListeningSock == INVALID_SOCKET) {
        log(LOG_DEBUG, "ERROR - %s - CFtpd::start()", LOG_ERROR_SOCKET);
        stop();
        return false;
    }

    // log(LOG_DEBUG, "Socket %d created\r\n", m_ListeningSock);

    // Define the SOCKET address
    sockAddr.sin_family = PF_INET;
    sockAddr.sin_port = htons(m_ListeningPort);
    sockAddr.sin_addr.s_addr = INADDR_ANY;

    // log(LOG_DEBUG, "Socket %d address defined\r\n", m_ListeningSock);

    // Bind the SOCKET
    nRet = bind(m_ListeningSock,(SOCKADDR *)&sockAddr, sizeof(SOCKADDR_IN));
    if (nRet == SOCKET_ERROR) {
        log(LOG_DEBUG, "ERROR - %s - CFtpd::start()", LOG_ERROR_BIND);
        stop();
        setInfoText(INFO_COULD_NOT_BIND);
        return false;
    }
    
    // log(LOG_DEBUG, "Socket %d bound\r\n", m_ListeningSock);

    // Listen for an incoming connection
    nRet = listen(m_ListeningSock, SOMAXCONN);
    if (nRet == SOCKET_ERROR) {
        log(LOG_DEBUG, "ERROR - %s - CFtpd::start()", LOG_ERROR_LISTEN);
        stop();
        return false;
    }

    // Start of the ListenThread
    m_ListeningThread = CreateThread(NULL, 50, ListenThread, this, 0, &ThreadAddr);
    if (m_ListeningThread == NULL) {
        log(LOG_DEBUG, "ERROR - %s - CFtpd::start()", LOG_ERROR_CREATE_THREAD);
        stop();
        return false;
    }
    // SetThreadPriority(listenThread, THREAD_PRIORITY_LOWEST);

    // log(LOG_DEBUG, LOG_STARTED);
    m_IsRunning = true;

    log(LOG_DEBUG, "DEBUG - ------------------------------------------\r\n");
    log(LOG_DEBUG, "DEBUG - smallftpd server was started\r\n");
    log(LOG_DEBUG, "DEBUG - smallftpd is now listening on port %d\r\n", m_ListeningPort);

    setInfoText(INFO_SERVER_RUNNING);
    return true;
}


//
//  STOP
//
bool CFtpd::stop() {
    
    // Stopping server needs calling WaitForSingleObject, which would block the application
    // If it's run from the main application thread.
    // So let's start another thread
    unsigned long   ThreadAddr;
    m_StopServerThread = CreateThread(NULL, 50, StopServerThread, this, 0, &ThreadAddr);

    log(LOG_DEBUG, "DEBUG - smallftpd server was stopped - CFtpd::stop()\r\n");
    
    setInfoText(INFO_SERVER_STOPPED);
    return true;
}

bool CFtpd::stopServer() {
    
    m_ShouldStop = true;

    // log(LOG_DEBUG, "DEBUG - closing all sessions - CFtpd::stopServer()\r\n");
    // Close all Sessions
    for (int i=nbSessions-1;i>-1;i--) {
        // m_Sessions[i]->Kill();
        // closeSession(i);
        m_Sessions[i]->close();
        // removeSession(i);
    }

    // Close all connections
    if (m_ListeningSock != 0 && closesocket(m_ListeningSock) != 0) {
        log(LOG_DEBUG, "ERROR - could not close m_PasvSock - error %d - CFtpd::stopServer()\r\n", WSAGetLastError()); 
    }
    m_ListeningSock = 0;    

    // m_ListeningSock = 0;
    WSACleanup();
    if (WaitForSingleObject(m_ListeningThread, MEDIUM_TIMEOUT) == WAIT_TIMEOUT) {
        TerminateThread(m_ListeningThread, 0);
    }

    // killThread(m_ListeningThread);;

    m_IsRunning = false;

    // Update main window interface
    updateUI();

    // log(LOG_DEBUG, "DEBUG - smallftpd server was stopped");
    

    return true;
}


// add USER
int CFtpd::addUser(char *login_, char *passwd_, int simConn_, int timeOut_) {
    CUser *u;
    int i;

    // It's forbidden to create blank logins
    if (lstrcmp(login_, "") == 0) return -1;

    /*
    if (rootDir_[strlen(rootDir_)-1]!='\\') {
        strcat(rootDir_, "\\\0");
    }
    */
    i = findUser(login_);
    if (i>=0) {
        // edit user
        // m_Users[i]->set(login_, passwd_, simConn_, timeOut_);
    }
    else {
        // add User
        u = (CUser*)new CUser();
        u->set(login_, passwd_, simConn_, timeOut_);
        m_Users[nbUsers] = u;
        i = nbUsers;
        nbUsers++;
    }
    
    return i;
}


bool CFtpd::deleteUser(int index_) {

    int i;
    for (i=index_;i<nbUsers-1;i++) {
        m_Users[i]->copyFrom(m_Users[i+1]);
    }
    delete m_Users[nbUsers-1];

    nbUsers--;
    return true;
}


int CFtpd::findUser(char *login_) {

    int i;

    for (i=0;i<nbUsers;i++) {
        if (lstrcmp(m_Users[i]->login,login_)==0) {
            // strcpy(user_, users[i]->user);
            return i;
        }
    }

    // memset((void*)user_, 0, sizeof(user_));
    return -1;
}


bool CFtpd::getUserProperties(int index_, char* login_, char* passwd_, int* simConn_, int* timeOut_) {

    if (index_<nbUsers && index_>=0) {
        lstrcpy(login_, m_Users[index_]->login);
        lstrcpy(passwd_, m_Users[index_]->passwd);
        //strcpy(rootDir_, m_Users[index_]->rootDir);
        *simConn_ = m_Users[index_]->simConn;
        *timeOut_ = m_Users[index_]->timeOut;
        return true;
    }

    memset((void*)login_, 0, sizeof(login_));
    memset((void*)passwd_, 0, sizeof(passwd_));
    // memset((void*)rootDir_, 0, sizeof(rootDir_));

    return false;
}


int CFtpd::getUserNbSessions(int userIndex) {
    int ret = 0;

    for (int i=0; i<nbSessions; i++) {
        if (m_Sessions[i]->loggedUser == userIndex) {
            ret++;
        }
    }
    return ret;
}


int CFtpd::getUserMaxConnections(int userIndex) {
    return m_Users[userIndex]->simConn;
}

int CFtpd::getNbConnections() {
    int ret = 0;

    for (int i=0; i<nbSessions; i++) {
        if (m_Sessions[i]->loggedUser>=0) {
            ret++;
        }
    }
    return ret;
}

int CFtpd::addSession(CSession *s) {

    m_Sessions[nbSessions] = s;
    nbSessions++;

    return nbSessions-1;

}

bool CFtpd::removeSession(int n) {

    log(LOG_DEBUG, "DEBUG - deleting session S%d - CFtpd::removeSession()\r\n", m_Sessions[n]->m_SessionIndex);

    delete m_Sessions[n];

	// Remove user line from the 'connected users' listBox
	HWND h;
	h = GetDlgItem(m_hWnd, IDC_CONNECTIONS);
	if (h!= NULL) {
		// SendMessage(h, LB_DELETESTRING, m_SessionIndex, 0);
		// remove the LAST line
		SendMessage(h, LB_DELETESTRING, n, 0);
		// m_SessionIndex = -1;
	}
	else {
	  log(LOG_DEBUG, "ERROR - Error : unable to find window handle to IDC_CONNECTIONS - CFtpd::removeSession()\r\n");
    }
    
    log(LOG_DEBUG, "DEBUG - removeSession S%d - %d total sessions\r\n", n, nbSessions);
    
    for (int i = n; i < nbSessions-1; i++) {
    
        m_Sessions[i] = m_Sessions[i+1];

        m_Sessions[i]->m_SessionIndex--;
        m_Sessions[i]->m_ListConnIndex--;
        
        // log(LOG_DEBUG, "DEBUG - Session S%d moved to S%d - sessionIndex=%d, listConnIndex=%d - CFtpd::removeSession()\r\n", i+1, i, m_Sessions[i]->m_SessionIndex, m_Sessions[i]->m_ListConnIndex);

    }

    
    nbSessions--;
    
	updateTipText();
    	
    return true;
}

bool CFtpd::setPasvPortRange(int min, int max) {

    m_MinPasvPort = min;
    m_MaxPasvPort = max;

    return true;
}


int CFtpd::getPasvPort() {
    
    int   i, j;
    bool  exist;
    
    // Get the first free PASV port
    for (i=m_MinPasvPort; i<=m_MaxPasvPort; i++) {
        exist = false;
        // check if it's free
        for (j=0; j<nbPasvPortsInUse; j++) {
            if(m_PasvPortsInUse[j]==i) exist=true;
        }
        if (!exist) break;
    }
    
    if (exist) {
        log(LOG_DEBUG, "ERROR - could not find a free passive port to give to client - CFtpd::getPasvPort()\r\n");
        i = -1;
    }
    log(LOG_DEBUG, "DEBUG - passive port %d given - CFtpd::getPasvPort()\r\n", i);
    m_PasvPortsInUse[nbPasvPortsInUse] = i;
    nbPasvPortsInUse++;

    return i;

}


bool CFtpd::freePasvPort(int port) {
    int i;

    for (i=0;i<nbPasvPortsInUse; i++) {
        if (m_PasvPortsInUse[i] == port) {
            // remove the port from array
            nbPasvPortsInUse--;
        }
        else {
            m_PasvPortsInUse[i] = m_PasvPortsInUse[i+1];
        }
    }

    log(LOG_DEBUG, "DEBUG - passive port %d was set free - CFtpd::freePasvPort()\r\n", port);

    return true;
}



//
// gotTimer : This function is called every TIMER_DELAY milliseconds, 
//            by the TIMER thread
//            It closes unclosed / unused connections, etc...
//
void CFtpd::gotTimer() {

    // TODO : Session destruction might cause errors.

    // Cleanout functions causes functions to 
    for (int i=0; i<nbSessions && m_IsRunning; i++) {
        // log(LOG_DEBUG, "got Timer !\r\n");
        // log(LOG_DEBUG, "DEBUG - checking session S%d - CFtpd::gotTimer()\r\n", i);
        if (m_Sessions[i]->hasExpired()) {
            
            m_Sessions[i]->close();
            // removeSession(i);
            // closeSession(i);
        }
        // log(LOG_DEBUG, "DEBUG - session S%d OK - CFtpd::gotTimer()\r\n", i);
    }
    
}



unsigned long __stdcall ListenThread(void *pVoid)
{
    sockaddr_in     remoteAddr_in;
    int             nLen;
    CFtpd           *ftp;

    ftp = (CFtpd*)pVoid;
    nLen = sizeof(sockaddr_in);

    while(!ftp->m_ShouldStop)   {
        DWORD nThreadID;

        SOCKET s = accept(ftp->getListeningSock(), (sockaddr *)&remoteAddr_in, &nLen);
        if (s != INVALID_SOCKET && !ftp->m_ShouldStop) {
            log(LOG_DEBUG, "DEBUG - main listening thread accepted a connection from %s - ListenThread()\r\n", inet_ntoa(remoteAddr_in.sin_addr));

            // Create a new CSession
            CSession *session;
            session = new CSession(s, ftp);
            session->setClientAddr ((in_addr)(remoteAddr_in.sin_addr));
            
            // Start a client thread to handle this request
            HANDLE h = CreateThread(0, 0, ClientThread, (void*)session, 0, &nThreadID);
            // SetThreadPriority(h, THREAD_PRIORITY_LOWEST);
            
            // Set Session's thread handle
            session->setMainThread(h);
            
            // Register Cftpd's session
            session->m_SessionIndex = ftp->addSession(session);
        }



    }

    return 0;
}



unsigned long __stdcall ClientThread(void* pVoid)
{
    CSession *session = (CSession*)pVoid;

    log(LOG_DEBUG, "DEBUG - client thread started for session S%d - ClientThread()\r\n", session->m_SessionIndex);

    // Send Welcome Message
    send(session->getMainSock(), MSG_CONNECTED_1, lstrlen(MSG_CONNECTED_1), 0);
    send(session->getMainSock(), VERSION, lstrlen(VERSION), 0);
    send(session->getMainSock(), MSG_CONNECTED_2, lstrlen(MSG_CONNECTED_2), 0);

    // Dialog with FTP client
    session->dialog();

    // Object is deleted by Ftpd object

    // wait for session to be closed
    while (!session->m_IsClosed) {
        Sleep(200);
    }

    // unregister session 
    (session->getCFtpd())->removeSession(session->m_SessionIndex);

    return 0;
}


unsigned long __stdcall PasvThread(void* pVoid)
{
    CSession *session = (CSession*)pVoid;
    sockaddr_in     remoteAddr_in;
    int             nLen;

    session->m_PasvThreadRunning = true;
    
    log(LOG_DEBUG, "DEBUG - passive thread was started, waiting for incoming connection - PasvThread()\r\n");

    nLen = sizeof(sockaddr_in);

    // Accept incoming connections on passive port
    SOCKET s = accept(session->getPasvSock(), (sockaddr *)&remoteAddr_in, &nLen);
    if (s != INVALID_SOCKET) {

        log(LOG_DEBUG, "DEBUG - passive thread received a connection (socket %d) - PasvThread()\r\n", s);
        
        if (session->getPasvSock2()!=0) {
            session->closePasvSock2();
        }
        session->setPasvSock2(s);
        // session->m_PasvPort = -1;
        // log(LOG_DEBUG, "Passive port free.\r\n");
    }

    session->closePasvSock();
    session->getCFtpd()->freePasvPort(session->getPasvPort());

    log(LOG_DEBUG, "DEBUG - passive thread was stopped - PasvThread()\r\n");
    session->m_PasvThreadRunning = false;
    return 0;
    

}

unsigned long __stdcall TimerThread(void *pVoid) {

    CFtpd* ftp;

    ftp = (CFtpd*)pVoid;

    while(1) {
        Sleep(TIMER_DELAY);
        ftp->gotTimer();
    }
    return 0;

}

unsigned long __stdcall StopServerThread(void *pVoid) {

    CFtpd* ftp;

    ftp = (CFtpd*)pVoid;
    ftp->stopServer();

    return 0;
}
