#include "about_model.h"
#include <QtConcurrent/QtConcurrent>

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
	auto include_tags_label = new QLabel("incl. tags");
	include_tags_label->setAlignment(Qt::AlignRight);
	
	auto ql_plugins = new QLabel("excluded Plugins");
	ql_plugins->setAlignment(Qt::AlignRight);
	auto ql_symbols = new QLabel("excl. symbols");
	ql_symbols->setAlignment(Qt::AlignRight);
	auto ql_reduced = new QLabel("&reduced");
	ql_reduced->setAlignment(Qt::AlignLeft);

	includeTags = new CheckBoxList();
	connect(includeTags, &CheckBoxList::currentTextChanged,[=](QStringList includes) { update_include_tags(includes); update_graph();});
	
	excludeP = new CheckBoxList();
	connect(excludeP, &CheckBoxList::currentTextChanged,[=](QStringList excludes) {update_plugin_excludes(excludes);update_graph();});

	excludeS = new CheckBoxList();
	connect(excludeS, &CheckBoxList::currentTextChanged, [=](QStringList excludes) {update_excludes(excludes);update_graph();} );

	reduced = new QCheckBox("reduced", this);
	connect(reduced, &QCheckBox::stateChanged, [=](int state){update_reduced(state);update_graph();} );
	update_reduced(Qt::Unchecked);
	ql_reduced->setBuddy(reduced);

	save_btn = new QPushButton(this);
// 	save_btn->setText("save");
	save_btn->setIcon(QIcon::fromTheme("document-save"));
	connect(save_btn,SIGNAL(clicked()),this, SLOT(svgOut()));
	
	layset->addWidget(reduced,0,Qt::AlignRight);
// 	layset->addWidget(ql_reduced,0,Qt::AlignLeft);
	layset->addStretch(1);
	layset->addWidget(includeTags,3);
	layset->addWidget(include_tags_label,1,Qt::AlignLeft);
	layset->addStretch(1);
// 	layset->addWidget(ql_plugins);
// 	layset->addWidget(excludeP,3);
// 	layset->addStretch(1);
	layset->addWidget(excludeS,3);
	layset->addWidget(ql_symbols,1,Qt::AlignLeft);
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
	
	connect(&waitForGraph, &QFutureWatcher<QString>::finished, this, &AboutModel::graphReady);
	lastGraph = "";
};

void AboutModel::update()
{
	qDebug()<<"UPDATE" ;
	
	title->blockSignals(true);
	description->blockSignals(true);
	reduced->blockSignals(true);
	excludeS->blockSignals(true);
	
	title->setText(model->getModelDescr().title);
	description->setText(model->getModelDescr().details);
	
	model->getRoot()->setStealth(true);
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	model->getRoot()->setStealth(false);
	
	QStringList qtl = dg->attribute("include-tags")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	for (auto& key : qtl) key = key.trimmed();

	QStringList qsl = dg->attribute("exclude-symbols")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	for (auto& key : qsl) key = key.trimmed();
	QStringList qpl = dg->attribute("exclude-plugins")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	for (auto& key : qpl) key = key.trimmed();
	
	includeTags->clear();
	auto tags = model->getRoot()->subtreeTags();
	tags.removeDuplicates();
	tags.sort();
	
	for (auto tag : tags) {
		includeTags->addItem(tag, qtl.contains(tag));
	}
	
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
	if (!isVisible()) return;
	qDebug() << "updating graph";
	MorphModel::GRAPH_TYPE type = web_render ? MorphModel::DOT : MorphModel::SVG;
	if (webGraph->url().fileName() == "model_graph_renderer.html") {
		webGraph->evaluateJS("setGenerating();", [](const QVariant& r){});
	}
	else if (webGraph->url().fileName() == "model_graph_viewer.html") {
		webGraph->evaluateJS("setGenerating();", [](const QVariant& r){});
	}
	else {
		QUrl url = (type== MorphModel::DOT)  ? QUrl("qrc:///model_graph_renderer.html") : QUrl("qrc:///model_graph_viewer.html");
// 		onLoadConnect = connect(webGraph, &WebViewer::loadFinished, [this](bool){
// 					qDebug() << "Graph loading finished!";
// 					webGraph->evaluateJS("setGenerating();", [](const QVariant& r){});
// 					disconnect(onLoadConnect);
// 				});
		webGraph->setUrl(url);
		onLoadConnect = 
			connect(webGraph, &WebViewer::loadFinished, 
				[type,this](bool){
					QFuture<QString> graph_returned = QtConcurrent::run( model.data(), &MorphModel::getDependencyGraph, type );
	waitForGraph.setFuture(graph_returned);
					disconnect(onLoadConnect);
				}
			);
		return;
	}
	QFuture<QString> graph_returned = QtConcurrent::run( model.data(), &MorphModel::getDependencyGraph, type );
	waitForGraph.setFuture(graph_returned);
}

