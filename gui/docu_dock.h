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
#include "widgets/webviewer.h"


class DocuDock : public QDockWidget
{
    Q_OBJECT

public:
    DocuDock(QWidget* parent);

signals :
	void elementDoubleClicked(QString element_name);
	
public slots :
	void setCurrentElement( QStringList xPath );
	void setCurrentElement( QString name);
	void setCurrentIndex(const QModelIndex& );
	void setCurrentURL( const QUrl& url );
	
private slots:
	void setRootOfHelpIndex();
	void resetStatus();
	void openHelpLink(const QUrl&);
	
protected:
    virtual void resizeEvent(QResizeEvent* event );

private:
	QAction *b_back, *b_forward;
    QHelpEngine* help_engine;
	QSplitter* splitter;
	
	WebViewer* help_view;

	QTimer *timer;
	QLineEdit* label_documentation;
	QTreeView* toc_widget;
	QSortFilterProxyModel* toc_model;
	QModelIndex modules_index, MorpheusML_index;
	QNetworkAccessManager* hnam ;
	bool root_reset;
	
// 	QListWidget*  index_view;
	
};

#endif // DOCUDOCK_H
