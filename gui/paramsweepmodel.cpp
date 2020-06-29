#include "paramsweepmodel.h"
#include "morpheus_model.h"

ParamSweepModel::ParamSweepModel ( QObject* parent ) : QAbstractItemModel ( parent )
{
	root_item = QSharedPointer<ParamItem>(new ParamItem(NULL,NULL));
}



QByteArray ParamSweepModel::store() const
{
	// create a QByteStream of the format
	// squeeze out all params
	QList<QList <QPair<QString,QString> > > attrib_data;
	for (int i=0; i<root_item->childCount(); i++) {
		ParamItem* item = root_item->child(i);
		if ( item->attribute() ) {
			attrib_data.append(QList< QPair<QString,QString> >());
			attrib_data.last().append(QPair<QString,QString>(item->attribute()->getXMLPath(),item->range()));
		}
		else {
			attrib_data.append(QList< QPair<QString,QString> >());
			for (int j=0; j<item->childCount(); j++) {
				attrib_data.last().append(QPair<QString,QString>(item->child(j)->attribute()->getXMLPath(),item->child(j)->range()));
			}
		}
			
	}
	QByteArray param_data;
	QDataStream param_data_stream(&param_data,QIODevice::WriteOnly);
	param_data_stream << attrib_data;
	return param_data;
}

bool ParamSweepModel::restore ( const QByteArray& data, nodeController* modelRootNode)
{
	QSharedPointer<ParamItem> new_root = QSharedPointer<ParamItem>( new ParamItem(NULL,NULL) );
	try {
		if (data.isEmpty())
			throw(QString("Parameter set is empty."));
		
		QList<QList<QPair<QString,QString> > > param_data;
		QDataStream param_data_stream(data);
		param_data_stream >> param_data;
		
		for (uint i=0; i<param_data.size();i++) {
			if (param_data[i].size() == 1 ) {
				AbstractAttribute* attr = modelRootNode->getAttributeByPath(param_data[i][0].first.split("/",QString::SkipEmptyParts));
				if (!attr)
					throw(QString("Cannot find attribute with XPath") + param_data[i][0].first + ".");
				ParamItem* item = new_root->addChild(attr);
				item->setRange(param_data[i][0].second);
			}
			else {
				for (uint j=0; j<param_data[i].size();j++) {
					AbstractAttribute* attr = modelRootNode->getAttributeByPath(param_data[i][j].first.split("/",QString::SkipEmptyParts));
					if (!attr)
						throw(QString("Cannot find attribute with XPath") + param_data[i][j].first + ".");
					ParamItem *item = new  ParamItem(new_root.data(), attr);
					item->setRange( param_data[i][j].second );
				}
			}
		}
		beginResetModel();
		root_item = new_root;
		endResetModel();
	}
	catch (QString s) {
		QMessageBox::information(NULL,QString("ParamSweep"), QString("Unable to restore the ParameterScan.\n") + s,QMessageBox::Ok,QMessageBox::NoButton);
		return false;
	}
	return true;
}

