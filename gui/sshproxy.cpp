#include "sshproxy.h"


#ifdef HAVE_LIBSSH 

class SleeperThread : public QThread
{
public:
    static void msleep(unsigned long msecs)
    {
        QThread::msleep(msecs);
    }
};


int sshProxy::instances = 0;
bool sshProxy::ssh_proxy_uploaded = false;
QList<sshProxy::ssh> sshProxy::sessions;



//---------------------------------------------------------------------------------------------------

sshProxy::sshProxy() {
    if (instances == 0) {
        sessions.clear();
        ssh s; s.available=true; s.session=NULL;
        while (sessions.size() < ssh_session_count) {
            sessions.push_back(s);
        }
    }

    instances++;
}

//---------------------------------------------------------------------------------------------------

sshProxy::~sshProxy() {
    --instances;
    if (instances == 0) {
        clearSessions();
    }
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::checkConnection() {
	ssh_session session = getSession();
	if (session) {
		freeSession(session);
		return true;
	}
	else {
		return false;
	}
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::checkConnection( QString host, QString user ) {

    ssh_session session = createSession(host, user);
    if (  session != NULL) {
        freeSession(session);
        return true;
    }
    else {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::exec(QString command, QString* answer) {

	QTime duration;
	duration.start();
// 	qDebug() << duration.elapsed() << "ms: Getting channel ... ";
	qDebug() << "sshProxy" << QThread::currentThread();
	ssh_channel chan = getChannel();
	if ( ! chan ) {
		return false;
	}
// 	qDebug() << duration.elapsed() << "ms: Sending request \"" << command << "\" ... "; cout.flush();

	if (ssh_channel_request_exec(chan, command.toStdString().c_str()) != SSH_OK)
	{
		error = QString("Unable to run command: ") + command + "!\n" + QString(ssh_get_error(chan));
		freeChannel(chan);
		return false;
	}
// 	qDebug() << duration.elapsed() << "ms: Polling output ... ";

	QString tmp;
	if (!answer)
		answer = &tmp;
	else
		answer->clear();

	int nbytes;
	const int b_size=256;
	char buffer[b_size];

	while (ssh_channel_is_open(chan)) {
		nbytes = ssh_channel_read(chan, buffer, sizeof(char)*(b_size-1), 0);
		if (nbytes <= 0)
			break;
		if (nbytes>0) {
			buffer[nbytes]='\0';
			answer->append(buffer);
		}
	}

	(*answer) = answer->trimmed();
// 	qDebug() << duration.elapsed() << "ms: Freeing channel ... ";
	freeChannel(chan);
// 	cout << " done" << endl;
	return true;
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::push_file(QFile& local, QString remote, int permission) {
    sftp_session sftp = NULL;
    sftp_file file = NULL;
    int rc;
    int access_type = O_WRONLY | O_CREAT | O_TRUNC;

    if(local.isReadable()){
        cout << "sshProxy::push_file: local file not readable" << endl;
    }

    ssh_session session = getSession(true);
    if (!session) return false;

    try {
        sftp = sftp_new(session);
        if (sftp == NULL)
            throw QString("Error allocating SFTP session: %1\n").arg(ssh_get_error(session));

        rc = sftp_init(sftp);
        if (rc != SSH_OK)
           throw QString("Error initializing SFTP session: %1.\n").arg(sftp_get_error(sftp));


        if ( ! local.isOpen() && ! local.open(QIODevice::ReadOnly))
           throw QString( "Can't open local file ") + local.objectName();

        file = sftp_open(sftp, remote.toStdString().c_str(), access_type, permission); // S_IRWXU
        if (file == NULL)
            throw QString("Can't open file for writing: %1\n").arg(ssh_get_error(session));


        int length = local.size();
//        QDataStream l_stream(&local);

        char* buffer = new char[length];
        local.read(buffer,length);
//        l_stream.readRawData(buffer,length);
        local.close();

        if ( length != sftp_write(file, buffer, length))
            throw QString("Can't write data to file: %1\n").arg(ssh_get_error(session));
    }
    catch(QString e) {
        error = e;
        if (file) {
            sftp_close(file);
        }
        if (sftp) {
            sftp_free(sftp);
        }
        freeSession(session);
        return false;
    }
    sftp_close(file);
    sftp_free(sftp);
    freeSession(session);
    return true;
}


bool sshProxy::push_directory(QString local, QString remote, int permission) {
	sftp_session sftp = NULL;
	sftp_dir rdir = NULL;
	int rc;
	int access_type = O_WRONLY | O_CREAT | O_TRUNC;

	ssh_session session = getSession(true);
	if (!session) return false;
	const int buffer_size = 1024 * 50;

	remote.remove(QRegExp("\\/$"));

	qDebug() << "pushing dir " << local << " to " << remote;
	QTime duration;
	duration.start();
	try
	{
		sftp = sftp_new(session);
		if (sftp == NULL)
			throw QString("Error allocating SFTP session: %1\n").arg(ssh_get_error(session));

		rc = sftp_init(sftp);
		if (rc != SSH_OK)
			throw QString("Error initializing SFTP session: %1.\n").arg(sftp_get_error(sftp));

		QDir ldir(local);
		if (! ldir.exists()) {
             throw QString("Error opening local directory: %1.\n").arg(local);
		}

		QStringList rdir_folders =  remote.split("/",QString::SkipEmptyParts);
		QString current_remote_dir;
		sftp_dir rdir;

		bool non_exist = false;
		for (uint i=0; i<rdir_folders.size(); i++) {
			if (current_remote_dir.isEmpty())
				current_remote_dir = rdir_folders[i];
			else
				current_remote_dir.append("/").append(rdir_folders[i]);

			if (!non_exist) {
				rdir = sftp_opendir(sftp,current_remote_dir.toStdString().c_str());
				if ( ! rdir )
					non_exist = true;
				else
					sftp_closedir(rdir);
			}
			if (non_exist) {
				// Try to create the remote dir
				if (sftp_mkdir(sftp, current_remote_dir.toStdString().c_str(), 0775) < 0)  {
					throw (QString("Unable to create remote dir \"%1\"!\n%2").arg(current_remote_dir).arg(ssh_get_error(session)));
				}
			}
		}

		QStringList l_files =  ldir.entryList(QDir::Files);
		char buffer[buffer_size];

		for (uint i=0; i<l_files.size(); i++) {
			QString file_name = QFileInfo(ldir, l_files[i]).fileName();
			QString remote_file = remote + "/" + file_name;

			sftp_file r_file = sftp_open(sftp, remote_file.toStdString().c_str(), access_type, permission);
			if (r_file == NULL)
				throw QString("Can't open remote file \"%1\" for writing: %2\n").arg(remote).arg(ssh_get_error(session));

			QFile l_file(ldir.absoluteFilePath(l_files[i]));
			l_file.open(QIODevice::ReadOnly);

			do {
				int data_read = l_file.read(buffer, buffer_size);
				if (data_read == 0) {
					throw QString("Can't read data from local file \"%1\"!").arg(l_file.fileName());
				}
				int data_written = sftp_write(r_file,buffer,data_read);
				if (data_read != data_written) {
					throw QString("Can't write data to remote file \"%1\" %2 != %3: %4\n").arg(remote_file).arg(data_read).arg(data_written).arg(ssh_get_error(session));
				}
			} while ( ! l_file.atEnd());

			l_file.close();
			sftp_close(r_file);
		}
	}
	catch(QString e) {
		error = e;
		if (sftp)
			sftp_free(sftp);
		freeSession(session);
		return false;
	}
	qDebug()<< "took " << duration.elapsed() << " ms";
	sftp_free(sftp);
	freeSession(session);
	return true;
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::pull_file(QString remote, QFile &local) {
    sftp_session sftp = NULL;
    sftp_file file = NULL;
    int rc;
    int access_type = O_RDONLY;

    ssh_session session = getSession(true);
    if (!session) return false;
    try {
        sftp = sftp_new(session);
        if (sftp == NULL)
            throw QString("Error allocating SFTP session: %1\n").arg(ssh_get_error(session));

        rc = sftp_init(sftp);
        if (rc != SSH_OK)
           throw QString("Error initializing SFTP session: %1.\n").arg(sftp_get_error(sftp));

        file = sftp_open(sftp, remote.toStdString().c_str(), access_type, 0);
        if (file == NULL)
            throw QString("Can't open remote file \"%1\" for reading: %2\n").arg(remote).arg(ssh_get_error(session));

        if ( ! local.isOpen() && ! local.open(QIODevice::WriteOnly | QIODevice::Truncate))
           throw QString( "Can't open local file ") + local.objectName();

        char buffer[1024];
        int nbytes = sftp_read(file, buffer, sizeof(buffer));
        while (nbytes > 0)
        {
          if (local.write(buffer, nbytes) != nbytes)
          {
            sftp_close(file);
            throw QString("Unable to write local to file.");
          }
          nbytes = sftp_read(file, buffer, sizeof(buffer));
        }
    }
    catch(QString e)
    {
        error = e;
        if (local.isOpen())
            local.close();
        if (file)
            sftp_close(file);
        if (sftp)
            sftp_free(sftp);
        freeSession(session);
        return false;
    }

    local.close();
    sftp_close(file);
    sftp_free(sftp);
    freeSession(session);
    return true;
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::pull_directory(QString remote, QString local) {
     sftp_session sftp = NULL;
     sftp_dir rdir = NULL;
     int rc;
     int access_type = O_RDONLY;

     ssh_session session = getSession(true);
     if (!session) return false;
     const int buffer_size = 1024;
     try
     {
         sftp = sftp_new(session);
         if (sftp == NULL)
             throw QString("Error allocating SFTP session: %1\n").arg(ssh_get_error(session));

         rc = sftp_init(sftp);
         if (rc != SSH_OK)
            throw QString("Error initializing SFTP session: %1.\n").arg(sftp_get_error(sftp));

         QDir ldir(local);
         if ( ! ldir.exists()) {
             ldir.mkpath(local);
         }
         rdir = sftp_opendir(sftp, remote.toStdString().c_str());
         if (rdir == NULL)
             throw QString("Can't open remote dir \"%1\" for reading!\n %2").arg(remote).arg(ssh_get_error(session));

         while ( ! sftp_dir_eof (rdir) )
         {
             sftp_attributes r_file = sftp_readdir(sftp, rdir);
             if (! r_file) {
                 break;
                 throw QString("Can't read files in remote dir \"%1\"! \n %2").arg(remote).arg(ssh_get_error(session));
             }
             if (r_file->type == SSH_FILEXFER_TYPE_REGULAR && r_file->name[0]!='.') {
                 QFileInfo l_file(ldir, r_file->name);
                 if ( (! l_file.exists()) || (l_file.size() != (int)r_file->size) ) {

                     sftp_file sf_remote = sftp_open(sftp,(remote + "/" + r_file->name).toStdString().c_str(), access_type, 0);
                     if (sf_remote == NULL)
                         throw QString("Can't open remote file \"%1\" for reading: %2\n").arg(remote + r_file->name).arg(ssh_get_error(session));
                     QFile qf_local(l_file.filePath());
                     if ( ! qf_local.open(QIODevice::WriteOnly | QIODevice::Truncate))
                        throw QString( "Can't open local file ") + qf_local.objectName();

                     char buffer[buffer_size];
                     int nbytes = sftp_read(sf_remote, buffer, sizeof(buffer));
                     while (nbytes > 0)
                     {
                       if (qf_local.write(buffer, nbytes) != nbytes)
                       {
                         sftp_close(sf_remote);
                         throw QString("Unable to write local to file.");
                       }
                       nbytes = sftp_read(sf_remote, buffer, sizeof(buffer));
                     }
                     qf_local.close();
                     sftp_close(sf_remote);
                 }

             }
             else if (r_file->type == SSH_FILEXFER_TYPE_DIRECTORY && r_file->name[0]!='.') {
                 pull_directory( remote + "/" + r_file->name, local + "/" + r_file->name );
             }
             sftp_attributes_free(r_file);
         }
         sftp_closedir(rdir);
         sftp_free(sftp);
     }
     catch(QString e) {
         error = e;
         if (sftp)
             sftp_free(sftp);
         freeSession(session);
         return false;
     }
     freeSession(session);
     return true;
}

//---------------------------------------------------------------------------------------------------

QString sshProxy::getLastError() { return error; }

//---------------------------------------------------------------------------------------------------

ssh_session sshProxy::createSession( QString host, QString user ) {
	ssh_session session;
	try {
		session = ssh_new();
		if (session == NULL) { throw(QString("Unable to create a SSH-Session!")); }

		ssh_options_set(session, SSH_OPTIONS_HOST, host.toStdString().c_str());
		ssh_options_set(session, SSH_OPTIONS_USER, user.toStdString().c_str());

		if (ssh_connect(session) != SSH_OK) { throw(QString("Couldn't connect to SSH-Host!")); }

		if ( ! verify_knownhost(session) ) { throw(QString("Could not verify SSH-Host!")); }

		if(ssh_userauth_autopubkey(session, NULL) != SSH_AUTH_SUCCESS) { throw (QString("Error authenticating with public/private key\n" + QString(ssh_get_error(session)))); }
		cout << "SSH Session authenticated" << endl;
	}
	catch (QString e) {
		error = e;
		if (session) {
			e+= "\nError: " + QString(ssh_get_error(session));
			ssh_free(session);
		}
		session = NULL;
	}
	return session;
}

ssh_session sshProxy::getSession( bool sftp )
{
    int session_id = -1;
    uint min,max;
    if (sftp) { min=2; max=sessions.length();}
    else { min=0; max=2; }
    while (1) {
        for (uint i=min; i<max; i++) {
            if (sessions[i].available) {
                cout << "session " << i << " is available"<< endl;
                sessions[i].available = false;
                session_id=i;
                break;
            }
            cout << "session " << i << " not available"<< endl;
        }
        if (session_id != -1) {
            break;
        }
        SleeperThread::msleep(100);
    }

    if ( ! sshProxy::sessions[session_id].session ) {
		ssh_session session = createSession(config::getApplication().remote_host, config::getApplication().remote_user);

		if (session) {
			sshProxy::sessions[session_id].session = session;
			if (!ssh_proxy_uploaded) {
				ssh_proxy_uploaded=true;
				// copy the proxy to the server
				QFile proxy(":/data/processProxy.sh");
				bool r = push_file( proxy, "processProxy.sh",0755);
				if ( ! r ) {
					ssh_proxy_uploaded=false;
					clearSessions();
					return NULL;
				}
			}
		}
		else {
			sessions[session_id].session = NULL;
			sessions[session_id].available = true;
			return NULL;
		}

    }
    cout << "sshProxy: Providing session number" << session_id << endl;
    return sshProxy::sessions[session_id].session;
}

//---------------------------------------------------------------------------------------------------

void sshProxy::freeChannel(ssh_channel ch)
{
    ssh_session session  = channel_get_session(ch);
    ssh_channel_send_eof(ch);
    ssh_channel_close(ch);
    ssh_channel_free(ch);
    freeSession(session);
}

//---------------------------------------------------------------------------------------------------

void sshProxy::freeSession(ssh_session session)
{
    int id=0;
    while (id < sessions.size()) {
        if (sessions[id].session == session) {
            break;
        }
        id++;
    }
    if (id >= sessions.size()) {
        cout << "trying to free an unknown ssh_session !!" << endl;
        return;
    }
	cout << "sshProxy: Getting back session number" << id << endl;
	sessions[id].available = true;
}

//---------------------------------------------------------------------------------------------------

void sshProxy::clearSessions() {
	for (int id=0; id < sessions.size(); id++) {
		if ( sessions[id].session ) {
			ssh_disconnect(sessions[id].session);
			ssh_free(sessions[id].session);
			sessions[id].session = NULL;
		}
	}
	sessions.clear();
}

//---------------------------------------------------------------------------------------------------

ssh_channel sshProxy::getChannel() {
    
    ssh_session session = getSession();
    if ( ! session )
    {
        return NULL;
    }

    ssh_channel chan = channel_new(session);

    if(chan == NULL)
    {
        error = QString("Unable to create a SSH-Channel!\n") + QString(ssh_get_error(session));
        freeSession(session);
        return NULL;
    }

    if(channel_open_session(chan) != SSH_OK)
    {
        error = QString("Can't open the SSH-Channel!\n") + QString(ssh_get_error(session));
        freeSession(session);
        return NULL;
    }
    return chan;
}

//---------------------------------------------------------------------------------------------------

bool sshProxy::verify_knownhost(ssh_session sess)
{
   int state, hlen;
   unsigned char *hash = NULL;
   char *hexa;
   state = ssh_is_server_known(sess);
   hlen = ssh_get_pubkey_hash(sess, &hash);

   if (hlen < 0)
    error = QString("An error occurred during getting the hash-code of the server-public-key!\nYour are now disconnected from the Server!");

   switch (state)
   {
     case SSH_SERVER_KNOWN_OK:
       {return true;}
     case SSH_SERVER_KNOWN_CHANGED:
       {
           stringstream sstr;
           sstr << "Host key for server changed: it is now:\n";
           sstr << hash << "\n";
           sstr << "For security reasons, connection will be stopped";

           error = QString::fromStdString(sstr.str());
           return false;
       }
     case SSH_SERVER_FOUND_OTHER:
       {
           stringstream sstr;
           sstr << "The server gave use a key of a type while we had an other type recorded.\n";
           sstr << "It is a possible attack.\n";
           sstr << "For security reasons, connection will be stopped";

           error = QString::fromStdString(sstr.str());
           return false;
       }
     case SSH_SERVER_FILE_NOT_FOUND:
       {
           state = SSH_SERVER_NOT_KNOWN;
       }
     case SSH_SERVER_NOT_KNOWN:
       {
           hexa = ssh_get_hexa(hash, hlen);

           stringstream sstr;
           sstr << "The server is unknown. Do you trust the host key?\n";
           sstr << "Public key hash: \n" << hexa;
           free(hexa);

           if(QMessageBox::question((QWidget*)0, "Server verification", sstr.str().c_str(), QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
               return false;
           }
           else {
               int err = ssh_write_knownhost(sess);
               if (err < 0)
               {
                   stringstream sstr;
                   sstr << "Error " << strerror(err);

                   error = QString::fromStdString(sstr.str());
                   return false;
               }
               return true;
           }

       }
     case SSH_SERVER_ERROR:
       {
           stringstream sstr;
           sstr << "Error: " << ssh_get_error(sess);

           error = QString::fromStdString(sstr.str());
       }
   }
   free(hash);
   return false;
}

#endif //noSSH
