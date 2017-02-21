// Revision : $Id: CSession.h,v 1.1.1.1 2004/04/05 21:26:54 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
// This is the header file for CSession.cpp
// 

#ifndef CSESSION_H_
#define CSESSION_H_

#include "mylib.h"

unsigned long __stdcall ActionThread(void *pVoid);

class CSession {

    public:
        CSession(SOCKET s, CFtpd* ftp);
        ~CSession();

        int    m_ListConnIndex;

        SYSTEMTIME  m_PasvThreadStartTime;
        bool        m_PasvThreadRunning;

        SYSTEMTIME  m_LastCommandTime;
        bool        m_TransferInProgress;

        bool        m_ActionThreadRunning;
        bool        m_IsClosed;
        
    private:

        bool   m_ShouldStopSession;
        bool   m_ShouldStopTransfer;
        
        bool   m_IsAboutToBeClosed;

        bool   m_Exit;

        SOCKET m_MainSock;
        SOCKET m_DataSock;
        SOCKET m_PasvSock;
        SOCKET m_PasvSock2;

        HANDLE m_ActionThread;
        HANDLE m_PasvThread;
        HANDLE m_MainThread;

        CFtpd *m_Ftpd;
        char  m_vCurrentDir[BUFFER_SIZE];
        char  m_rCurrentDir[BUFFER_SIZE];
        // char m_vCurrentDir[1024];
        // char *m_rRootDir;               // TO REMOVE : root dir depends on user !

        char m_LastCmd[BUFFER_SIZE];
        char lastParam[BUFFER_SIZE];

        char m_RenameFrom[BUFFER_SIZE];
        long m_Rest;
        
        
        // Transfer Mode : ACTIVE_MODE or PASSIVE_MODE
        int m_Mode;

        int m_DataPort;
        int m_PasvPort;
        in_addr m_ClientAddr;
        in_addr m_DataAddr;

        int loggingUser;

    public:

        int m_SessionIndex;
        int loggedUser;


        // inline functions
        CFtpd*    getCFtpd() {return m_Ftpd;}
        void      setMainThread(HANDLE h) {m_MainThread = h;}
        HANDLE    getMainThread() {return m_MainThread;}

//        char*     getLogFile() {return m_LogFile;}
        
        void      setMainSock(SOCKET s) {if(s!=INVALID_SOCKET) m_MainSock=s; else m_MainSock=0;}
        SOCKET    getMainSock() {return m_MainSock;}
        // void      closeMainSock() {if (m_MainSock!=0) closesocket(m_MainSock); m_MainSock=0;}

        void      setPasvSock(SOCKET s) {if(s!=INVALID_SOCKET) m_PasvSock=s; else m_PasvSock=0;}
        SOCKET    getPasvSock() {return m_PasvSock;}
        void      closePasvSock() {if (m_PasvSock!=0) closesocket(m_PasvSock); m_PasvSock=0;}

        void      setPasvSock2(SOCKET s) {if(s!=INVALID_SOCKET) m_PasvSock2=s; else m_PasvSock2=0;}
        SOCKET    getPasvSock2() {return m_PasvSock2;}
        void      closePasvSock2() {if (m_PasvSock2!=0) closesocket(m_PasvSock2); m_PasvSock2=0;}

        void      setDataSock(SOCKET s) {if(s!=INVALID_SOCKET) m_DataSock=s; else m_DataSock=0;}
        SOCKET    getDataSock() {return m_DataSock;}
        // void      closeDataSock() {if (m_DataSock!=0) closesocket(m_DataSock); m_DataSock=0;}

        void      setClientAddr(in_addr ia) {m_ClientAddr = ia;}
        in_addr   getClientAddr() {return m_ClientAddr;}

        // void      terminateMainThread() {killThread(m_MainThread);}
        // void      terminatePasvThread() {killThread(m_PasvThread); m_PasvThreadRunning = false;}

        void      setPasvPort(int p) {m_PasvPort = p;}
        int       getPasvPort() {return m_PasvPort;}
        
        
        int       changeDirectory(char *newDir);
        void      setCurrentDir(char *newDir) {strcpy(m_vCurrentDir, newDir);};

        bool      close();
        bool      hasExpired();
        bool      dialog();
        bool      parse();

    private:




        bool updateListMsg(char *msg);
        bool sendToClient(char* msg);


        // FTP commands
        int abor();
        int cdup();
        int cwd();
        
        int list(bool nlst);
        int pass();
        int pasv();
        int port();
        int pwd();
        int quit();
        
        int rest();
        int retr();
        int size();
        int stor();
        int syst();
        int type();
        int user();

        int rnfr();
        int rnto();

        int mkd();
        int rmd();

        int dele();
        
        
        
        
        
        
        
};



#endif
