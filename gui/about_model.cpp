#include "about_model.h"
AboutModel::AboutModel(SharedMorphModel model, QWidget* parent) : QWidget(parent)
{
	this->model = model;
	QBoxLayout *central = new QBoxLayout(QBoxLayout::Down);
	this->setLayout(central);
	
	auto l_title = new QLabel(this);
	l_title->setText("Title: ");
	central->addWidget(l_title);
	
	title = new QLineEdit(this);
	title->setText(model->rootNodeContr->getModelDescr().title);
	connect(title,SIGNAL(textChanged(QString)),this, SLOT(assignTitle(QString)));
	central->addWidget(title);
	
	auto l_descr = new QLabel(this);
	l_descr->setText("Description:");
	central->addWidget(l_descr);
	
	description = new QTextEdit(this);
	description->setReadOnly(false);
	description->setText(model->rootNodeContr->getModelDescr().details);
	connect(description,SIGNAL(textChanged()),this, SLOT(assignDescription()));
	central->addWidget(description);
	
	layset= new QGridLayout(this);
	layset->addWidget(new QLabel("exclude-plugins"),0,0);
	layset->addWidget(new QLabel("exclude-symbols"),1,0);
	layset->addWidget(new QLabel("reduced"),2,0);
	layset->addWidget(new QComboBox(),0,1);
	layset->addWidget(new QComboBox(),1,1);
	QComboBox* qcb1 = new QComboBox();
	qcb1->addItems({"true","false"});
	layset->addWidget(qcb1,2,1);
	central->addLayout(layset);
	
	webGraph = new QWebView(this);
	central->addWidget(webGraph);
	
	save_btn = new QPushButton(this);
	central->addWidget(save_btn);
	save_btn->setText("save");
	connect(save_btn,SIGNAL(clicked()),this, SLOT(svgOut()));
	save_btn->show();
};

void AboutModel::update()
{
	title->blockSignals(true);
	title->setText(model->rootNodeContr->getModelDescr().title);
	title->blockSignals(false);
	description->blockSignals(true);
	description->setText(model->rootNodeContr->getModelDescr().details);
	description->blockSignals(false);
	
	// at least delay the update until the widget shows up
	QMetaObject::invokeMethod(this,"update_graph",Qt::QueuedConnection);
}

void AboutModel::update_graph()
{
	// save file to tmp folder
	QString graph = model->getDependencyGraph();
	if (!graph.isEmpty()) {
		qDebug() << "showing dep_graph: Filename: " << QString(graph);
		if (graph.endsWith("png")) {
			url = (QString("file://") + graph);
			webGraph->setUrl(url);
		} else if (graph.endsWith("svg")) {
			url = (QString("file://") + graph);
			webGraph->setUrl(url);
		} else if (graph.endsWith("dot")){
			QFile dotgraph(":/template.html");
			QFile dotsource(graph);
// 			QWebView qwv_tmp(dep_graph);
			QByteArray dotHTML;
			if (dotgraph.open(QIODevice::ReadOnly))
			{
				QByteArray dotContent = dotgraph.readAll();
				dotsource.open(QIODevice::ReadOnly);
				dotContent.replace("@@template@@",dotsource.readAll().replace("\n","\\\n"));
				//dotContent.replace("\\\"","\"");
				std::cout << dotContent.toStdString();
				dotHTML = dotContent;
				dotsource.close();
				dotgraph.close();
			}
			else {
				qDebug() << "Cannot open graph template";
			}
			
			webGraph->setHtml(dotHTML);
			
		}
		webGraph->show();
	}
	// run morpsi on it
	// reload the resulting dependency_graph.png/svg
	// display an error, id something went wrong.
	
}

void AboutModel::svgOut()
{
	QVariant qv;
	QString fileName = QFileDialog::getSaveFileName(this,"/home","");
	QFile qf(fileName);
	qf.open(QIODevice::WriteOnly);
	QTextStream out(&qf);
	qv = webGraph->page()->mainFrame()->evaluateJavaScript("retSVG()");
	out<<"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
	out<<qv.toString().replace(QRegularExpression("</a>\\n"),"").replace(QRegularExpression("<a\\s.*?>"),"");
	qf.close();
}


void AboutModel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}


void AboutModel::assignTitle(QString title)
{
	nodeController *node = model->rootNodeContr->find(QStringList() << "Description" << "Title");
	if (node)
		node->setText(title);
}

void AboutModel::assignDescription()
{
	nodeController *node = model->rootNodeContr->find(QStringList() << "Description" << "Details");
	if (node)
		node->setText(description->toPlainText());
}



