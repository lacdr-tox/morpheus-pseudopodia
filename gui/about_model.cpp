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
	title->setText(model->getModelDescr().title);
	connect(title,SIGNAL(textChanged(QString)),this, SLOT(assignTitle(QString)));
	central->addWidget(title);
	
	auto l_descr = new QLabel(this);
	l_descr->setText("Description:");
	central->addWidget(l_descr);
	
	description = new QTextEdit(this);
	description->setReadOnly(false);
	description->setText(model->getModelDescr().details);
	connect(description,SIGNAL(textChanged()),this, SLOT(assignDescription()));
	central->addWidget(description);
	
	auto layset= new QBoxLayout(QBoxLayout::Direction::LeftToRight);
	auto ql_plugins = new QLabel("exclude-plugins");
	ql_plugins->setAlignment(Qt::AlignRight);
	auto ql_symbols = new QLabel("exclude-symbols");
	ql_symbols->setAlignment(Qt::AlignRight);
	auto ql_reduced = new QLabel("&reduced");
	ql_reduced->setAlignment(Qt::AlignLeft);

	excludeP = new CheckBoxList();
	connect(excludeP, &CheckBoxList::currentTextChanged,[=](QStringList excludes){update_plugin_excludes(excludes);update_graph();});

	excludeS = new CheckBoxList();
	connect(excludeS, &CheckBoxList::currentTextChanged, [=](QStringList excludes){update_excludes(excludes);update_graph();} );

	reduced = new QCheckBox();
	connect(reduced, &QCheckBox::stateChanged, [=](int state){update_reduced(state);update_graph();} );
	update_reduced(Qt::Unchecked);
	ql_reduced->setBuddy(reduced);

	save_btn = new QPushButton(this);
	save_btn->setText("save");
	connect(save_btn,SIGNAL(clicked()),this, SLOT(svgOut()));
	
	layset->addWidget(reduced,0,Qt::AlignRight);
	layset->addWidget(ql_reduced,0,Qt::AlignLeft);
	layset->addStretch(1);
	layset->addWidget(ql_plugins);
	layset->addWidget(excludeP,3);
	layset->addStretch(1);
	layset->addWidget(ql_symbols);
	layset->addWidget(excludeS,3);
	layset->addStretch(2);
	layset->addWidget(save_btn);
	
	central->addLayout(layset);

	webGraph = new WebViewer(this);
	connect(webGraph, &WebViewer::linkClicked, this, &AboutModel::openLink);
#ifdef GRAPHVIZ_WEB_RENDERER
	web_render = true;
	qDebug() << "Using Web Graph renderer.";
#else
	web_render = false;
	qDebug() << "Using GraphViz renderer.";
#endif
	
	auto frame = new QFrame();
	frame->setLayout(new QBoxLayout(QBoxLayout::Down));
	frame->layout()->addWidget(webGraph);
	frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
	frame->setLineWidth(0);
	frame->setStyleSheet("background-color:white;");
// 	style->setProperty(Style::S)
	central->addWidget(frame,4);
	webGraph->show();
	
	lastGraph = "";
};

