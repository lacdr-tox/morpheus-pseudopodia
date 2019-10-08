#include "announcement.h"
#include "config.h"
#include "job_queue.h"



AnnouncementDialog::AnnouncementDialog(QWidget* parent)
{
	QSettings settings;
	settings.beginGroup("preferences");
	service_url = settings.value("announcement_url", service_url).toString();
	announcement_seen = settings.value("announcement_seen",-1).toInt();
	uuid = settings.value("uuid", "").toString();
	if (uuid.isEmpty()) {
		uuid = QUuid::createUuid().toString().remove('{').remove('}');
		settings.setValue("uuid", uuid);
	}
	settings.endGroup();
	
	auto central_layout = new QVBoxLayout(this);
	this->setMinimumWidth(425);
	this->setMinimumHeight(300);

	web_view = new WebViewer(this);
	connect(web_view, SIGNAL(linkClicked(const QUrl&)), this, SLOT(openLink(const QUrl&)));
	central_layout->addWidget(web_view);
	
	auto button_layout = new QHBoxLayout(this);
	central_layout->addLayout(button_layout);
	button_layout->addSpacing(10);
	button_layout->addStretch(1);
	
	back_button  = new QPushButton("");
	back_button->setIcon(QIcon::fromTheme("go-previous", QIcon(":/go-previous.png")));
	back_button->setDefault(false);
	connect(back_button,SIGNAL(clicked()),this, SLOT(last()));
	button_layout->addWidget(back_button);
	
	forth_button  = new QPushButton("");
	forth_button->setIcon(QIcon::fromTheme("go-next",QIcon(":/go-next.png")));
	forth_button->setDefault(false);
	connect(forth_button,SIGNAL(clicked()),this, SLOT(next()));
	button_layout->addWidget(forth_button);
	
	button_layout->addStretch(1);
	auto ok_button = new QPushButton("Close");
	ok_button->setDefault(true);
	connect(ok_button,SIGNAL(clicked()),this, SLOT(accept()));
	button_layout->addWidget(ok_button);

}

void AnnouncementDialog::check()
{
	QNetworkRequest request(service_url+"?uuid="+uuid);
	auto reply = config::getNetwork()->get(request);
	connect(reply,SIGNAL(finished()), this, SLOT(replyReceived()));
}

void AnnouncementDialog::showAnnouncements(bool also_old)
{
	this->show_old_announcements = also_old;
	this->check();
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
	else {
		 qDebug() << "AnnouncementDialog::setIndex:  There is no index " << idx;
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
// 				qDebug() << "All announcements seen";
				have_new_announcements = false;
				setIndex(announcements.count()-1);
			}
			else if (it+1 == announcements.end()) {
// 				qDebug() << "All but one announcements seen";
				have_new_announcements = false;
				setIndex(it.key());
			}
			else {
				have_new_announcements = true;
				setIndex((it++).key());
			}
		}
	}
	
	
	if ( ! announcements.isEmpty() && (have_new_announcements || show_old_announcements)) {
		this->exec();
	}
}
