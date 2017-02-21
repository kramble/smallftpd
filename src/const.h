// Revision : $Id: const.h,v 1.1.1.1 2004/04/05 21:26:52 smallftpd Exp $
// Author : Arnaud Mary <smallftpd@free.fr>
// Project : Smallftpd
//
// For more information, visit http://smallftpd.sourceforge.net
// 
// This file contains the #define insctructions
// 

#ifndef CONST_H_
#define CONST_H_

#define     CRLF                    "\r\n"

#define     SOFTWARE                "smallftpd"
#define     VERSION                 "1.0.4"
#define     AUTHOR                  "http://smallftpd.free.fr/"

// #define     DEBUG_FILE              "debug.log"
// #define     ACTIVITY_FILE           "activity.log"

#define     ACTIVE_MODE             0
#define     PASSIVE_MODE            1

#define     LOG_DEBUG               0
#define     LOG_TRANSFER            1

#define     BUFFER_SIZE             512
#define     TX_BUFFER_SIZE          50000     // 8192
#define     MAX_USERS               32
#define     MAX_SESSIONS            32
#define     MAX_PASV_PORTS          256

// Constants


// Delays (milliseconds)
#define     SMALL_TIMEOUT           1000     //  1 second
#define     MEDIUM_TIMEOUT         10000     // 10 seconds
#define     BIG_TIMEOUT            30000     // 30 seconds

#define     TIMER_DELAY             5000


#define     DEFAULT_PERMISSIONS     "LR"

#define     INFO_CONFIG_LOADED      "Configuration loaded"
#define     INFO_CONFIG_SAVED       "Configuration saved"
#define     INFO_SERVER_RUNNING     "FTP server is running"
#define     INFO_SERVER_STOPPED     "FTP server stopped"
#define     INFO_COULD_NOT_BIND     "Unable to bind socket. Make sure no other server is running."


//
// DEFAULT Values
//
#define     DEFAULT_PORT            21
#define     DEFAULT_MAXCONN         10
#define     DEFAULT_USERMAXCONN     10

#define     DEFAULT_MIN_PASV_PORT   5000
#define     DEFAULT_MAX_PASV_PORT   5010
#define     DEFAULT_PASV_URL        "account.dyndns.org"

#define     DEFAULT_SIM_CONNECTIONS               3
#define     DEFAULT_INACTIVITY_TIMEOUT            120



//
// ERROR Messages
//

#define     LOG_ERROR_SOCKET            "Unable to create socket. \r\n"
#define     LOG_ERROR_LISTEN            "Unable to make socket listen. \r\n"
#define     LOG_ERROR_BIND              "Unable to bind socket to local address. \r\n"
#define     LOG_ERROR_CREATE_THREAD     "Unable to create thread. \r\n"


//
// FTP Messages
//
#define     PORT_OPENED             "150 Data connection ready. \r\n"

#define     PORT_SUCCESS            "200 Port command successful. \r\n"

#define     TYPE_SET_1              "200 Type set to "
#define     TYPE_SET_2              ". \r\n"
#define     FILE_STATUS             "213 "
#define     SYSTEM_MSG              "215 UNIX Type: L8\r\n"
#define     MSG_CONNECTED_1         "220- smallftpd "
#define     MSG_CONNECTED_2         "\r\n220- check http://smallftpd.free.fr for more information\r\n220 report bugs to smallftpd@free.fr\r\n"
#define     LOGOUT                  "221 Good bye. \r\n"
#define     LIST_SUCCESS            "226 Transfer complete. \r\n"
#define     PASV_SUCCESS            "227 Entering Passive Mode (%s,%d,%d) \r\n"

#define     LOG_SUCCESS             "230 User logged in. \r\n"
#define     PORT_CLOSE              "226 Transfer complete. \r\n"
#define     ABOR_SUCCESS            "226 ABOR command successful. \r\n"
#define     CWD_SUCCESS             "250 CWD command successful. \r\n"
#define     LS_SUCCESS              "250 list command successful. \r\n"
#define     RNTO_SUCCESS            "250 RNTO command successful.\r\n"
#define     RMD_SUCCESS             "250 RMD command successful.\r\n"
#define     DELE_SUCCESS            "250 DELE command successful.\r\n"
#define     MKD_SUCCESS_1           "250 \""
#define     MKD_SUCCESS_2           "\" directory created.\r\n"
#define     CDUP_1                  "250 Directory changed to "
#define     CDUP_2                  " \r\n"

#define     PWD_1                   "257 \""
#define     PWD_2                   "\" is current directory. \r\n"

#define     MSG_REQ_PWD             "331 User name okay, password required.\r\n"
#define     REST_1                  "350 Restarting at "
#define     REST_2                  " - send STORE or RETRIEVE to initiate transfer. \r\n"
#define     RNFR_SUCCESS            "350 Ready for destination name.\r\n"

#define     TOO_MANY_USERS          "421 Too many users. Retry later.\r\n"
#define     TRANSFER_ABORTED        "426 Transfer aborted. Data connection closed.\r\n"


#define     ERR_OVERFLOW            "500 Command length too large.\r\n"
#define     NOT_UNDERSTOOD          "500 Command not understood.\r\n"
#define     TRANSFER_IN_PROGRESS    "500 Transfer in progress. Send ABOR to stop it.\r\n"

#define     BAD_SEQUENCE            "503 Bad sequence of commands. \r\n"
#define     NOT_LOGGED_IN           "530 Not logged in. \r\n"
#define     LOG_FAIL                "530 Login incorrect. \r\n"

#define     FILE_AXX_2              "550 No such file or directory. \r\n"
#define     FILE_RIGHTS             "550 Permission denied. \r\n"
#define     CWD_FAILED              "550 CWD command failed. \r\n"
#define     RNTO_FAILED             "550 Unable to move file to specified path. \r\n"
#define     RMD_FAIL                "550 Unable to remove directory. \r\n"     // TODO :  CHANGE
#define     PASV_ERROR              "550 Unable to create listening socket for passive mode. \r\n"     // TODO : CHANGE


//
// LOG Messages
//

/*
#define     LOG_CONNECTED           "New connection established with %s \r\n"
#define     LOG_ENDCONN             "Connection with %s terminated\n"
#define     LOG_STARTING            "Starting FTP Server...\r\n"
#define     LOG_STARTED             "FTP Server started. OK.\r\n"
#define     LOG_STOPPED             "FTP Server stopped.\r\n"
#define     LOG_SESSIONSTOPPED      "Client thread stopped.\r\n"
*/

// #define      MSG_CWD             "%s: Changed path to %s\r\n"



#endif
