#include "docu_dock.h"

#include "QtNetwork/QNetworkAccessManager"
#include "QtNetwork/QNetworkReply"

class HelpNetworkReply : public QNetworkReply
{
        public:
                HelpNetworkReply(const QNetworkRequest &request, const QByteArray &fileData, const QString &mimeType);
                
                virtual void abort() override {}
                virtual qint64 bytesAvailable() const override { return data.length() + QNetworkReply::bytesAvailable(); }
                
        protected:
                virtual qint64 readData(char *data, qint64 maxlen) override;
                
        private:
                QByteArray data;
                qint64 origLen;
};
 

class HelpNetworkAccessManager : public QNetworkAccessManager
{
        public:
                HelpNetworkAccessManager(QHelpEngineCore *engine, QObject *parent)
                        : QNetworkAccessManager(parent), m_helpEngine(engine) {}
 
        protected:
                virtual QNetworkReply *createRequest(Operation op,
                        const QNetworkRequest &request, QIODevice *outgoingData = 0) override;
 
        private:
                QHelpEngineCore *m_helpEngine;
};



HelpNetworkReply::HelpNetworkReply(const QNetworkRequest &request,
        const QByteArray &fileData, const QString &mimeType)
    : data(fileData), origLen(fileData.length())
{
    setRequest(request);
    setOpenMode(QIODevice::ReadOnly);
 
    setHeader(QNetworkRequest::ContentTypeHeader, mimeType);
    setHeader(QNetworkRequest::ContentLengthHeader, QByteArray::number(origLen));
    QTimer::singleShot(0, this, SIGNAL(metaDataChanged()));
    QTimer::singleShot(0, this, SIGNAL(readyRead()));
}
 
qint64 HelpNetworkReply::readData(char *buffer, qint64 maxlen)
{
        qint64 len = qMin(qint64(data.length()), maxlen);
        if (len) {
                memcpy(buffer, data.constData(), len);
                data.remove(0, len);
        }
        if (!data.length())
                QTimer::singleShot(0, this, SIGNAL(finished()));
        return len;
}
 
QNetworkReply *HelpNetworkAccessManager::createRequest(Operation op, const QNetworkRequest &request, QIODevice *outgoingData)
{
// 	qDebug() << "Creating Help File Request for URL " << request.url();
	QString scheme = request.url().scheme();
	if (scheme == QLatin1String("qthelp") || scheme == QLatin1String("about")) {
			QString mimeType/* = QMimeDatabase().mimeTypeForUrl(request.url()).name()*/;
			if (request.url().path().endsWith(".png")) 
				mimeType = "image/png";
			else if (request.url().path().endsWith(".html"))
				mimeType = "text/html";
			else if (request.url().path().endsWith(".js"))
				mimeType = "text/javascript";
			else if (request.url().path().endsWith(".css"))
				mimeType = "text/css";
			auto data = m_helpEngine->fileData(request.url());
			if (data.isEmpty())
				qDebug() << "Empty URL" << request.url().path() << " / " << mimeType << " of " << data.size() << " bytes";
			return new HelpNetworkReply(request, data , mimeType);
	}
	else {
		QDesktopServices::openUrl(request.url());
		return new HelpNetworkReply(request, QByteArray() , "text/html");
	}
	return QNetworkAccessManager::createRequest(op, request, outgoingData);
}


DocuDock::DocuDock(QWidget* parent) : QDockWidget("Documentation", parent)
{
	timer = NULL;
	splitter = new QSplitter(Qt::Horizontal, this);
	help_engine = config::getHelpEngine();
	
	connect(help_engine,SIGNAL(setupFinished()),this,SLOT(setRootOfHelpIndex()));
	help_engine->setupData();
	
	HelpNetworkAccessManager* hnam = new HelpNetworkAccessManager(help_engine,this);
	help_view = new QWebView();
	
	help_view->page()->setNetworkAccessManager(hnam);
// 	help_view->page()->settings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);
	help_view->page()->settings()->setAttribute(QWebSettings::JavascriptEnabled, true);
	help_view->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessRemoteUrls, false);
	help_view->page()->settings()->setAttribute(QWebSettings::LocalContentCanAccessFileUrls, true);

	splitter->addWidget(help_engine->contentWidget());
	help_engine->contentWidget()->setRootIsDecorated(true);
	root_reset = false;
	splitter->addWidget(help_view);
	help_view->show();
	connect(help_engine->contentWidget(),SIGNAL(linkActivated(const QUrl&)), this, SLOT(setCurrentURL(const QUrl&)));
	
	this->setWidget(splitter);
	
}


void DocuDock::setCurrentNode(nodeController* node)
{
	QMap <QString, QUrl > identifiers;
	while (node && identifiers.empty()) {
		identifiers = help_engine->linksForIdentifier(node->getName());
		if (! identifiers.empty()) {
			setCurrentURL(identifiers.begin().value());
			break;
		}
		node = node->getParent();
	}
}


void DocuDock::setCurrentElement(QString name) {
	QMap <QString, QUrl > identifiers = help_engine->linksForIdentifier(name);
// 	qDebug() << "Searching for help for " << name;
	if (!identifiers.empty()) {
		setCurrentURL(identifiers.begin().value());
	}
}


void DocuDock::setCurrentURL(const QUrl& url) {
	if (help_view->url() != url) {
		help_view->setUrl(url);
// 		qDebug() << url;
	}
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
	QHelpContentWidget* help_index = help_engine->contentWidget();
	QHelpContentModel* model = help_engine->contentModel();
	if (model->isCreatingContents()) {
		if ( ! timer) {
			timer = new QTimer(this);
			timer->setSingleShot(true);
			connect(timer,SIGNAL(timeout()),this, SLOT(setRootOfHelpIndex()));
		}
		timer->start(500);
		return;
	}
	QModelIndex root = model->index(0,0);
	int root_rows = model->rowCount(root);
// 	qDebug() << "I am getting the Docu " <<root_rows ;
	int modules_row = -1;
	for (uint row=0; row<root_rows; row++) {
// 		qDebug() << row <<  model->data(model->index(row,0,root),0);
		 if ( model->data(model->index(row,0,root),0).toString() == "Modules" )
			 modules_row = row;
	}
	
	if (modules_row>=0) {
		help_index->setRootIndex(model->index(modules_row,0,root));
		help_index->setExpanded(model->index(modules_row,0,root),true);
		
		root_reset = true;
	}
}

