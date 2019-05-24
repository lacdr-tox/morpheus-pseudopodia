#include "webviewer.h"

#include "../config.h"

#ifdef USE_QTextBrowser

WebViewer::WebViewer(QWidget* parent): QTextBrowser(parent) {
	connect(this, QTextBrowser::historyChanged(const QUrl&)), this, &WebViewer::urlChanged);
}


#elif defined USE_QWebKit
#include <QWebView>
#include <QWebHistory>
WebViewer::WebViewer(QWidget* parent) : QWebView(parent) {
	this->page()->setNetworkAccessManager(config::getNetwork());
	this->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
	this->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
	this->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
	this->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
}

bool WebViewer::debug(bool state) {
	this->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, state);
	return state;
}


#elif defined USE_QWebEngine
#include <QWebEngineSettings>
// #include "../network_schemes.h"

void AdaptiveWebPage::delegateScheme(QString scheme) {
	delegate_schemes.append(scheme);
}

bool AdaptiveWebPage::acceptNavigationRequest(const QUrl& url, QWebEnginePage::NavigationType type, bool isMainFrame) {
	if (delegate_schemes.contains(url.scheme()) && type == QWebEnginePage::NavigationType::NavigationTypeLinkClicked) {
		emit linkClicked(url);
		return false;
		
	}
	return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

WebViewer::WebViewer(QWidget* parent): QWebEngineView(parent) {
	adaptive_page = new AdaptiveWebPage(this);
	adaptive_page->delegateScheme("http");
	adaptive_page->delegateScheme("https");
	adaptive_page->delegateScheme("morph");
	connect(adaptive_page, &AdaptiveWebPage::linkClicked, this, &WebViewer::linkClicked);
	setPage(adaptive_page);
}

bool WebViewer::debug(bool state) {
// 	this->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, state);
	return false;
}

#endif
