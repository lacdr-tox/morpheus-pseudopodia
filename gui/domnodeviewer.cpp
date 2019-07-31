#include "domnodeviewer.h"

//------------------------------------------------------------------------------

domNodeViewer::domNodeViewer(QWidget* parent): QWidget(parent)
{
    createLayout();
    createMenu();
}


//------------------------------------------------------------------------------

domNodeViewer::~domNodeViewer()
{
    treeMenu->deleteLater();
}

//------------------------------------------------------------------------------

void domNodeViewer::createLayout()
{

    model_tree_view = new QTreeView();
    model_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    model_tree_view->setUniformRowHeights(true);
    model_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
	model_tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    model_tree_view->setDragEnabled(true);
    model_tree_view->setAcceptDrops (true);
    model_tree_view->setDefaultDropAction(Qt::MoveAction);
// 	model_tree_view->setAutoExpandDelay(200);
	model_tree_view->setAnimated(true);
	
	node_editor = new domNodeEditor(this);

    symbol_list_wid = new QTreeWidget(this);
    symbol_list_wid->setAlternatingRowColors(true);
    symbol_list_wid->setColumnCount(2);
    symbol_list_wid->setHeaderLabels(QStringList() << "Symbol" << "Description");
    symbol_list_wid->setRootIsDecorated(true);
    symbol_list_wid->setFocusPolicy(Qt::NoFocus);
    symbol_list_wid->setColumnWidth(0, 200);
    symbol_list_wid->setSortingEnabled(true);
    symbol_list_wid->sortByColumn(0, Qt::AscendingOrder);

	
    plugin_tree_widget = new QTreeWidget(this);
    plugin_tree_widget->setAlternatingRowColors(true);
    plugin_tree_widget->setColumnCount(2);
    plugin_tree_widget ->setHeaderLabels(QStringList() << "Plugin" << "Category");
    plugin_tree_widget->setColumnWidth(0, 200);
    plugin_tree_widget->setRootIsDecorated(false);
    plugin_tree_widget->setSortingEnabled(true);
    plugin_tree_widget->sortByColumn(1, Qt::AscendingOrder);
    plugin_tree_widget->setFocusPolicy(Qt::NoFocus);


    splitter = new QSplitter(this);
    QWidget *rightWid = new QWidget(this);

    QVBoxLayout *vl = new QVBoxLayout(rightWid);
	vl->addWidget(node_editor);
    vl->addWidget(new QLabel("Symbols"));
    vl->addWidget(symbol_list_wid);
    vl->addWidget(new QLabel("Plugins"));
    vl->addWidget(plugin_tree_widget);
    //vl->addStretch();

    lFont.setFamily( lFont.defaultFamily() );
    lFont.setWeight( QFont::Light );
    lFont.setItalic( true );

    connect(plugin_tree_widget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(pluginTreeDoubleClicked(QTreeWidgetItem*, int)));
    connect(plugin_tree_widget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(pluginTreeItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    splitter->addWidget(model_tree_view);
    splitter->addWidget(rightWid);

    QHBoxLayout *hl = new QHBoxLayout(this);
    hl->addWidget(splitter);
	this->setFocusPolicy(Qt::NoFocus);
	this->setFocusProxy(model_tree_view);
	updateConfig();

    QObject::connect(model_tree_view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(createTreeContextMenu(QPoint)));
    QObject::connect(symbol_list_wid, SIGNAL(doubleClicked(QModelIndex)), SLOT(insertSymbolIntoEquation(QModelIndex)));
    QObject::connect(splitter, SIGNAL(splitterMoved(int, int)), this, SLOT(splitterPosChanged()));
	QObject::connect(model_tree_view->header(),SIGNAL(sectionResized(int,int,int)),this,SLOT(treeViewHeaderChanged()));

}

//------------------------------------------------------------------------------

void domNodeViewer::createMenu()
{
    treeMenu = new QMenu();
    addNodeAction = treeMenu->addAction(QThemedIcon("list-add",QIcon(":/list-add")),"Add");
    copyNodeAction = treeMenu->addAction(QThemedIcon("edit-copy",QIcon(":/edit-copy.png")),"Copy");
    pasteNodeAction = treeMenu->addAction(QThemedIcon("edit-paste",QIcon(":/edit-paste.png")),"Paste");
    pasteNodeAction->setMenu(new QMenu()); // Placeholder for the submenu of paste codes ...
    cutNodeAction = treeMenu->addAction(QThemedIcon("list-cut",QIcon(":/edit-cut.png")),"Cut");
    removeNodeAction = treeMenu->addAction(QThemedIcon("list-remove",QIcon(":/list-remove")),"Remove");

    treeMenu->addSeparator();

    disableNodeAction = treeMenu->addAction(QThemedIcon("media-playback-pause",style()->standardIcon(QStyle::SP_MediaPause) ),"Disable");
    disableNodeAction->setCheckable(true);

    sweepNodeAction = treeMenu->addAction(QThemedIcon("media-seek-forward",style()->standardIcon(QStyle::SP_MediaSeekForward)),"ParamSweep");
    sweepNodeAction->setCheckable(true);

    QObject::connect(treeMenu, SIGNAL(triggered(QAction*)), this, SLOT(doContextMenuAction(QAction*)));
}

void domNodeViewer::setModelPart(QString part_name) {
	
	for (uint i=0; i<model->parts.size(); i++) {
		if (model->parts[i].label == part_name) {
			setModelPart(i);
		}
	}
}

//------------------------------------------------------------------------------

void domNodeViewer::setModelPart(int part) {
	if (model->parts[part].enabled && model->parts[part].element_index.isValid()) {
		QModelIndex root;
		root = model->parts[part].element_index;
		model_tree_view->setRootIndex(root);
		model_tree_view->setCurrentIndex(root);
	}
}

//------------------------------------------------------------------------------

void domNodeViewer::setModel(SharedMorphModel mod, int part) {
    model = mod;
    model_tree_view->setModel(model);
// 	QObject::connect(model_tree_view, SIGNAL(), this, SLOT());
	
	QObject::connect(model_tree_view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(setTreeItem( const QModelIndex& )));
	QObject::connect(model, SIGNAL(rowsMoved ( const QModelIndex & , int , int , const QModelIndex & , int  )), this, SLOT(selectMovedItem(const QModelIndex & , int , int , const QModelIndex & , int )),Qt::QueuedConnection);
	QObject::connect(model, SIGNAL(rowsInserted( const QModelIndex & , int , int)), this, SLOT(selectInsertedItem(const QModelIndex & , int , int )),Qt::QueuedConnection);
	setModelPart(part);
}

void domNodeViewer::selectMovedItem(const QModelIndex & sourceParent, int sourceRow, int , const QModelIndex & destParent, int destRow) {
	if (sourceParent == destParent && sourceRow < destRow)
		destRow-=1;
	setTreeItem(destParent.child(destRow,0));
}


void domNodeViewer::selectInsertedItem(const QModelIndex & destParent, int destRowFirst, int destRowLast) {
	setTreeItem(destParent.child(destRowFirst,0));
}

//------------------------------------------------------------------------------

//slot welcher aufgerufen wird, wenn im baum ein anderer Knoten ausgewählt wird
void domNodeViewer::setTreeItem( const QModelIndex& index)
{
// 	qDebug() << "selecting new Item " << index;
	model_tree_view->blockSignals(true);
    if (model_tree_view->currentIndex() != index) {
        model_tree_view->setCurrentIndex(index);
    }
	QModelIndex exp_idx = index;
	while (exp_idx.isValid()) {
		model_tree_view->expand(exp_idx);
		exp_idx = exp_idx.parent();
	}
	
    nodeController* node = model->indexToItem(index);
	
	node_editor->setNode(node,model);
    if (! node) return;
	
	// Always show the list of available symbols 
	symbol_list_wid->clear();
	QMap<QString,QString> sym_names = model->rootNodeContr->getModelDescr().getSymbolNames("cpmDoubleSymbolRef");
	QMap<QString,QString> vsym_names = model->rootNodeContr->getModelDescr().getSymbolNames("cpmVectorSymbolRef");
	QMap<QString,QString>::Iterator it;
	QStringList parents_names;
	QList<QTreeWidgetItem*> parents;
	parents.push_back(symbol_list_wid->invisibleRootItem());
	parents_names.push_back("Root");
	for (it=sym_names.begin(); it!=sym_names.end(); it++) {
		QStringList subnames = it.key().split(".");
		{
			int level = 0;
			QString subname = "";
			while (level < subnames.size()-1) {
				if (subname.isEmpty())
					subname = subnames[level];
				else
					subname.append(".").append(subnames[level]);
				
				if (parents_names.size()-1>level && parents_names[level+1] == subnames[level]) {
					level++;
				}
				else {
					while (parents_names.size()-1>level) {
						parents_names.removeLast();
						parents.removeLast();
					}
					QString subdescr;
					if (vsym_names.contains(subname)) {
						subdescr = vsym_names[subname];
					}
                     QTreeWidgetItem* trWItem_sub = new QTreeWidgetItem(parents.back(),QStringList() << subnames[level] << subdescr);
                     trWItem_sub->setFont(1, lFont);
                     parents.push_back(trWItem_sub);
					parents_names.push_back(subnames[level]);
					level++;
				}
			}
			while (parents_names.size()-1>level) {
				parents_names.removeLast();
				parents.removeLast();
			}
			
            QTreeWidgetItem* trWItem = new QTreeWidgetItem(parents.back(), QStringList() << subnames.back() << it.value() << it.key());
            trWItem->setFont(1, lFont);
		}
	}

    plugin_tree_widget->clear();
    QStringList allChilds = node->getAddableChilds(true);
    QStringList addableChilds = node->getAddableChilds(false);
    for(int i = 0; i < allChilds.size(); i++)
    {
        QTreeWidgetItem* trWItem = new QTreeWidgetItem(plugin_tree_widget, QStringList() << allChilds.at(i)  << node->childInformation(allChilds.at(i)).type->pluginClass);
        if (! addableChilds.contains(allChilds.at(i))) {
            trWItem->setIcon(0,style()->standardIcon(QStyle::SP_MessageBoxWarning));
            trWItem->setToolTip(0,"This node will disable an existing node!");
        }
        trWItem->setFont(1, lFont);
    }
    plugin_tree_widget->sortByColumn(1, Qt::AscendingOrder);
	model_tree_view->blockSignals(false);

    emit nodeSelected(node);
	emit xmlElementSelected(node->getXPath());
}

//------------------------------------------------------------------------------

void domNodeViewer::createTreeContextMenu(QPoint point)
{
    bool root_index = false;
    treePopupIndex = model_tree_view->indexAt(point);
    if (! treePopupIndex.isValid()) {
        treePopupIndex = model_tree_view->rootIndex();
        root_index = true;
    }
    else {
        model_tree_view->setExpanded(treePopupIndex,true);
    }

    nodeController *contr = model->indexToItem(treePopupIndex);
    if (contr->getName() == "#document") return;

    Q_ASSERT (treePopupIndex.isValid());

    if (contr) {
        bool disabled = contr->isDisabled() ||contr->isInheritedDisabled() ;
        addNodeAction->setEnabled(! disabled && ! contr->getAddableChilds(true).empty());  // Add

        cutNodeAction->setEnabled( ! contr->isInheritedDisabled() && contr->isDeletable() && ! root_index ); // Cut
        removeNodeAction->setEnabled(  ! contr->isInheritedDisabled() && contr->isDeletable() && ! root_index ); // Remove

        copyNodeAction->setEnabled(! root_index); // Copy

        pasteNodeAction->menu()->clear();
        QList<QString> childs = contr->getAddableChilds(true);
		
		// Get Nodes from local cache
		QList<QDomNode> nodes = config::getInstance()->getNodeCopies();
		for(int i = 0; i < nodes.size(); i++) {
			QDomNode n = nodes.at(i);
			if (childs.contains(n.nodeName())) {
				QDomDocument doc("");
				doc.appendChild(n);
				QStringList xml_hint = doc.toString(4).split("\n");
				if (xml_hint.size()>6) {
					QStringList new_text;
					new_text.append(xml_hint[0]);
					new_text.append(xml_hint[1]);
					new_text.append(xml_hint[2]);
					new_text.append("\t\t...");
					new_text.append(xml_hint[xml_hint.size()-3]);
					new_text.append(xml_hint[xml_hint.size()-2]);
					new_text.append(xml_hint[xml_hint.size()-1]);
					xml_hint = new_text;
				}
				infoAction* act = new infoAction(n.nodeName(), xml_hint.join("\n"), this);
				auto data = QVariant::fromValue(n);
				act->setData(data);
				pasteNodeAction->menu()->addAction(act);// Paste
			}
        }
        pasteNodeAction->setEnabled(! disabled && ! pasteNodeAction->menu()->isEmpty() );

        // ---- check deactivatable
        disableNodeAction->setEnabled( ( ! contr->isInheritedDisabled() && contr->isDeletable()) && ! root_index );  // Disable
        disableNodeAction->setChecked( contr->isDisabled() || contr->isInheritedDisabled() );

        // ---- check sweepable
		if (! disabled && contr->hasText()) {
			sweepNodeAction->setEnabled(true);
			sweepNodeAction->setChecked( model->isSweeperAttribute(contr->textAttribute()) );
		}
		else if (! disabled &&  contr->attribute("value") && contr->attribute("value")->isActive()) {
			sweepNodeAction->setEnabled(true);
			sweepNodeAction->setChecked( model->isSweeperAttribute(contr->attribute("value")) );
		}
		else {
			sweepNodeAction->setEnabled(false);
			sweepNodeAction->setChecked(false);
		}

        treeMenu->exec(model_tree_view->mapToGlobal(point));
    }
}

void domNodeViewer::pluginTreeDoubleClicked(QTreeWidgetItem* item, int column)
{
     model->insertNode(model_tree_view->currentIndex(),item->text(0));
}


void domNodeViewer::pluginTreeItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (current) {
		 emit xmlElementSelected( model->indexToItem(model_tree_view->currentIndex())->getXPath() << current->text(0) );
	 }
}

//------------------------------------------------------------------------------

void domNodeViewer::doContextMenuAction(QAction *action)
{
    if (action == addNodeAction)
    {
        nodeController *contr = model->indexToItem(treePopupIndex);
        addNodeDialog dia(contr);
        if (dia.exec() == QDialog::Accepted) {
            model->insertNode(treePopupIndex,dia.nodeName);
        }
        return;
    }

    if (action == copyNodeAction)
    {
        xmlElementCopied( model->indexToItem(treePopupIndex)->cloneXML());
        return;
    }

    if (action == cutNodeAction)
    {
        if (treePopupIndex.isValid()) {
            xmlElementCopied( model->indexToItem(treePopupIndex)->cloneXML());
            model->removeNode(treePopupIndex.parent(), treePopupIndex.row());
        }
        return;
    }

    if (action == removeNodeAction)
    {
        if (treePopupIndex.isValid()) {
            QMessageBox::StandardButton r = QMessageBox::question(this,
                        "Remove node",
                        QString("Are you sure?\t"),
                        QMessageBox::Yes | QMessageBox::No
                        );
            if (r == QMessageBox::No)
                return;
            model->removeNode(treePopupIndex.parent(), treePopupIndex.row());
        }
        return;
    }

    QMenu *m = pasteNodeAction->menu(); // Paste
    if( m && m->actions().contains(action)) {
		if (!action->data().value<QDomNode>().isNull())
			model->insertNode(treePopupIndex, action->data().value<QDomNode>().cloneNode());
        return;
    }

    if(action == disableNodeAction) {
        model->setDisabled(treePopupIndex,disableNodeAction->isChecked() );
		setTreeItem(treePopupIndex);
        return;
    }

//     if (action == sweepAttribAction)
//     {
//         AbstractAttribute *attr = map_rowToAttribute[table_popup_row];
//         if (sweepAttribAction->isChecked()) {
//             model->addSweeperAttribute(attr);
//             model_attribute_table->item(table_popup_row,0)->setIcon(QThemedIcon("media-seek-forward",style()->standardIcon(QStyle::SP_MediaSeekForward)));
//         }
//         else {
//             model->removeSweeperAttribute(attr);
//             model_attribute_table->item(table_popup_row,0)->setIcon(QIcon());
//         }
//         return;
//     }

    if (action == sweepNodeAction)
    {
		nodeController* node = model->indexToItem(treePopupIndex);
        AbstractAttribute *attr = node->hasText() ? node->textAttribute() : node->attribute("value");
        if (sweepNodeAction->isChecked()) {
            model->addSweeperAttribute(attr);
        }
        else {
            model->removeSweeperAttribute(attr);
        }
        model_tree_view->dataChanged(treePopupIndex,treePopupIndex);
// 		setAttributeEditor(node);
        return;
    }
}

//------------------------------------------------------------------------------

void domNodeViewer::splitterPosChanged() {
    // store layout settings
    QSettings().setValue("DOMViewer/Splitter", splitter->saveState());
}

void domNodeViewer::treeViewHeaderChanged() {
	// store layout settings
	QSettings().setValue("DOMViewer/TreeViewHeader", model_tree_view->header()->saveState());
}

//------------------------------------------------------------------------------

void domNodeViewer::updateConfig() {
	// shared splitter config is not loaded yet
	if (QSettings().contains("DOMViewer/Splitter") ) {
		splitter->restoreState(QSettings().value("DOMViewer/Splitter").toByteArray());
	}
	if (QSettings().contains("DOMViewer/TreeViewHeader") ) {
		model_tree_view->header()->restoreState( QSettings().value("DOMViewer/TreeViewHeader").toByteArray());
	}    
}

//------------------------------------------------------------------------------

void domNodeViewer::insertSymbolIntoEquation(const QModelIndex &idx) {
	// Pass the data of the NodeEditor
	node_editor->paste(symbol_list_wid->currentItem()->text(2));
}

//------------------------------------------------------------------------------

bool domNodeViewer::selectNode(QString path) {
	QStringList xml_path = path.split("/",QString::SkipEmptyParts);
	if (xml_path[0] == "MorpheusModel") xml_path.pop_front();
	nodeController* node = model->rootNodeContr->find(xml_path);
	if (node) {
		setModelPart(xml_path[0]);
		setTreeItem( model->itemToIndex(node));
		return true;
	}
	else {
		qDebug() << "domNodeViewer::selectNode: Unable to select node " << path;
		return false;
	}
}

//------------------------------------------------------------------------------

bool domNodeViewer::selectNode(QDomNode n) {

	nodeController* part_node = NULL;
	
	for (uint i=0; i<model->parts.size(); i++) {
		if ( (part_node = model->parts[i].element->find(n)) ) {
			setModelPart(i);
			setTreeItem( model->itemToIndex(part_node));
			return true;
		}
	}
	
	qDebug() << "domNodeViewer::selectNode: Unable to select node " << n.nodeName();
	return false;
	
}
