// Revision : $Id: main.cpp,v 1.1.1.1 2004/04/05 21:26:55 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
//   This is the main file, it contains the WinMain() function
// This file contains the functions to deal with the UI.
// 
#include <windows.h>

#include "resource.h"

#include "const.h"
#include "CUser.h"
#include "main.h"
#include "CFtpd.h"
#include "CSession.h"
#include "mylib.h"
#include "URLCtrl.h"




HWND               mainHwnd;               // main HWND
HINSTANCE          hInst;                  // main HINSTANCE
CFtpd              *ftp;                   // main CFtpd class

NOTIFYICONDATA     tIcon;

// User Options  [used for user properties panel]

// char     login[BUFFER_SIZE];

/*
char        passwd[BUFFER_SIZE];
int         simConn;
int         timeOut;
*/

CUser*             currUser;
/*
int                nDirs;
int                nPerms;
virtualDir* dirs;
permission* perms;
*/

int         currUserID;

virtualDir  currDir;
permission  currPerm;


// char     rootDir[BUFFER_SIZE];


char        mainDir[BUFFER_SIZE];
char        configFile[BUFFER_SIZE];


bool        usersUpdated;
bool        writingLog;



// Config
int         port;
int         max_connections;
int         auto_run;
bool        usePasvUrl;
char        passive_url[BUFFER_SIZE];
int         pasv1;
int         pasv2;





BOOL MainDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HICON hIcon;
    int ret;
    HMENU menu;

    POINT pt;

    switch (uMsg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED) {
            Shell_NotifyIcon(NIM_ADD, &tIcon);
            ShowWindow(mainHwnd, SW_HIDE);
            
            // why should that be done ? I don't know
            updateTipText();
        }
        break;
    case WM_TRAY_MSG:
        if (wParam == tIcon.uID) {
            switch (lParam) 
            {
            case WM_LBUTTONUP:
                Shell_NotifyIcon(NIM_DELETE, &tIcon);
                ShowWindow(mainHwnd, SW_RESTORE);
                break;
            case WM_RBUTTONUP:
                menu = CreatePopupMenu();
                AppendMenu(menu,0,STM_SHOW,"Show");
                AppendMenu(menu,0,STM_EXIT,"Exit");
                SetMenuDefaultItem(menu, 0, TRUE);
                GetCursorPos(&pt);
                TrackPopupMenu(menu,0,pt.x,pt.y,0,mainHwnd,NULL);
                DestroyMenu(menu);
                break;
            }
        }
        break;
    case WM_INITDIALOG:
        mainHwnd = hWnd;
        writingLog = false;

        hIcon = LoadIcon (hInst, MAKEINTRESOURCE(ICO_RUN));
        SendMessage (GetDlgItem(mainHwnd, IDSTART), BM_SETIMAGE, WPARAM (IMAGE_ICON), LPARAM (hIcon));
        hIcon = LoadIcon (hInst, MAKEINTRESOURCE(ICO_STOP));
        SendMessage (GetDlgItem(mainHwnd, IDSTOP), BM_SETIMAGE, WPARAM (IMAGE_ICON), LPARAM (hIcon));

        hIcon = LoadIcon (hInst, MAKEINTRESOURCE(ICO_MAIN));
        
        ret = SendMessage (mainHwnd, WM_SETICON, WPARAM (ICON_SMALL), (LPARAM) hIcon);
        ret = SendMessage (mainHwnd, WM_SETICON, WPARAM (ICON_BIG), (LPARAM) hIcon);

        char caption[BUFFER_SIZE];
        wsprintf(caption, "%s %s", SOFTWARE, VERSION);
        ret = SendMessage(mainHwnd, WM_SETTEXT, 0, (LPARAM) caption);

        GetCurrentDirectory(BUFFER_SIZE, mainDir);
        lstrcpy(configFile, mainDir);
        lstrcat(configFile, "\\ftpd.ini");

        ftp = new CFtpd();
        ftp->setHwnd(hWnd);
        auto_run = 0;
        loadConfig();

        if (auto_run>0) startFtpd();
        updateUI();
        StaticToURLControl(hWnd, IDC_URL, "http://smallftpd.free.fr/", 0);

        // SetTimer(mainHwnd, 10, TIMER_DELAY, NULL);


        // Initialize tIcon structure for later

        tIcon.cbSize = sizeof(NOTIFYICONDATA);
        tIcon.hWnd = mainHwnd;
        tIcon.uID = ID_TRAY_ICON;
        tIcon.hIcon = hIcon;
        tIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        tIcon.uCallbackMessage = WM_TRAY_MSG;
        lstrcpy(tIcon.szTip, "This is a TEST");

        break;
    case WM_COMMAND:
        return MainDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        stopFtpd();
        saveConfig();
        // EndDialog(hWnd, 0);
        delete ftp;
        Shell_NotifyIcon(NIM_DELETE, &tIcon);
        DestroyWindow(hWnd);
        return TRUE;
    case WM_DESTROY:
        PostQuitMessage(0);
        return TRUE;
    }
    return FALSE;

}

