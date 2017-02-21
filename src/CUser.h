// Revision : $Id: CUser.h,v 1.1.1.1 2004/04/05 21:26:54 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
// This is the header file for CUser.cpp
// 

#ifndef CUSER_H_
#define CUSER_H_

typedef struct {
    char  real[BUFFER_SIZE];
    char  virt[BUFFER_SIZE];

} virtualDir;

typedef struct {
    char  path[BUFFER_SIZE];
    char  type[5];

} permission;


class CUser {
    public:
        // Construction & Destruction
        CUser();
        ~CUser();

        void          set(char*, char*, int, int);
        bool          copyFrom(CUser *u_);
        int           getVirtualSubDirs(char*, char*, char*);

        void          checkVirtPath(char* virt);
        bool          setRealPath(char* virt, char* real, bool force);
        void          setVirtPath(char* real, char* virt);

    public:
        char          login[BUFFER_SIZE];
        char          passwd[BUFFER_SIZE];

        int           simConn;
        int           timeOut;

//      char          rootDir[BUFFER_SIZE];

        bool          addVirtualDir(char*, char*);
        bool          delVirtualDir(int);
        bool          addPermission(char*, char*);
        bool          delPermission(int);
        
        bool          hasPermission(char* path_, char* type_);
        bool          getRootDir(char*);

        void          getVirtualDirs(virtualDir** vd, int* nb);
        void          getPermissions(permission** pe, int* nb);

        int           m_nVDs;
        virtualDir*   m_VDs;

        int           m_nPerms;
        permission*   m_Perms;

};

#endif

