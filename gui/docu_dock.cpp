#include "docu_dock.h"

#ifndef MORPHEUS_NO_QTWEBKIT
#include <QWebHistory>
#endif

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QThread>    

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};

class HelpBrowser : public QTextBrowser {
public:
	QVariant loadResource(int type, const QUrl & name) override;
	void setNetworkAccessManager(QNetworkAccessManager* nam) { this->nam = nam; };
private:
	QNetworkAccessManager* nam;
};

QVariant HelpBrowser::loadResource(int type, const QUrl & name) {
	QNetworkRequest request(name);
// 	qDebug() << "Requesting " << name;
	auto reply = nam->get(request);
	return reply->readAll();
}

DocuDock::DocuDock(QWidget* parent) : QDockWidget("Documentation", parent)
{
	timer = NULL;
	help_engine = config::getHelpEngine();
	connect(help_engine,SIGNAL(setupFinished()),this,SLOT(setRootOfHelpIndex()));
	
	hnam = config::getNetwork();

#ifdef MORPHEUS_NO_QTWEBKIT
	auto realViewer = new HelpBrowser();
	realViewer->setNetworkAccessManager(hnam);
	help_view = realViewer;
#else
	help_view = new QWebView();
	
	help_view->page()->setNetworkAccessManager(hnam);
// 	help_view->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	help_view->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
	help_view->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
	help_view->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);
	help_view->page()->setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
	connect(help_view, SIGNAL(linkClicked(const QUrl&)),this, SLOT(openHelpLink(const QUrl&)));
#endif
	
	toc_widget = help_engine->contentWidget();
	toc_widget->setRootIsDecorated(true);
	toc_model = new QSortFilterProxyModel(toc_widget);
	toc_model->setSourceModel(help_engine->contentModel());
	toc_widget->setModel(toc_model);
	toc_widget->setSortingEnabled(true);
	toc_widget->sortByColumn(0,Qt::AscendingOrder);
	
	root_reset = false;
	
	auto tb = new QToolBar();

	b_back = new QAction(QThemedIcon("go-previous", style()->standardIcon(QStyle::SP_ArrowLeft)),"Back",this);
	tb->addAction(b_back);
	b_back->setEnabled(false);

	b_forward = new QAction(QThemedIcon("go-next", style()->standardIcon(QStyle::SP_ArrowRight)),"Fwd",this);
	tb->addAction(b_forward);
	b_forward->setEnabled(false);
	
	tb->addSeparator();
	
	QPixmap pm(":/morpheus.png");
	auto l_icon = new QLabel();
	l_icon->setPixmap(pm.scaled(25,25,Qt::KeepAspectRatio,Qt::SmoothTransformation));
	tb->addWidget(l_icon);
	auto l_doc = new QLineEdit(" Morpheus 2.1 Documentation");
	l_doc->setEnabled(false);
	tb->addWidget(l_doc);
	
	auto vl = new QVBoxLayout();
	vl->setSpacing(0);
	vl->addWidget(tb);
	
	
	vl->addWidget(help_view);
	auto w_bottom = new QWidget();
	w_bottom->setLayout(vl);
	
	splitter = new QSplitter(Qt::Horizontal, this);
	splitter->addWidget(toc_widget);
	splitter->addWidget(w_bottom);
	
	help_view->show();

#ifdef MORPHEUS_NO_QTWEBKIT
	connect(b_back, SIGNAL(triggered()), help_view, SLOT(backward()));
	connect(b_forward, SIGNAL(triggered()), help_view, SLOT(forward()));
	connect(help_view, SIGNAL(backwardAvailable(bool)), b_back, SLOT(setEnabled(bool)) );
	connect(help_view, SIGNAL(forwardAvailable(bool)), b_forward, SLOT(setEnabled(bool)) );
// 	connect(help_view, SIGNAL(historyChanged(const QUrl&)), this, SLOT(resetStatus()) );
#else
	connect(b_back, SIGNAL(triggered()), help_view, SLOT(back()));
	connect(b_forward, SIGNAL(triggered()), help_view, SLOT(forward()));
	connect(help_view, SIGNAL(urlChanged(const QUrl&)), this, SLOT(resetStatus()) );
