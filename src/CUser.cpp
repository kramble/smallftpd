// Revision : $Id: CUser.cpp,v 1.1.1.1 2004/04/05 21:26:54 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
//   A CUser object represents a user connected to the server
// This class manages the real and the virtual folders
// 

#include <windows.h>
#include <process.h>

#include "const.h"
#include "CUser.h"
#include "CFtpd.h"
#include "main.h"
#include "mylib.h"



CUser::CUser() {
    m_nVDs = 0;
    m_nPerms = 0;
    lstrcpy(login, "");
    lstrcpy(passwd, "");
    simConn = DEFAULT_SIM_CONNECTIONS;
    timeOut = DEFAULT_INACTIVITY_TIMEOUT;
}

CUser::~CUser() {
    if (m_nVDs>0) free((void*)m_VDs);
    if (m_nPerms>0) free((void*)m_Perms);
    
}



void CUser::set(char *login_, char *passwd_, int simConn_, int timeOut_) {
    lstrcpy(login, login_);
    lstrcpy(passwd, passwd_);

    simConn = simConn_;
    timeOut = timeOut_;

//  lstrcpy(rootDir, rootDir_);
}


bool CUser::copyFrom(CUser *u_) {
    lstrcpy(login, u_->login);
    lstrcpy(passwd, u_->passwd);
    simConn = u_->simConn;
    timeOut = u_->timeOut;

    int i;

    // delete previous VDS & perms
    for (i=0; i<m_nVDs; i++) delVirtualDir(i);
    for (i=0; i<m_nPerms; i++) delPermission(i);

    for (i=0; i<u_->m_nVDs; i++) {
        addVirtualDir(u_->m_VDs[i].real, u_->m_VDs[i].virt);
    }
    for (i=0; i<u_->m_nPerms; i++) {
        addPermission(u_->m_Perms[i].path, u_->m_Perms[i].type);
    }
    return true;
}


bool CUser::addVirtualDir(char* real_, char* virt_) {
    
    // check virt is unique in m_VDs
    for (int i=0; i<m_nVDs; i++) {
        if (strcmp(m_VDs[i].virt, virt_) == 0) return false;
    }

    // Allocate memory
    if (m_nVDs==0){
        m_VDs = (virtualDir*)malloc(sizeof(virtualDir));
    } else {
        m_VDs = (virtualDir*)realloc(m_VDs, (m_nVDs+1)*sizeof(virtualDir));
    }

    // Copy string buffers
    lstrcpy(m_VDs[m_nVDs].real, real_);
    lstrcpy(m_VDs[m_nVDs].virt, virt_);

    m_nVDs++;
    return true;
}

bool CUser::delVirtualDir(int i_) {
    int i;

    for (i=i_; i<m_nVDs-1; i++) {
        lstrcpy(m_VDs[i].real, m_VDs[i+1].real);
        lstrcpy(m_VDs[i].virt, m_VDs[i+1].virt);
    }
    m_nVDs--;

    return true;
}

bool CUser::delPermission(int i_) {
    int i;

    for (i=i_; i<m_nPerms-1; i++) {
        lstrcpy(m_Perms[i].path, m_Perms[i+1].path);
        lstrcpy(m_Perms[i].type, m_Perms[i+1].type);
    }
    m_nPerms--;

    return true;
}


bool CUser::addPermission(char* path_, char* type_) {
    
    // 
    checkVirtPath(path_);

    // check path is unique in m_Perms
    for (int i=0; i<m_nPerms; i++) {
        if (strcmp(m_Perms[i].path, path_) == 0) return false;
    }

    //
    if (m_nPerms==0){
        m_Perms = (permission*)malloc(sizeof(permission));
    } else {
        m_Perms = (permission*)realloc(m_Perms, (m_nPerms+1)*sizeof(permission));
    }
    lstrcpy(m_Perms[m_nPerms].path, path_);
    lstrcpy(m_Perms[m_nPerms].type, type_);

    m_nPerms++;
    return true;
}

