#ifndef URI_HANDLER_H
#define URI_HANDLER_H

#include "config.h"
#include <QList>


bool parseCmdLine(QCommandLineParser& parser, const QStringList& arguments);
int handleSBMLConvert(QString arg);

class uriOpenHandler : public QObject
{
	Q_OBJECT
	
	struct URITask;
	
public:
	uriOpenHandler(QObject* parent) : QObject(parent) {};
	static bool isValidUrl(const QUrl& url);
	
signals:
	void uriOpenProgress(const QUrl& uri, int progress);
	
public slots:
	void processUri(const QUrl &uri);
	
private slots:
	void uriFetchFinished(URITask task, QNetworkReply* reply);
	
protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
	
private:

	struct URITask {
		enum Task { Open, Import, Process };
		QUrl m_uri;
		QUrl m_model_url;
		Task method;
		QMap<QString,QString> parameter_overrides;
		SharedMorphModel model;
	};
	QList<URITask> queue;

	URITask parseUri(const QUrl& uri);
	void processTask(URITask task);
	
};


#endif
