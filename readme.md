smallftpd is forked from [http://smallftpd.sourceforge.net] ([https://sourceforge.net/projects/smallftpd])

All credits to the original author, see below.

This gitgub fork was created because the original smallftpd did not work with a Windows 10 FTP command line client due to changes introduced by Microsoft (it sends an OPTS command, which crashes the smallftpd server). As smallftpd no longer seems to be supported, I thought I'd share the fix on github.

Compilation

smallftpd compiles with Dev-C++ (http://www.bloodshed.net/devcpp.html). I used version Dev-Cpp 5.11 TDM-GCC 4.9.2 from http://orwelldevcpp.blogspot.co.uk/ on 32 bit Windows Vista (yeah, ancient, but I didn't want to mess up my Win10 boxes with yet another compiler).

Load the project (smallftpd.dev), which will probably complain about a missing library directory, just say yes.

Change the dropdown on the toolbar from 64bit to 32bit release compilation (I haven't tested 64 bit), and compile (F9).

There will be a lot of compilation warnings, and with the original version a few errors (now fixed).

Depending on your Dev-C++ install location, the linker may fail to find libwsock32.a - fix this by opening Project/ProjectOptions/Parameters/Linker and browsing to MinGW64/x86_64-w64-mingw32/lib32/libwsock32.a

Note that the current code produces debug output (debug.log, transfer.log) by default. This can be disabled by modifying the log() function in main.cpp (uncomment "return false" near the top of the function).

For testing it is useful to start the FTP client with -d -n options, and if NOT using a Windows 10 FTP client the "literal opts" command can be used to emulate the problematic behaviour.

ORIGINAL README.TXT FOLLOWS ...

 ------------------------------------
 ------------------------------------
   smallftpd   1.0.4
   by Arnaud Mary 
   http://smallftpd.sourceforge.net
 ------------------------------------
 ------------------------------------ 


 1. Description :

    - The first goal of this application was to allow me to specify a hostname to be resolved by the FTP server when going to passive mode.
    - Today, Smallftpd is an open-source project, a simple and small FTP server.
    - Smallftpd needs your remarks, your suggestions, and tour c++ skills !

 2. Installation : 
    
    - run the smallftpd.exe
    - check configuration, create user accounts,
    - press "play". ;-)


 3. Features :

    - Multi-threaded FTP server
    - Active / Passive mode
    - Multi-users
    - Manage List, Read & Write rights for every user
    - Advanced filesystem.


 4. List of supported commands :

    - ABOR, CDUP, CWD, DELE, LIST, MKD, PASS, PASV, PORT, PWD, QUIT, REST, 
    RETR, RMD, RNFR, RNTO, SIZE, STORE, SYST, TYPE, USER.


 5. History :  

    - 1.0.3 Fix (May 21st 2003) :
          * Fixed a stupid bug concerning transfer performances.
          * disabled debug logging
  
    - 1.0.3 :
          * Fixed major security bug (http://securitytracker.com/alerts/2003/Apr/1006685.html)
          * Fixed bug when trying to retrieve a file that does not exist
          * window position is saved
          * display a live estimation of transfer rate in GUI
          * fixed minor bugs
    - 1.0.2 :
          * fixed several [major] bugs
          * some UI improvements
          * ABOR command is now correctly managed.

    - 0.93 alpha :
          * Better UI, window minimizes in systray.
          * Better log system. All transfers are logged in transfers.log           

    - 0.9 alpha ( June 2002 ) :
          * First operational version
          * Added Rename files/folders feature
          * Manage user virtual filesystem
          * Added an inactivity timeout

    - 0.5 ( May 2002 ) :
          * Added Rename files/folders feature  (RNFR & RNTO)
          * Added Delete files feature (DELE)
          * Added Create/delete folder (MKD & RMD)

    - 0.3 ( May 2002 ) : 
          * Configuration is now automatically loaded and saved in a .ini file.
          * Added button icons for ftp running & stopped.
          * Added a List box showing users connected.

    - 0.2 ( April 2002 ) : 
          * Passive mode added. 
          * Also fixed some bugs.
     
    - 0.1 ( January 2002 ) : 
          * very first version.
          * Active mode is supported.


 6. Next features :
 
    - Manage Max number of threads per user / total threads.
    - Reduce Memory taken by process 
    - prevent users from hammering.
    - restrict access by IP
    - check for memory problems. Check Memory overflow possible problems.
    - use CopyMemory, MoveMemory, etc... in order to reduce the size of the .exe file.
    - respect the RFC
    - improve SECURITY !

    - ... 