//
// getVirtualSubDirs : returns the list of virtual subDirectories
// 'rSubDirs_' contains the list of real subDirectories (separated by '\0')
// Function returns the number of subDirs
//
int CUser::getVirtualSubDirs(char* vDir_, char* rSubDirs_, char* buf_) {
    int    i;
    int    l = lstrlen(vDir_);
    char*  cmp;
    int    j;
    int    n = 0;
    int    index = 0;

    buf_[0] = '\0';

    for (i=0; i<m_nVDs; i++) {
        // check if this virtualDir has to appear in vDir_
        log(LOG_DEBUG, "DEBUG - virtualDir=%s - CUser::getVirtualSubDirs()\r\n", m_VDs[i].virt);
        if (lstrlen(m_VDs[i].virt) <= l) {
            continue;
        }
        cmp = (char*)malloc(lstrlen(m_VDs[i].virt));
        lstrcpy(cmp, m_VDs[i].virt);

        if (cmp[l] != '/' && lstrcmp("/", vDir_) != 0) {
            continue;
        }

        cmp[l] = '\0';
        log(LOG_DEBUG, "DEBUG - comparing %s and %s - CUser::getVirtualSubDirs()\r\n", vDir_, cmp);

        if(lstrcmp(vDir_, cmp) == 0) {
            
            // YES !
            lstrcpy(cmp, m_VDs[i].virt+l);
            
            // delete 1st '/' if exists
            if (cmp[0] == '/') lstrcpy(cmp, m_VDs[i].virt+1+l);
            
            // delete everything after 2nd '/'
            for (j=0; j<lstrlen(cmp); j++) {
                if (cmp[j] == '/') {
                    cmp[j] = '\0';
                }
            }
            
            // *buf_ = (char*)realloc(buf_, 55+lstrlen(cmp));
            if (inList(rSubDirs_, cmp)<0) {         
                lstrcat(buf_, "drw-rw-rw- 1 user group              0 Jan 01 00:00 ");
                lstrcat(buf_, cmp);
                lstrcat(buf_, "\r\n");

                // increment number of subDirs
                n++;
            }
        }
        free((void*)cmp);
    }
    return n;
}


bool CUser::getRootDir(char* dir) {
    lstrcpy(dir, "");
    return false;
}


//
// setRealPath : sets physical path in "char* real", from virtual ftp path
//               virt must be a complete virtual path. 
//
bool CUser::setRealPath(char* virt, char* real, bool force) {

    char buf[BUFFER_SIZE];
    bool isset = false;
    int max = 0;
    int i;

    if (lstrlen(virt)==0) lstrcpy(virt, "/");
    checkVirtPath(virt);
    // lstrcpy(real, "");

    // Loop through all directories
    for (i=0; i<m_nVDs; i++) {
        if (inStr(virt, m_VDs[i].virt) == 0 && lstrlen(m_VDs[i].virt) > max) {
            max = lstrlen(m_VDs[i].virt);
            lstrcpy(buf, m_VDs[i].real);
            isset = true;
        }
    }

    if (!isset) {
        log(LOG_DEBUG, "ERROR - Could not find real path for virtual %s - CUser::setRealPath\r\n", virt);
        return false;
    }

    if (buf[lstrlen(buf)-1] != '\\') lstrcat(buf, "\\");

    int j;
    int k;
    if (virt[max]=='/') max++;
    for (j=max; j<lstrlen(virt); j++) {
        k = lstrlen(buf);
        buf[k] = virt[j];
        buf[k+1] = '\0';
    }

    for (j=0; j<lstrlen(buf); j++) {
        if (buf[j] == '/') buf[j] = '\\';
    }

    if (buf[lstrlen(buf)-1] != '\\') lstrcat(buf, "\\");

    bool ret = false;

    // force is set to true for MKD & RNTO commands
    if (force) {
        lstrcpy(real, buf);
        ret = true;
    }
    else {
        // check if buf directory really exists :
        char tmp[BUFFER_SIZE];
        GetCurrentDirectory(BUFFER_SIZE, tmp);
        if (SetCurrentDirectory(buf)!=0) {
            lstrcpy(real, buf);
            ret = true; 
        }
        SetCurrentDirectory( tmp ); 
    }

    return ret;
}


