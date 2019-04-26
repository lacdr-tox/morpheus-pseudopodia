#ifndef NETWORK_SCHEMES_H
#define NETWORK_SCHEMES_H

#include <QWebEngineUrlSchemeHandler>
#include <QWebEnginePage>
#include "network_access.h"


class AdaptiveWebPage : public QWebEnginePage {
Q_OBJECT
public:
	AdaptiveWebPage(QObject* parent = nullptr) : QWebEnginePage(parent) {};
	void delegateScheme(QString scheme);
	signals:
		void linkClicked(const QUrl&);

protected:
	bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame) override;
private:
	QStringList delegate_schemes;

};

/// Help scheme handler for the QWebEngine Browser
class HelpNetworkScheme : public QWebEngineUrlSchemeHandler
{
public:
	HelpNetworkScheme(ExtendedNetworkAccessManager* nam, QObject* parent = nullptr);
	void requestStarted(QWebEngineUrlRequestJob *) override;
	static QString scheme() { return "qthelp"; };
private:
	ExtendedNetworkAccessManager* nam;
};

/// Ressource Schema handler for the QWebEngine Browser
class QtRessourceScheme : public QWebEngineUrlSchemeHandler
{
public:
	QtRessourceScheme(ExtendedNetworkAccessManager* nam, QObject* parent = nullptr);
	void requestStarted(QWebEngineUrlRequestJob *) override;
	static QString scheme() { return "qrc"; };
private:
	ExtendedNetworkAccessManager* nam;
};	void linkActivated(QUrl);


#endif // NETWORK_SCHEMES_H
