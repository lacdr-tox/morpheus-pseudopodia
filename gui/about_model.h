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
#include <QListView>
#include "config.h"
#include "widgets/checkboxlist.h"
#include "widgets/webviewer.h"
#include <QFutureWatcher>

class AboutModel : public QWidget {
	Q_OBJECT
	
	SharedMorphModel model;
	QLineEdit* title;
	QTextEdit* description;
	
	WebViewer* webGraph;
	bool web_render;

	QUrl url;
	QFrame* webFrame;
	CheckBoxList* excludeP;
	CheckBoxList* excludeS;
	QCheckBox* reduced;
	QPushButton* save_btn;
	QMetaObject::Connection onLoadConnect;
	QString lastGraph;
	
	QFutureWatcher<QString> waitForGraph;
	
public:
	AboutModel(SharedMorphModel model, QWidget* parent = NULL);
	void update();
    virtual void resizeEvent(QResizeEvent* event);

signals:
	void nodeSelected(QString path);
	
private slots:
	void update_graph();
	void graphReady();
	void assignTitle(QString title);
	void assignDescription();
	void openLink(const QUrl&);
	void svgOut();
	void update_excludes(QStringList qsl);
	void update_plugin_excludes(QStringList qsl);
	void update_reduced(int);
};

#endif
