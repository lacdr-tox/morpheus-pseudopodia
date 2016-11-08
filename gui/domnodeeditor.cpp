#include "domnodeeditor.h"

domNodeEditor::domNodeEditor(QWidget* parent) : QWidget(parent)
{
	QVBoxLayout *main_layout = new QVBoxLayout(this);
	value_label = new QLabel("Value:");
	main_layout->addWidget(value_label);
	all_edits.append(value_label);
	
	multi_line_math_editor = new mathTextEdit(this);
	eq_highlighter = new equationHighlighter(multi_line_math_editor);
	main_layout->addWidget(multi_line_math_editor);
	all_edits.append(multi_line_math_editor);
	
	multi_line_text_editor = new QTextEdit(this);
	main_layout->addWidget(multi_line_text_editor);
	all_edits.append(multi_line_text_editor);
	
	enum_editor = new QComboBox(this);
	main_layout->addWidget(enum_editor);
	all_edits.append(enum_editor);
	
	line_editor = new QLineEdit(this);
	main_layout->addWidget(line_editor);
	all_edits.append(line_editor);
	
	attribute_label = new QLabel("Attributes:");
	main_layout->addWidget(attribute_label);
	all_edits.append(attribute_label);
	
	attribute_editor = new QTableWidget();
	attribute_editor->setGridStyle(Qt::NoPen);
	attribute_editor->setSelectionMode(QAbstractItemView::NoSelection);
	attribute_editor->setEditTriggers(QAbstractItemView::AllEditTriggers); //QAbstractItemView::CurrentChanged | QAbstractItemView::DoubleClicked);
	attribute_editor->verticalHeader()->setVisible(false);
	attribute_editor->horizontalHeader()->setVisible(false);
	attribute_editor->setContextMenuPolicy(Qt::CustomContextMenu);
	attribute_editor->setTabKeyNavigation(false);
	attribute_editor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

	main_layout->addWidget(attribute_editor);
	all_edits.append(attribute_editor);
	
    tableMenu = new QMenu();
	sweepAttribAction = new QAction("ParamSweep",tableMenu);
	sweepAttribAction->setCheckable(true);
	sweepAttribAction->setIcon(QThemedIcon("media-seek-forward",style()->standardIcon(QStyle::SP_MediaSeekForward)));
	tableMenu->addAction(sweepAttribAction);
	QObject::connect(tableMenu, SIGNAL(triggered(QAction*)), this, SLOT(doContextMenuAction(QAction*)));

	
	currentNode = nullptr;
	for (auto wid: all_edits) { wid->hide();}
	
	QObject::connect(multi_line_math_editor, SIGNAL(textChanged()), this, SLOT(updateNodeText()));
	QObject::connect(multi_line_math_editor, SIGNAL(emitParentheses(int)), eq_highlighter, SLOT(highlightParentheses(int)));
	QObject::connect(multi_line_text_editor, SIGNAL(textChanged()), this, SLOT(updateNodeText()));
	QObject::connect(line_editor, SIGNAL(editingFinished()), this, SLOT(updateNodeText()));
	QObject::connect(enum_editor, SIGNAL(currentIndexChanged ( const QString & )), this, SLOT(updateNodeText()));
	
	QObject::connect(attribute_editor, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(createAttributeEditContextMenu(QPoint)));
	QObject::connect(attribute_editor, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(changedAttributeEditItem(QTableWidgetItem*)));
	QObject::connect(attribute_editor, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(changedAttributeEditItem(QTableWidgetItem*)));
	
	this->setLayout(main_layout);
}


void domNodeEditor::createAttributeEditContextMenu(QPoint point)
{
	QTableWidgetItem *tmp = attribute_editor->itemAt(point);
	if(tmp != NULL)
	{
		table_popup_row = tmp->row();
		AbstractAttribute *attr = map_rowToAttribute[table_popup_row];

		if ( ! attr->isActive() ) {
			tableMenu->actions()[0]->setEnabled(false);
		}
		else {
			tableMenu->actions()[0]->setEnabled(true);
			if ( model->isSweeperAttribute(attr) ) {
				tableMenu->actions()[0]->setChecked(true);
			}
			else {
				tableMenu->actions()[0]->setChecked(false);
			}
		}

		tableMenu->exec(attribute_editor->mapToGlobal(point));
	}
//    else
//    {QMessageBox::information(this->main, "Error", "No QTableWidgetItem at this pos!");}
}