bool MainDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    usersUpdated = false;

    switch (wCommand)
    {
    case IDSTART:
        if (!ftp->isRunning()) startFtpd();
        updateUI();
        break;
    case IDSTOP:
        if (ftp->isRunning()) stopFtpd();
        updateUI();
        break;
    case ME_START:
        startFtpd();
        updateUI();
        break;
    case ME_STOP:
        stopFtpd();
        updateUI();
        break;
    case STM_SHOW:
        ShowWindow(mainHwnd, SW_RESTORE);
        Shell_NotifyIcon(NIM_DELETE, &tIcon);
        break;
    case STM_EXIT:
    case ME_QUIT:
        SendMessage(hWnd, WM_CLOSE, 0,0);
        break;
    case ME_SETTINGS:
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SETTINGS), hWnd, (DLGPROC)SettingsDialogProc, 0);
        break;
    case ME_ADVANCED:
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ADVANCED), hWnd, (DLGPROC)AdvancedDialogProc, 0);
        break;
    case ME_USERS:
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_USERS), hWnd, (DLGPROC)UsersDialogProc, 0);
        break;
    case ME_LOADCONFIG:
        loadConfig();
        break;
    case ME_SAVECONFIG:
        saveConfig();
        break;
    }

    if (usersUpdated) {
        // ftp->addUser("", login, passwd, simConn, timeOut);
        updateUsersComboBox(hWnd);
        usersUpdated = false;
    }
    return TRUE;
}


int WINAPI WinMain(HINSTANCE hInst_, HINSTANCE, LPSTR, int) 
{
    hInst = hInst_;

    mainHwnd = CreateDialog(hInst_, MAKEINTRESOURCE (IDD_MAINDIALOG), 0, (DLGPROC)MainDialogProc);
    ShowWindow(mainHwnd, SW_SHOW);

    MSG  msg;
    int status;
    while ((status = GetMessage (& msg, 0, 0, 0)) != 0)
    {
        if (status == -1)
            return -1;
        if (!IsDialogMessage (mainHwnd, &msg))
        {
            TranslateMessage ( &msg );
            DispatchMessage ( &msg );
        }
    }


    return msg.wParam;

}



BOOL SettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDlgItemInt(hWnd, IDC_PORT, port, false);
        SetDlgItemInt(hWnd, IDC_MAXCONN, max_connections, false);
        if (auto_run==1) {
            SendMessage( GetDlgItem(hWnd, IDC_AUTORUN), BM_SETCHECK, true, 0);
        } else {
            SendMessage( GetDlgItem(hWnd, IDC_AUTORUN), BM_SETCHECK, false, 0);
        }
        break;
    case WM_COMMAND:
        return SettingsDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;

}

bool SettingsDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    BOOL success;
    switch (wCommand)
    {
    case IDOK:
        port = GetDlgItemInt(hWnd, IDC_PORT, &success, false);
        max_connections = GetDlgItemInt(hWnd, IDC_MAXCONN, &success, false);
        if (SendMessage( GetDlgItem(hWnd, IDC_AUTORUN), BM_GETCHECK, false, 0) == BST_CHECKED) {
            auto_run = 1;
        } else {
            auto_run = 0;
        }
        // GetDlgItemText(hWnd, IDC_ROOTDIR, rootDir, BUFFER_SIZE);
        EndDialog(hWnd, 0);
        break;
    case IDCANCEL:
        EndDialog(hWnd, 0);
        break;
    }
    return TRUE;
}