void AboutModel::update()
{
// 	qDebug()<<"UPDATE" ;
	
	title->blockSignals(true);
	description->blockSignals(true);
	reduced->blockSignals(true);
	excludeS->blockSignals(true);
	
	title->setText(model->getModelDescr().title);
	description->setText(model->getModelDescr().details);
	
// 	auto e = model->getRoot()->getModelDescr().edits;
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	QStringList qsl = dg->attribute("exclude-symbols")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	for (auto& key : qsl) key = key.trimmed();
	QStringList qpl = dg->attribute("exclude-plugins")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	for (auto& key : qpl) key = key.trimmed();
	
	excludeS->clear();
	QMap<QString,QString> qsm = model->getModelDescr().getSymbolNames("cpmDoubleSymbolRef");
	qsm.unite(model->getModelDescr().getSymbolNames("cpmVectorSymbolRef"));
	QRegExp sub_syms ("\\.(x|y|z|abs|phi|theta)$");
	
	for(auto& plug:qsm.keys()) {
		if (sub_syms.indexIn(plug)>=0)
			continue;
		excludeS->addItem(plug,qsl.contains(plug));
	}
	
	excludeP->clear();
	auto qpm = model->getModelDescr().pluginNames;
	for(auto& plug:qpm.keys()) {
		excludeP->addItem(plug,qpl.contains(plug));
	}
	
	if(dg->attribute("reduced")->get() == "true") {
		reduced->setCheckState(Qt::Checked);
	}
	else {
		reduced->setCheckState(Qt::Unchecked);
	}
	excludeP->setData(qpl);
	
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
	if (!isVisible()) return;
	MorphModel::GRAPH_TYPE type;
	if ( web_render ) {
		type= MorphModel::DOT;
	}
	else {
		type= MorphModel::SVG;
	}
	QString graph = model->getDependencyGraph(type);
	if (!graph.isEmpty()) {
		qDebug() << "Showing dependency graph: " << QString(graph);
		
		// Workaround for crashing chrome page on external links
		if (webGraph->url().scheme() == "chrome-error") {
			webGraph->reset();
		}
			
		if (graph.endsWith("png")) {
			url = (QString("file://") + graph);
			webGraph->setUrl(url);
		} else if (graph.endsWith("svg")) {
			QFile data(graph);
			if (data.open(QFile::ReadWrite)) {
				QDomDocument svg("svg-graph");
				svg.setContent(&data,true);
				auto svg_el = svg.elementsByTagName("svg").at(0).toElement();
				svg_el.setAttribute("style","margin: auto auto");
				svg_el.attribute("width").toInt();
				
				data.resize(0);
				QTextStream out(&data);
				out << svg.toString();
				data.close();
			}
			
			url = (QString("file://") + graph);
			webGraph->setUrl(url);

		} else /*if (graph.endsWith("dot"))*/ {
			url = "qrc:///template.html";
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

			auto dot = toR;
			toR.prepend("transition('").append("');");
			dotsource.close();
			if(/*webGraph->url() != dotgraph || */lastGraph != toR)
			{
// 				qDebug()<<"UPDATE GRAPH";
				if (lastGraph.isEmpty()) {
					onLoadConnect = connect(webGraph, &WebViewer::loadFinished, [&](bool){ 
						webGraph->evaluateJS(lastGraph, [](const QVariant& r){});
						qDebug() << "Graph loading finished!";
						disconnect(onLoadConnect);
					});
					webGraph->setUrl(url);
				}
				else {
					webGraph->evaluateJS(toR, [](const QVariant& r){});
				}
				lastGraph= toR;
				webGraph->show();
			}
		}
	}
	else {
		qDebug() << "Morpheus did not provide a dependency graph rendering!!";
	}
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
	QString fileName = webGraph->url().path().split("/").last();
	if (web_render) fileName="dependency-graph.svg";
	fileName.prepend(QDir::currentPath()+"/");
	QString format;
	if (fileName.endsWith("svg")) {
		format = "Scalable Vector Graphics (*.svg)";
	}
	else if (fileName.endsWith("png")) {
		format = "Portable Network Graphics (*.png)";
	}
	else if (fileName.endsWith("dot")) {
		format = "Graphiz dot Graph (*.dot)";
	}
	
	fileName = QFileDialog::getSaveFileName(this,tr("Save Image"), fileName , format);
	
	if (web_render) {
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
	else {
		if (QFile::exists(fileName))
			QFile::remove(fileName);
		
		QFile::copy(url.path(), fileName);
	}
}


void AboutModel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}


void AboutModel::assignTitle(QString title)
{
	nodeController *node = model->find(QStringList() << "Description" << "Title");
	if (node)
		node->setText(title);
}

void AboutModel::assignDescription()
{
	nodeController *node = model->find(QStringList() << "Description" << "Details");
	if (node)
		node->setText(description->toPlainText());
}

void AboutModel::update_excludes(QStringList qsl)
{
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	dg->attribute("exclude-symbols")->setActive(true);
	dg->attribute("exclude-symbols")->set(qsl.join(","));
}

void AboutModel::update_plugin_excludes(QStringList qsl)
{
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	
	dg->attribute("exclude-plugins")->setActive(true);
	dg->attribute("exclude-plugins")->set(qsl.join(","));
}


void AboutModel::update_reduced(int state)
{
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
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