void domNodeEditor::setNode(nodeController* node, SharedMorphModel model)
{
	 // hideall
	for (auto wid : all_edits) { wid->hide(); }
	current_value_edit = NoEdit;
	
	if (!node) {
		currentNode = nullptr;
		this->model = model;
		return;
	}
	
	currentNode = node;
	this->model = model;
	
	bool is_showing_something = false;
	
	if (node->hasText())
	{
		value_label->show();
		is_showing_something = true;
		if (node->textType()->is_enum) {
			enum_editor->blockSignals(true);
			enum_editor->clear();
			enum_editor->addItems(node->textType()->value_set);
			enum_editor->setCurrentIndex(node->textType()->value_set.indexOf(node->getText()));
			enum_editor->blockSignals(false);
			enum_editor->show();
			current_value_edit = EnumBox;
		}
		else if ( node->textType()->name == "cpmMathExpression" ||  node->textType()->name == "cpmVectorMathExpression") {
			multi_line_math_editor->setText(node->getText());
			multi_line_math_editor->show();
			current_value_edit = MathText;
		}
		else if ( node->textType()->name == "cpmText") {
			multi_line_text_editor->setText(node->getText());
			multi_line_text_editor->show();
			current_value_edit = MultiText;
		}
		else {
			
			line_editor->setValidator(node->textType()->validator);
			line_editor->setText(node->getText());
			line_editor->show();
			current_value_edit = LineText;
		}
	}
	
	if (node->getOptionalAttributes().size() || node->getRequiredAttributes().size()){
		attribute_label->show();
		setAttributeEditor(node);
		attribute_editor->show();
		is_showing_something = true;
	}
	
	if (!is_showing_something) {
		attribute_label->show();
		setAttributeEditor(node);
		attribute_editor->show();
	}
	

}


void domNodeEditor::setAttributeEditor(nodeController* node)
{
	attribute_editor->blockSignals(true);
	attribute_editor->clear();
	attribute_editor->setRowCount(0);
	attribute_editor->setColumnCount(2);
	
	attribute_editor->setTextElideMode(Qt::ElideRight);

	map_rowToAttribute.clear();

	QList<AbstractAttribute*> requiredItems = node->getRequiredAttributes();
	QList<AbstractAttribute*> optionalItems = node->getOptionalAttributes();

	//für jedes attribut des plugins wird ein neuer tabelleneintrag angelegt
	//je nach dem ob es required oder optional ist wird es in maps sortiert
	attribute_editor->setRowCount(requiredItems.size() + optionalItems.size() );

	//zuerst werden die required attributes der tabelle hinzugefügt (hat den grund, damit die reihenfolge gleich bleibt)
	int row = 0;
	for (int i = 0; i < requiredItems.size() + optionalItems.size(); i++)
	{
		bool optional = i>=requiredItems.size();
		AbstractAttribute *tmp_attr = optional ? optionalItems.at(i-requiredItems.size()) : requiredItems.at(i);
		QString attrName = tmp_attr->getName();

		QTableWidgetItem *tmp_item = new QTableWidgetItem(tmp_attr->get());
		if (tmp_item->text().isEmpty()) {
// 			tmp_item->setData(Qt::EditRole,"");
			tmp_item->setData(Qt::DisplayRole,"...");
		}
		else {
// 			tmp_item->setData(Qt::EditRole,tmp_attr->get());
			tmp_item->setData(Qt::DisplayRole,tmp_attr->get().replace(QRegExp("\\s+")," ") );
		}
		
		QTableWidgetItem *tmp_header = new QTableWidgetItem(attrName);
		if (model->isSweeperAttribute(tmp_attr))
			tmp_header->setIcon(QThemedIcon("media-seek-forward",style()->standardIcon(QStyle::SP_MediaSeekForward)));
		map_rowToAttribute[row] = tmp_attr;

		tmp_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);

		tmp_header->setBackgroundColor(QColor(239, 235, 231, 255));
		
		if (optional) {
			tmp_header->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable);
			if (tmp_attr->isActive()) {
				tmp_header->setCheckState(Qt::Checked);
			}
			else{
				tmp_header->setCheckState(Qt::Unchecked);
				tmp_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				tmp_item->setTextColor(Qt::lightGray);
			}
		}
		else {
			tmp_header->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		}

		attribute_editor->setItem(row, 1, tmp_item);
		attribute_editor->setItem(row, 0, tmp_header);
		attribute_editor->setItemDelegateForRow(row, new attrController(this, tmp_attr) );
		row++;
	}
	
	attribute_editor->blockSignals(false);
}


