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

#ifndef JOBSUMMARY_H
#define JOBSUMMARY_H

#include <QtGui>
#include "abstractattribute.h"
#include "job_queue.h"

using namespace std;

/*!
This dialog gets a set of parameters and their values from the parametersweeper.<br>
It calculates how much jobs will be started, in which order and shows you a summary.<br>
Accepting the summary will send a signal, that this sweep can be started.<br>
Canceling will reject this set.
*/
class jobSummary : public QDialog
{
Q_OBJECT

public:
    jobSummary(QList<AbstractAttribute*> params, QList<QStringList> param_sets, QString sweep_name);
    /*!<
      Constructs a summary of parameters and their values, for which simulation-processes will be started.
      \param params List of attribtues, who contains the values for the parameters
      \param param_sets List which describes the order in which the parameters will be executed
      */
    QList< QStringList > getJobList(); /*!< Returns the list of jobs which will be started. */
    QString getGroupName(); /*!< Returns the name of the group, in which the jobs will be started. */
    ~jobSummary();

private:
    QList<AbstractAttribute*> parameters; /*!< List of attributes. */
    QList< QStringList > parameter_sets; /*!< List which describes the order of attributes. */
    QTreeWidget *jobListWidget, *parameterListWidget;
    QLabel *lb_nrOfJobs; /*!< Label which displays the total number of jobs, which will be started. */
    QLineEdit *le_groupName; /*!< LineEdit which displays the group-name in which the jobs will be started. */

    void updateJobCount(); /*!< Updates the number of jobs. */
    void showJobs(); /*!< Shows all jobs which will be started and theri informations. */
};

#endif // JOBSUMMARY_H