void AboutModel::graphReady() {
	auto graph = waitForGraph.result();
	if (!graph.isEmpty()) {
		qDebug() << "Showing dependency graph: " << QString(graph);
		
		// Workaround for crashing chrome page on external links
		if (webGraph->url().scheme() == "chrome-error") {
			webGraph->reset();
		}
			
		if (graph.endsWith("png")) {
			url = "qrc:///model_graph_viewer.html";
			QString command = QString("setGraph('file://")+ graph + "');";
			
			if (webGraph->url() == url && webGraph) {
				webGraph->evaluateJS(command, [](const QVariant& r){});
			}
			else {
				onLoadConnect = connect(webGraph, &WebViewer::loadFinished, [command,this](bool){
					qDebug() << "Graph loading finished!";
					webGraph->evaluateJS(command, [](const QVariant& r){});
					disconnect(onLoadConnect);
				});
				webGraph->setUrl(url);
			}
			lastGraph = graph;

		} else if (graph.endsWith("svg")) {
			url = "qrc:///model_graph_viewer.html";
			QString command = QString("setGraph('file://")+ graph + "');";
			
			if (webGraph->url() == url) {
				webGraph->evaluateJS(command, [](const QVariant& r){});
			}
			else {
				onLoadConnect = connect(webGraph, &WebViewer::loadFinished, [command,this](bool){
					qDebug() << "Graph loading finished!";
					webGraph->evaluateJS(command, [](const QVariant& r){});
					disconnect(onLoadConnect);
				});
				webGraph->setUrl(url);
			}
			lastGraph = graph;
			
// 			QFile data(graph);
// 			if (data.open(QFile::ReadWrite)) {
// 				QDomDocument svg("svg-graph");
// 				svg.setContent(&data,true);
// 				auto svg_el = svg.elementsByTagName("svg").at(0).toElement();
// 				svg_el.setAttribute("style","margin: auto auto");
// 				svg_el.attribute("width").toInt();
// 				auto titles = svg.elementsByTagName("title");
// 				for (int i=0; i<=titles.size(); i++) {
// 					const auto& title = titles.at(i).toElement();
// 					title.parentNode().removeChild(title);
// 				}
// 				data.resize(0);
// 				QTextStream out(&data);
// 				out << svg.toString();
// 				data.close();
// 			}
// 			
// 			url = (QString("file://") + graph);
// 			webGraph->setUrl(url);

		} else if (graph.endsWith("dot")) {
			url = "qrc:///model_graph_renderer.html";
			
			QFile dotsource(graph);
			QString dot;
			if (dotsource.exists()) {
				dotsource.open(QIODevice::ReadOnly);
				dot  = dotsource.readAll();
				dotsource.close();
			}
			
			QString command;
			
			if (dot.isEmpty()) {
				command= "setFailed();";
			}
			else /*if (dot != lastGraph)*/ {
// 				qDebug()<<"UPDATE GRAPH";
				lastGraph = dot;
				dot = dot.replace("\n"," ").replace("\t"," ").trimmed();
				if (webGraph->url() != url ) {
					command = QString("setGraph('")+ dot + "');";
				}
				else {
					command = QString("transitionGraph('") + dot + "');";
				}
			}
			if (webGraph->url() != url ) {
				webGraph->setUrl(url);
				onLoadConnect = connect(webGraph, &WebViewer::loadFinished, [command,this](bool){
					qDebug() << "Graph loading finished!";
					qDebug() << command;
					webGraph->evaluateJS(command, [](const QVariant& r){});
					disconnect(onLoadConnect);
				});
			}
			else {
				webGraph->evaluateJS(command, [](const QVariant& r){});
			}
// 				webGraph->show();
		}
	}
	else {
		if (webGraph->url().fileName()=="model_graph_renderer.html") {
			webGraph->evaluateJS("setFailed();", [](const QVariant& r){});
		}
		else if (webGraph->url().fileName()=="model_graph_viewer.html") {
			webGraph->evaluateJS("setFailed();", [](const QVariant& r){});
		}
		else {
			webGraph->setUrl(QUrl("qrc:///graph_failed.html"));
		}
		qDebug() << "Morpheus did not provide a dependency graph rendering!!" << graph;
	}
	// run mopsi on it
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
	
	
	if (web_render) {
		QString fileName="dependency-graph.svg";
		QString format = "Scalable Vector Graphics (*.svg)";
		fileName = QFileDialog::getSaveFileName(this,tr("Save Image"), fileName , format);
		
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
		QString fileName = lastGraph.split("/").last();
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
		
		if (QFile::exists(fileName))
			QFile::remove(fileName);
		
		QFile::copy(lastGraph, fileName);
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
	model->getRoot()->setStealth(true);
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	if (qsl.isEmpty()) {
		dg->attribute("exclude-symbols")->set("");
		dg->attribute("exclude-symbols")->setActive(false);
	}
	else {
		dg->attribute("exclude-symbols")->setActive(true);
		dg->attribute("exclude-symbols")->set(qsl.join(","));
	}
	model->getRoot()->setStealth(false);
}

void AboutModel::update_plugin_excludes(QStringList qsl)
{
	model->getRoot()->setStealth(true);
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	dg->attribute("exclude-plugins")->setActive(true);
	dg->attribute("exclude-plugins")->set(qsl.join(","));
	model->getRoot()->setStealth(false);
}


void AboutModel::update_reduced(int state)
{
	model->getRoot()->setStealth(true);
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
	model->getRoot()->setStealth(false);
}


void AboutModel::update_include_tags(QStringList includes) {
	model->getRoot()->setStealth(true);
	auto dg = model->find(QStringList() << "Analysis" << "DependencyGraph",true);
	if (includes.isEmpty()) {
		dg->attribute("include-tags")->set("");
		dg->attribute("include-tags")->setActive(false);
	}
	else {
		dg->attribute("include-tags")->setActive(true);
		dg->attribute("include-tags")->set(includes.join(","));
	}
	model->getRoot()->setStealth(false);
		
}