#endif
	connect(toc_widget, SIGNAL(clicked(const QModelIndex&)), this, SLOT(setCurrentIndex(const QModelIndex&)) );
	
	this->setWidget(splitter);
	
	help_engine->setupData();
	resetStatus();
}

void DocuDock::openHelpLink(const QUrl& url) {
	if (url.scheme() == "qthelp") {
#ifdef MORPHEUS_NO_QTWEBKIT
		help_view->setSource(url);
#else
		help_view->setUrl(url);
#endif
	}
	else 
		QDesktopServices::openUrl(url);
}

void DocuDock::setCurrentElement(QStringList xPath)
{
// 	qDebug() << xPath;
	auto key_index = MorpheusML_index;
	QString help_key = "MorpheusML";
	xPath.pop_front();
	
	while ( !xPath.isEmpty() ) {
		// try to find the key in the current key_index
		QString key = xPath.front();
		int rows = toc_model->rowCount(key_index);
		for (int r=0; ; r++) {
			// No suitable tag found
			if (r>=rows) {
				xPath.clear();
				break;
			}
			if ( key_index.child(r,0).data() == xPath.front() ) {
				help_key = xPath.front();
				xPath.pop_front();
				key_index = key_index.child(r,0);
				break;
			}
		}
	}
	
	setCurrentElement(help_key);
	toc_widget->setCurrentIndex(key_index);
}


void DocuDock::setCurrentElement(QString name) {
	QMap <QString, QUrl > identifiers = help_engine->linksForIdentifier(name);
// 	qDebug() << "Searching for help for " << name;
	if (!identifiers.empty()) {
		setCurrentURL(identifiers.begin().value());
	}
}

void DocuDock::setCurrentIndex(const QModelIndex& idx)
{
	setCurrentElement(idx.data(Qt::DisplayRole).toString());
}



void DocuDock::setCurrentURL(const QUrl& url) {
#ifdef MORPHEUS_NO_QTWEBKIT
	help_view->setSource(url);
#else
	if (help_view->url() != url) {
		help_view->setUrl(url);
// 		qDebug() << url;
	}
#endif
}


void DocuDock::resetStatus() {
#ifdef MORPHEUS_NO_QTWEBKIT
	b_back->setEnabled(help_view->isBackwardAvailable());
	b_back->setEnabled(help_view->isForwardAvailable());
#else
	b_back->setEnabled(help_view->history()->canGoBack());
	b_forward->setEnabled(help_view->history()->canGoForward());
#endif
}

void DocuDock::resizeEvent(QResizeEvent* event)
{
	if (double(event->size().height()) / double(event->size().width()) > 1) {
		if (splitter->orientation() != Qt::Vertical)
			splitter->setOrientation(Qt::Vertical);
	}
	else {
		if (splitter->orientation() != Qt::Horizontal)
			splitter->setOrientation(Qt::Horizontal);
	}
	QDockWidget::resizeEvent(event);
	
}

void DocuDock::setRootOfHelpIndex()
{
	auto help_model =  help_engine->contentModel();
	if (help_model->isCreatingContents()) {
		if ( ! timer) {
			timer = new QTimer(this);
			timer->setSingleShot(true);
			connect(timer,SIGNAL(timeout()),this, SLOT(setRootOfHelpIndex()));
		}
		timer->start(500);
		return;
	}
	
	QModelIndex root = toc_model->index(0,0);
	int rows = toc_model->rowCount(root);
// 	qDebug() << "I am getting the Docu " <<root_rows ;
	int modules_row = -1;
	for (uint row=0; row<rows; row++) {
// 		qDebug() << row <<  model->data(model->index(row,0,root),0);
		 if ( root.child(row,0).data(Qt::DisplayRole) == "Modules" ) {
			 modules_index =  root.child(row,0);
			 modules_row = row;
		 }
	}
	
	rows = toc_model->rowCount(modules_index);
	for (uint row=0; row<rows; row++) {
		if ( modules_index.child(row,0).data(Qt::DisplayRole) == "MorpheusML" ) {
			 MorpheusML_index =  modules_index.child(row,0);
		 }
	}
	
	if (modules_row>=0) {
		toc_widget->setRootIndex(toc_model->index(modules_row,0,root));
		toc_widget->setExpanded(toc_model->index(modules_row,0,root),true);
		root_reset = true;
	}
}

