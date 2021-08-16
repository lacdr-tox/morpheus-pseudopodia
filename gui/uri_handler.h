#ifndef URI_HANDLER_H
#define URI_HANDLER_H

#include "config.h"
#include <QList>


bool parseCmdLine(QCommandLineParser& parser, const QStringList& arguments);

int handleSBMLConvert(QString arg);

class uriOpenHandler : public QObject
{
	Q_OBJECT
	
	
public:
	uriOpenHandler(QObject* parent) : QObject(parent) {};
	static bool isValidUrl(const QUrl& url);
	struct URITask {
		enum Task { Open, Import, Process, Convert, None };
		QUrl m_uri;
		QUrl m_model_url;
		Task method;
		QMap<QString,QString> parameter_overrides;
		SharedMorphModel model;
		QString output_file;
	};
	void processTask(URITask task, bool interactive);
	
signals:
	void uriOpenProgress(const QUrl& uri, int progress);
	
public slots:
	void processUri(const QUrl &uri);
	
private slots:
	void processNetworkReply(URITask task, QNetworkReply* reply);
	
protected:
	bool eventFilter(QObject *obj, QEvent *event) override;
	
private:
	

	QList<URITask> queue;

	URITask parseUri(const QUrl& uri);
	bool processFile(URITask& task);
	void processOverrides(URITask& task);
	
};


#endif
