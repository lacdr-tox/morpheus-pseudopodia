#ifndef NETWORK_SCHEMES_H
#define NETWORK_SCHEMES_H

#include <QWebEngineUrlSchemeHandler>
// #include <QWebEnginePage>
#include "network_access.h"

/// Help scheme handler for the QWebEngine Browser
class HelpNetworkScheme : public QWebEngineUrlSchemeHandler
{
public:
	HelpNetworkScheme(ExtendedNetworkAccessManager* nam, QObject* parent = nullptr);
	void requestStarted(QWebEngineUrlRequestJob *) override;
	static QByteArray scheme() { return "qthelp"; };
private:
	ExtendedNetworkAccessManager* nam;
};

/// Ressource Schema handler for the QWebEngine Browser
class QtRessourceScheme : public QWebEngineUrlSchemeHandler
{
public:
	QtRessourceScheme(ExtendedNetworkAccessManager* nam, QObject* parent = nullptr);
	void requestStarted(QWebEngineUrlRequestJob *) override;
	static QByteArray scheme() { return "qrc"; };
private:
	ExtendedNetworkAccessManager* nam;
};	void linkActivated(QUrl);


#endif // NETWORK_SCHEMES_H
