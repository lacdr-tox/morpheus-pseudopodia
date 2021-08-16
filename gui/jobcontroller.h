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

#ifndef JOBCONTROLLER_H
#define JOBCONTROLLER_H

#include <QtGui>
#include <QGroupBox>
#include <QSharedPointer>
#include <QFileSystemModel>
#include "localprocess.h"
#include "remoteprocess.h"
#include "config.h"
#include "job_queue.h"
#include "imagetable.h"
#include "moviedialog.h"

using namespace std;

/*!
With this class it is possible to start and stop simulations of a loaded cpm-model.<br>
Simulations can be started on the local machine or remote on another computer.<br>
The jobcontroller shows informations about all pending, running and finished jobs in a QTreeWidget.<br>
If the program (morpheus-gui) is closed before all jobs are done, the jobcontroller will store the informations in the QSettings<br>
and restore them on the next start of morpheus-gui.
*/
class JobView : public QWidget
{
Q_OBJECT

public:
    JobView();
    /*!<
      Creates a widget which displays all simulation-jobs whether or not they are finished.
      The jobs are listed in a treewidget in the middle of the widget.
      On the right you see a textedit which contains the output of the simulations.
      By the buttons stop and remove jobs can be stopped or removed.
      */
    ~JobView();

    void setJob(int job_id); // display job (normal job or sweep job)
    void setSweep(QList<int> job_ids); // display sweep info
	virtual void resizeEvent ( QResizeEvent * event ) { QWidget::resizeEvent(event); image_resize_timer.stop();image_resize_timer.start();}

private:
    QSharedPointer<abstractProcess> current_job;
    struct SweepDetails{
        int id;
        QString name;
        QString subdir;
        QString fullpath;
        QString summary_filename;
		QList< int > job_ids;
    } ;
    SweepDetails current_sweep;
    bool view_sweep;

    QPlainTextEdit *te_output; /*!< Textedit which shows the output of the selected process. */
    int text_shown;
    QFileSystemModel* dir_model;
	QItemSelectionModel* selection_model;
    QTreeView* output_files; 
	QSplitter* splitter_tree_text;
	QSplitter* splitter_output_preview;
	QSplitter* splitter_params_output;
	QStackedWidget *previewStack;
	QGraphicsView* imagePreview;
	QTimer image_resize_timer;
	QPlainTextEdit* textPreview;
    QAction *stop_action, *open_terminal_action, *open_output_action, *make_movie_action;
    QLabel *title;
	QGroupBox *gr_params/*, *gr_preview*/;
	QGroupBox* job_group_box;
    QListWidget *parameter_list;
	QLabel *par_sweep_title;
// 	QLineEdit *par_path;
	QPushButton* b_restore_sweep;
    QPushButton* b_make_movie;
    QPushButton* b_make_table;

    QMap<int, abstractProcess*> map_idToProcess; /*!< Map which connects the id of every process with the correspondending process. */

signals: 
	void selectSweep(int sweep_id);
	void stopJob(int job_id);
	
// signals:
	void imagePreviewChanged();
	
public slots:


private slots:
	void restoreSweep();
    void makeMovie();
	void makeTable();
    void updateJobState();
    void updateJobData();
    void updateSweepState();
    void updateSweepData( QString path );
    void stopJob();
    void fileClicked(QModelIndex);
	void previewSelectedFile(const QModelIndex &, const QModelIndex &);
	void resizeGraphicsPreview();
    void openOutputFolder();
    void openTerminal();

};

#endif // JOBCONTROLLER_H
