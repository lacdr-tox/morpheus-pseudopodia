#ifndef FEEDBACK_H
#define FEEDBACK_H

#include <QtGui>
#include <QDialog>
#include <QLayout>
#include <QPushButton>
#include <QNetworkReply>

#ifdef USE_QTextBrowser
#include <QTextBrowser>
#warning Compiling without QtWebKit
#elif defined USE_QWebKit
#include <QWebView>
#elif defined USE_QWebEngine
#include <QWebEngineView>
#endif


class AnnouncementDialog : public QDialog {
	Q_OBJECT
	QString service_url = "https://imc.zih.tu-dresden.de/morpheus/service/announcements";
	QString uuid;
	int announce_idx;
	int announcement_seen;
	QMap<int, QString> announcements;
	QPushButton *forth_button, * back_button;
	bool have_new_announcements = false;
	bool show_old_announcements = false;
	
#ifdef USE_QTextBrowser
	QTextBrowser* web_view;
#elif defined USE_QWebKit
	QWebView* web_view;
#elif defined USE_QWebEngine
	QWebEngineView* web_view;
#endif
	void setIndex(int idx);
	void showAnnouncements(bool also_old);
	void check();
	public:
		  AnnouncementDialog(QWidget* parent);
		  bool hasAnnouncements();
	
	public slots:
		void showAnnouncements() { showAnnouncements(false); };
		void showAllAnnouncements() { showAnnouncements(true); }
	private slots:
		void next();
		void last();
		void openLink(const QUrl & url);
		void replyReceived();
};

#endif
