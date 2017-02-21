// Revision : $Id: CFtpd.h,v 1.1.1.1 2004/04/05 21:26:52 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
// This is the header file for CFtpd.cpp
// 

#ifndef CFTPD_H_
#define CFTPD_H_


unsigned long __stdcall ListenThread(void *pVoid);
unsigned long __stdcall ClientThread(void *pVoid);
unsigned long __stdcall PasvThread(void *pVoid);
unsigned long __stdcall TimerThread(void *pVoid);
unsigned long __stdcall StopServerThread(void *pVoid);


class CUser;
class CSession;

class CFtpd {
    private:
        // General member variables
        HWND        m_hWnd;
        HANDLE      m_ListeningThread;
        HANDLE      m_TimerThread;
        HANDLE      m_StopServerThread;
        SOCKET      m_ListeningSock;
        HANDLE      exitEvent;

        // General Parameters
        int         m_ListeningPort;
        int         m_MaxConnections;
        
        // Advanced Options
        bool        m_UsePasvUrl;
        int         m_MinPasvPort;
        int         m_MaxPasvPort;
        char        m_PasvUrl[BUFFER_SIZE];


        // Other member variables
        bool        m_IsRunning;
        int         nbPasvPortsInUse;
        int         m_PasvPortsInUse[MAX_PASV_PORTS];
        int         nbSessions;
        CSession    *m_Sessions[MAX_SESSIONS];
        int         nbUsers;
        CUser       *m_Users[MAX_USERS];

        

    public:

        // Construction / Destruction
        CFtpd();
        ~CFtpd();

        bool        m_ShouldStop;

        // inline functions
        bool        isRunning() {return m_IsRunning;}
        int         getNbUsers() {return nbUsers;}
        int         getNbSessions() {return nbSessions;}
        int         getNbPasvPortsInUse() {return nbPasvPortsInUse;}
        bool        usePasvUrl () {return m_UsePasvUrl;}
        char*       getPasvUrl() {return m_PasvUrl;}

        void        setUsePasvUrl(bool b_) {m_UsePasvUrl = b_;}
        void        setPasvUrl(char* pu) {strcpy(m_PasvUrl, pu);}
        
        // Settings
        void        setHwnd(HWND hWnd_) {m_hWnd = hWnd_;}
        HWND        getHwnd() {return m_hWnd;}
        void        setListeningPort(int port_) {m_ListeningPort = port_;}
        int         getListeningPort() {return m_ListeningPort;}
        void        setMaxConnections(int n_) {m_MaxConnections = n_;}
        int         getMaxConnections() {return m_MaxConnections;}
        bool        setPasvPortRange(int min, int max);

        SOCKET      getListeningSock() {return m_ListeningSock;}

        // Passive Ports functions
        int         getPasvPort();
        bool        freePasvPort(int port);

        // User functions
        int         addUser(char*, char*, int, int);
        bool        deleteUser(int index_);
        int         findUser(char *user_);
        bool        getUserProperties(int index_, char*, char*, int*, int*);
        CUser*      getUser(int index_) {return m_Users[index_];}
        int         getUserNbSessions(int);
        int         getNbConnections();
        int         getUserMaxConnections(int);

        // Daemon Control functions
        bool        start();
        bool        stop();
        bool        stopServer();

        // Session functions
        int         addSession(CSession *s);
        bool        removeSession(int n);

        // gotTimer function
        void        gotTimer();
};

#endif
