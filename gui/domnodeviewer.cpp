#include "domnodeviewer.h"


bool TagFilterSortProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
	if (!filtering)
		return true;
	auto row_index = sourceModel()->index(source_row,0,source_parent);
	auto tags = row_index.data(MorphModel::TagsRole).toStringList();
	if (tags.empty() && filter_tags.contains("#untagged")) {
		return true;
	}
	if (tags.size()==1 && tags[0] == "*") {
		return true;
	}
#if QT_VERSION >= 0x056000
	return tags.toSet().intersects(filter_tags);
#else
	return ! tags.toSet().intersect(filter_tags).isEmpty();
#endif
};

void TagFilterSortProxyModel::setFilterTags(QStringList tag_list) {
	filter_tags = tag_list.toSet();
	invalidateFilter();
};

void TagFilterSortProxyModel::setFilteringEnabled(bool enabled)
{
	filtering = enabled;
	invalidateFilter();
}

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

class MorpheusTreeView : public QTreeView {
public:
	MorpheusTreeView(QWidget * parent = nullptr) : QTreeView(parent) {};
	QModelIndex indexAt(const QPoint & point) const override { auto idx = QTreeView::indexAt(point); if (!idx.isValid()) return rootIndex(); return idx;};
};

void domNodeViewer::createLayout()
{

    model_tree_view = new MorpheusTreeView();
    model_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    model_tree_view->setUniformRowHeights(true);
    model_tree_view->setSelectionMode(QAbstractItemView::SingleSelection);
	model_tree_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    model_tree_view->setDragEnabled(true);
    model_tree_view->setAcceptDrops (true);
    model_tree_view->setDefaultDropAction(Qt::MoveAction);
	model_tree_view->setAutoExpandDelay(100);
	model_tree_view->setAnimated(true);
	model_tree_filter = new TagFilterSortProxyModel();
	model_tree_filter->setFilterRole(MorphModel::TagsRole);
	model_tree_filter->setSortRole(Qt::DisplayRole);
	model_tree_filter->setSortCaseSensitivity(Qt::CaseInsensitive);
	model_tree_view->setModel(model_tree_filter);
	node_editor = new domNodeEditor(this);

	/// Symbol List ///
	
    symbol_list_wid = new QTreeWidget(this);
    symbol_list_wid->setAlternatingRowColors(true);
    symbol_list_wid->setColumnCount(2);
    symbol_list_wid->setHeaderLabels(QStringList() << "Symbol" << "Description");
    symbol_list_wid->setRootIsDecorated(true);
    symbol_list_wid->setFocusPolicy(Qt::NoFocus);
    symbol_list_wid->setColumnWidth(0, 200);
    symbol_list_wid->sortByColumn(0, Qt::AscendingOrder);

	/// Model Component List ///
	
    plugin_tree_widget = new QTreeWidget(this);
    plugin_tree_widget->setAlternatingRowColors(true);
    plugin_tree_widget->setColumnCount(2);
    plugin_tree_widget->setHeaderLabels(QStringList() << "Component" << "Category");
    plugin_tree_widget->setColumnWidth(0, 200);
    plugin_tree_widget->setRootIsDecorated(false);
    plugin_tree_widget->setSortingEnabled(true);
    plugin_tree_widget->sortByColumn(1, Qt::AscendingOrder);
    plugin_tree_widget->setFocusPolicy(Qt::NoFocus);


    splitter = new QSplitter(this);
	
	QWidget *leftWid = new QWidget(this);
	leftWid->setLayout(new QVBoxLayout(this));
	leftWid->layout()->setContentsMargins(0, 10, 4, 0);
	leftWid->layout()->setSpacing(0);
	
	
	/// TOOLBAR /// 
	
	auto model_toolbar = new QToolBar("View",this);
	// Add Button
	model_tree_add_action = new QAction(QIcon::fromTheme("list-add"),"add",this);
	auto addMenu = new QMenu(this);
	model_tree_add_action->setMenu(addMenu);
// 	connect(addMenu, &QMenu::aboutToShow,
// 			[this]() {
// 				auto model_index = model_tree_filter->mapToSource(model_tree_view->currentIndex());
// 				if (!model_index.isValid())
// 					model_index = model_tree_filter->mapToSource(model_tree_view->rootIndex());
// 				nodeController *contr = model->indexToItem(model_index);
// 				if (!contr) { qDebug() << "Invalid current model index"; return;}
// 				qDebug() << "Creating Popup for " << contr->getName();
// 				QStringList allChilds = contr->getAddableChilds(true);
// 				QStringList addableChilds = contr->getAddableChilds(false);
// 				auto addMenu = model_tree_add_action->menu();
// 				addMenu->clear();
// 				if (allChilds.isEmpty()) {
// 					addMenu->addAction(new QAction("none"));
// 					addMenu->actions()[0]->setEnabled(false);
// 				}
// 				else {
// 					for(int i = 0; i < allChilds.size(); i++) {
// 						if (! addableChilds.contains(allChilds.at(i)))
// 							addMenu->addAction(new QAction(style()->standardIcon(QStyle::SP_MessageBoxWarning),allChilds.at(i)));
// 						else 
// 							addMenu->addAction(new QAction(allChilds.at(i)));
// 					}
// 				}
// 			}
// 	);
	connect(addMenu, &QMenu::triggered, [this](QAction* action){
		auto model_index = model_tree_filter->mapToSource(model_tree_view->currentIndex());
		if (!model_index.isValid())
					model_index = model_tree_filter->mapToSource(model_tree_view->rootIndex());
		model->insertNode(model_index, action->text());
	});
	model_toolbar->addAction(model_tree_add_action);
	
	// Remove Button
	model_tree_remove_action = new QAction(QIcon::fromTheme("list-remove"),"remove",this);
	model_tree_remove_action->setShortcut(QKeySequence(QKeySequence::Delete));
	connect(model_tree_remove_action, &QAction::triggered, [this](){
		if (current_index.isValid()) {
			QMessageBox::StandardButton r = QMessageBox::question(this,
						"Remove node",
						QString("Are you sure?\t"),
						QMessageBox::Yes | QMessageBox::No
						);
			if (r == QMessageBox::Yes)
				model->removeNode(current_index.parent(), current_index.row());
		}
	});
	model_toolbar->addAction(model_tree_remove_action);
	
	// Sort Button
	model_tree_sort_action = new QAction(QIcon::fromTheme("view-sort-ascending"),"sort", this);
	model_tree_sort_action->setCheckable(true);
	connect(model_tree_sort_action, &QAction::toggled, 
		[this](bool enabled){ 
			if (enabled) {
				this->model_tree_view->setSortingEnabled(true);
				this->model_tree_view->sortByColumn(sort_state.column, sort_state.order);
			}
			else {
				this->sort_state.column = this->model_tree_filter->sortColumn();
				this->sort_state.order = this->model_tree_filter->sortOrder();
				this->model_tree_view->setSortingEnabled(false);
				this->model_tree_filter->sort(-1,Qt::AscendingOrder);
			} 
		}
	);
	model_toolbar->addAction(model_tree_sort_action);
	model_toolbar->addSeparator();
	
	// Filter Tag Box
	filter_tag_list = new CheckBoxList(this);
	filter_tag_list->addItem("#untagged",true);
	filter_tag_list->setData(QStringList() << "#untagged");
	filter_tag_list->setEnabled(false);
	model_toolbar->addWidget(filter_tag_list);
	connect(filter_tag_list, &CheckBoxList::currentTextChanged, model_tree_filter, &TagFilterSortProxyModel::setFilterTags);
	model_tree_filter->setFilterTags(QStringList() << "#untagged");
	
	// Filter Button
	model_tree_filter_action = new QAction(QThemedIcon("view-filter",QIcon(":/view_filter.png")),"tags",this);
	model_tree_filter_action->setCheckable(true);
	connect(model_tree_filter_action, &QAction::toggled, [this](bool state) {
		disconnect(model_tree_filter, nullptr, this, SLOT(selectInsertedItem(const QModelIndex & , int , int )));
		filter_tag_list->setEnabled(state);
		model_tree_filter->setFilteringEnabled(state);
		
		connect(model_tree_filter, SIGNAL(rowsInserted( const QModelIndex & , int , int)), this, SLOT(selectInsertedItem(const QModelIndex & , int , int )),Qt::QueuedConnection);
	});
	
	model_toolbar->addAction(model_tree_filter_action);

	leftWid->layout()->addWidget(model_toolbar);
	leftWid->layout()->addWidget(model_tree_view);
	
	/// Right Widget ///

	QWidget *rightWid = new QWidget(this);
	rightWid->setLayout(new QVBoxLayout(rightWid));
	rightWid->layout()->setContentsMargins(4, 10, 0, 0);
	rightWid->layout()->setSpacing(8);
	rightWid->layout()->addWidget(node_editor);
	rightWid->layout()->addWidget(new QLabel("Model Symbols"));
	rightWid->layout()->addWidget(symbol_list_wid);
	rightWid->layout()->addWidget(new QLabel("Model Components"));
	rightWid->layout()->addWidget(plugin_tree_widget);

    lFont.setFamily( lFont.defaultFamily() );
    lFont.setWeight( QFont::Light );
    lFont.setItalic( true );

    connect(plugin_tree_widget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)),this,SLOT(pluginTreeDoubleClicked(QTreeWidgetItem*, int)));
    connect(plugin_tree_widget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(pluginTreeItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

	
    splitter->addWidget(leftWid);
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
	copyXPathAction = treeMenu->addAction(QThemedIcon("edit-copy",QIcon(":/edit-copy.png")),"Copy XPath");
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

void domNodeViewer::updateTagList() {
	auto current_selection = filter_tag_list->currentData(Qt::UserRole).toStringList();
	filter_tag_list->clear();
	auto tags = model->rootNodeContr->subtreeTags();
	tags.removeDuplicates();
	tags.prepend("#untagged");
	tags.sort();
	for (auto tag : tags) {
		filter_tag_list->addItem(tag, current_selection.contains(tag));
	}
}

void domNodeViewer::setModelPart(QString part_name) {
	
	for (uint i=0; i<model->parts.size(); i++) {
		if (model->parts[i].label == part_name) {
			setModelPart(i);
			return;
		}
	}
	qDebug() << "Requested model part " << part_name << " cannot be found";
}

//------------------------------------------------------------------------------

void domNodeViewer::setModelPart(int part) {
	auto part_root = model->parts[part].element_index;
	if (model->parts[part].enabled && part_root.isValid()) {
		auto view_index = model_tree_filter->mapFromSource(part_root);
		if (!view_index.isValid()) {
			qDebug() << "Requested model part " << part << " is filtered out !!";
		}
		model_tree_view->setRootIndex(view_index);
		model_tree_view->setCurrentIndex(view_index);
		model_tree_view->setSortingEnabled(model_tree_sort_action->isChecked());
		updateTagList();
		model_tree_filter->setFilteringEnabled(model_tree_filter_action->isChecked());
		setTreeItem(view_index);
	}
	else {
		qDebug() << "Requested model part " << part << " is not enabled";
	}
}

//------------------------------------------------------------------------------

void domNodeViewer::setModel(SharedMorphModel mod, int part) {
    model = mod;
	model_tree_filter->setSourceModel(model);
	
	
	QObject::connect(model_tree_view->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(setTreeItem( const QModelIndex& )));
	QObject::connect(model_tree_filter, SIGNAL(rowsMoved ( const QModelIndex & , int , int , const QModelIndex & , int  )), this, SLOT(selectMovedItem(const QModelIndex & , int , int , const QModelIndex & , int )),Qt::QueuedConnection);
	connect(model_tree_filter, SIGNAL(rowsInserted( const QModelIndex & , int , int)), this, SLOT(selectInsertedItem(const QModelIndex & , int , int )),Qt::QueuedConnection);
	setModelPart(part);
	connect(model_tree_filter, &TagFilterSortProxyModel::dataChanged, this, &domNodeViewer::updateTagList );
	connect(model_tree_filter, &TagFilterSortProxyModel::rowsRemoved, this, &domNodeViewer::updateTagList );
}

void domNodeViewer::selectMovedItem(const QModelIndex & sourceParent, int sourceRow, int , const QModelIndex & destParent, int destRow) {
	if (sourceParent == destParent && sourceRow < destRow)
		destRow-=1;
	auto child_idx = model_tree_filter->index(destRow,0,destParent);
	setTreeItem( child_idx );
}


void domNodeViewer::selectInsertedItem(const QModelIndex & destParent, int destRowFirst, int /*destRowLast*/) {
	auto child_idx = model_tree_filter->index(destRowFirst,0,destParent);
	if (!lazy_mode) setTreeItem( child_idx );
}

//------------------------------------------------------------------------------

//slot welcher aufgerufen wird, wenn im baum ein anderer Knoten ausgewählt wird
void domNodeViewer::setTreeItem( const QModelIndex& index)
{
// 	qDebug() << "selecting new Item " << index;
	
	QModelIndex view_index = index;
	if (!view_index.isValid()) {
		view_index = model_tree_view->rootIndex();
	}
	if (index.model() == model) {
		qDebug() << "setTreeItem: Got a source index";
		view_index = model_tree_filter->mapFromSource(index);
	}

	bool within_part = false;
	QModelIndex exp_idx = view_index;
	while (exp_idx.isValid()) {
		if (exp_idx == model_tree_view->rootIndex()) {
			within_part = true;
			break;
		}
		model_tree_view->expand(exp_idx);
		exp_idx = exp_idx.parent();
	}
	if (!within_part) return;
	
	model_tree_view->blockSignals(true);
	// Make item visible
	if (model_tree_view->currentIndex() != view_index) {
		model_tree_view->setCurrentIndex(view_index);
	}
	model_tree_view->blockSignals(false);
	current_index = model_tree_filter->mapToSource(view_index);
	
	// Select the proper editor
    nodeController* node = view_index.data(MorphModel::NodeRole).value<nodeController*>();
    if (! node) {
		qDebug() << "Critical error! No model node for index";
		return;
	}
	node_editor->setNode(node,model);
	
	// Always update the list of available symbols 
	updateSymbolList();

	// Update the list of available Plugins
	updateNodeActions();

    emit nodeSelected(node);
	emit xmlElementSelected(node->getXPath());
}

void domNodeViewer::updateSymbolList() {
// 	symbol_list_wid->setSortingEnabled(false);
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
	symbol_list_wid->setSortingEnabled(true);
}

void domNodeViewer::updateNodeActions() {
	auto node = current_index.data(MorphModel::NodeRole).value<nodeController*>();
	
	model_tree_remove_action->setEnabled(node->isDeletable());
	
	plugin_tree_widget->clear();
	auto addMenu = model_tree_add_action->menu();
	addMenu->clear();
	QStringList allChilds = node->getAddableChilds(true);
	QStringList addableChilds = node->getAddableChilds(false);
	for(int i = 0; i < allChilds.size(); i++)
	{
		QTreeWidgetItem* trWItem = new QTreeWidgetItem(plugin_tree_widget, QStringList() << allChilds.at(i)  << node->childInformation(allChilds.at(i)).type->pluginClass);
		trWItem->setFont(1, lFont);
		if (! addableChilds.contains(allChilds.at(i))) {
			trWItem->setIcon(0,style()->standardIcon(QStyle::SP_MessageBoxWarning));
			trWItem->setToolTip(0,"This node will disable an existing node!");
			addMenu->addAction(new QAction(style()->standardIcon(QStyle::SP_MessageBoxWarning),allChilds.at(i),this));
		}
		else {
			addMenu->addAction(new QAction(allChilds.at(i),this));
		}
	}
	plugin_tree_widget->sortByColumn(1, Qt::AscendingOrder);
}

//------------------------------------------------------------------------------

void domNodeViewer::createTreeContextMenu(QPoint point)
{
	// Adjust view
	bool root_index = false;
	treePopupIndex = model_tree_view->indexAt(point);
	if (! treePopupIndex.isValid()) {
		treePopupIndex = model_tree_view->rootIndex();
		root_index = true;
	}
	else {
		model_tree_view->setExpanded(treePopupIndex,true);
	}
	// translate to model index -- no, it's a view index
	Q_ASSERT (treePopupIndex.isValid());

	auto *contr = treePopupIndex.data(MorphModel::NodeRole).value<nodeController*>();
	if (!contr || contr->getName() == "#document") return;

	// Adjust popup menue entries
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


void domNodeViewer::pluginTreeDoubleClicked(QTreeWidgetItem* item, int column)
{
	auto model_index = model_tree_filter->mapToSource( model_tree_view->currentIndex() );
	model->insertNode(model_index, item->text(0));
}


void domNodeViewer::pluginTreeItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
    if (current) {
		 emit xmlElementSelected( model_tree_view->currentIndex().data(MorphModel::XPathRole).value<QStringList>() << current->text(0));
	 }
}

//------------------------------------------------------------------------------

void domNodeViewer::doContextMenuAction(QAction *action)
{
	auto model_index = model_tree_filter->mapToSource(treePopupIndex);
    if (action == addNodeAction)
    {
        nodeController *contr = model->indexToItem(model_index);
        addNodeDialog dia(contr);
        if (dia.exec() == QDialog::Accepted) {
            model->insertNode(model_index,dia.nodeName);
        }
        return;
    }

    if (action == copyNodeAction)
    {
        xmlElementCopied( model->indexToItem(model_index)->cloneXML());
        return;
    }
	if (action == copyXPathAction) {
		QGuiApplication::clipboard()->setText(model_index.data(MorphModel::XPathRole).toStringList().join("/"));
		return;
	}

    if (action == cutNodeAction)
    {
    if (model_index.isValid()) {
//             xmlElementCopied( model->indexToItem(model_index)->cloneXML());
            auto element = model->removeNode(model_index.parent(), model_index.row());
			xmlElementCopied(element->cloneXML());
        }
        return;
    }

    if (action == removeNodeAction)
    {
        if (model_index.isValid()) {
            QMessageBox::StandardButton r = QMessageBox::question(this,
                        "Remove node",
                        QString("Are you sure?\t"),
                        QMessageBox::Yes | QMessageBox::No
                        );
            if (r == QMessageBox::No)
                return;
            model->removeNode(model_index.parent(), model_index.row());
        }
        return;
    }

    QMenu *m = pasteNodeAction->menu(); // Paste
    if( m && m->actions().contains(action)) {
		if (!action->data().value<QDomNode>().isNull())
			model->insertNode(model_index, action->data().value<QDomNode>().cloneNode());
        return;
    }

    if(action == disableNodeAction) {
        model->setDisabled(model_index,disableNodeAction->isChecked() );
		setTreeItem(model_index);
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
		nodeController* node = model->indexToItem(model_index);
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
