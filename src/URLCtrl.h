//  Revision : $Id: URLCtrl.h,v 1.1.1.1 2004/04/05 21:26:56 smallftpd Exp $
//  Author : J Brown
//
//  Implementation of a standalone URL control,
//  based around a static text label.
//
//  Written by J Brown 30/10/2001.
//  Freeware.
//

#ifndef URLCTRL_INCLUDED
#define URLCTRL_INCLUDED

BOOL StaticToURLControl(HWND hDlg, UINT staticid, TCHAR *szURL, COLORREF crLink);
void CleanupURLControl();

#endif
