#include "announcement.h"
#include "config.h"
#include "job_queue.h"



AnnouncementDialog::AnnouncementDialog(QWidget* parent)
{
	QSettings settings;
	settings.beginGroup("preferences");
	service_url = settings.value("announcement_url", service_url).toString();
	announcement_seen = settings.value("announcement_seen",2).toInt();
	uuid = settings.value("uuid", "").toString();
	if (uuid.isEmpty()) {
		uuid = QUuid::createUuid().toString().remove('{').remove('}');
		settings.setValue("uuid", uuid);
	}
	settings.endGroup();
	
	auto central_layout = new QVBoxLayout();
	this->setLayout(central_layout);
	this->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
	this->setMinimumWidth(600);
	this->setMinimumHeight(500);

	web_view = new WebViewer(this);
	connect(web_view, SIGNAL(linkClicked(const QUrl&)), this, SLOT(openLink(const QUrl&)));
	central_layout->addWidget(web_view);
	
	auto button_layout = new QHBoxLayout();
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
	if (announcements.contains(idx)) {
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
		auto json_announcements = QJsonDocument::fromJson(reply->readAll());
#if QT_VERSION >= 0x050600
		auto my_version = QVersionNumber::fromString(config::getVersion());
#else
		auto my_version = config::getVersion();
#endif
		for ( const auto& a : json_announcements.array() ) {
			auto ao = a.toObject();
			announcements[ao["id"].toInt()] = ao["url"].toString();
			if (ao.contains("release")) {
#if QT_VERSION >= 0x050600				
				if (QVersionNumber::fromString(ao["release"].toString()) <= my_version) {
#else
				if (ao["release"].toString() == my_version) {
#endif
					announcement_seen = max(ao["id"].toInt(), announcement_seen );
				}
			}
		};

		if ( announcements.isEmpty() ) {
			announcements[0] = "qrc:///no_announcement.html";
			setIndex(0);
		} else {
			auto it = announcements.lowerBound(announcement_seen);
			if (it.key() == announcement_seen) it++;
			if (it == announcements.end() ) {
				have_new_announcements = false;
				setIndex(announcements.count()-1);
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
