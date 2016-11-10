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

#ifndef ATTRCONTROLLER_H
#define ATTRCONTROLLER_H

#include <QtGui>
#include <QItemDelegate>
#include <QSharedPointer>
#include <QRegExpValidator>
#include "abstractattribute.h"
#include "config.h"
#include "mathtextedit.h"

#include <iostream>
using namespace std;

/*!
This class creates a delegate on the given QObject, depending on the type of given AbstractAttribute.
If the user wants to change the value, of the QObject, which represents the value of the Attribute, then the delegate masks
the input of user or it allows only specific values, which are possible for the AbstractAttribute.
*/
class attrController : public QItemDelegate
{
Q_OBJECT

public:
    attrController(QObject *parent, AbstractAttribute* attr, bool range = false);
    /*!< Constructs a delegate for the Object (parent), which handles the value(s) of the given abstract-attribute.*/
	AbstractAttribute* getAttribute() const { return attr; }
	void setAttribute( AbstractAttribute* attribute );

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool eventFilter ( QObject * editor, QEvent * event ) override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;

private:
	enum EditWid { NoEdit=4, MathText=0 , EnumBox=2, LineText=3, SystemPath=4 } widget_type;
	
    AbstractAttribute* attr; /*!< abstract-attribute to which the delegate belongs and whom editing shall be masked. */
    QRegExpValidator val; /*!< Validator which mask the input. */
    bool is_range;
	QString pattern;
    void setValidator(); /*!< Sets the matching validator on the editor-object. */
};
#endif // ATTRCONTROLLER_H