BOOL AdvancedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_PASV_URL, passive_url);
        SetDlgItemInt(hWnd, IDC_PASV_1, pasv1, false);
        SetDlgItemInt(hWnd, IDC_PASV_2, pasv2, false);
        SendMessage( GetDlgItem(hWnd, IDC_USEPASVURL), BM_SETCHECK, usePasvUrl, 0);
        // Update GUI here
        if (SendMessage( GetDlgItem(hWnd, IDC_USEPASVURL), BM_GETCHECK, false, 0) == BST_CHECKED)
            EnableWindow(GetDlgItem(hWnd, IDC_PASV_URL), false);
        else
            EnableWindow(GetDlgItem(hWnd, IDC_PASV_URL), true);
        break;
    case WM_COMMAND:
        return AdvancedDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;

}

bool AdvancedDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    BOOL success;



    /*
    if (SendMessage( GetDlgItem(hWnd, IDC_USEPASVURL), BM_GETCHECK, false, 0) == BST_CHECKED)
        usePasvUrl = true;
    else
        usePasvUrl = false;
    */
    switch (wCommand)
    {
    case IDOK:
        pasv1 = GetDlgItemInt(hWnd, IDC_PASV_1, &success, false);
        pasv2 = GetDlgItemInt(hWnd, IDC_PASV_2, &success, false);
        GetDlgItemText(hWnd, IDC_PASV_URL, passive_url, BUFFER_SIZE);
        if (SendMessage( GetDlgItem(hWnd, IDC_USEPASVURL), BM_GETCHECK, false, 0) == BST_CHECKED)
            usePasvUrl = true;
        else
            usePasvUrl = false;
        // GetDlgItemText(hWnd, IDC_ROOTDIR, rootDir, BUFFER_SIZE);
        EndDialog(hWnd, 0);
        break;
    case IDCANCEL:
        EndDialog(hWnd, 0);
        break;
    }
    // Update GUI here
    if (SendMessage( GetDlgItem(hWnd, IDC_USEPASVURL), BM_GETCHECK, false, 0) == BST_CHECKED)
        EnableWindow(GetDlgItem(hWnd, IDC_PASV_URL), false);
    else
        EnableWindow(GetDlgItem(hWnd, IDC_PASV_URL), true);
    
    return TRUE;
}



BOOL UsersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        updateUsersComboBox(hWnd);
        break;
    case WM_COMMAND:
        return UsersDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;

}

bool UsersDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    HWND h;
    

    switch (wCommand)
    {
    case IDOK:
        // GetDlgItemText(hWnd, IDC_ROOTDIR, rootDir, BUFFER_SIZE);
        EndDialog(hWnd, 0);
        break;
    case IDCANCEL:
        EndDialog(hWnd, 0);
        break;
    case IDC_ADDUSER:
        /*
        login[0]=0;
        passwd[0]=0;
        simConn = DEFAULT_SIM_CONNECTIONS;
        timeOut = DEFAULT_INACTIVITY_TIMEOUT;
        */
        // nDirs = 0;
        // nPerms = 0;
        // rootDir[0]=0;

        // currUserID = ftp->addUser("", "", "", DEFAULT_SIM_CONNECTIONS, DEFAULT_INACTIVITY_TIMEOUT);
        currUserID = -1;
        currUser = new CUser();
        // ftp->getUserProperties(currUserID, login, passwd, &simConn, &timeOut);
        // currUser->getVirtualDirs(&dirs, &nDirs);
        // currUser->getPermissions(&perms, &nPerms);
        
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_USERDIALOG), hWnd, (DLGPROC)UserDialogProc, 0);
        updateUsersComboBox(hWnd);
        delete currUser;
        break;
    case IDC_EDITUSER:
        char login[BUFFER_SIZE];
        h = GetDlgItem(hWnd, IDC_USERS);
        currUserID = SendMessage(h, CB_GETCURSEL, 0, 0);
        if (SendMessage(h, CB_GETLBTEXT, currUserID, (LPARAM)login)>=0) {
            currUserID = ftp->findUser(login);
            currUser = new CUser();
            currUser->copyFrom(ftp->getUser(currUserID));

            /*
            ftp->getUserProperties(currUserID, login, passwd, &simConn, &timeOut);
            ftp->getUser(currUserID)->getVirtualDirs(&dirs, &nDirs);
            ftp->getUser(currUserID)->getPermissions(&perms, &nPerms);
            */
            DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_USERDIALOG), hWnd, (DLGPROC)UserDialogProc, 0);
            delete currUser;
        }
        updateUsersComboBox(hWnd);
        break;
    case IDC_DELETEUSER:
        h = GetDlgItem(hWnd, IDC_USERS);
        currUserID = SendMessage(h, CB_GETCURSEL, 0, 0);
        if (SendMessage(h, CB_GETLBTEXT, currUserID, (LPARAM)login)>=0) {
            currUserID = ftp->findUser(login);
            ftp->deleteUser(currUserID);
        }
        updateUsersComboBox(hWnd);
        break;
    }
    return TRUE;
}



