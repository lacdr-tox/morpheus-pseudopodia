#include "about_model.h"
#include <QtConcurrent/QtConcurrent>


AboutModel::AboutModel(SharedMorphModel model, QWidget* parent) : QSplitter(Qt::Vertical, parent)
{
	this->model = model;
	this->setHandleWidth(5);
	this->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	auto w_top = new QWidget();
	QBoxLayout *descr_lay = new QBoxLayout(QBoxLayout::Down);
	w_top->setLayout(descr_lay);
	this->addWidget(w_top);
	
	auto l_title = new QLabel(this);
	l_title->setText("Title: ");
	descr_lay->addWidget(l_title);
	
	title = new QLineEdit(this);
	title->setText(model->getModelDescr().title);
	connect(title,SIGNAL(textChanged(QString)),this, SLOT(assignTitle(QString)));
	descr_lay->addWidget(title);
	
	auto l_descr = new QLabel(this);
	l_descr->setText("Description:");
	descr_lay->addWidget(l_descr);
	
	description = new QTextEdit(this);
	description->setReadOnly(false);
	description->setText(model->getModelDescr().details);
	connect(description,SIGNAL(textChanged()),this, SLOT(assignDescription()));
	descr_lay->addWidget(description);
	
	auto w_bottom = new QWidget();
	QBoxLayout *central = new QBoxLayout(QBoxLayout::Down);
	w_bottom->setLayout(central);
	w_bottom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	this->addWidget(w_bottom);
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
	layset->addWidget(excludeS,3);
	layset->addWidget(ql_symbols,1,Qt::AlignLeft);
	layset->addStretch(2);
	layset->addWidget(save_btn);
	
	central->addLayout(layset);

	webGraph = new WebViewer(this);
	connect(webGraph, &WebViewer::linkClicked, this, &AboutModel::openLink);
#ifdef GRAPHVIZ_WEB_RENDERER
	web_render = true;
// 	qDebug() << "Using Web Graph renderer.";
#else
	web_render = false;
// 	qDebug() << "Using GraphViz renderer.";
#endif
	graph_state = GraphState::EMPTY;
	auto frame = new QFrame();
	frame->setLayout(new QBoxLayout(QBoxLayout::Down));
	frame->layout()->addWidget(webGraph);
	frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
	frame->setLineWidth(0);
	frame->setStyleSheet("background-color:white;");
// 	style->setProperty(Style::S)
	central->addWidget(frame,4);
	webGraph->show();
	
	setStretchFactor(1, 1);
	setStretchFactor(2, 3);
	
	connect(&waitForGraph, &QFutureWatcher<QString>::finished, this, &AboutModel::graphReady);
	   current_graph = "";
};

void AboutModel::update()
{
// 	qDebug()<<"UPDATE" ;
	
	title->blockSignals(true);
	title->setText(model->getModelDescr().title);
	title->blockSignals(false);
	
	description->blockSignals(true);
	description->setText(model->getModelDescr().details);
	description->blockSignals(false);
	
	model->getRoot()->setStealth(true);
	auto dg = model->find(QStringList() << "Analysis" << "ModelGraph",true);
	model->getRoot()->setStealth(false);
	

	QStringList qsl = dg->attribute("exclude-symbols")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
	for (auto& key : qsl) key = key.trimmed();
	
	auto tags = model->getRoot()->subtreeTags();
	tags.removeDuplicates();
	tags.sort();
	tags.prepend("#untagged");
	
	QStringList active_tags;
	if (!dg->attribute("include-tags")->isActive()) {
		active_tags = includeTags->currentData().toStringList();
	}
	else {
		active_tags = dg->attribute("include-tags")->get().split(",", QString::SplitBehavior::SkipEmptyParts);
		for (auto& key : active_tags) key = key.trimmed();
	}
	
	for (auto tag : tags) {
		int index = includeTags->findText(tag);
		if (index == -1) {
			// add a newly created tag and auto enable
			includeTags->addItem(tag, true);
		}
		else {
			includeTags->setItemData(index, active_tags.contains(tag),Qt::UserRole);
		}
	}
	for (int indx =0; indx < includeTags->count(); indx++ ) {
		QString tag = includeTags->itemData(indx, Qt::DisplayRole).toString();
		// Remove all Items for tags that are no more
		if (! tags.contains( tag ) ) {
			includeTags->removeItem(indx);
			indx--;
		}
	}
// 	includeTags->updateText();
	update_include_tags(includeTags->currentData().toStringList());
	
	excludeS->blockSignals(true);
	excludeS->clear();
	QMap<QString,QString> qsm = model->getModelDescr().getSymbolNames("cpmDoubleSymbolRef");
	qsm.unite(model->getModelDescr().getSymbolNames("cpmVectorSymbolRef"));
	QRegExp sub_syms ("\\.(x|y|z|abs|phi|theta)$");
	
	for(auto& plug:qsm.keys()) {
		if (sub_syms.indexIn(plug)>=0)
			continue;
		excludeS->addItem(plug,qsl.contains(plug));
	}
	excludeS->blockSignals(false);
	
	reduced->blockSignals(true);
	if(dg->attribute("reduced")->get() == "true") {
		reduced->setCheckState(Qt::Checked);
	}
	else {
		reduced->setCheckState(Qt::Unchecked);
	}
	reduced->blockSignals(false);

	// at least delay the update until the widget shows up
	QMetaObject::invokeMethod(this,"update_graph",Qt::QueuedConnection);
// 	update_graph();
}

