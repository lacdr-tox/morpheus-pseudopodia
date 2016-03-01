#include "jobsummary.h"

jobSummary::jobSummary(QList<AbstractAttribute*> params, QList<QStringList> param_sets, QString sweep_name)
{
    parameter_sets = param_sets;
    parameters = params;

    QLabel *lb_sweepName = new QLabel("Name of sweep", this );
    QLabel *lb_parameters = new QLabel("Parameter names (P)", this );
    QLabel *lb_summary = new QLabel("Parameter sets:", this);
    le_groupName = new QLineEdit();

    
    le_groupName->setText(sweep_name);

    jobListWidget = new QTreeWidget(this);
    jobListWidget->setRootIsDecorated(false);
    parameterListWidget = new QTreeWidget(this);
    parameterListWidget->setMaximumHeight(100);
    parameterListWidget->setHeaderLabels(QStringList() << "Parameter" << "XML path");;
    parameterListWidget->setRootIsDecorated(false);

    lb_nrOfJobs = new QLabel("Number of jobs: ", this);

    QPushButton *bt_createJobs = new QPushButton("Submit jobs", this);
    QPushButton *bt_cancel = new QPushButton("Cancel", this);

    //QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

    QGridLayout *lay = new QGridLayout(this);
    lay->addWidget(lb_sweepName,0,0,1,3);
    lay->addWidget(le_groupName,1,0,1,3);
    lay->addWidget(lb_parameters,2,0,1,3);
    lay->addWidget(parameterListWidget, 3, 0, 1, 3);
    lay->addWidget(lb_summary, 4, 0, 1, 3);
	lay->addWidget(jobListWidget, 5, 0, 1, 3);
    lay->addWidget(lb_nrOfJobs, 6, 0, 1, 1);
    lay->addWidget(bt_createJobs, 7, 0, 1, 1);
    lay->addWidget(bt_cancel, 7, 1, 1, 1);

    showJobs();
    updateJobCount();

    connect(bt_createJobs, SIGNAL(clicked()), SLOT( accept() ));
    connect(bt_cancel, SIGNAL(clicked()), SLOT( reject() ));
}

//------------------------------------------------------

QList< QStringList > jobSummary::getJobList() { return parameter_sets; };

//------------------------------------------------------

QString jobSummary::getGroupName() { return le_groupName->text(); };

//------------------------------------------------------


void jobSummary::showJobs()
{
    jobListWidget->clear();

    QStringList headers;
    headers << "Job";
    parameterListWidget->setColumnCount(2);
    parameterListWidget->setMaximumHeight(100);
    for (int i=0; i< parameters.size(); i++ ){
        QTreeWidgetItem *item = new QTreeWidgetItem(parameterListWidget);
        QString paramCode = QString("P")+QString::number(i+1);
        item->setData(0,0,paramCode);
        item->setData(1,0,parameters[i]->getXMLPath().remove("MorpheusModel"));

        headers << paramCode;
    }
    jobListWidget->setHeaderLabels(headers);
    jobListWidget->setColumnCount(parameter_sets[0].size()+1);

    for (int i=0; i< parameter_sets.size(); i++ )
    {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setData(0, 0, i+1);

        for (int j = 0; j < parameter_sets[i].size(); j++)
        {
            item->setData(j+1, 0, parameter_sets[i][j]);
        }
        jobListWidget->addTopLevelItem(item);
    }
}

//------------------------------------------------------

void jobSummary::updateJobCount()
{
    int jobs = parameter_sets.size();
    lb_nrOfJobs->setTextFormat(Qt::RichText);

    lb_nrOfJobs->setText(QString("<b>Number of jobs: ") + QString::number(jobs) + QString("</b>" ));
}

//------------------------------------------------------

jobSummary::~jobSummary()
{}
