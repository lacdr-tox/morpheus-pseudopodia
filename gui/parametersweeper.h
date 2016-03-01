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

#ifndef PARAMETERSWEEPER_H
#define PARAMETERSWEEPER_H

#include <QtGui>
#include "abstractattribute.h"
#include "jobsummary.h"
#include "attrcontroller.h"



using namespace std;

typedef attrController delegate_class;

/*!
This class represents a parametersweeper on form of a widget, which is used to give a xml-attribute a range or set of values,<br>
which will be executed in separate simulations.<br>
On the left side of the QWidget you see a QTreeWidget, which includes all parameters<br>
you added to the list of attribtues.<br>
You can move the parameters position to change the pairwise or crosswise execution of simulations.<br>
*/
class parameterSweeper : public QWidget
{
Q_OBJECT

public:
	parameterSweeper();
	~parameterSweeper();

private:
	SharedMorphModel model;
	QSharedPointer<attrController> edit_delegate;
	QTreeView *param_sweep_view;
	QLineEdit* sweep_name, * n_sweeps;
	int newSweepID();

signals:
	void createSweep(SharedMorphModel model, int);
	void attributeDoubleClicked ( AbstractAttribute* attrib );

public slots:
	void submitSweep(); /*!< Creates a summary with all simulation process which will be started from the parameters. */
	void selectModel(int index);
	void loadSweep(int sweep_id);

private slots:
	void paramDoubleClicked(const QModelIndex& index);
	void updateJobCount(); /*!< Updates the list of values for the parameters. */
};

#endif // PARAMETERSWEEPER_H