void AboutModel::setGraphState(GraphState state, std::function<void()> ready_fun  = [](){}) {
	// Workaround for crashing chrome page on external links
	if (webGraph->url().scheme() == "chrome-error") {
		webGraph->reset();
		graph_state=GraphState::EMPTY;
	}
	
	bool set_url = false;;
	
	MorphModel::GRAPH_TYPE render_type = web_render ? MorphModel::DOT : MorphModel::SVG;
	QUrl url = QUrl("qrc:///model_graph_viewer.html");
	
	if ( webGraph->url() != url ) {
		graph_state=GraphState::EMPTY;
	}
	
	if (state==GraphState::PENDING) {
		if (graph_state != GraphState::EMPTY) {
			webGraph->evaluateJS("setGenerating();", [](const QVariant& r){});
			ready_fun();
			graph_state = GraphState::PENDING;
		}
		else {
			MorphModel::GRAPH_TYPE type = web_render ? MorphModel::DOT : MorphModel::SVG;
// 			QUrl url = (type==MorphModel::DOT)  ? QUrl("qrc:///model_graph_renderer.html") : QUrl("qrc:///model_graph_viewer.html");
			current_graph="";
			set_url=true;
			graph_state = GraphState::PENDING_EMPTY;
		}
	}
	
	if (state==GraphState::UPTODATE) {
		// apply the current graph
		if (current_graph.endsWith("dot")) {
			render_type = MorphModel::DOT;
			
			QFile dotsource(current_graph);
			QString dot;
			if (dotsource.exists()) {
				dotsource.open(QIODevice::ReadOnly);
				dot  = dotsource.readAll();
				dotsource.close();
			}
			
			if (dot.isEmpty()) {
				if (graph_state == GraphState::PENDING_EMPTY || graph_state == GraphState::EMPTY || graph_state == GraphState::FAILED) {
					state = GraphState::FAILED;
				}
				else {
					state = GraphState::OUTDATED;
				}
			}
			else {
// 				qDebug()<<"UPDATE GRAPH";
				dot = dot.replace(QRegularExpression("[\r\n\t]+")," ").trimmed();
				if (webGraph->url() != url ) {
					ready_fun = [ready_fun, dot, this](){ 
						webGraph->evaluateJS(QString("setDotGraph('")+ dot + "');", [](const QVariant& r){});
						ready_fun();
					};
				}
				else {
					webGraph->evaluateJS(QString("setDotGraph('")+ dot + "');", [](const QVariant& r){});
					ready_fun();
				}
				graph_state = GraphState::UPTODATE;
			}
			
		}
		else if (current_graph.endsWith("svg") || current_graph.endsWith("png")) {
			render_type = MorphModel::SVG;
			QString graph_url = "file://";
			if (graph_url.startsWith("/"))
				graph_url+=current_graph;
			else
				graph_url+="/"+current_graph;
			
			if (webGraph->supportsJS()) {
				QString command = QString("setGraph('%1');").arg(graph_url);
				if ( webGraph->url() == url ) {
					webGraph->evaluateJS(command, [](const QVariant& r){});
					ready_fun();
				}
				else {
					set_url=true;
					ready_fun = [this,command,ready_fun](){
						webGraph->evaluateJS(command, [](const QVariant& r){});
						ready_fun();
					};
				}
			}
			else {
				set_url = true;
				url = graph_url;
			}
			graph_state = GraphState::UPTODATE;
		}
		else {
			state=GraphState::FAILED;
		}
	}
	
	if (state==GraphState::OUTDATED) {
		if (graph_state==GraphState::EMPTY) {
			graph_state = GraphState::EMPTY;
		}
		else if ( graph_state == GraphState::PENDING_EMPTY || graph_state == GraphState::FAILED) {
			state = GraphState::FAILED;
		}
		else {
			webGraph->evaluateJS("setOutdated();", [](const QVariant& r){});
			ready_fun();
			graph_state=GraphState::OUTDATED;
		}
	}
	
	if (state==GraphState::FAILED) {
		if (graph_state!=GraphState::EMPTY) {
			webGraph->evaluateJS("setFailed();", [](const QVariant& r){});
			ready_fun();
		}
		else {
// 			url = "qrc:///graph_failed.html";
			set_url=true;
			ready_fun = [this,ready_fun](){ webGraph->evaluateJS("setFailed()", [](const QVariant& r){}); ready_fun(); };
		}
		graph_state=state;
	}
// 	else {
// 		qDebug() << "setGraphState() not implemented " << int(graph_state);
// 	}
	
	if (set_url) {
		onLoadConnect = connect(webGraph, &WebViewer::loadFinished,
					[=](bool success ){ 
						disconnect(onLoadConnect); 
						qDebug() << "Model graph loading finished" << (success ? "successfully" : "failing") << "(" << webGraph->url() << ")";
						ready_fun();
					});
		webGraph->load(url);
	}
	
	QMap<GraphState,QString> state_map = {
		{GraphState::EMPTY, "EMPTY"},
		{GraphState::PENDING,"PENDING"},
		{GraphState::PENDING_EMPTY,"PENDING_EMPTY"},
		{GraphState::UPTODATE,"UPTODATE"},
		{GraphState::OUTDATED,"OUTDATED"},
		{GraphState::FAILED,"FAILED"}
	};
	
// 	qDebug() << "Graph state was set to" <<state_map[graph_state];
}