void ParamSweepModel::createJobList( QList< AbstractAttribute* >& params, QList< QStringList >& values ) const
{
	// Use a linked list here due to invasive editing ...
	QList<QList<QString> >allJobs;

	for(int i = 0; i < root_item->childCount(); i++)
	{
		ParamItem* item_1 = root_item->child(i);
		QList<QStringList> value_lists;
		int min_params=0;
		
		if (item_1->attribute()) {
			params.append(item_1->attribute());
			value_lists.push_back(item_1->values());
			min_params = item_1->values().size();
		}
		else {
			// calculate the number of available parameter sets
			for(int j = 0; j < item_1->childCount(); j++) {
				params.append(item_1->child(j)->attribute());
				value_lists.push_back( item_1->child(j)->values());
				if (j==0)
					min_params = value_lists.back().size();
				else if( min_params > value_lists.back().size() )
					min_params = value_lists.back().size();
			}
		}
		
		if (allJobs.isEmpty()) {
			allJobs.push_back(QList<QString>());
		}
		// Expand all entries in the JobList by permutations of the current pairwise parameter sets
		QList<QList<QString> >::Iterator base_job_entry;
		for (base_job_entry= allJobs.begin(); base_job_entry != allJobs.end(); ) {
			// process all parameter sets
			for (int param_pos=0; param_pos<min_params; param_pos++) {
				// duplicate base_job_entry, i.e. use it as a pattern
				QList<QList<QString> >::Iterator current_job_entry = allJobs.insert(base_job_entry, *base_job_entry);
				// process all parameters
				for(int j = 0; j < value_lists.size(); j++)
				{
					current_job_entry->push_back(value_lists[j][param_pos]);
				}
			}
			// erase the base job, that was used as a template and move base_job to the next element.
			allJobs.erase(base_job_entry++);
		}
	}
	QList<QList<QString> >::ConstIterator cps;
	for (cps=allJobs.begin(); cps!= allJobs.end(); cps++) {
		values.append(*cps);
	}
}


bool ParamSweepModel::contains ( AbstractAttribute* attr ) const
{
	return root_item->findItem(attr) != NULL;
}


void ParamSweepModel::clear()
{
	beginResetModel();
	root_item = QSharedPointer<ParamItem>( new ParamItem(NULL,NULL) );
	endResetModel();
}


ParamItem* ParamSweepModel::indexToItem ( const QModelIndex& index ) const
{
	if (index.isValid())  {
		return static_cast<ParamItem*>(index.internalPointer());
	}
	else
		return root_item.data();
}

QModelIndex ParamSweepModel::itemToIndex( ParamItem* item, int col ) const
{
	// no item
	if (!item || item==root_item.data())
		return QModelIndex();
	
	return createIndex(item->row(),col,(void*)item);
}


QModelIndex ParamSweepModel::index ( int row, int column, const QModelIndex& parent ) const
{
	ParamItem * item = indexToItem(parent);
	if (item->childCount()>row ) {
		return createIndex(row,column, (void*) item->child(row));
	}
	else {
		qDebug() << "Requesting an index for an invalid child " << row << " of Item " << (item==root_item.data() ? "root" : (item->attribute() ? item->attribute()->getXMLPath():"paired")) << " having " << item->childCount() << " Children";
		return QModelIndex();
	}
}

QModelIndex ParamSweepModel::parent ( const QModelIndex& child ) const
{
	return itemToIndex(indexToItem(child)->parent());
}

int ParamSweepModel::columnCount ( const QModelIndex& parent ) const
{
	return 3;
}

int ParamSweepModel::rowCount ( const QModelIndex& parent ) const
{
	return indexToItem(parent)->childCount();
}

QVariant ParamSweepModel::data ( const QModelIndex& index, int role ) const
{
	if (role == Qt::DisplayRole) {
		ParamItem* item = indexToItem(index);
		if ( ! item->attribute() ) {
			// group item
			if (index.column()==0)
				return QString("pairwise");
		}
		else {
			if (index.column()==0) {
				return item->attribute()->getXMLPath().remove(0,13);  // remove "MorpheusModel" prefix
			}
			else if (index.column() == 1) {
				return item->type();
			}
			else if (index.column() == 2) {
				return item->values().join(";");
				return item->range();
			}
		}
	}
	else if (role == Qt::EditRole) {
		ParamItem* item = indexToItem(index);
		if ( item->attribute() ) {
			if (index.column() == 2) {
// 				qDebug() << "sending value " <<item->range();
				return item->range();
			}
		}
	}
	else if (role == Qt::DecorationRole) {
		if (index.column() == 1) {
			ParamItem* item = indexToItem(index);
			if ( item->attribute() ) {
				if (item->type() == "UnsignedDouble") {
					return QIcon(":/ufloat.png");
				}
				else if (item->type() == "Double") {
					return QIcon(":/float.png");
				}
				else if (item->type() == "UnsignedInteger") {
					return QIcon(":/uint.png");
				}
				else if (item->type() == "Integer") {
					return QIcon(":/int.png");
				}
				else if (item->type() == "String") {
					return QIcon(":/string.png");
				}
				else {
					return QIcon(":/unknown_type.png");
				}
			}
		}
	}
	return QVariant();
}

