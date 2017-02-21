// Revision : $Id: main.h,v 1.1.1.1 2004/04/05 21:26:55 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
// This is the header file for main.cpp
// 

BOOL UserDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL SettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL AdvancedDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL UsersDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL DirsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
BOOL PermsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool MainDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);
bool UserDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);
bool SettingsDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);
bool AdvancedDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);
bool UsersDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);
bool DirsDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);
bool PermsDialog_OnCommand(HWND hWnd, WORD wCommand, WORD wNotify, HWND hControl);


void updateUsersComboBox(HWND h_);
void updateUserListBoxes(HWND h_);


void updateTipText();
void setInfoText(LPCSTR s);

bool loadConfig();
bool saveConfig();

bool startFtpd();
bool stopFtpd();

bool updateUI();


// logging
bool log(short type, char* lpFormat, ...);

