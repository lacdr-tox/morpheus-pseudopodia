#include "docu_dock.h"
#include <QThread>    

class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs){QThread::usleep(usecs);}
    static void msleep(unsigned long msecs){QThread::msleep(msecs);}
    static void sleep(unsigned long secs){QThread::sleep(secs);}
};


DocuDock::DocuDock(QWidget* parent) : QDockWidget("Documentation", parent)
{
	timer = NULL;
	help_engine = config::getHelpEngine();

	help_view = new WebViewer(this);
	connect(help_view, SIGNAL(linkClicked(const QUrl&)),this, SLOT(openHelpLink(const QUrl&)));
	
	toc_widget = help_engine->contentWidget();
	toc_widget->setRootIsDecorated(true);
	toc_model = new QSortFilterProxyModel(toc_widget);
	toc_model->setSourceModel(help_engine->contentModel());
	toc_widget->setModel(toc_model);
	toc_widget->setSortingEnabled(true);
	toc_widget->sortByColumn(0,Qt::AscendingOrder);
	
	root_reset = false;
	element_on_reset = "MorpheusML";
	
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
	label_documentation = new QLineEdit(" Loading Documentation ...");
	label_documentation->setEnabled(false);
	tb->addWidget(label_documentation);
	
	auto vl = new QVBoxLayout();
	vl->setSpacing(0);
	vl->addWidget(tb);
	
	
	vl->addWidget(help_view);
	auto w_bottom = new QWidget();
	w_bottom->setLayout(vl);
	
	splitter = new QSplitter(Qt::Horizontal, this);
	splitter->addWidget(toc_widget);
	splitter->addWidget(w_bottom);
	splitter->setStretchFactor(1,4);
	

	connect(b_back, SIGNAL(triggered()), help_view, SLOT(back()));
	connect(b_forward, SIGNAL(triggered()), help_view, SLOT(forward()));
	connect(help_view, SIGNAL(urlChanged(const QUrl&)), this, SLOT(resetStatus()) );
	
	connect(toc_widget, SIGNAL(clicked(const QModelIndex&)), this, SLOT(setCurrentIndex(const QModelIndex&)), Qt::QueuedConnection );
	
	this->setWidget(splitter);
	
	connect(help_engine->contentModel(), SIGNAL(contentsCreated()),this,SLOT(setRootOfHelpIndex()));

	resetStatus();
	help_view->show();
// 	if (help_engine->setupData() == false) {
// 		qDebug() << "Help engine setup failed";
// 	}
	
}

void DocuDock::openHelpLink(const QUrl& url) {
	if (url.scheme() == "qthelp") {
		help_view->setUrl(url);
	}
	else 
		QDesktopServices::openUrl(url);
}

void DocuDock::setCurrentElement(QStringList xPath)
{
	qDebug() << xPath;
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
	if (!root_reset) {
		qDebug() << "Deferring docu element selection " << name;
		element_on_reset = name;
		return;
	}
	QMap <QString, QUrl > identifiers = help_engine->linksForIdentifier(name);
// 	qDebug() << "Searching help for " << name;
	if (!identifiers.empty()) {
		setCurrentURL(identifiers.begin().value());
	}
	else {
		qDebug() << "No help for " << name;
	}
}

void DocuDock::setCurrentIndex(const QModelIndex& idx)
{
	setCurrentURL( help_engine->contentModel()->contentItemAt(toc_model->mapToSource(idx))->url());
}



void DocuDock::setCurrentURL(const QUrl& url) {
	if (help_view->url() != url) {
		help_view->setUrl(url);
		qDebug() << "Setting Docu"<< url;
	}
	else 
		qDebug() << "Docu"<< url << "already set";
	
}


void DocuDock::resetStatus() {
	b_back->setEnabled(help_view->canGoBack());
	b_forward->setEnabled(help_view->canGoForward());
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
	QModelIndex root = toc_model->index(0,0);
	label_documentation->setText(root.data().toString() + " Documentation");
	int rows = toc_model->rowCount(root);
	qDebug() << help_engine->error();
	qDebug() << "I am getting the Docu " << rows ;
	int modules_row = -1;
	for (uint row=0; row<rows; row++) {
// 		qDebug() << "Checking help section " << row <<  root.child(row,0).data(Qt::DisplayRole);
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
		
// 		qDebug() << "setting deferred docu element " << element_on_reset;
// 		setCurrentElement(element_on_reset);

		if ( ! timer) {
			timer = new QTimer(this);
			timer->setSingleShot(true);
			connect(timer, &QTimer::timeout, [this]{ 
				qDebug() << "setting deferred docu element " << element_on_reset;
				this->setCurrentElement(element_on_reset); 
			} );
		}
		timer->start(300);

	}
}