AbstractAttribute* ParamSweepModel::getAttribute ( const QModelIndex& index )
{
	return indexToItem(index)->attribute();
}


bool ParamSweepModel::setData ( const QModelIndex& index, const QVariant& value, int role )
{
	if ( index.column()==2  && indexToItem(index)->attribute() ) {
		indexToItem(index)->setRange(value.toString());
		emit dataChanged(index, index);
		return true;
	}
	return false;
}

QVariant ParamSweepModel::headerData ( int section, Qt::Orientation orientation, int role ) const
{
	if (role==Qt::DisplayRole && orientation==Qt::Horizontal) {
		if (section==0)
			return QString("Attribute");
		else if (section==1)
			return QString("Type");
		else if (section==2)
			return QString("Values");
	}
	return QVariant();
}

Qt::DropActions ParamSweepModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

Qt::ItemFlags ParamSweepModel::flags ( const QModelIndex& index ) const
{
	if (!index.isValid())
		return Qt::ItemIsDropEnabled;
	Qt::ItemFlags f(Qt::ItemIsSelectable);
	ParamItem* item = indexToItem(index);
	if ( item->attribute() ) {
		if (item->attribute()->isActive())
			f |= Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
		if (item->parent() == root_item.data())
			f |= Qt::ItemIsDropEnabled;
		if (index.column()==2)
			f |= Qt::ItemIsEditable;
	} else {
		 f |= Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
	}
	return f;
}

void ParamSweepModel::addAttribute ( AbstractAttribute* item )
{
	beginInsertRows(itemToIndex(root_item.data()), root_item->childCount(), root_item->childCount());
	root_item->addChild(item);
	endInsertRows();
}


void ParamSweepModel::removeAttribute ( AbstractAttribute* attr )
{
	// Find the attribute and delete it ..
	ParamItem* item = root_item->findItem(attr);
	if (item) {
		beginRemoveRows(itemToIndex(item->parent()), item->row(),item->row());
		item->parent()->removeChild(item->row());
		endRemoveRows();
	}
	else
		qDebug() << "ParamSweeper::removeAttribute : Unable to find attribute " << attr->getXMLPath();
}



QStringList ParamSweepModel::mimeTypes () const {
    QStringList types;
    types << "text/plain" << IndexListMimeData::indexList;
    return types;
}

QMimeData* ParamSweepModel::mimeData(const QModelIndexList &indexes) const {
	if ( ! indexes.empty()) {
		QModelIndex dragIndex = indexes.front();

		IndexListMimeData* m_data = new IndexListMimeData();
		m_data->setIndexList(indexes);

		AbstractAttribute* attr = indexToItem(indexes.first())->attribute();
		if (attr)
			m_data->setText(attr->getXMLPath());
		return m_data;
	}
	return 0;
}

