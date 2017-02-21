// Revision : $Id: mylib.h,v 1.1.1.1 2004/04/05 21:26:55 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
// This is the header file for mylib.cpp
// 

#ifndef MYLIB_H_
#define MYLIB_H_


int inStr(char*, char*);
int inList(char*, char*);

bool formatSize(long size, char *output);
bool getTransferRate(long nBytes, long milliseconds, char *output);

bool killThread(HANDLE);

long getTimeAge(SYSTEMTIME t1);

#endif

