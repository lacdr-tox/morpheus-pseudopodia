#include <QtGui>
#include <QNetworkReply>

#ifdef MORPHEUS_NO_QTWEBKIT
#include <QTextBrowser>
#warning Compiling without QtWebKit
#else 
#include <QtWebKit/QWebView>
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
	
#ifdef MORPHEUS_NO_QTWEBKIT
	QTextBrowser* web_view;
#else
	QWebView* web_view;
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
