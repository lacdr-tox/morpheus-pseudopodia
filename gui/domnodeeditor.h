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

#ifndef DOMNODEEDITOR_H
#define DOMNODEEDITOR_H

#include <QtGui/QWidget>
#include "config.h"
#include "nodecontroller.h"
#include "mathtextedit.h"
#include "equationhighlighter.h"
#include "attrcontroller.h"

class domNodeEditor : public QWidget
{
	Q_OBJECT
public:
	domNodeEditor(QWidget* parent = NULL);
	void setNode(nodeController* node, SharedMorphModel model);
		
private:
	enum EditWid { MathText=0 , MultiText=1, EnumBox=2, LineText=3, NoEdit=4 };
	
	nodeController *currentNode;
	SharedMorphModel model;
	
	QLabel *value_label;
	QLabel *attribute_label;
	
	QList<QWidget*> all_edits;
	QTableWidget* attribute_editor; /*!< TableWidget in which the xml-attributes of the selected xml-node will be shown.*/
	map<int, AbstractAttribute*> map_rowToAttribute; /*!< Map which connects the row-number of the attribut-table with the correspondending attribut. */
	int table_popup_row; /*!< Number of the selected table-row. */
	QMenu *tableMenu; /*!< Menu which appears, when requesting a context-menu over the tablewidget. */
	QAction *sweepAttribAction;
    
	mathTextEdit *multi_line_math_editor; /*!< Textedit which appears instead of a table, if a pure text-node is selected in the treewidget. */
	equationHighlighter *eq_highlighter; /*!< Highlighter which highlights the value of the selected pure text-node. */
	QTextEdit *multi_line_text_editor; /*!< Textedit which appears instead of a table, if a pure text-node is selected in the treewidget. */
	QLineEdit *line_editor;
	QComboBox *enum_editor; /*!< ComboBox which appears instead of a table, if a enum-node is seleceted in the treewidget. */
    EditWid current_value_edit;
	
	void setAttributeEditor(nodeController* node);
	
signals:

private slots:
	void updateNodeText();
	void createAttributeEditContextMenu(QPoint point);
	void changedAttributeEditItem(QTableWidgetItem* attributeItem);
	void doContextMenuAction(QAction* action);
		
public slots:
	void paste(QString a);
};

#endif // DOMNODEEDITOR_H