void AboutModel::update_graph()
{
	if (!isVisible()) return;
	
// 	qDebug() << "updating graph";
	MorphModel::GRAPH_TYPE type = web_render ? MorphModel::DOT : MorphModel::SVG;
	setGraphState(GraphState::PENDING,
		[this, type](){
		QFuture<QString> graph_returned = QtConcurrent::run( model.data(), &MorphModel::getDependencyGraph, type );
		waitForGraph.setFuture(graph_returned);
	});
}

void AboutModel::graphReady() {
	auto graph = waitForGraph.result();
	
	if (graph.isEmpty()) {
		qDebug() << "Morpheus did not provide a model graph rendering!!" << graph;
		setGraphState(GraphState::OUTDATED);
	}
	else {
// 		qDebug() << "Showing model graph: " << QString(graph);
		current_graph = graph;
		setGraphState(GraphState::UPTODATE);
	}
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
		QString fileName="model-graph.svg";
// 		fileName="model-graph";
		QString format = "Scalable Vector Graphics (*.svg);;Portable Network Graphics (*.png)";
		fileName = QFileDialog::getSaveFileName(this,tr("Save Image"), fileName , format);
		if (!fileName.isEmpty()) {
			if (fileName.endsWith(".png")) {
				QImage image(webGraph->size()*2,QImage::Format_ARGB32);
				QPainter img_p(&image);
				img_p.setTransform(QTransform::fromScale(2,2));
				webGraph->render(&img_p);
				img_p.end();
				image.save(fileName);
			}
			else {
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
		}
	}
	else {
		QString fileName = current_graph.split("/").last();
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
		
		QFile::copy(current_graph, fileName);
	}
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
	auto dg = model->find(QStringList() << "Analysis" << "ModelGraph",true);
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


void AboutModel::update_reduced(int state)
{
	model->getRoot()->setStealth(true);
	auto dg = model->find(QStringList() << "Analysis" << "ModelGraph",true);
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
	auto dg = model->find(QStringList() << "Analysis" << "ModelGraph",true);

	dg->attribute("include-tags")->setActive(true);
	dg->attribute("include-tags")->set(includes.join(","));

	model->getRoot()->setStealth(false);
}