bool ParamSweepModel::dropMimeData ( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
	if (action == Qt::MoveAction) {
		if (data->formats().contains(IndexListMimeData::indexList)) {
			const IndexListMimeData* index_list = qobject_cast<const IndexListMimeData*>(data);
			QModelIndex move_index = index_list->getIndexList().front();
			QModelIndex parent_index = parent;
			ParamItem* parent_item = indexToItem(parent);
			ParamItem* item_to_move = indexToItem(move_index);

			// drop on a group child
			// add to the parenting group
			// This should never happen, since the model prevents such drops ...
			if (parent_item != root_item.data() && parent_item->parent() != root_item.data()) {
				row = parent_index.row()+1;
				parent_index = parent_index.parent();
				parent_item = parent_item->parent();
			}
			// Paste on an item
			if (row==-1) {
				if (parent_item==root_item.data())
					row = parent_item->childCount();
				else 
					row = 0;
			}
			/// Case A: Item is moved to another position within the same group
			if (move_index.parent() == parent_index) {
				

				int new_row;

				if (move_index.row() < row) {
					new_row = row-1;
				}
				else /* if (move_index.row() >= row)*/ {
					new_row = row;
				}
				if (new_row == move_index.row())
					return true;
				qDebug() << "Move item " << item_to_move->attribute()->getXMLPath() << " from " << item_to_move->row() << " before element at " << row << " beeing at row " << new_row << " after move!" ;
				Q_ASSERT(itemToIndex(item_to_move->parent()) == parent_index);
				try {
					beginMoveRows(parent_index, move_index.row(), move_index.row(), parent_index, row);
					parent_item->moveChild(move_index.row(), new_row);
					endMoveRows();
				}
				catch (...) {
					qDebug() << "We have a problem";
					Q_ASSERT(0); exit(0);
				}
			}
			else {
				try {
// 				QModelIndexList persis =  persistentIndexList();
// 				foreach(const QModelIndex& idx, persis) {
// 					ParamItem* item = indexToItem(idx);
// 					qDebug() << "Item " << ( item == root_item.data() ? "root": ( item->attribute() ? item->attribute()->getXMLPath() : QString("pair") ) )  << " r" << idx.row() << " c" << idx.column();
// 				}
// 				qDebug() << "----------------";

// 				beginResetModel();
				emit layoutAboutToBeChanged();
				QList<QModelIndex> old_indices;
				old_indices.append(itemToIndex(item_to_move,0));
				old_indices.append(itemToIndex(item_to_move,1));
				old_indices.append(itemToIndex(item_to_move,2));
				
				bool new_parent_gets_grouped = false;
				ParamItem* new_sibling = NULL;
				if (parent_item->attribute()) {
					// new parent item will be moved one level down ...
					row = 1;
					new_parent_gets_grouped = true;
					new_sibling = parent_item;
					old_indices.append(itemToIndex(new_sibling,0));
					old_indices.append(itemToIndex(new_sibling,1));
					old_indices.append(itemToIndex(new_sibling,2));
				}
				
				bool old_sibling_gets_ungrouped = false;
				ParamItem* old_sibling = NULL;
				if (item_to_move->parent()->childCount() == 2 && item_to_move->parent()!=root_item.data()) {
					old_sibling_gets_ungrouped = true;
					old_indices.append(itemToIndex(item_to_move->parent(),0));
					old_indices.append(itemToIndex(item_to_move->parent(),1));
					old_indices.append(itemToIndex(item_to_move->parent(),2));
					old_sibling = item_to_move->parent()->child(item_to_move->row()==0 ? 1 : 0);
					old_indices.append(itemToIndex(old_sibling,0));
					old_indices.append(itemToIndex(old_sibling,1));
					old_indices.append(itemToIndex(old_sibling,2));
					old_sibling = old_sibling->parent();
				}
				
				QSharedPointer<ParamItem> move_item = item_to_move->parent()->removeChild(move_index.row());
				parent_item->addChild(move_item,row);
				
				// change the indices of all columns of the moved item
				changePersistentIndex(old_indices.takeFirst(), itemToIndex(move_item.data(),0));
				changePersistentIndex(old_indices.takeFirst(), itemToIndex(move_item.data(),1));
				changePersistentIndex(old_indices.takeFirst(), itemToIndex(move_item.data(),2));
				if (new_parent_gets_grouped) {
					Q_ASSERT(row == 1);
					new_sibling = new_sibling->child(row == 1 ? 0 : 1);
					changePersistentIndex(old_indices.takeFirst(), itemToIndex(new_sibling,0));
					changePersistentIndex(old_indices.takeFirst(), itemToIndex(new_sibling,1));
					changePersistentIndex(old_indices.takeFirst(), itemToIndex(new_sibling,2));
				}
				if (old_sibling_gets_ungrouped) {
					// remove the indices of the old pair
					changePersistentIndex(old_indices.takeFirst(), QModelIndex());
					changePersistentIndex(old_indices.takeFirst(), QModelIndex());
					changePersistentIndex(old_indices.takeFirst(), QModelIndex());
					// move the remaining child up
					changePersistentIndex(old_indices.takeFirst(), itemToIndex(old_sibling,0));
					changePersistentIndex(old_indices.takeFirst(), itemToIndex(old_sibling,1));
					changePersistentIndex(old_indices.takeFirst(), itemToIndex(old_sibling,2));
				}
				// Now we might also change the indices of the items below remove and insert index ....
				emit layoutChanged();
// 				endResetModel();
				}
				catch (...) {
					Q_ASSERT(0); exit(-1);
				}
			}
		}
		return true;
	}
	return false;
}



