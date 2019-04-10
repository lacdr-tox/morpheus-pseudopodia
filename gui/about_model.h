//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef ABOUTMODEL_H
#define ABOUTMODEL_H


#include <QWidget>
#include <QGraphicsScene>
#include <QtSvg/QGraphicsSvgItem>
#include <QtWebEngineWidgets/QWebEngineView>
#include "config.h"

class AboutModel : public QWidget {
	Q_OBJECT
	
	SharedMorphModel model;
	QLineEdit* title;
	QTextEdit* description;
	QWebEngineView* webGraph;
	QUrl url;
	QGraphicsView* dep_graph;
public:
	AboutModel(SharedMorphModel model, QWidget* parent = NULL);
	
	void update();
    virtual void resizeEvent(QResizeEvent* event);
	
private slots:
	void update_graph();
	void assignTitle(QString title);
	void assignDescription();
};

#endif
