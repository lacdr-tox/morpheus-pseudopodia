#include "nodeadapter.h"

LatticeStructureAdapter::LatticeStructureAdapter(QObject* parent, AbstractAttribute* structure, AbstractAttribute* size) :
QObject(parent), a_structure(structure), a_size(size)
{
	const QString default_length = "100";
	stored_lengths << default_length << default_length << default_length;

	connect( a_structure, SIGNAL(changed(AbstractAttribute*)), this, SLOT(structureChanged()) );
	connect( a_size ,SIGNAL(changed(AbstractAttribute*)), this, SLOT(sizeChanged()) );
	connect( a_structure, SIGNAL(deleted(AbstractAttribute*)), this, SLOT(reset()) );
	connect( a_size ,SIGNAL(deleted(AbstractAttribute*)), this, SLOT(reset()) );	
	
	this->structureChanged();

}

int LatticeStructureAdapter::getDimensions(QString structure)
{
	int dimensions = 0;
	if (structure == "linear") {
		dimensions = 1;
	}
	else if (structure == "square" ) {
		dimensions = 2;
	}
	else if (structure == "hexagonal" ) {
		dimensions = 2;
	}
	else if (structure == "cubic" ) {
		dimensions = 3;
	}
	return dimensions;
}


void LatticeStructureAdapter::structureChanged()
{
	bool changed= false;
	
	if (!a_structure) { qDebug() << "Structure Attribute already destroyed in LatticeStructureAdapter::structureChanged()"; return; }
	if (!a_size) { qDebug() << "Size Attribute already destroyed in LatticeStructureAdapter::structureChanged()"; return; }
	
	int dimensions = getDimensions(a_structure->get());
	QStringList lengths = a_size->get().split(QRegExp("[\\s,]+"));
// 	qDebug() << "Got lattice size " << lengths;

	if (lengths.size() < 3) {
		lengths = stored_lengths;
		changed = true;
	}

	for (uint dim=0; dim<3; dim++) {
		if (dim < dimensions) {
			if (lengths[dim].toInt() < 1) {
				lengths[dim] = stored_lengths[dim];
				changed = true;
			}
			else
				stored_lengths[dim] = lengths[dim];
		}
		else {
			if (lengths[dim].toInt() != 0) {
				lengths[dim] = "0";
				changed = true;
			}
		}
	}

	if (changed) {
		QMetaObject::invokeMethod( a_size, "set", Qt::AutoConnection, Q_ARG(QString, lengths.join(", ") ) );
	}
}

void LatticeStructureAdapter::sizeChanged()
{
	
	if (!a_structure) { qDebug() << "Cannot convert structure Attribute into SharedPoint in LatticeStructureAdapter::sizeChanged()"; return; }
	int dimensions = getDimensions(a_structure->get());

	if (!a_size) { qDebug() << "Cannot convert size Attribute into SharedPoint in LatticeStructureAdapter::sizeChanged()"; return; }
	QStringList lengths = a_size->get().split(QRegExp("[\\s,]+"));

	bool changed = false;

	for (uint dim=0; dim<3; dim++) {
		if (dim < dimensions) {
			if (lengths[dim].toInt() < 1) {
				lengths[dim] = stored_lengths[dim];
				changed = true;
			}
			else {
				stored_lengths[dim] = lengths[dim];
			}
		}
		else {
			if (lengths[dim].toInt() != 0) {
				lengths[dim] = "0";
				changed = true;
			}
		}
	}

	if (changed)
		QMetaObject::invokeMethod( a_size, "set", Qt::AutoConnection, Q_ARG(QString, lengths.join(", ") ) );
}

void LatticeStructureAdapter::reset()
{
	a_structure = nullptr; 
	a_size = nullptr;
}