ParamItem::ParamItem(ParamItem* parent, AbstractAttribute* item)
{
	if (parent) {
		parent_item = parent;
		attrib2item = parent->attrib2item;
		data_item = item;
		if (data_item) {
			type_name  = data_item->getType()->name;
			type_name.replace("cpm","");
			data_values.append(data_item->get());
			range_value = data_item->get();
			Q_ASSERT((*attrib2item).find(data_item) == (*attrib2item).end());
			(*attrib2item)[data_item] = this;
		}
		else {
			// create a plain group
		}
	}
	else {
		// plain root item
		attrib2item = new QMap<const AbstractAttribute*,ParamItem*>();
		parent_item = NULL;
		data_item = NULL;
	}
}


ParamItem::~ParamItem()
{
	childs.clear();
	if (data_item)
		(*attrib2item).remove(data_item);
	if (!parent_item)
		delete attrib2item;
	// disconnect all listeners ... 
}

void ParamItem::setRange(QString range)
{
	this->range_value = range;
	// decode the range into a value set
	bool integer = data_item->getType()->base_type == "xs:integer";

	QStringList list_values;
	
	QStringList tokens_colon = range.split(":");
	if( tokens_colon.size() == 3 ){
		bool fixed_num_intervals=false;
		bool logarithmic = false;
		if( tokens_colon[1].startsWith("#") ){
			tokens_colon[1].remove('#');
			fixed_num_intervals=true;
		}
		if( tokens_colon[1].endsWith("log") ){
			tokens_colon[1].remove("log");
			logarithmic=true;
		}
		
		double min = tokens_colon[0].toDouble();
		double max = tokens_colon[2].toDouble();
		
		bool reversed = false;
		if( max < min ){
			double temp_max = max;
			max = min;
			min = temp_max;
			reversed = true;
		}
		
		// linear spacing with explicit increment
		if( !fixed_num_intervals && !logarithmic){

			double increment = fabs(tokens_colon[1].toDouble());
			for (double i=min; i<=max; i+=increment) {
				if(integer)
					list_values.append( QString::number(int(i)) );
				else
					list_values.append( QString::number(i) );
			}
		} 
		if( fixed_num_intervals) { 
			
			int num_interval = abs(tokens_colon[1].toInt());
			if( num_interval < 1){
				//QMessageBox::warning(this, "At least one interval required");
				num_interval = 1;
			}  
			// linear spacing
			if( !logarithmic){
							
				double increment = (max-min) / (num_interval);
				for(uint i = 0; i < num_interval+1; i++){
					double v = min + i * increment;
					if(integer)
						list_values.append( QString::number(int(v)) );
					else
						list_values.append( QString::number(v) );
				}
			}
			// logarithmic spacing
			else if( logarithmic){
				
				if(min <= 0 || max <= 0){
					QMessageBox::warning(NULL, "ParamSweep", QString("Minimum and maximum must be greater than 0 for logarithmic sequence."),QMessageBox::Ok);
					return;
				}
				
				double min_exp = log10(min);
				double max_exp = log10(max);
				double increment_exp = (max_exp-min_exp) / double(num_interval);
				for (double i=0; i<num_interval+1; i++) {
					
					double exp = (min_exp + i*increment_exp );
					double y = pow(10.0, exp);
					
					if(integer)
						list_values.append( QString::number(int(y)) );
					else
						list_values.append( QString::number(y) );
				}
			}
			else{
				qDebug() << "Parameter sequence: Fixed_num_intervals but not linear or logarithmic";
			}
		}
		
		if(reversed){
			QList<QString> reversed_list;
			for(int i=list_values.size()-1; i>=0;i--){
				reversed_list.append( list_values[i] );
			}
			list_values = reversed_list;
		}
		
	}
	else { // semicolon separated list (list of values)
		QStringList tokens = range.split(";");
		for (int i=0; i<tokens.size(); i+=1) {
			list_values.append(tokens.at(i));
		}
	}
	data_values = list_values;
}


