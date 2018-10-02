#include "feedback.h"
#include "config.h"
#include "job_queue.h"


void FeedbackRequestWindow::sendFeedBack()
{
	QSettings settings;
	auto app = config::getApplication();
	settings.beginGroup("preferences");
	service_url = settings.value("feedback_url", service_url).toString();
	auto allow_feedback = app.preference_allow_feedback;
	auto uuid = settings.value("uuid", "").toString();
	int notif = settings.value("notif",-1).toInt();
	
	if (uuid=="") {
		uuid = QUuid::createUuid().toString().remove('{').remove('}');
		settings.setValue("uuid", uuid);
	}
	
	bool query_user = false;
	
	if (! allow_feedback) {
		if (notif > 0 && 3.0/( 3.0 + notif) < qrand()) {
			query_user = true;
		} 
		else {
			settings.setValue("notif",notif+1);
		}
	}
	
	if (query_user) {
		// executing the Dialog
		allow_feedback = this->exec() == QDialog::Accepted;
		app.preference_allow_feedback = allow_feedback;
		config::setApplication(app);
		settings.setValue("notif",0);
	}
	
	if (allow_feedback) {
		
		if (!netw) {
			netw = config::getNetwork();
		}
		
// 		QMap<QString, QString> feedback_data;
		data["uuid"]        = uuid;
		data["sim_count"]   = QString::number( config::getJobQueue()->jobCount() );
		data["model_count"] = QString::number( config::getJobQueue()->modelCount() );
		data["version"]     = QString(config::getVersion());
		// TODO QT5
		// data["os"] = QSysInfo::prettyProductName();
#ifdef Q_OS_LINUX
		data["os"] = "linux";
#elif defined Q_OS_MAC
		data["os"] = "mac";
#elif defined Q_OS_WIN
		data["os"] = "win";
#else 
		data["os"] = "other";
#endif
		
		QNetworkRequest request;
		request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
		request.setUrl(QUrl(service_url));
		qDebug() << "Sending feedback to Morpheus @ TU-Dresden";
		
		QString message;
		message += "{\n";
		auto dit = data.constBegin();
		while (dit != data.constEnd()) {
			message +=  QString("  \"%1\" : \"%2\"" ).arg(dit.key()).arg(dit.value());
			
			dit++;
			if (dit == data.constEnd()) {
				message += "\n";
				break;
			}
			message += ",\n";
		}
		message += "}\n";
		
// 		qDebug() << "Message: " << message;
		auto reply = netw->post(request, QByteArray(message.toStdString().c_str()));
		this->connect(reply, SIGNAL(finished()), this, SLOT(feedbackFinished()));
	}
	settings.endGroup();

}


FeedbackRequestWindow::FeedbackRequestWindow(QWidget* parent, Qt::WindowFlags f) : QDialog(parent,f) {
	
	auto central_layout = new QVBoxLayout(this);
	this->setMinimumWidth(425);
	this->setMinimumHeight(300);

	auto text_layout = new QHBoxLayout();
	central_layout->addLayout(text_layout,1);
	
	auto text = new QTextEdit(this);
	text->setReadOnly(true);
	QFile html(":/feedback_request.html");
	html.open(QIODevice::ReadOnly);
	
	text->setHtml(html.readAll());
	text_layout->addWidget(text);
	
	auto button_layout = new QHBoxLayout(this);
	central_layout->addLayout(button_layout);
	button_layout->addStretch(1);
	
	auto no_button  = new QPushButton("Not now");
	no_button->setDefault(false);
	connect(no_button,SIGNAL(clicked()),this, SLOT(reject()));
	button_layout->addWidget(no_button);
	
	button_layout->addSpacing(10);
	auto agree_button = new QPushButton("I agree");
	agree_button->setDefault(true);
	connect(agree_button,SIGNAL(clicked()),this, SLOT(accept()));
	button_layout->addWidget(agree_button);
}


void FeedbackRequestWindow::feedbackFinished() {
	QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
	qDebug() << reply->readAll();
}


