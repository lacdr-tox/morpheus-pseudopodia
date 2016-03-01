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

/*  This is a Collection of NodeAdapters for particular purposes.
 *  Maybe one can also creat more sophisticated GeneralPurposeAdapters => Mutually Exclusive Attributes
 *
 */

#ifndef NODEADAPTER_H
#define NODEADAPTER_H

#include <QObject>
#include "abstractattribute.h"

class LatticeStructureAdapter : public QObject
{
    Q_OBJECT

public:
    LatticeStructureAdapter(QObject* parent, AbstractAttribute* structure, AbstractAttribute* size);

private:
	QStringList stored_lengths;
	AbstractAttribute* a_structure;
	AbstractAttribute* a_size;

	int getDimensions (QString structure);

private Q_SLOTS:
	void structureChanged();
	void sizeChanged();
	void reset();
};

#endif // NODEADAPTER_H
