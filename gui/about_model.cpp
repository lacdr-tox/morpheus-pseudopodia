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
	QLabel* ql1 = new QLabel("exclude-plugins");
	ql1->setAlignment(Qt::AlignRight);
	layset->addWidget(ql1,0,0);
	QLabel* ql2 = new QLabel("exclude-symbols");
	ql2->setAlignment(Qt::AlignRight);
	layset->addWidget(ql2,0,2);
	QLabel* ql3 = new QLabel("reduced");
	ql3->setAlignment(Qt::AlignRight);
	layset->addWidget(ql3,0,4);
	excludeP = new CheckBoxList();
	excludeS = new CheckBoxList();
	//connect(excludeP, SIGNAL(currentTextChanged(QStringList)),this,SLOT());
	connect(excludeS, &CheckBoxList::currentTextChanged,[=](QStringList excludes){update_excludes(excludes);
		update_graph();});
	reduced = new QCheckBox();
	connect(reduced, &QCheckBox::stateChanged, [=](int state){update_reduced(state);
		qDebug()<<"REDUCED CHECKBOX CHANGED";
		update_graph();});
	update_reduced(Qt::Unchecked);
	layset->addWidget(excludeP,0,1);
	layset->addWidget(excludeS,0,3);
	layset->addWidget(reduced,0,5);
	central->addLayout(layset);

// 	auto an = model->rootNodeContr->firstActiveChild("Analysis");
// 	if (!an) an = model->rootNodeContr->insertChild("Analysis");
// 	nodeController* dg = an->firstActiveChild("DependencyGraph");
	auto dg = model->rootNodeContr->find(QStringList() << "Analysis" << "DependencyGraph",true);
	QStringList qsl = dg->attribute("exclude-symbols")->get().split(", ");
	QMap<QString,QString> qm1 = model->rootNodeContr->getModelDescr().getSymbolNames("cpmDoubleSymbolRef");
	qm1.unite(model->rootNodeContr->getModelDescr().getSymbolNames("cpmVectorSymbolRef"));
	for(auto& plug:qm1.keys())
	{
		if(plug != "")
		{
			if(qsl.contains(plug)){
				excludeS->addItem(plug,true);
			}else{
				excludeS->addItem(plug,false);
			}
		}
	}
	webGraph = new WebViewer(this);
	connect(webGraph, &WebViewer::linkClicked, this, &AboutModel::openLink);
	auto frame = new QFrame();
	frame->setLayout(new QBoxLayout(QBoxLayout::Down));
	frame->layout()->addWidget(webGraph);
	frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
	frame->setLineWidth(0);
	frame->setStyleSheet("background-color:white;");
// 	style->setProperty(Style::S)
	central->addWidget(frame,4);
	webGraph->show();
	
	save_btn = new QPushButton(this);
	central->addWidget(save_btn);
	save_btn->setText("save");
	connect(save_btn,SIGNAL(clicked()),this, SLOT(svgOut()));
	save_btn->show();
	lastGraph = "";
};

void AboutModel::update()
{
	qDebug()<<"UPDATE" ;
	
	title->blockSignals(true);
	description->blockSignals(true);
	reduced->blockSignals(true);
	excludeS->blockSignals(true);
	
	title->setText(model->rootNodeContr->getModelDescr().title);
	description->setText(model->rootNodeContr->getModelDescr().details);
	
	auto dg = model->rootNodeContr->find(QStringList() << "Analysis" << "DependencyGraph",true);
	QStringList qsl = dg->attribute("exclude-symbols")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	excludeS->setData(qsl);
	if(dg->attribute("reduced")->get() == "true")
	{
		reduced->setCheckState(Qt::Checked);
	}else{
		reduced->setCheckState(Qt::Unchecked);
	}
	
	title->blockSignals(false);
	description->blockSignals(false);
	reduced->blockSignals(false);
	excludeS->blockSignals(false);
	// at least delay the update until the widget shows up
	QMetaObject::invokeMethod(this,"update_graph",Qt::QueuedConnection);
// 	update_graph();
}

void AboutModel::update_graph()
{
	// save file to tmp folder
	
	QString graph = model->getDependencyGraph();
// 	if (!graph.isEmpty()) {
		//qDebug() << "showing dep_graph: Filename: " << QString(graph);
		if (graph.endsWith("png")) {
			url = (QString("file://") + graph);
			webGraph->setUrl(url);
		} else if (graph.endsWith("svg")) {
			url = (QString("file://") + graph);
			webGraph->setUrl(url);
		} else /*if (graph.endsWith("dot"))*/ {
			QUrl dotgraph("qrc:///template.html");
			QFile dotsource(graph);
			QString toR;
			if (dotsource.exists()) {
				dotsource.open(QIODevice::ReadOnly);
				toR  = dotsource.readAll().replace("\n"," ").replace("\t"," ").trimmed();
			}
			if (toR.isEmpty()) {
				toR = "digraph { compound=true; subgraph cluster{ labelloc=\"t\";label=\"Global\";bgcolor=\"#2341782f\"; empty} }";
// 				toR.replace("\"","\\\"");
			}
			qDebug() << "Updating graph from " << sender()  << "\ncurrent url is " <<webGraph->url().path() << "\n" << toR;
			toR.prepend("transition('").append("');");
			dotsource.close();
			if(webGraph->url() != dotgraph || lastGraph != toR)
			{
				lastGraph= toR;
				qDebug()<<"UPDATE GRAPH";
				if (webGraph->page()->url() != dotgraph) {
					onLoadConnect = connect(webGraph, &WebViewer::loadFinished, [&](bool){ 
						webGraph->evaluateJS(lastGraph, [](const QVariant& r){});
						qDebug() << "Graph loading finished!";
						disconnect(onLoadConnect);
					});
					webGraph->load(dotgraph);
				}
				else {
					webGraph->evaluateJS(toR, [](const QVariant& r){});
				}
				webGraph->show();
			}
		}
// 	}
	// run morpsi on it
	// reload the resulting dependency_graph.png/svg
	// display an error, id something went wrong.
	
}

void AboutModel::openLink(const QUrl& url)
{
	if (url.scheme() == "morph") {
		emit nodeSelected(url.path());
	}
}


void AboutModel::svgOut()
{
	QVariant qv;
	QString fileName = QFileDialog::getSaveFileName(this,tr("Save Image"), "/home", tr("Image Files (*.png *.jpg )"));
	
	webGraph->evaluateJS("retSVG()",  
		[=](const QVariant& r){
			qDebug() << "saving SVG";
			QFile qf(fileName);
			qf.open(QIODevice::WriteOnly);
			QTextStream out(&qf);
			out<<"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
			out<< r.toString()
			       .replace(QRegularExpression("</a>\\n"),"")
			       .replace(QRegularExpression("<a\\s.*?>"),"");
			qf.close();
		} 
	);
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

void AboutModel::update_excludes(QStringList qsl)
{
	auto dg = model->rootNodeContr->find(QStringList() << "Analysis" << "DependencyGraph",true);
	dg->attribute("exclude-symbols")->setActive(true);
	dg->attribute("exclude-symbols")->set(qsl.join(","));
}

void AboutModel::update_reduced(int state)
{
	auto dg = model->rootNodeContr->find(QStringList() << "Analysis" << "DependencyGraph",true);
	dg->attribute("reduced")->setActive(true);
	if (reduced->checkState() != state)
		reduced->setCheckState(static_cast<Qt::CheckState>(state));
		
	if(state == Qt::Checked)
	{
		dg->attribute("reduced")->set("true");
	}else{
		dg->attribute("reduced")->set("false");
	}
}
