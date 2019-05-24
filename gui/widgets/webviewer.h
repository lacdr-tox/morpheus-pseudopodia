#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include <QString>
#include <QUrl>

#ifdef USE_QTextBrowser

#include <QTextBrowser>
	
class WebViewer : public QTextBrowser {

Q_OBJECT

public:
	WebViewer(QWidget* parent = nullptr);
	setURL(QUrl url) { setSource(url); }
	bool supportsJS() const { return false;}
	
	template <class F>
	void evaluateJS(QString code, F callable) {
		/* drop */ 
		QVariant r;
		callable(r);
	}
	
	bool canGoBack() const { return this->isBackwardAvailable(); };
	bool canGoForward() const { return this->isForwardAvailable(); };
	
	bool debug(bool state) { return false; };

signals:
	void linkClicked(const QUrl& url);
	void urlChanged();
	void loadFinished(bool ok);

public slots:
	void forward() { back(); };
	void backward() { back(); };
	
};
	
#elif defined USE_QWebKit

#include <QWebView>
#include <QWebFrame>
#include <QWebHistory>

class WebViewer : public QWebView {

Q_OBJECT

public:
	WebViewer(QWidget* parent = nullptr);
	bool supportsJS() const { return true;}
	
	template <class F>
	void evaluateJS(QString code, F callable) {
		auto ret = page()->mainFrame()->evaluateJavaScript(code);
		callable(ret);
	}
	
	void setURL(const QUrl& url) {
		QWebView::setUrl(url);
		emit loadFinished(true);
	}
	
	bool canGoBack() const { return this->history()->canGoBack(); };
	bool canGoForward() const { return this->history()->canGoForward(); };
	
	bool debug(bool state);

signals:
	void loadFinished(bool ok);
};
#elif defined USE_QWebEngine

#include<QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include "../network_schemes.h"


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

class WebViewer : public QWebEngineView {

Q_OBJECT

public: 
	WebViewer(QWidget* parent = nullptr);
	bool supportsJS() const { return true;}
	
	template <class F>
	void evaluateJS(QString code, F callable) {
		page()->runJavaScript(code, callable);
	}

	bool canGoBack() const { return this->history()->canGoBack(); };
	bool canGoForward() const { return this->history()->canGoForward(); };
	
	bool debug(bool state);
	
signals:
	void linkClicked(const QUrl& url);
	
public slots:
	void backward() { back(); };
private slots:
// 	void reset() {if (adaptive_page) setPage(adaptive_page);}
private:
	AdaptiveWebPage* adaptive_page = nullptr;
	
};
#endif // USE_QTextBrowser


#endif // WEBVIEWER_H