AnnouncementDialog::AnnouncementDialog(QWidget* parent)
{
	QSettings settings;
	settings.beginGroup("preferences");
	service_url = settings.value("announcement_url", service_url).toString();
	announcement_seen = settings.value("announcement_seen",-1).toInt();
	settings.endGroup();
	
	auto central_layout = new QVBoxLayout(this);
	this->setMinimumWidth(425);
	this->setMinimumHeight(300);

	
	auto nam = config::getNetwork();
	
#ifdef MORPHEUS_NO_QTWEBKIT
	web_view = new TextBrowser(this);
	web_view->setOpenLinks(false);
	connect(web_view, SIGNAL(anchorClicked(const QUrl&)), this, SLOT(openLink(const QUrl&)));
#else
	web_view = new QWebView(this);
	web_view->page()->setNetworkAccessManager(nam);
	web_view->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
	web_view->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
	web_view->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, false);
	web_view->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, false);
	web_view->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
	connect(web_view, SIGNAL(linkClicked(const QUrl&)),this, SLOT(openLink(const QUrl&)));
#endif
	central_layout->addWidget(web_view);
	
	
	auto button_layout = new QHBoxLayout(this);
	central_layout->addLayout(button_layout);
	button_layout->addSpacing(10);
	button_layout->addStretch(1);
	
	back_button  = new QPushButton("");
	back_button->setIcon(QIcon::fromTheme("go-previous"));
	back_button->setDefault(false);
	connect(back_button,SIGNAL(clicked()),this, SLOT(last()));
	button_layout->addWidget(back_button);
	
	forth_button  = new QPushButton("");
	forth_button->setIcon(QIcon::fromTheme("go-next"));
	forth_button->setDefault(false);
	connect(forth_button,SIGNAL(clicked()),this, SLOT(next()));
	button_layout->addWidget(forth_button);
	
	button_layout->addStretch(1);
	auto ok_button = new QPushButton("Leave");
	ok_button->setDefault(true);
	connect(ok_button,SIGNAL(clicked()),this, SLOT(accept()));
	button_layout->addWidget(ok_button);
	QNetworkRequest request(service_url);
	auto reply = nam->get(request);
	connect(reply,SIGNAL(finished()), this, SLOT(replyReceived()));
}


void AnnouncementDialog::showAnnouncements(bool also_old)
{
	if (this->hasAnnouncements() || also_old)
		this->exec();
}

void AnnouncementDialog::openLink(const QUrl& url)
{
	// Send all link requests to the outside ...
	QDesktopServices::openUrl(url);
}


void AnnouncementDialog::last() {
	setIndex(announce_idx-1);
}

void AnnouncementDialog::next() {
	setIndex(announce_idx+1);
}

void AnnouncementDialog::setIndex(int idx) {
	if (announcements.count(idx)) {
// 		qDebug() << "Setting announcement " << idx << " = " << announcements[idx];
		announce_idx = idx;
		web_view->setUrl(announcements[idx]);

		if (announce_idx > announcement_seen){
			announcement_seen = announce_idx;
			// put it into settings;
			QSettings settings;
			settings.beginGroup("preferences");
			settings.setValue("announcement_seen",announcement_seen);
			settings.endGroup();
		}
	}
	forth_button->setEnabled(announcements.count(announce_idx+1));
	back_button->setEnabled(announcements.count(announce_idx-1));
}

bool AnnouncementDialog::hasAnnouncements()
{
	return have_new_announcements;
}


void AnnouncementDialog::replyReceived() 
{
	if (announcements.isEmpty()) {
		QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
		if (reply->error() != QNetworkReply::NoError) {
			qDebug() << "Could not fetch current announcements";
			announcements[0] = "qrc:///no_announcement.html";
			setIndex(0);
		}
		else {
			// TODO QT5 use QJsonDocument
			// For now, we know how the document looks like :-))
			// [{"id":1,"url":"imc.zih.tu-dresden.de/morpheus/announcement0.html"}]
			QRegExp json_match("\"id\":(\\d+),\"url\":\"(.+)\"");
			json_match.setMinimal(true);
			QString data = reply->readAll();
			int pos = 0;
			while ((pos = json_match.indexIn(data,pos)) !=-1) {
				auto res = json_match.capturedTexts();
				announcements[res[1].toInt()] = res[2];
				pos += json_match.matchedLength();
// 				qDebug() << res[1] << " -> " << res[2];
			}
			
			if ( announcements.isEmpty() ) {
				announcements[0] = "qrc:///no_announcement.html";
				setIndex(0);
			} else {
				auto it = announcements.lowerBound(announcement_seen);
				if (it == announcements.end()) {
					have_new_announcements = false;
					setIndex((it--).key());
				}
				else if (it+1 == announcements.end()) {
					have_new_announcements = false;
					setIndex(it.key());
				}
				else {
					have_new_announcements = true;
					setIndex((it++).key());
				}
			}
		}
	}
}
