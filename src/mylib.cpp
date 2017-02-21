// Revision : $Id: mylib.cpp,v 1.2 2004/04/22 13:35:22 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
//   mylib contains various generic functions used in the program
// 

#include <windows.h>

#include "mylib.h"
#include "main.h"
#include "const.h"


// inStr()
// returns 0-based index of first occurrence of 'search' in 'src'.
// returns -1 if not found
int inStr(char* src, char* search) {
  
    search = strstr(src, search);
    if (search == NULL) return -1;
    return search - src;
}
    
// inList()
// returns 0-based index of 'search' in list of chars 'list'
// returns -1 if 'search' was not found
// list must match the following format : 'one\0two\0three\0four\0\0'
int inList(char* list, char* search) {
    int index = 0;
    int count = 0;

    while(strlen(list+index)>0) {
        if (strcmp(list+index, search) == 0) return count;
        
        index += strlen(list+index)+1;
        count++;
    }

    return -1;
}

// formatSize()
// input  : size in bytes
// output : formatted string
bool formatSize(long size, char *output) {

    if (size > 3*1024*1024) {
        long l = size/(1024*1024);
        long d = (long)((double)(100*size))/(1024*1024); // - (double)(100*l));
        double dd = ((double)size)/(1024*1024);
        double ddd = 100 * (dd - (double)l);
        long ll = (long)(ddd);
        
        if (ll<10) {
            wsprintf(output, "%d.0%d MB", l, ll);
        } else {
            wsprintf(output, "%d.%d MB", l, ll);
        }
    } else {
        wsprintf(output, "%d kB", size/1024);
    }
    
    return true;

}

bool getTransferRate(long nBytes, long milliseconds, char *output) {
    
    // if time is too short, don't even evaluate transfer rate
    if (milliseconds < 50) {
        wsprintf(output, "#.# kB/s");
        return false;
    }
    
    double tmp = (double)1000 / (double)1024;
    double value = tmp * (double)nBytes/(double)milliseconds;

    long l = (long)value;
    // double d = ((value) - (double)l * milliseconds);
    double d = 100*(value - (double)l);
    long ll = (long)d;
    
    if (ll<10) {
        wsprintf(output, "%d.0%d kB/s", l, ll);
    } else {
        wsprintf(output, "%d.%d kB/s", l, ll);
    }

    // log(LOG_DEBUG, "getTransferRate(%d, %d) = %s\r\n", nBytes, milliseconds, output);
    
    return true;
    
}



bool killThread_(HANDLE thread) {

    // TODO : add a timer

    // terminateThread
    // wait for thread to be REALLY stopped
    WaitForSingleObject(thread, INFINITE);
    
    return true;

}


//
// 
//
long getTimeAge(SYSTEMTIME st1) {
    FILETIME ft1;
    FILETIME ft2;
    SYSTEMTIME st2;

    GetLocalTime(&st2);
    SystemTimeToFileTime(&st1, &ft1);
    SystemTimeToFileTime(&st2, &ft2);

    return ((ft2.dwLowDateTime - ft1.dwLowDateTime)/10000);

}


