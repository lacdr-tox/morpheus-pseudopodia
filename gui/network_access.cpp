#include "network_access.h"
#include <QTimer>
#include <QDesktopServices>
#include <QDebug>
#include <QMimeDatabase>

QRCNetworkReply::QRCNetworkReply(QObject* parent, const QNetworkRequest &request) : QNetworkReply(parent)
{
	setRequest(request);
	setOpenMode(QIODevice::ReadOnly);
	auto path = request.url().path();
	
	// search the file in the QT Ressources
	df.setFileName(QString(":") + path);
	
	if ( ! df.exists()) {
		qDebug() << QString(":") + path << " does not exist";
		this->setError(NetworkError::ContentNotFoundError,"File not found in QT Ressources");
		emit error(NetworkError::ContentNotFoundError);
		return;
	}
	
	QString mimeType = QMimeDatabase().mimeTypeForUrl(request.url()).name();
// 	if (path.endsWith(".png")) 
// 		mimeType = "image/png";
// 	else if (path.endsWith(".html"))
// 		mimeType = "text/html";
// 	else if (path.endsWith(".js"))
// 		mimeType = "text/javascript";
// 	else if (path.endsWith(".css"))
// 		mimeType = "text/css";
	setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
	setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(df.size()));
	qDebug() << QString(":") + path << " : " << df.size();
	
	QTimer::singleShot(0, this, SIGNAL(metaDataChanged()));
// 	QTimer::singleShot(0, this, SIGNAL(readyRead()));
	setFinished(true);
	QTimer::singleShot(0, this, SIGNAL(finished()));
}

qint64 QRCNetworkReply::readData(char *buffer, qint64 maxlen)
{
	
	if ( ! df.isOpen() && ! df.open(QIODevice::ReadOnly)) {
		this->setError(NetworkError::ContentNotFoundError,"File not found in QT Ressources");
		return 0;
	}

	qint64 len = df.read(buffer,maxlen);
// 	qDebug() << "QRC read " << len  << " of " << df.size() << " bytes leaving " << df.size() - df.pos();
// 	if (df.atEnd())
// 		QTimer::singleShot(0, this, SIGNAL(finished()));
	return len;
}


HelpNetworkReply::HelpNetworkReply(QObject* parent, const QNetworkRequest &request,
        const QByteArray &fileData)
    : QNetworkReply(parent), data(fileData), origLen(fileData.length())
{
	setRequest(request);
	setOpenMode(QIODevice::ReadOnly);
	if (data.isEmpty()) {
		this->setError(NetworkError::ContentNotFoundError,"File not found in Help Engine");
		emit error(NetworkError::ContentNotFoundError);
		return;
	}
	QString mimeType = QMimeDatabase().mimeTypeForUrl(request.url()).name();
// 	auto path = request.url().path();
// 	if (path.endsWith(".png")) 
// 		mimeType = "image/png";
// 	else if (path.endsWith(".html"))
// 		mimeType = "text/html";
// 	else if (path.endsWith(".js"))
// 		mimeType = "text/javascript";
// 	else if (path.endsWith(".css"))
// 		mimeType = "text/css";
	
	setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
	setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(origLen));
	
	QTimer::singleShot(0, this, SIGNAL(metaDataChanged()));
	setFinished(true);
	QTimer::singleShot(0, this, SIGNAL(finished()));
}
 
qint64 HelpNetworkReply::readData(char *buffer, qint64 maxlen)
{
	qint64 len = qMin(qint64(data.length()), maxlen);
	if (len) {
		memcpy(buffer, data.constData(), len);
		data.remove(0, len);
	}
// 	if (!data.length())
// 		QTimer::singleShot(0, this, SIGNAL(finished()));
	return len;
}
 
QNetworkReply *ExtendedNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
// 	qDebug() << "Creating Network Request for URL " << request.url();
	QString scheme = request.url().scheme();
	if (scheme == QLatin1String("qthelp") || scheme == QLatin1String("about")) {
		auto url = request.url();
		auto path = url.path();
		path.replace("$relpath^","");
		url.setPath(path);
		auto data = m_helpEngine->fileData(url);
		if (data.isEmpty())
			qDebug() << "Empty URL" << path << " of " << data.size() << " bytes";
		return new HelpNetworkReply(this, request, data);
	}
	else if (scheme == QLatin1String("qrc") ) {
		return new QRCNetworkReply(this, request);
	}
	else {
		return QNetworkAccessManager::createRequest(op, request, outgoingData);
	}
}