ParamItem* ParamItem::addChild(AbstractAttribute* attr, int pos)
{
	ParamItem* new_item = new ParamItem(this, attr);
	addChild(QSharedPointer<ParamItem>(new_item),pos);
	return new_item;
}

int ParamItem::addChild ( QSharedPointer< ParamItem > item, int pos)
{
	if (data_item) {
		(*attrib2item).remove(data_item);
		childs.append(QSharedPointer<ParamItem>(new ParamItem(this,data_item)));
		childs.back()->setRange(range_value);
		data_item = NULL;
	}
	item->parent_item = this;
	if (pos==-1) {
		childs.append(item);
		return childs.size()-1;
	}
	else {
		childs.insert(pos,item);
		return pos;
	}
}

QSharedPointer<ParamItem> ParamItem::removeChild(int i)
{
	// what is the policy for removing items ??? The item takes care for it's destruction ??
	QSharedPointer<ParamItem> item = childs[i];
	childs.removeAt(i);
	if (childs.size()==1 && parent_item) {
		this->data_item = childs.first()->attribute();
		this->setRange(childs.first()->range());
		type_name  = data_item->getType()->name;
		type_name.replace("cpm","");
		childs.clear();
		Q_ASSERT((*attrib2item).find(data_item) == (*attrib2item).end());
		(*attrib2item)[data_item] = this;
	}
	return item;
}

void ParamItem::moveChild(int from, int to)
{
	if (from >= childs.size() || to >= childs.size()) {
		qDebug() << "Invalid child move " << from << " -> " << to ;
		return;
	}
	
	childs.move(from,to);
}


QString ParamItem::range() const
{
	return range_value;
}

QStringList ParamItem::values() const
{
	return data_values;
}

QString ParamItem::type() const
{
	return type_name;
}

AbstractAttribute* ParamItem::attribute() const
{
	return data_item;
}

ParamItem* ParamItem::child(int i) const
{
// 	qDebug() << "Item " << (this->data_item ? data_item->getXMLPath() : (parent_item ? QString("pair %1").arg(parent_item->childPos(this)):QString("root") )) << " returning child " << i << " of " << childs.size();
	Q_ASSERT(i<childs.size());
	return childs[i].data();
}


int ParamItem::childCount() const
{
// 	qDebug() << "Item " << (this->data_item ? data_item->getXMLPath() : (parent_item ? QString("pair %1").arg(parent_item->childPos(this)):QString("root") )) << " returning child count " << childs.size();
	return childs.size();
}

int ParamItem::childPos(const ParamItem* item) const
{
	for (int i=0; i< childs.size(); i++) {
		if (childs[i].data() == item)
			return i;
	}
	return -1;
}

int ParamItem::row() const {
	if (parent_item) {
		return parent_item->childPos(this);
	}
	else
		return -1;
}

ParamItem* ParamItem::findItem ( const AbstractAttribute* attr ) const
{
	QMap<const AbstractAttribute*,ParamItem*>::const_iterator i;
	i = attrib2item->find(attr);
	if (i!=attrib2item->end())
		return *i;
	else {
		return NULL;
	}
}

ParamItem* ParamItem::parent() const
{
	return parent_item;
}