void domNodeEditor::paste(QString a)
{
	QWidget * widget = focusWidget();
	if (widget) {
		if (dynamic_cast<QLineEdit*>(widget)) {
			dynamic_cast<QLineEdit*>(widget)->insert(a);
		}
		else if (dynamic_cast<QTextEdit*>(widget)) {
			dynamic_cast<QTextEdit*>(widget)->insertPlainText(a);
		}
		else if (dynamic_cast<QComboBox*>(widget)) {
			int idx = dynamic_cast<QComboBox*>(widget)->findData(a);
			if (idx>=0) {
				dynamic_cast<QComboBox*>(widget)->setCurrentIndex(idx);
			}
		}
	}
}


void domNodeEditor::updateNodeText()
{
	if (!currentNode) return;
	switch (current_value_edit) {
// 	switch (widget_stack->currentIndex()) {
		case MathText:
			currentNode->setText(multi_line_math_editor->toPlainText());
			break;
		case MultiText:
			currentNode->setText(multi_line_text_editor->toPlainText());
			break;
		case LineText:
			currentNode->setText(line_editor->text());
			break;
		case EnumBox:
			currentNode->setText(enum_editor->currentText());
			break;
		default: 
			qDebug() << "no text widget selected";
			break;
	}
}

void domNodeEditor::changedAttributeEditItem(QTableWidgetItem* attributeItem)
{
	int row = attributeItem->row();
	QTableWidgetItem *col2item = attribute_editor->item(row, 1);
	AbstractAttribute *attrib = map_rowToAttribute[row];

	if (attributeItem->column() == 0)
	{
		if ( attrib->isRequired() || attributeItem->checkState() == Qt::Checked)
		{
			//nodeContr + xml-knoten hinzufügen
			attrib->setActive(true);

			col2item->setTextColor(Qt::black);
			col2item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable);
		}
		else
		{
			col2item->setTextColor(Qt::lightGray);
			col2item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

			attrib->setActive(false);

			if (model->isSweeperAttribute(attrib))
				model->removeSweeperAttribute(attrib);
		}
	}

	if (attributeItem->column() == 1)
	{
		attributeItem->setData(Qt::DisplayRole,attributeItem->data(Qt::EditRole).toString().replace("\n"," "));
	}
}

void domNodeEditor::doContextMenuAction(QAction* action) {
    if (action == sweepAttribAction)
    {
        AbstractAttribute *attr = map_rowToAttribute[table_popup_row];
        if (sweepAttribAction->isChecked()) {
            model->addSweeperAttribute(attr);
            attribute_editor->item(table_popup_row,0)->setIcon(QThemedIcon("media-seek-forward",style()->standardIcon(QStyle::SP_MediaSeekForward)));
        }
        else {
            model->removeSweeperAttribute(attr);
            attribute_editor->item(table_popup_row,0)->setIcon(QIcon());
        }
        return;
    }
}

void domNodeEditor::resizeEvent(QResizeEvent* e)
{
	QWidget::resizeEvent(e);
	attribute_editor->resizeColumnToContents(0);
	attribute_editor->setColumnWidth(1,attribute_editor->width()-attribute_editor->columnWidth(0)-attribute_editor->frameWidth()*2);
}
