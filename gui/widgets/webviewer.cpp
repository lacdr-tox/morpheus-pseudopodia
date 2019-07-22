#include "webviewer.h"

#include "../config.h"

#ifdef USE_QTextBrowser

WebViewer::WebViewer(QWidget* parent): QTextBrowser(parent) {
	connect(this, &QTextBrowser::sourceChanged, this, &WebViewer::urlChanged);
	this->nam = config::getNetwork();
}

QVariant WebViewer::loadResource(int type, const QUrl &name) {
	auto reply = nam->get(QNetworkRequest(name));
	return reply->readAll();
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
#include <QWheelEvent>

void AdaptiveWebPage::delegateScheme(QString scheme) {
	delegate_schemes.append(scheme);
}

bool AdaptiveWebPage::acceptNavigationRequest(const QUrl& url, QWebEnginePage::NavigationType type, bool isMainFrame) {
	if (delegate_schemes.contains(url.scheme()) && type == QWebEnginePage::NavigationType::NavigationTypeLinkClicked) {
// 		qDebug() << "Forwarding weblink on page " <<  this->url() << " to " << url << " as Signal";
		emit linkClicked(url);
		setUrl(url);
		return false;
	}
	return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

WebViewer::WebViewer(QWidget* parent): QWebEngineView(parent) {
	reset();
}

void WebViewer::reset() {
	adaptive_page = new AdaptiveWebPage(this);
	adaptive_page->delegateScheme("http");
	adaptive_page->delegateScheme("https");
	adaptive_page->delegateScheme("morph");
	connect(adaptive_page, &AdaptiveWebPage::linkClicked, this, &WebViewer::linkClicked,Qt::QueuedConnection);
	setPage(adaptive_page);
}

void WebViewer::wheelEvent(QWheelEvent *event) {
	if (event->modifiers() == Qt::ControlModifier) {
		double factor = 0.002;
		setZoomFactor(zoomFactor() * (1 + event->angleDelta().y() * factor));
		event->accept();
		
	}
	else
		QWebEngineView::wheelEvent(event);
}

bool WebViewer::debug(bool state) {
// 	this->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, state);
	return false;
}

#endif


// ZoomFilter::ZoomFilter(WebViewer* view) :QObject(view), view(view) {};
// bool ZoomFilter::eventFilter(QObject *obj, QEvent *event) {
// 	if (event->type() == QEvent::Wheel ) {
// 		auto wheel = qobject_cast<QWheelEvent>(event);
// 		
// 		return
// 	}
// 	else
// 		return QObject::eventFilter(obj,event);
// }

