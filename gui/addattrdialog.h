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

#ifndef ADDATTRDIALOG_H
#define ADDATTRDIALOG_H

#include <QtGui>
#include <QString>
#include <QDialog>
#include <QTreeWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QLayout>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <vector>
#include "morpheusML/model_node.h"
#include "widgets/webviewer.h"

/*!
This class represents an add-dialog.<br>
In the middle of the window you can see a list with all childnodes that can be added to the chosen nodecontroller.<br>
Selecting a childnode displays stored documentation and informations on the right.<br>
With the abort-button you can close window without adding an element and accepting will send a signal with the chosen name.
*/
class addNodeDialog : public QDialog
{
    Q_OBJECT

public:
    addNodeDialog(nodeController *nodeContr);
    /*!<
      Opens a dialog with a list of attributes, which can be added to the named nodeController.
      Cancelling the dialog discards all selections and accepting the selection will send a signal with the chosen attribut-name.
      \param nodeContr nodeController to whom shall added a new xml-attribut
      */
    ~addNodeDialog();
QString nodeName; /*!< Name of the currently selected node, which shall eventually be added to the nodeController. */

private:
    nodeController *contr; /*!< nodeController whose addable attributes are shown in dialog. */   
    QTreeWidget *trW; /*!< QTreeWidget which lists all addable attributes of nodeController. */
	WebViewer *docu_view;
    QSplitter *splitter; /*!< QSplitter which shares the view of dialog. */

//    void keyReleaseEvent(QKeyEvent *);

private slots:
    void clickedTreeItem(); /*!< Shows the documentation of the selected attribut. */
};

#endif // ADDATTRDIALOG_H
