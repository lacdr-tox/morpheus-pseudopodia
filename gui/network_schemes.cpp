#include "network_schemes.h"
#include <QWebEngineUrlRequestJob>
#include <qmimedatabase.h>


HelpNetworkScheme::HelpNetworkScheme(ExtendedNetworkAccessManager* nam, QObject* parent) : QWebEngineUrlSchemeHandler(parent), nam(nam) {}

void HelpNetworkScheme::requestStarted(QWebEngineUrlRequestJob *job) {
	if (job->requestMethod() == QString("GET")) {
		auto reply = nam->get(QNetworkRequest(job->requestUrl()));
		auto content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
		job->reply(content_type.toUtf8(),reply);
	}
	else {
		qDebug() << "Unsupported request type " << job->requestMethod() << " to Help network.";
		job->fail(QWebEngineUrlRequestJob::RequestFailed);
	}
}

QtRessourceScheme::QtRessourceScheme(ExtendedNetworkAccessManager* nam, QObject* parent) : QWebEngineUrlSchemeHandler(parent), nam(nam) {}

void QtRessourceScheme::requestStarted(QWebEngineUrlRequestJob *job) {
	if (job->requestMethod() == QString("GET")) {
		auto reply = nam->get(QNetworkRequest(job->requestUrl()));
		auto content_type = reply->header(QNetworkRequest::ContentTypeHeader).toString();
		job->reply(content_type.toUtf8(),reply);
	}
	else {
		qDebug() << "Unsupported request type " << job->requestMethod() << " to Ressource network.";
		job->fail(QWebEngineUrlRequestJob::RequestFailed);
	}
}
