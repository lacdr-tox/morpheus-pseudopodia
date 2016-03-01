//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef SSHPROXY_H
#define SSHPROXY_H



#include "config.h"

#ifdef HAVE_LIBSSH
#include <QtGui>
#include <libssh/sftp.h>
#include <libssh/libssh.h>

#include <fcntl.h>
#include <sstream>


#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

using namespace std;

/*!
This class provides 4 tasks using ssh.<br>
You can execute a command on the external machine (named in QSettings), push a file to it, pull a file from it or<br>
pull a whole directory from it.<br>
To use this class you have to install a additional library called 'libssh'.
*/
class sshProxy {
public:
    sshProxy(); /*!< Creates a proxy. */
    ~sshProxy();

	bool checkConnection(); /*!< Returns if a connection can be successful established. */
    bool checkConnection(QString host, QString user); /*!< Returns if a connection usind the @p host and @p user provided can be successfully established. */

    bool exec(QString command,  QString* answer = NULL);
    /*!<
      Trys to execute a command on the external machine and returns if succeed or fail.
      \param command Command which will be executed on the external machine.
      \param answer Variable in which the answer is stored (for example error-messages, etc.)
      */
    bool push_file(QFile &local, QString remote, int permission=0664);
    /*!<
      Trys to push a file to the external machine and returns if succeed or fails.
      \param local File on the local machine which will be pushed to the external.
      \param remote Path on the external machine where the file should be pushed.
      \param permission Permissions the new file shall have on the external machine.
      */
	bool push_directory(QString local, QString remote, int permission=0664);
    /*!<
      Trys to push the files in directory to the external machine. Returns if succeeds.
      \param local Local directory to be pushed to the remote machine.
      \param remote Directory on the remote machine.
      \param permission Permissions the new file shall have on the external machine.
      */
    bool pull_file(QString remote, QFile &local);
    /*!<
      Trys to pull a file from the external machine, stores it on the local machine and returns if succeed or fails.
      \param remote Path of the external file, which should be pulled
      \param local Localfile, in which the pulled file should be stored on the local machine.
      */
    bool pull_directory(QString remote, QString local);
    /*!<
      Trys to pull a directory from the external machine, stores it on the local and returns if succeed or fails.
      \param remote Path to the external-directory which should be pulled
      \param local Pathname where the pulled directory should be stored
      */

    void clearSessions(); /*!< Closes all opened ssh-sessions. */
    QString getLastError(); /*!< Returns the last error-message. */

private:
    /*! Container which declares the state of a ssh-session. */
    struct ssh {
        ssh_session session; /*!< Session, which can be used. */
        bool available; /*!< State if the session is free or not. */
    };

    static const int ssh_session_count = 5; /*!< Maximal number of sessions, which can be used at one time. */
    static bool ssh_proxy_uploaded; /*!< Describes if the necessary sshProxy was uploaded to the external machine. */

    static QList<ssh> sessions; /*!< List of all ssh-Sessions. */
    static int instances; /*!< Number of the current used sessions. */

    QString error; /*!< Last error-message. */
    ssh_channel getChannel(); /*!< Returns a new ssh-Channel. */
    void freeChannel(ssh_channel); /*!< Frees the given ssh-Channel. */
    ssh_session getSession(bool sftp=false); /*!< Returns a free ssh-Session. */
    void freeSession(ssh_session); /*!< Frees the given ssh-Session. */
    bool verify_knownhost(ssh_session sess); /*!< Trys to verify the host and returns if succeed or fails. */
    ssh_session createSession ( QString remote_host, QString remote_user );
};

#else // HAVE_LIBSSH

class sshProxy {
public:
    bool checkConnection() { return false; };
	bool checkConnection(QString host, QString user) { return false; };
    bool exec(QString command,  QString* answer = NULL) { return false; };
    bool push_file(QFile &local, QString remote, int permission=0664) { return false; };
	bool push_directory(QString local, QString remote, int permission=0664) {return false; }
    bool pull_file(QString remote, QFile &local) { return false; };
    bool pull_directory(QString remote, QString local) { return false; };
    QString getLastError() { return "Not compiled with ssh support!"; };
    void clearSessions(){return;}; /*!< Closes all opened ssh-sessions. */

};

#endif // HAVE_LIBSSH

#endif // SSHPROXY_H
