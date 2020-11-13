#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include <QString>
#include <QUrl>
#include <QCommandLineOption>

#ifdef USE_QTextBrowser

#include <QTextBrowser>
#include <QNetworkAccessManager>
	
class WebViewer : public QTextBrowser {

Q_OBJECT

public:
	WebViewer(QWidget* parent = nullptr);
	void setUrl(const QUrl& url) { setSource(url); }
	QUrl url() { return source(); }
	bool supportsJS() const { return false;}
	bool supportsSVG() const { return false;}
	
	void reset() {};
	template <class F>
	void evaluateJS(QString code, F callable) {
		/* drop */ 
		QVariant r;
		callable(r);
	}
	
	bool canGoBack() const { return this->isBackwardAvailable(); };
	bool canGoForward() const { return this->isForwardAvailable(); };
	
	bool debug(bool state) { return false; };
	
	QVariant loadResource(int type, const QUrl &name) override;
	static QList<QCommandLineOption> commandLineOptions() { return QList<QCommandLineOption>(); };

signals:
	void linkClicked(const QUrl& url);
	void urlChanged();
	void loadFinished(bool ok);

public slots:
	void back() { backward(); };
	
private:
	QUrl current_url;
	QNetworkAccessManager* nam;
	
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
	bool supportsSVG() const { return true;}
	
	void reset() {};
	template <class F>
	void evaluateJS(QString code, F callable) {
		auto ret = page()->mainFrame()->evaluateJavaScript(code);
		callable(ret);
	}
	
	void setUrl(const QUrl& url) {
		QWebView::setUrl(url);
		emit loadFinished(true);
	}
	
	bool canGoBack() const { return this->history()->canGoBack(); };
	bool canGoForward() const { return this->history()->ca nGoForward(); };
	
	bool debug(bool state);
	
	static QList<QCommandLineOption> commandLineOptions() { return QList<QCommandLineOption>(); };

signals:
	void loadFinished(bool ok);
	
protected:
	void wheelEvent(QWheelEvent *event) override;
	bool event(QEvent * event) {
		// Intercept the creation of ToolTips
		if (event->type() == QEvent::ToolTip) {
			return true;
		}
		return QWebView::event(event);
	}
	
};
#elif defined USE_QWebEngine

#include<QWebEngineView>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QPointer>
#include <QOpenGLWidget>
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
	bool supportsSVG() const { return true;}
	
	void reset();
	bool event(QEvent* e) override;
	template <class F>
	void evaluateJS(QString code, F callable) {
		page()->runJavaScript(code, callable);
	}

	bool canGoBack() const { return this->history()->canGoBack(); };
	bool canGoForward() const { return this->history()->canGoForward(); };
	
	bool debug(bool state);
	
	static QList<QCommandLineOption> commandLineOptions();
	
signals:
	void linkClicked(const QUrl& url);
	
public slots:
// 	void backward() { back(); };
private slots:
// 	void reset() {if (adaptive_page) setPage(adaptive_page);}
protected:
	bool eventFilter(QObject *obj, QEvent *ev) override;
private:
	AdaptiveWebPage* adaptive_page = nullptr;
	QPointer<QWidget> child_;
	
};
#endif // USE_QTextBrowser


// class ZoomFilter: public QObject
// {
//     Q_OBJECT
//     ZoomFilter(WebViewer* view);
// 
// protected:
//     bool eventFilter(QObject *obj, QEvent *event) override;
// private:
// 	WebViewer* view;
// };


#endif // WEBVIEWER_H
