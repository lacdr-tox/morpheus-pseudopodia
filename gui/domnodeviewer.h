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

#ifndef DOMNODEVIEWER_H
#define DOMNODEVIEWER_H

#include <QtGui>
#include <QSharedPointer>
#include <QDesktopServices>
#include <QSortFilterProxyModel>

#include "morpheusML/morpheus_model.h"
#include "domnodeeditor.h"
#include "attrcontroller.h"

#include "addattrdialog.h"         // Add DOM Node Dialoge
#include "infoaction.h"
#include "widgets/checkboxlist.h"

#include "config.h"
//#include "parametersweeper.h"

class TagFilterSortProxyModel : public QSortFilterProxyModel {
Q_OBJECT
private:
	QSet<QString> filter_tags;
protected:
	 bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
	 bool filtering;
public slots:
	void setFilterTags(QStringList tag_list);   /// comma separated list
	void setFilteringEnabled(bool enabled);   /// comma separated list
};


/*!
This class represents a widget, which shows all informations about a named nodeController.<br>
On the left side it shows all xml-nodes and their child-nodes of the nodeController in a QTreeWidget.<br>
On the right side you can see a table with all xml-attributes, of the selected xml-node inclusive their values.<br>
Below this you got shown the documentation of every node or attribut.<br>
Also this class provides a context-menu by pressing the right mouse-key,<br>
via which nodes and attributes can be deleted, added or edited.
*/
class domNodeViewer : public QWidget
{
Q_OBJECT

public:
    domNodeViewer(QWidget* parent = NULL);
    /*!<
        Constructs an object which handles the viewing and changing of properties of the description of the named nodeController.
        \param contr Pointer to nodeController which should be shown.
        \param tabWid Parent which holds the new widget.
        \param tabName Name of the new Widget ( will be shown in the header ).
    */
    ~domNodeViewer();

	void setModelPart(QString part_name);
    void setModelPart(int part);
    void setModel(SharedMorphModel mod, int part);
    void updateConfig(); /*!< Sets the splitter of widget to the old position. */
    bool selectNode(QDomNode);
	bool selectNode(QString path);


private:
    SharedMorphModel model;

    map<QAction, QString> action_names;

	QTreeView* model_tree_view; /*!< TreeWidget in which the xml-nodes will be shown.*/
// 	QModelIndex current_index;
	TagFilterSortProxyModel* model_tree_filter;
	CheckBoxList* filter_tag_list;
	QAction *model_tree_remove_action, *model_tree_filter_action, *model_tree_sort_action;
	QMenu *addNodeMenu;
	QSet<QString> filter_tags;
	struct { int column; Qt::SortOrder order; } sort_state;
	QSplitter* splitter; /*!< Splitter which divide the view of the widget. */
	domNodeEditor* node_editor;
	QTreeWidget *symbol_list_wid;
    QTreeWidget *plugin_tree_widget;
    //QListWidget *child_list_widget;
    QFont lFont;
	bool lazy_mode;

    void createLayout(); /*!< Creates the sub-widgets and their layout */
    void createMenu(); /*!< Adds actions to the treeMenu and tableMenu.*/
	void updateSymbolList();
	void updateNodeActions();
    QMenu *treeMenu; /*!< Menu which appears, when requesting a context-menu over the treewidget. */
    QAction *addNodeAction, *copyNodeAction, *copyXPathAction, *pasteNodeAction, *cutNodeAction, *removeNodeAction;
    QAction *sweepNodeAction, *disableNodeAction;
    QModelIndex treePopupIndex;
    
    int table_popup_row; /*!< Number of the selected table-row. */
    QMenu *tableMenu; /*!< Menu which appears, when requesting a context-menu over the tablewidget. */
    
    QAction *sweepAttribAction;

private slots:
    void setTreeItem(const QModelIndex& ); /*!< Slot which reload parameters, when another xmlnode was selected in the treeViewWidget.*/
	void selectMovedItem(const QModelIndex& sourceParent, int sourceRow, int, const QModelIndex& destParent, int destRow );
	void selectInsertedItem(const QModelIndex& destParent, int destRowFirst, int);
    void insertSymbolIntoEquation(const QModelIndex&); /*!< inserts the symbol given by model index from the symbolList to the equation editor. */

    void createTreeContextMenu(QPoint); /*!< Updates the state for the menu of the TreeItem at the given position.*/
    void doContextMenuAction(QAction*); /*!< Executes the method which transforms the given action.*/

    void splitterPosChanged(); /*!< stores the new position of the splitter to QSettings. */
	void treeViewHeaderChanged(); /*!< stores the new TreeView header geometry to QSettings. */
    //void childListDoubleClicked(QListWidgetItem* item);
    //void childListItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
    void pluginTreeDoubleClicked(QTreeWidgetItem*, int);
    void pluginTreeItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
	void updateTagList();
signals:
//    void nameChanged(nodeController* node); /*!< Signal sends when the specified name of the root node changed. */
    void xmlElementCopied(QDomNode node); /*!< Signal sends a xmlNode which shall be copied. */
	void nodeSelected(nodeController* node);
	void xmlElementSelected(QStringList xPath);
};

#endif // DOMNODEVIEWER_H