BOOL UserDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND h;
    char buf[BUFFER_SIZE];

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_USER, currUser->login);
        SetDlgItemText(hWnd, IDC_PASSWD, currUser->passwd);
        wsprintf(buf, "%d", currUser->simConn);
        SetDlgItemText(hWnd, IDC_SIMCONN, buf);
        wsprintf(buf, "%d", currUser->timeOut);
        SetDlgItemText(hWnd, IDC_TIMEOUT, buf);

        updateUserListBoxes(hWnd);

        h = GetDlgItem(hWnd, IDC_USER);
        /*
        if (ftp->findUser(login)>=0 && strcmp(login, "") != 0) {
            EnableWindow(h,false);
            h = GetDlgItem(hWnd, IDC_PASSWD);
        }
        */
        SetFocus(h);
        break;
    case WM_COMMAND:
        return UserDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:


        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;

}

bool UserDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    int success;
    int sel;

    switch (wCommand)
    {
    case IDOK:
        char login[BUFFER_SIZE];
        char passwd[BUFFER_SIZE];
        int simConn;
        int timeOut;
        GetDlgItemText(hWnd, IDC_USER, login, BUFFER_SIZE);
        GetDlgItemText(hWnd, IDC_PASSWD, passwd, BUFFER_SIZE);
        simConn = GetDlgItemInt(hWnd, IDC_SIMCONN, &success, false);
        timeOut = GetDlgItemInt(hWnd, IDC_TIMEOUT, &success, false);

        currUser->set(login, passwd, simConn, timeOut);
        // GetDlgItemText(hWnd, IDC_ROOTDIR, rootDir, BUFFER_SIZE);
        if (currUserID < 0)
            currUserID = ftp->addUser(login, passwd, simConn, timeOut);
        if (currUserID >= 0)
            ftp->getUser(currUserID)->copyFrom(currUser);

        usersUpdated = true;
        EndDialog(hWnd, 0);
        break;
    case IDC_ADDDIR:
        lstrcpy(currDir.virt, "/");
        lstrcpy(currDir.real, "");
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGDIR), hWnd, (DLGPROC)DirsDialogProc, 0);
        updateUserListBoxes(hWnd);
        break;
    case IDC_ADDPERM:
        lstrcpy(currPerm.path, "/");
        lstrcpy(currPerm.type, "LR");
        DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOGPERM), hWnd, (DLGPROC)PermsDialogProc, 0);
        updateUserListBoxes(hWnd);
        break;
    case IDC_DELDIR:
        sel = SendMessage(GetDlgItem(hWnd, IDC_LISTDIRS), LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR) {
            currUser->delVirtualDir(sel);
            updateUserListBoxes(hWnd);
        }
        break;
    case IDC_DELPERM:
        sel = SendMessage(GetDlgItem(hWnd, IDC_LISTPERMS), LB_GETCURSEL, 0, 0);
        if (sel != LB_ERR) {
            currUser->delPermission(sel);
            updateUserListBoxes(hWnd);
        }
        break;
    case IDCANCEL:
        /*
        if (strcmp, login, "")
            ftp->deleteUser(currUserID);
        */
        EndDialog(hWnd, 0);
        break;
    }
    return TRUE;
}


BOOL DirsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND h;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_DIR, currDir.real);
        SetDlgItemText(hWnd, IDC_PATH, currDir.virt);

        h = GetDlgItem(hWnd, IDC_PATH);
        SetFocus(h);
        break;
    case WM_COMMAND:
        return DirsDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;

}

bool DirsDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    char buf1[BUFFER_SIZE];
    char buf2[BUFFER_SIZE];

    switch (wCommand)
    {
    case IDOK:
        GetDlgItemText(hWnd, IDC_DIR, buf1, BUFFER_SIZE);
        GetDlgItemText(hWnd, IDC_PATH, buf2, BUFFER_SIZE);

        currUser->addVirtualDir(buf1, buf2);

        EndDialog(hWnd, 0);
        break;
    case IDCANCEL:
        EndDialog(hWnd, 0);
        break;
    }
    return TRUE;
}


