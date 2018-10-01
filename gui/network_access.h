#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QHelpEngineCore>

/// Helper Class to gain access to the HelpEngine Data
class HelpNetworkReply : public QNetworkReply
{
	Q_OBJECT
	public:
		HelpNetworkReply(QObject* parent, const QNetworkRequest& request, const QByteArray& fileData);
			
		virtual void abort() override {}
		virtual qint64 bytesAvailable() const override { return data.length() + QNetworkReply::bytesAvailable(); }
			
	protected:
		virtual qint64 readData(char *data, qint64 maxlen) override;
			
	private:
		QByteArray data;
		qint64 origLen;
};
 
/// Helper Class to gain access to the Qt Ressources Data
class QRCNetworkReply : public QNetworkReply 
{
	Q_OBJECT
	public:
		QRCNetworkReply(QObject* parent, const QNetworkRequest &request);
		virtual void abort() override {}
		virtual qint64 bytesAvailable() const override { return df.size() - df.pos()  + QNetworkReply::bytesAvailable(); }
	protected:
		virtual qint64 readData(char *data, qint64 maxlen) override; 
	private:
		QFile df;
		qint64 origLen;
};

/// NetworkAccessManager also supporting access to Qt Ressources and A QHelpEngine
class ExtendedNetworkAccessManager : public QNetworkAccessManager
{
	Q_OBJECT
	public:
		ExtendedNetworkAccessManager(QObject *parent, QHelpEngineCore *engine)
			: QNetworkAccessManager(parent), m_helpEngine(engine) {}

	protected:
		virtual QNetworkReply *createRequest(Operation op,
		const QNetworkRequest &request, QIODevice *outgoingData = 0) override;

	private:
		QHelpEngineCore *m_helpEngine;
};



