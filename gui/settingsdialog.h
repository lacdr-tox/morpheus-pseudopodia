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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtGui>
#include "remoteprocess.h"

/*!
This dialog provides 3 tabs. Every tab is used to type in and cofigure the settings of the application.<br>
For example: where should the simulation-data be stored, what are the login-informations for the external simulation-machine,<br>
how much jobs can be running parallel and so on.<br>
After typing in the informations you can save them and check whether they are correct and if a connection is possible.
*/
class settingsDialog : public QDialog
{
Q_OBJECT

public:
    settingsDialog(); /*!< Creates the dialog, but didn't open it. To open use the show()-function. */
    ~settingsDialog();

private:
    config::application app; /*!< Variable stores the application-informations from the config-class. */
    QLineEdit *le_general_outputDir; /*!< LineEdit to type in the local-directory where the simulation-data should be stored. */

    QSpinBox *sb_stdout_limit; /*!< Spinbox to type the maximum size of model.xml.out */
    QSpinBox *sb_max_recent_files; /*!< Spinbox to type the maximum number of recent files */
    QSpinBox *sb_jobqueue_interval; /*!< Spinbox to type the interval between updates of jobs in JobQueue */
    QSpinBox *sb_jobqueue_interval_remote; /*!< Spinbox to type the interval between updates of jobs in JobQueue */

    QLineEdit *le_local_executable; /*!< LineEdit to type in the path to the executable of the local morpheus executable. */
    QLineEdit *le_local_GnuPlot_executable; /*!< LineEdit to type in the path to the executable of the local GnuPlot. */
    QLineEdit *le_local_FFmpeg_executable; /*!< LineEdit to type in the path to the executable of the local FFmpeg. */
    QSpinBox *sb_local_maxThreads; /*!< SpinBox to type in the maximal number of threads used on the local machine. */
    QSpinBox *sb_local_maxConcurrentJobs; /*!< Spinbox to type in the maximal number of parallel running jobs on the local machine. */
    QSpinBox *sb_local_timeout; /*!< Spinbox to type in the maximal duration of a local job, before he is stopped. */
    QLabel *lb_localSimStatus; /*!< Label which displays the state of local simulation. */
    QLabel *lb_localSimRev; /*!< Label which displays the Revision of the local used simulator. */

    QLineEdit *le_remote_user; /*!< LineEdit to type in the username for external jobs. */
    QLineEdit *le_remote_host; /*!< LineEdit to type in the hostname of the external machine. */
    QLineEdit *le_remote_executable; /*!< LineEdit to type in the path to the remote Morpheus executable. */
    QLineEdit *le_remote_GnuPlot_executable; /*!< LineEdit to type in the path to the remote GnuPlot executable. */
    QLineEdit *le_remote_FFmpeg_executable; /*!< LineEdit to type in the path to the remote FFmpeg executable. */
    QLineEdit *le_remote_simDir; /*!< LineEdit to type in the directory of the external machine where simulation should be run. */
    QSpinBox *sb_remote_maxThreads; /*!< Spinbox to type in the maximal number of threads used on the external machine. */
    QComboBox *cb_remote_dataSyncType; /*!< Combobox to choose the type of syncing the simulation-data. */
    QLabel *lb_remoteConnectionStatus; /*!< Label which displays the connection-state of the external machine. */
    QLabel *lb_remoteSimStatus; /*!< Label which displays the state of the external simulator. */
    QLabel *lb_remoteSimRev; /*!< Label which displays the revision of the used external simulator. */

    bool checkValues(); /*!< Returns if the entered informations are correct or not. */
    void createGeneralTab(QTabWidget *tabWid);
    /*!<
      Creates a tab for all general settings.
      \param tabWid Parent which holds the new tab.
      */
    void createLocalTab(QTabWidget *tabWid);
    /*!<
      Creates a tab for all local settings.
      \param tabWid Parent which holds the new tab.
      */
    void createRemoteTab(QTabWidget *tabWid);
    /*!<
      Creates a tab for all external settings.
      \param tabWid Parent which holds the new tab.
      */
    void createPreferenceTab(QTabWidget *tabWid);
    /*!<
      Creates a tab for all GUI preferences.
      \param tabWid Parent which holds the new tab.
      */

private slots:
    void saveSettings(); /*!< Stores the entered settings in the config-class. */
    void openFileDialogOutputDir(); /*!< Opens a dialog to select a directory on the filesystem, where the output should be stored. */
	void openFileDialogExecutable(); /*!< Opens a dialog to select a directory on the filesystem, where the output should be stored. */
	void openFileDialogGnuPlotExecutable(); /*!< Opens a dialog to select a directory on the filesystem, where the output should be stored. */
    void openFileDialogFFmpegExecutable(); /*!< Opens a dialog to select a directory on the filesystem, where the output should be stored. */
    void checkLocal(); /*!< Checks if the local simulator is reachable. */
    void checkRemote(); /*!< Checks if the external simulator is reachable. */
};

#endif // SETTINGSDIALOG_H