BOOL PermsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND h;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hWnd, IDC_PATH, currPerm.path);
        SetDlgItemText(hWnd, IDC_TYP, currPerm.type);

        h = GetDlgItem(hWnd, IDC_PATH);
        SetFocus(h);
        break;
    case WM_COMMAND:
        return PermsDialog_OnCommand(hWnd, LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
    case WM_CLOSE:
        EndDialog(hWnd, 0);
        return TRUE;
    }
    return FALSE;

}

bool PermsDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl)
{
    char buf1[BUFFER_SIZE];
    char buf2[BUFFER_SIZE];
    int t;

    switch (wCommand)
    {
    case IDOK:
        t = GetDlgItemText(hWnd, IDC_PATH, buf1, BUFFER_SIZE);
        t = GetDlgItemText(hWnd, IDC_TYP, buf2, BUFFER_SIZE);

        buf2[5]=0;
        currUser->addPermission(buf1, buf2);

        EndDialog(hWnd, 0);
        break;
    case IDCANCEL:
        EndDialog(hWnd, 0);
        break;
    }
    return TRUE;
}


//
//
//
void updateUserListBoxes(HWND h_) {
    HWND h;
    char buf[BUFFER_SIZE];
    int i;
    int sel;

    h = GetDlgItem(h_, IDC_LISTDIRS);
    sel = SendMessage(h, LB_GETCURSEL, 0, 0);
    SendMessage(h, LB_RESETCONTENT, 0, 0);
    for (i=0; i<currUser->m_nVDs; i++) {
        wsprintf(buf, "%s  -->  %s", currUser->m_VDs[i].virt, currUser->m_VDs[i].real);
        SendMessage(h, LB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessage(h, LB_SETCURSEL, sel, 0);

    h = GetDlgItem(h_, IDC_LISTPERMS);
    sel = SendMessage(h, LB_GETCURSEL, 0, 0);
    SendMessage(h, LB_RESETCONTENT, 0, 0);
    for (i=0; i<currUser->m_nPerms; i++) {
        wsprintf(buf, "%s  :  %s", currUser->m_Perms[i].path, currUser->m_Perms[i].type);
        SendMessage(h, LB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessage(h, LB_SETCURSEL, sel, 0);
}


//
// Update Users ComboBox
//
void updateUsersComboBox(HWND h_) {
    int i;
    HWND    h;
    bool enabled = true;

    h = GetDlgItem(h_, IDC_USERS);
    SendMessage(h, CB_RESETCONTENT, 0, 0);

    for (i=0;i<ftp->getNbUsers();i++) {
        SendMessage(h, CB_ADDSTRING, 0, (LPARAM)ftp->getUser(i)->login);
    }
    if (i==0) {
        SendMessage(h, CB_ADDSTRING, 0, (LPARAM)"<< 0  users >>");
        enabled = false;
    }
    SendMessage(h, CB_SETCURSEL, 0, 0);

    i = SendMessage(h, CB_GETCURSEL, 0, 0);
    if (i<0 && enabled) enabled = false;

    h = GetDlgItem(h_, IDC_EDITUSER);
    EnableWindow(h, enabled);
    h = GetDlgItem(h_, IDC_DELETEUSER);
    EnableWindow(h, enabled);
}


//
// Defines the status bar text
//
void setInfoText(LPCSTR s) {
    
    HWND h = GetDlgItem(mainHwnd, IDC_INFO);
    SendMessage(h, WM_SETTEXT, 0, (LPARAM)s);
}



//
// Load parameters from .ini file
//
bool loadConfig() {

    // Parameters to be read from .ini file
    char section[BUFFER_SIZE];
    char user_name[BUFFER_SIZE];
    char user_passwd[BUFFER_SIZE];
    char buf[BUFFER_SIZE];
    char buf1[BUFFER_SIZE];
    char buf2[BUFFER_SIZE];
    // char user_root[BUFFER_SIZE];
    int user_simConn;
    int user_timeOut;


    int left = GetPrivateProfileInt("application", "left", 200, configFile);
    int top = GetPrivateProfileInt("application", "top", 200, configFile);
    
    SetWindowPos(mainHwnd, HWND_TOP, left, top, 0, 0, SWP_NOSIZE);
    

    port = GetPrivateProfileInt("server", "port", DEFAULT_PORT, configFile);
    max_connections = GetPrivateProfileInt("server", "max_connections", DEFAULT_MAXCONN, configFile);
    auto_run = GetPrivateProfileInt("server", "auto_run", 0, configFile);

    GetPrivateProfileString("passive_mode", "pasv_url", DEFAULT_PASV_URL, passive_url, BUFFER_SIZE, configFile);
    GetPrivateProfileString("passive_mode", "use_pasv_url", "", buf, BUFFER_SIZE, configFile);
    if (strcmp(buf, "1") == 0)
        usePasvUrl = true;
    else
        usePasvUrl = false;
    pasv1 = GetPrivateProfileInt("passive_mode", "pasv_min_port", DEFAULT_MIN_PASV_PORT, configFile);
    pasv2 = GetPrivateProfileInt("passive_mode", "pasv_max_port", DEFAULT_MAX_PASV_PORT, configFile);

    
    DWORD res;
    int i, j, userIndex;
    i=0;
    
    while(1) {
        wsprintf(section, "user_%i",i);
        res = GetPrivateProfileString(section, "user_Login", "", user_name, BUFFER_SIZE, configFile);
        if (res<1) break;
        res = GetPrivateProfileString(section, "user_Password", "", user_passwd, BUFFER_SIZE, configFile);
        if (res<1) break;
        user_simConn = GetPrivateProfileInt(section, "user_Simultaneous_Connections", DEFAULT_SIM_CONNECTIONS, configFile);
        user_timeOut = GetPrivateProfileInt(section, "user_Inactivity_Timeout", DEFAULT_INACTIVITY_TIMEOUT, configFile);


        // create user
        userIndex = ftp->addUser(user_name, user_passwd, user_simConn, user_timeOut);

        j=0;
        while (1) {
            wsprintf(buf, "permission_Path_%d", j);
            res = GetPrivateProfileString(section, buf, "", buf1, BUFFER_SIZE, configFile);
            if (res<1) break;
            wsprintf(buf, "permission_Type_%d", j);
            res = GetPrivateProfileString(section, buf, "", buf2, BUFFER_SIZE, configFile);
            if (res<1) break;

            ftp->getUser(userIndex)->addPermission(buf1, buf2);
            j++;
        }

        j=0;
        while (1) {
            wsprintf(buf, "directory_Virtual_%d", j);
            res = GetPrivateProfileString(section, buf, "", buf1, BUFFER_SIZE, configFile);
            if (res<1) break;
            wsprintf(buf, "directory_Physical_%d", j);
            res = GetPrivateProfileString(section, buf, "", buf2, BUFFER_SIZE, configFile);
            if (res<1) break;

            ftp->getUser(userIndex)->addVirtualDir(buf2, buf1);
            j++;
        }

        // res = GetPrivateProfileString(section, "user_root", "", user_root, BUFFER_SIZE, configFile);
        // if (res<1) break;
        

        i++;
    }
    
    
    log(LOG_DEBUG, "DEBUG - configuration loaded - %i user(s) found.\r\n", i);
    setInfoText(INFO_CONFIG_LOADED);
    

    // Fill edit boxes
    /*SetDlgItemInt(mainHwnd, IDC_PORT, port, false);
    SetDlgItemInt(mainHwnd, IDC_MAXCONN, max_connections, false);
    
    SetDlgItemText(mainHwnd, IDC_PASV_URL, pasv_url );
    SetDlgItemInt(mainHwnd, IDC_PASV_1, pasv_min_port, false);
    SetDlgItemInt(mainHwnd, IDC_PASV_2, pasv_max_port, false);*/

    return true;

}

//
// Write parameters into .ini file
//
bool saveConfig() {
    char  tmp[BUFFER_SIZE];

    char  lLogin[BUFFER_SIZE];
    char  lPasswd[BUFFER_SIZE];
    // char  rootDir[BUFFER_SIZE];
    int   lSimConn;
    int   lTimeOut;
    char  section[BUFFER_SIZE];

    int j;

//    HANDLE f;
//    f = CreateFile(configFile, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
//    CloseHandle(f);

    // General Application Settings
    RECT rect;
    GetWindowRect(mainHwnd, &rect);
    wsprintf(tmp, "%d", rect.left );
    WritePrivateProfileString("application", "left", tmp, configFile);
    wsprintf(tmp, "%d", rect.top);
    WritePrivateProfileString("application", "top", tmp, configFile);
    
    // General Server Settings
    wsprintf(tmp, "%d", port);
    WritePrivateProfileString("server", "port", tmp, configFile);
    wsprintf(tmp, "%d", max_connections);
    WritePrivateProfileString("server", "max_connections", tmp, configFile);
    wsprintf(tmp, "%d", auto_run);
    WritePrivateProfileString("server", "auto_run", tmp, configFile);
    
    wsprintf(tmp, "%d", pasv1);
    WritePrivateProfileString("passive_mode", "pasv_min_port", tmp, configFile);
    wsprintf(tmp, "%d", pasv2);
    WritePrivateProfileString("passive_mode", "pasv_max_port", tmp, configFile);
    if (usePasvUrl) {
        WritePrivateProfileString("passive_mode", "use_pasv_url", "1", configFile);
    } else {
        WritePrivateProfileString("passive_mode", "use_pasv_url", "0", configFile);
    }
    WritePrivateProfileString("passive_mode", "pasv_url", passive_url, configFile);

    for (int i=0;i<ftp->getNbUsers();i++) {
        wsprintf(section, "user_%d", i);
        if (ftp->getUserProperties(i, lLogin, lPasswd, &lSimConn, &lTimeOut)) {
            WritePrivateProfileString(section, "user_Login", lLogin, configFile);
            WritePrivateProfileString(section, "user_Password", lPasswd, configFile);
            wsprintf(tmp, "%d", lSimConn);
            WritePrivateProfileString(section, "user_Simultaneous_Connections", tmp, configFile);

            wsprintf(tmp, "%d", lTimeOut);
            WritePrivateProfileString(section, "user_Inactivity_Timeout", tmp, configFile);
        }
        else break;

        for (j=0; j<ftp->getUser(i)->m_nPerms; j++) {
            wsprintf(tmp, "permission_Path_%d", j);
            WritePrivateProfileString(section, tmp, ftp->getUser(i)->m_Perms[j].path, configFile);
            wsprintf(tmp, "permission_Type_%d", j);
            WritePrivateProfileString(section, tmp, ftp->getUser(i)->m_Perms[j].type, configFile);
        }

        for (j=0; j<ftp->getUser(i)->m_nVDs; j++) {
            wsprintf(tmp, "directory_Virtual_%d", j);
            WritePrivateProfileString(section, tmp, ftp->getUser(i)->m_VDs[j].virt, configFile);
            wsprintf(tmp, "directory_Physical_%d", j);
            WritePrivateProfileString(section, tmp, ftp->getUser(i)->m_VDs[j].real, configFile);
        }

    }

    setInfoText(INFO_CONFIG_SAVED);
    return true;
}



//
// Get parameters & Start the server
//
bool startFtpd() {

    EnableWindow(GetDlgItem(mainHwnd, IDSTART), false);
    EnableMenuItem(GetMenu(mainHwnd), ME_START, MF_GRAYED);
    EnableMenuItem(GetMenu(mainHwnd), ME_STOP, MF_GRAYED);

    if (port<1 || pasv1<1 || pasv2<1 || pasv2<pasv1) return false;
  
    // General settings
    ftp->setListeningPort(port);
    ftp->setMaxConnections(max_connections);

    // Advanced settings
    ftp->setUsePasvUrl(!usePasvUrl);
    ftp->setPasvPortRange(pasv1, pasv2);
    ftp->setPasvUrl(passive_url);


    ftp->start();

    return true;
}


//
// Stop the server
//
bool stopFtpd() {

    EnableWindow(GetDlgItem(mainHwnd, IDSTART), false);
    EnableMenuItem(GetMenu(mainHwnd), ME_START, MF_GRAYED);
    EnableMenuItem(GetMenu(mainHwnd), ME_STOP, MF_GRAYED);
    
    ftp->stop();
    return true;
}


// 
// Update the main button icon   (running - stopped)
//
bool updateUI() {

    if (ftp->isRunning()) {
        EnableWindow(GetDlgItem(mainHwnd, IDSTART), false);
        EnableWindow(GetDlgItem(mainHwnd, IDSTOP), true);

        EnableWindow(GetDlgItem(mainHwnd, IDC_USERS), false);
        EnableWindow(GetDlgItem(mainHwnd, IDC_ADDUSER), false);
        EnableWindow(GetDlgItem(mainHwnd, IDC_EDITUSER), false);
        EnableWindow(GetDlgItem(mainHwnd, IDC_DELETEUSER), false);
        EnableWindow(GetDlgItem(mainHwnd, IDC_PASV_URL), false);
        EnableWindow(GetDlgItem(mainHwnd, IDC_PASV_1), false);
        EnableWindow(GetDlgItem(mainHwnd, IDC_PASV_2), false);
        
        // Menu items
        EnableMenuItem(GetMenu(mainHwnd), ME_START, MF_GRAYED);
        EnableMenuItem(GetMenu(mainHwnd), ME_STOP, MF_ENABLED);
        EnableMenuItem(GetMenu(mainHwnd), ME_SETTINGS, MF_GRAYED);
        EnableMenuItem(GetMenu(mainHwnd), ME_ADVANCED, MF_GRAYED);
    }
    else {
        EnableWindow(GetDlgItem(mainHwnd, IDSTART), true);
        EnableWindow(GetDlgItem(mainHwnd, IDSTOP), false);

        EnableWindow(GetDlgItem(mainHwnd, IDC_USERS), true);
        EnableWindow(GetDlgItem(mainHwnd, IDC_ADDUSER), true);
        EnableWindow(GetDlgItem(mainHwnd, IDC_EDITUSER), true);
        EnableWindow(GetDlgItem(mainHwnd, IDC_DELETEUSER), true);
        EnableWindow(GetDlgItem(mainHwnd, IDC_PASV_URL), true);
        EnableWindow(GetDlgItem(mainHwnd, IDC_PASV_1), true);
        EnableWindow(GetDlgItem(mainHwnd, IDC_PASV_2), true);
        EnableWindow(GetDlgItem(mainHwnd, ME_START), true);

        // Menu items
        EnableMenuItem(GetMenu(mainHwnd), ME_START, MF_ENABLED);
        EnableMenuItem(GetMenu(mainHwnd), ME_STOP, MF_GRAYED);
        EnableMenuItem(GetMenu(mainHwnd), ME_SETTINGS, MF_ENABLED);
        EnableMenuItem(GetMenu(mainHwnd), ME_ADVANCED, MF_ENABLED);
    }
    
    return true;


}


//
// Log text to the specified file
//
bool log(short type, char* lpFormat, ...) {
    
    char *file;
    if (type == LOG_DEBUG) {
        file = "debug.log";
        
        // This is for release version = no debugging
        // return false;
    } else if (type == LOG_TRANSFER) {
        file = "transfers.log";
    } else {
        return false;
    }
    
    HANDLE f;

    char szBuf[BUFFER_SIZE];
    va_list Marker;
    SYSTEMTIME systime;
    char timestr[8];
    char datestr[10];
    char *toWrite;
    unsigned long ul;

    CHAR ret[3];
    ret[0]=13;
    ret[1]=10;
    ret[2]=0;

    // Get Date & Time
    GetLocalTime(&systime);
    GetDateFormat(0, 0, &systime, "dd/MM/yyyy", datestr, 10);
    GetTimeFormat(0, 0, &systime, "HH:mm:ss", timestr, 8);
    datestr[10]=0;
    timestr[8]=0;

    // Write text to string
    va_start(Marker, lpFormat);
    wvsprintf(szBuf, lpFormat, Marker);
    va_end(Marker);

    // char *ftmp;
    // ftmp = (char*)malloc(sizeof(mainDir)+lstrlen(file)+3);
    char ftmp[BUFFER_SIZE];
    wsprintf(ftmp, "%s\\%s", mainDir, file);

    toWrite = (char*)malloc(lstrlen(datestr)+lstrlen(timestr)+lstrlen(szBuf)+3);
    wsprintf(toWrite, "%s %s %s", datestr, timestr, szBuf);

    // TODO : allow an escape
    while (writingLog) {
        Sleep(250);
    }
    writingLog = true;

    // Open file pointer
    f = CreateFile(ftmp, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0);

    if (f==INVALID_HANDLE_VALUE) {
        writingLog = false;

        // TODO : LOG the error
        DWORD t = GetLastError();
        return false;
    }

    // Set the file pointer to the end
    SetFilePointer(f, 0, NULL, FILE_END);

    // Write data
    WriteFile(f, toWrite, lstrlen(toWrite), &ul, NULL);

    free((void*)toWrite);

    // Close file pointer
    CloseHandle(f);

    writingLog = false;
    
    return true;
}

void updateTipText() {

    if (ftp->isRunning()) {
        int n = ftp->getNbConnections();
        wsprintf(tIcon.szTip, "%d users connected", n);
    } else {
        wsprintf(tIcon.szTip, "smallftpd server is not running", ftp->getNbConnections());
    }
    
    Shell_NotifyIcon(NIM_MODIFY, &tIcon);

}
