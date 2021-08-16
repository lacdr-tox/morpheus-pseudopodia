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

#ifndef INFOACTION_H
#define INFOACTION_H

#include <QAction>
#include <QtGui>
#include <QToolTip>
/*!
infoAction is a subclass of QAction. <br>
If you move your mouse over this action a tooltip will appear and describe the object more exact. <br>
To change the tooltip-text after creating the object use setToolTip().
*/
class infoAction : public QAction
{
Q_OBJECT

public:
    infoAction(QString n, QString s, QObject *o);
    /*!<
      Creates a instance of this class.
      \param n Text of the action.
      \param s Tooltip-text which is shown when the mouse hover over the action.
      \param o Parent of the action, which will handle it.
      */

private slots:
    void showInfo(); /*!< Opens the current tooltip. */
};

#endif // INFOACTION_H