void CUser::checkVirtPath(char* virt) {
    char buf[BUFFER_SIZE];
    int j;

    log(LOG_DEBUG, "DEBUG - virt=%s - CUser::checkVirtPath\r\n", virt);

    // prefix a '/' if needed
    if (virt[0] != '/') {
        lstrcpy(buf, virt);
        lstrcpy(virt, "/");
        lstrcat(virt, buf);
    }

    // TODO : find a better way to deal with ..
    // removes '..'
    int i = inStr(virt, "/..");
    while(i>-1) {
        CopyMemory(buf, virt, i);
        buf[i]=0;
        for (j=i;j>=0;j--) {
            if (buf[j]=='/') {
                buf[j]=0;
                break;
            }
        }
        lstrcat(buf, virt+i+3);
        lstrcpy(virt, buf);
        i = inStr(virt, "/..");
    }

    i = inStr(virt, "\\..");
    while(i>-1) {
        CopyMemory(buf, virt, i);
        buf[i]=0;
        for (j=i;j>=0;j--) {
            if (buf[j]=='\\') {
                buf[j]=0;
                break;
            }
        }
        lstrcat(buf, virt+i+3);
        // CopyMemory(buf+i, virt+i+2, lstrlen(virt)-i);
        lstrcpy(virt, buf);
        i = inStr(virt, "\\..");
    }
    
    // remove '\'
    i = inStr(virt, "\\");
    while(i>-1) {
        CopyMemory(buf, virt, i);
        CopyMemory(buf+i, virt+i+1, lstrlen(virt)-i);
        lstrcpy(virt, buf);
        i = inStr(virt, "\\");
    }


    // remove '//'
    i = inStr(virt, "//");
    while(i>-1) {
        CopyMemory(buf, virt, i);
        CopyMemory(buf+i, virt+i+2, lstrlen(virt)-i);
        lstrcpy(virt, buf);
        i = inStr(virt, "//");
    }


    // suffix a '/' if needed
    if (strlen(virt)==0) {
        lstrcat(virt, "/");
    }
    if (virt[lstrlen(virt)-1] == '/' && lstrlen(virt) > 1) {
        virt[lstrlen(virt)-1] = '\0';
    }

    log(LOG_DEBUG, "DEBUG - virt=%s - CUser::checkVirtPath\r\n", virt);
}


//
// hasPermission
// path_ must be absolute
//
bool CUser::hasPermission(char* path_, char* type_) {

    int i;
    int max = 0;
    bool isset = false;
    char perm[5];
    perm[0] = '\0';

    char* pathBuf;
    pathBuf = (char*)malloc(lstrlen(path_));
    lstrcpy(pathBuf, path_);

    //
    checkVirtPath(path_);



    // Loop on all permissions for current user
    for (i=0; i<m_nPerms; i++) {
        if (inStr(pathBuf, m_Perms[i].path) == 0 && lstrlen(m_Perms[i].path) > max) {
            max = lstrlen(m_Perms[i].path);
            lstrcpy(perm, m_Perms[i].type);
            isset = true;
        }
    }

    free ((void*)pathBuf);

    // manage default permissions
    if (!isset) lstrcpy(perm, DEFAULT_PERMISSIONS);

    if (inStr(perm, type_)>-1) return true;

    return false;   

}


void CUser::getVirtualDirs(virtualDir** vd, int* nb) {

    *vd = m_VDs;
    *nb = m_nVDs;
}

void CUser::getPermissions(permission** pe, int* nb) {
    *pe = m_Perms;
    *nb = m_nPerms;
}
