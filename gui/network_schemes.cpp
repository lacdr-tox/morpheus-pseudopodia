#include "network_schemes.h"
#include <QWebEngineUrlRequestJob>
#include <qmimedatabase.h>

void AdaptiveWebPage::delegateScheme(QString scheme) {
	delegate_schemes.append(scheme);
}

bool AdaptiveWebPage::acceptNavigationRequest(const QUrl& url, QWebEnginePage::NavigationType type, bool isMainFrame) {
	if (delegate_schemes.contains(url.scheme())) {
		emit linkClicked(url);
		return false;
	}
	return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}


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
