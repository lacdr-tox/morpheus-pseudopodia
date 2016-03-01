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

#ifndef DOCUDOCK_H
#define DOCUDOCK_H

#include "config.h"
#include "nodecontroller.h"
#include "QtWebKit/QWebView"

class DocuDock : public QDockWidget
{
    Q_OBJECT

public:
    DocuDock(QWidget* parent);

signals :
	void elementDoubleClicked(QString element_name);
	
public slots :
	void setCurrentNode( nodeController *node );
	void setCurrentElement( QString name);
	void setCurrentURL( const QUrl& url );
	
private slots:
	void setRootOfHelpIndex();
	
protected:
    virtual void resizeEvent(QResizeEvent* event );

private:
    QHelpEngine* help_engine;
	QSplitter* splitter;
	QWebView* help_view;
	QTimer *timer;
	bool root_reset;
// 	QListWidget*  index_view;
	
};

#endif // DOCUDOCK_H
