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
	dep_graph = new QGraphicsView(this);
	dep_graph->setScene(new QGraphicsScene(dep_graph));
	central->addWidget(dep_graph,4);
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
			dep_graph->scene()->clear();
			QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(QImage(graph)));
			item->setTransformationMode(Qt::SmoothTransformation);
			dep_graph->scene()->addItem(item);
			dep_graph->fitInView(item,Qt::KeepAspectRatio);
			dep_graph->scene()->setSceneRect(item->boundingRect());
		} else if (graph.endsWith("svg")) {
			dep_graph->scene()->clear();
			QGraphicsSvgItem *item = new QGraphicsSvgItem(graph);
			dep_graph->scene()->addItem(item);
			dep_graph->fitInView(item,Qt::KeepAspectRatio);
			dep_graph->scene()->setSceneRect(item->boundingRect());
		}
	}
	
	// run morpsi on it
	// reload the resulting dependency_graph.png/svg
	// display an error, id something went wrong.
	
}

void AboutModel::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
	
	if (!dep_graph->scene()) return;
	if (dep_graph->scene()->items().isEmpty()) return;
	QGraphicsItem* item = dep_graph->scene()->items().first();
	if (item) {
		dep_graph->fitInView(item,Qt::KeepAspectRatio);
		dep_graph->scene()->setSceneRect(item->boundingRect());
	}

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



