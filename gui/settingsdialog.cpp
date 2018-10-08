#include "settingsdialog.h"

settingsDialog::settingsDialog()
{
    app = config::getApplication();

    QTabWidget *tabWid = new QTabWidget(this);
    createGeneralTab(tabWid);
    createLocalTab(tabWid);
    createRemoteTab(tabWid);
    createPreferenceTab(tabWid);

    QPushButton *bt_cancel = new QPushButton("Cancel", this);
    QPushButton *bt_save = new QPushButton("Save", this);

    QGridLayout *gl = new QGridLayout(this);
	gl->setRowStretch(0,1);
    gl->addWidget(tabWid, 0, 0, 1, 3);
	gl->addItem(new QSpacerItem(0,0,QSizePolicy::Expanding,QSizePolicy::Fixed), 1,0,1,1);
    gl->addWidget(bt_save, 1, 1, 1, 1);
    gl->addWidget(bt_cancel, 1, 2, 1, 1);

    QObject::connect(bt_cancel, SIGNAL(clicked()), this, SLOT(close()));
    QObject::connect(bt_save, SIGNAL(clicked()), this, SLOT(saveSettings()));

    setWindowTitle( "Settings" );
}

//---------------------------------------------------------------------------------------------------

settingsDialog::~settingsDialog()
{}

//---------------------------------------------------------------------------------------------------

// QGroupBox settingsDialog::createTerminalBox {
// 	QGroupBox *groupBox = new QGroupBox(tr("Terminals"));
// 	QStringList terminal_apps;
// #ifdef Q_OS_WIN32
// 	terminal_apps << "cmd.exe";
// #elseif Q_OS_LINUX
// 	terminal_apps << "konsole" << "terminal";
// #else
// 	terminal_apps << 
// #endif
// 		 
//      QRadioButton *radio1 = new QRadioButton(tr(""));
//      QRadioButton *radio2 = new QRadioButton(tr("R&adio button 2"));
//      QRadioButton *radio3 = new QRadioButton(tr("Ra&dio button 3"));
// 	 
// 
//      radio1->setChecked(true);
// The first group box contains and manages three radio buttons. Since the group box contains only radio buttons, it is exclusive by default, so only one radio button can be checked at any given time. We check the first radio button to ensure that the button group contains one checked button.
// 
//      QVBoxLayout *vbox = new QVBoxLayout;
//      vbox->addWidget(radio1);
//      vbox->addWidget(radio2);
//      vbox->addWidget(radio3);
//      vbox->addStretch(1);
//      groupBox->setLayout(vbox);
// 	      return groupBox;
// }

void settingsDialog::createGeneralTab(QTabWidget *tabWid)
{

    QWidget *general = new QWidget(tabWid);

    QLabel *lb_outputDir = new QLabel("Output directory: ", general);
    le_general_outputDir = new QLineEdit(general);
    le_general_outputDir->setText(app.general_outputDir);
    QPushButton *bt_outputDir = new QPushButton(QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)),"", general);
		
	QLabel *lb_feedback = new QLabel("Permit usage feedback: ", general);
	cb_feedback = new QCheckBox(general);
	cb_feedback->setChecked( app.preference_allow_feedback );

    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);



    QGridLayout *lay = new QGridLayout(general);
    lay->setColumnStretch(1,1);
    uint row=0;
//     lay->addWidget(lb_xsdPath,          row, 0, 1, 1);
//     lay->addWidget(le_general_xsdPath,  row, 1, 1, 1);
//     lay->addWidget(bt_xsdPath,          row, 2, 1, 1);
// 
//     row++;
    lay->addWidget(lb_outputDir,        row, 0, 1, 1);
    lay->addWidget(le_general_outputDir,row, 1, 1, 1);
    lay->addWidget(bt_outputDir,        row, 2, 1, 1);

	row++;
	lay->addWidget(lb_feedback,     row, 0, 1, 1);
	lay->addWidget(cb_feedback,     row, 1, 1, 1);
	
    row++;
    lay->addItem(spacer, row, 3, 1, 1);

    tabWid->addTab(general, "General");

    QObject::connect(bt_outputDir, SIGNAL(clicked()), this, SLOT(openFileDialogOutputDir()));
//     QObject::connect(bt_xsdPath,   SIGNAL(clicked()), this, SLOT(openFileDialogXSD()));

    le_general_outputDir->setText(config::getApplication().general_outputDir);
}

void settingsDialog::createPreferenceTab(QTabWidget *tabWid)
{

    QWidget *pref = new QWidget(tabWid);

    QLabel *lb_recent_files = new QLabel("Recent files: ", pref);
    sb_max_recent_files = new QSpinBox(pref);
    sb_max_recent_files->setMinimum(0);
    sb_max_recent_files->setMaximum(100);
    sb_max_recent_files->setValue( app.preference_max_recent_files );

    QLabel *lb_stdout_limit = new QLabel("Output text limit: ", pref);
    sb_stdout_limit = new QSpinBox(pref);
    sb_stdout_limit->setMinimum(1);
    sb_stdout_limit->setMaximum(100000000);
    sb_stdout_limit->setValue( app.preference_stdout_limit );
    QLabel *lb_stdout_limit_unit = new QLabel("Mb ", pref);

    QLabel *lb_jobqueue_interval = new QLabel("Update local jobs: ", pref);
    sb_jobqueue_interval = new QSpinBox(pref);
    sb_jobqueue_interval->setMinimum(100);
    sb_jobqueue_interval->setMaximum(600000);
    sb_jobqueue_interval->setSingleStep(1000);
    sb_jobqueue_interval->setValue( app.preference_jobqueue_interval );
    QLabel *lb_jobqueue_interval_unit = new QLabel("msec ", pref);

    QLabel *lb_jobqueue_interval_remote = new QLabel("Update remote jobs: ", pref);
    sb_jobqueue_interval_remote = new QSpinBox(pref);
    sb_jobqueue_interval_remote->setMinimum(100);
    sb_jobqueue_interval_remote->setMaximum(600000);
    sb_jobqueue_interval_remote->setSingleStep(1000);
    sb_jobqueue_interval_remote->setValue( app.preference_jobqueue_interval_remote );
    QLabel *lb_jobqueue_interval_remote_unit = new QLabel("msec ", pref);

    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);


    // layout
    QGridLayout *lay = new QGridLayout(pref);
    lay->setColumnStretch(1,1);
    uint row=0;

    lay->addWidget(lb_recent_files,     row, 0, 1, 1);
    lay->addWidget(sb_max_recent_files,     row, 1, 1, 1);

    row++;
    lay->addWidget(lb_stdout_limit,     row, 0, 1, 1);
    lay->addWidget(sb_stdout_limit,     row, 1, 1, 1);
    lay->addWidget(lb_stdout_limit_unit,row, 2, 1, 1);

    row++;
    lay->addWidget(lb_jobqueue_interval,     row, 0, 1, 1);
    lay->addWidget(sb_jobqueue_interval,     row, 1, 1, 1);
    lay->addWidget(lb_jobqueue_interval_unit,row, 2, 1, 1);

    row++;
    lay->addWidget(lb_jobqueue_interval_remote,     row, 0, 1, 1);
    lay->addWidget(sb_jobqueue_interval_remote,     row, 1, 1, 1);
    lay->addWidget(lb_jobqueue_interval_remote_unit ,row, 2, 1, 1);
	
    row++;
    lay->addItem(spacer, row, 3, 1, 1);

    tabWid->addTab(pref, "GUI");

}

//---------------------------------------------------------------------------------------------------

void settingsDialog::createLocalTab(QTabWidget *tabWid)
{

    QWidget *local = new QWidget(tabWid);

    QLabel *lb_executable = new QLabel("Simulator executable: ", local);
    le_local_executable = new QLineEdit(local);
    le_local_executable->setText(  app.local_executable );
	QPushButton *bt_executable = new QPushButton(QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), "Browse...", local);
	bt_executable->setText("");
	connect(bt_executable,SIGNAL(clicked(bool)),SLOT(openFileDialogExecutable()));

	QCheckBox* cb_local_GnuPlotExecutable = new QCheckBox(local);
	cb_local_GnuPlotExecutable->setText("Gnuplot executable: ");
	cb_local_GnuPlotExecutable->setCheckState(app.local_GnuPlot_executable.isEmpty() ? Qt::Unchecked : Qt::Checked);
    QString gnuplot_exec = config::getPathToExecutable( "gnuplot" );
	le_local_GnuPlot_executable = new QLineEdit(local);
    le_local_GnuPlot_executable->setText(  (app.local_GnuPlot_executable.isEmpty()?gnuplot_exec:app.local_GnuPlot_executable) );
	le_local_GnuPlot_executable->setEnabled(cb_local_GnuPlotExecutable->checkState() == Qt::Checked);
	connect(cb_local_GnuPlotExecutable,SIGNAL(toggled(bool)),le_local_GnuPlot_executable, SLOT(setEnabled(bool)));
	QPushButton *bt_GnuPlot_executable = new QPushButton(QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), "Browse...", local);
	bt_GnuPlot_executable->setText("");
	connect(bt_GnuPlot_executable,SIGNAL(clicked(bool)),SLOT(openFileDialogGnuPlotExecutable()));
	bt_GnuPlot_executable->setEnabled(cb_local_GnuPlotExecutable->checkState() == Qt::Checked);
	connect(cb_local_GnuPlotExecutable,SIGNAL(toggled(bool)),bt_GnuPlot_executable, SLOT(setEnabled(bool)));

    QCheckBox* cb_local_FFmpegExecutable = new QCheckBox(local);
    cb_local_FFmpegExecutable->setText("FFmpeg executable: ");

    cb_local_FFmpegExecutable->setCheckState(app.local_FFmpeg_executable.isEmpty() ? Qt::Unchecked : Qt::Checked);
    QString ffmpeg_exec = config::getPathToExecutable( "ffmpeg" );
    le_local_FFmpeg_executable = new QLineEdit(local);
    le_local_FFmpeg_executable->setText(  (app.local_FFmpeg_executable.isEmpty()?ffmpeg_exec:app.local_FFmpeg_executable) );
    le_local_FFmpeg_executable->setEnabled(cb_local_FFmpegExecutable->checkState() == Qt::Checked);
    connect(cb_local_FFmpegExecutable,SIGNAL(toggled(bool)),le_local_FFmpeg_executable, SLOT(setEnabled(bool)));
    QPushButton *bt_FFmpeg_executable = new QPushButton(QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), "Browse...", local);
    bt_FFmpeg_executable->setText("");
    connect(bt_FFmpeg_executable,SIGNAL(clicked(bool)),SLOT(openFileDialogFFmpegExecutable()));
    bt_FFmpeg_executable->setEnabled(cb_local_FFmpegExecutable->checkState() == Qt::Checked);
    connect(cb_local_FFmpegExecutable,SIGNAL(toggled(bool)),bt_FFmpeg_executable, SLOT(setEnabled(bool)));


    QLabel *lb_localSim = new QLabel("Check simulator: ", local);
    lb_localSimStatus = new QLabel(local);
    lb_localSimStatus->setPixmap(QThemedIcon("dialog-question" ,style()->standardIcon(QStyle::SP_MessageBoxQuestion)).pixmap(24));
    lb_localSimRev = new QLabel("", local);

    QPushButton *bt_checkLocal = new QPushButton("Test", local);
    bt_checkLocal->setFixedWidth(bt_checkLocal->width());

    QLabel *lb_maxLocalThreads = new QLabel("Threads per job: ", local);
    sb_local_maxThreads = new QSpinBox(local);
    sb_local_maxThreads->setMinimum(1);
    sb_local_maxThreads->setValue( app.local_maxThreads );

    QLabel *lb_maxLocalJobs = new QLabel("Concurrent jobs: ", local);
    sb_local_maxConcurrentJobs = new QSpinBox(local);
    sb_local_maxConcurrentJobs->setMinimum(1);
    sb_local_maxConcurrentJobs->setValue( app.local_maxConcurrentJobs );

    QLabel *lb_maxLocalTime = new QLabel("Timeout (min): ", local);
    sb_local_timeout = new QSpinBox(local);
    sb_local_timeout->setMinimum(1);
    sb_local_timeout->setMaximum(36000);
    sb_local_timeout->setValue( app.local_timeOut );

    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QGridLayout *lay = new QGridLayout(local);


    lay->setColumnMinimumWidth(0,150);
	lay->setColumnStretch(2,10);

    int row=0;
    lay->addWidget(lb_executable,               row, 0, 1, 1);
    lay->addWidget(le_local_executable,         row, 1, 1, 2);
    lay->addWidget(bt_executable,               row, 3, 1, 1);

    row++;
    lay->addWidget(cb_local_GnuPlotExecutable,   row, 0, 1, 1);
    lay->addWidget(le_local_GnuPlot_executable, row, 1, 1, 2);
    lay->addWidget(bt_GnuPlot_executable,       row, 3, 1, 1);
	
    row++;
    lay->addWidget(cb_local_FFmpegExecutable,   row, 0, 1, 1);
    lay->addWidget(le_local_FFmpeg_executable, row, 1, 1, 2);
    lay->addWidget(bt_FFmpeg_executable,      row, 3, 1, 1);

    row++;
    lay->addWidget(lb_localSim,                 row, 0, 1, 1);
    lay->addWidget(lb_localSimStatus,           row, 1, 1, 1);
    lay->addWidget(lb_localSimRev,              row, 2, 1, 2);
    
    row++;
    lay->addWidget(bt_checkLocal,               row, 1, 1, 2);

    row++;
    lay->addWidget(lb_maxLocalThreads,          row, 0, 1, 1);
    lay->addWidget(sb_local_maxThreads,         row, 1, 1, 1);

    row++;
    lay->addWidget(lb_maxLocalJobs,             row, 0, 1, 1);
    lay->addWidget(sb_local_maxConcurrentJobs,  row, 1, 1, 1);

    row++;
    lay->addWidget(lb_maxLocalTime,             row, 0, 1, 1);
    lay->addWidget(sb_local_timeout,            row, 1, 1, 1);
    lay->addItem(spacer,                        row, 0, 1, 4);

    tabWid->addTab(local, "Local");

    config::application app = config::getApplication();

    sb_local_maxConcurrentJobs->setValue(app.local_maxConcurrentJobs);
    sb_local_maxThreads->setValue(app.local_maxThreads);
    sb_local_timeout->setValue(app.local_timeOut);

    connect(bt_checkLocal, SIGNAL(clicked()), this, SLOT(checkLocal()));
}

//---------------------------------------------------------------------------------------------------

void settingsDialog::createRemoteTab(QTabWidget *tabWid)
{
    QWidget *remote = new QWidget(this);

    QLabel *lb_user = new QLabel("Username: ", remote);
    le_remote_user = new QLineEdit(remote);
    le_remote_user->setText( app.remote_user );

    QLabel *lb_host = new QLabel("Host: ", remote);
    le_remote_host = new QLineEdit(remote);
    le_remote_host->setText( app.remote_host );

    QLabel *lb_executableRemote = new QLabel("Simulator executable: ", remote);
    le_remote_executable = new QLineEdit(remote);
    le_remote_executable->setText(  app.remote_executable );


	QCheckBox* cb_GnuPlotExecutableRemote = new QCheckBox(remote);
	cb_GnuPlotExecutableRemote->setText("Gnuplot executable: ");
	cb_GnuPlotExecutableRemote->setCheckState(app.remote_GnuPlot_executable.isEmpty() ? Qt::Unchecked : Qt::Checked);
    le_remote_GnuPlot_executable = new QLineEdit(remote);
    le_remote_GnuPlot_executable->setText(  app.remote_GnuPlot_executable );
	le_remote_GnuPlot_executable->setEnabled(cb_GnuPlotExecutableRemote->checkState()==Qt::Checked);
	connect(cb_GnuPlotExecutableRemote,SIGNAL(toggled(bool)),le_remote_GnuPlot_executable, SLOT(setEnabled(bool)));

    QLabel *lb_remoteSimDir = new QLabel("Simulation folder: ", remote);
    le_remote_simDir = new QLineEdit(remote);
    le_remote_simDir->setText(  app.remote_simDir );


    QLabel *lb_maxRemoteThreads = new QLabel("Threads per job: ", remote);
    sb_remote_maxThreads = new QSpinBox(remote);
    sb_remote_maxThreads->setMinimum(1);
    sb_remote_maxThreads->setMaximum(100);
    sb_remote_maxThreads->setValue( app.remote_maxThreads );

    QLabel *lb_syncing = new QLabel("Syncing files: ", remote);
    cb_remote_dataSyncType = new QComboBox(remote);
    cb_remote_dataSyncType->addItems(QStringList()<<"Afterwards"<<"Continuous");
    cb_remote_dataSyncType->setCurrentIndex( ((app.remote_dataSyncType == "Afterwards") ? 0 : 1) );


    QLabel *lb_remoteConnection = new QLabel("Check connection: ", remote);
    lb_remoteConnectionStatus = new QLabel(remote);
    lb_remoteConnectionStatus->setPixmap(QThemedIcon("dialog-question" ,style()->standardIcon(QStyle::SP_MessageBoxQuestion)).pixmap(24));

    QLabel *lb_remoteSim = new QLabel("Check simulator: ", remote);
    lb_remoteSimStatus = new QLabel(remote);
    lb_remoteSimStatus->setPixmap(QThemedIcon("dialog-question" ,style()->standardIcon(QStyle::SP_MessageBoxQuestion)).pixmap(24));
    lb_remoteSimRev = new QLabel("", remote);


    QPushButton *bt_checkRemote = new QPushButton("Test", remote);
    bt_checkRemote->setFixedWidth(bt_checkRemote->width());



    QSpacerItem *spacer = new QSpacerItem(1, 1, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    QGridLayout *lay = new QGridLayout(remote);
    lay->setColumnMinimumWidth(0,150);
	lay->setColumnMinimumWidth(0,75);
	lay->setColumnMinimumWidth(2,150);
	lay->setColumnStretch(2,1);

    lay->addWidget(lb_user,                     0, 0, 1, 1);
    lay->addWidget(le_remote_user,              0, 1, 1, 2);

    lay->addWidget(lb_host,                     1, 0, 1, 1);
    lay->addWidget(le_remote_host,              1, 1, 1, 2);


    lay->addWidget(lb_executableRemote,         2, 0, 1, 1);
    lay->addWidget(le_remote_executable,        2, 1, 1, 1);

	lay->addWidget(cb_GnuPlotExecutableRemote,  3, 0, 1, 1);
    lay->addWidget(le_remote_GnuPlot_executable,3, 1, 1, 2);

    lay->addWidget(lb_remoteSimDir,             4, 0, 1, 1);
    lay->addWidget(le_remote_simDir,            4, 1, 1, 2);


    lay->addWidget(lb_maxRemoteThreads,         5, 0, 1, 1);
    lay->addWidget(sb_remote_maxThreads,        5, 1, 1, 1);


    lay->addWidget(lb_syncing,                  6, 0, 1, 1);
    lay->addWidget(cb_remote_dataSyncType,      6, 1, 1, 1);


    lay->addWidget(lb_remoteConnection,         7, 0, 1, 1);
    lay->addWidget(lb_remoteConnectionStatus,   7, 1, 1, 1);

    lay->addWidget(lb_remoteSim,                8, 0, 1, 1);
    lay->addWidget(lb_remoteSimStatus,          8, 1, 1, 1);
    lay->addWidget(lb_remoteSimRev,             8, 2, 1, 1);
    lay->addWidget(bt_checkRemote,              9, 1, 1, 1);

    lay->addItem(spacer,                        10, 3, 1, 1);

    int idx = tabWid->addTab(remote, "Remote");
//  TODO Remote config disabled
	tabWid->setTabEnabled(idx, false);

    config::application app = config::getApplication();
    le_remote_user->setText(app.remote_user);
    le_remote_host->setText(app.remote_host);
    le_remote_simDir->setText(app.remote_simDir);
    le_remote_executable->setText(app.remote_executable);
    sb_remote_maxThreads->setValue(app.remote_maxThreads);
    if(app.remote_dataSyncType == "Afterwards")
        cb_remote_dataSyncType->setCurrentIndex(0);
    else
        cb_remote_dataSyncType->setCurrentIndex(1);

    connect(bt_checkRemote, SIGNAL(clicked()), this, SLOT(checkRemote()));
}

//---------------------------------------------------------------------------------------------------

void settingsDialog::saveSettings()
{
    if(checkValues())
    {
        // TODO - write to settings.conf-file
        config::application app_new = config::getApplication();

        // General
        app_new.general_outputDir           = le_general_outputDir->text();

        // Preferences
        app_new.preference_stdout_limit = sb_stdout_limit->value();
        app_new.preference_max_recent_files = sb_max_recent_files->value();
        app_new.preference_jobqueue_interval= sb_jobqueue_interval->value();
        app_new.preference_jobqueue_interval_remote= sb_jobqueue_interval_remote->value();
		app_new.preference_allow_feedback = cb_feedback->isChecked();

        qDebug() << "Preferences: "<< endl;
        qDebug() << "\t app.preference_stdout_limit             :  " <<app_new.preference_stdout_limit << endl;
        qDebug() << "\t app.preference_max_recent_files         :  " << app_new.preference_max_recent_files << endl;
        qDebug() << "\t app.preference_jobqueue_interval        :  " << app_new.preference_jobqueue_interval << endl;
        qDebug() << "\t app.preference_jobqueue_interval_remote :  " << app_new.preference_jobqueue_interval_remote << endl << endl;

        // Local
        app_new.local_executable            = le_local_executable->text();
        app_new.local_GnuPlot_executable    = (le_local_GnuPlot_executable->isEnabled() ? le_local_GnuPlot_executable->text() : "");
        app_new.local_FFmpeg_executable     = (le_local_FFmpeg_executable->isEnabled() ? le_local_FFmpeg_executable->text() : "");
        app_new.local_maxConcurrentJobs     = sb_local_maxConcurrentJobs->value();
        app_new.local_maxThreads            = sb_local_maxThreads->value();
        app_new.local_timeOut               = sb_local_timeout->value();
        qDebug() << "Local : "<< endl;
        qDebug() << "\t app.local_executable            :  " << app_new.local_executable << endl;
        qDebug() << "\t app.local_GnuPlot_executable    :  " << QString(app_new.local_GnuPlot_executable) << endl;
        qDebug() << "\t app.local_FFmpeg_executable     :  " << QString(app_new.local_FFmpeg_executable) << endl;
        qDebug() << "\t app.local_maxConcurrentJobs     :  " << app_new.local_maxConcurrentJobs << endl;
        qDebug() << "\t app.local_maxThreads            :  " << app_new.local_maxThreads<< endl;
        qDebug() << "\t app.local_timeOut               :  " << app_new.local_timeOut     << endl << endl;

        // Remote
        app_new.remote_user                 = le_remote_user->text();
        app_new.remote_host                 = le_remote_host->text();
        app_new.remote_executable           = le_remote_executable->text();
		app_new.remote_GnuPlot_executable   = (le_remote_GnuPlot_executable->isEnabled() ? le_remote_GnuPlot_executable->text() : "");
        app_new.remote_maxThreads           = sb_remote_maxThreads->value();
        app_new.remote_simDir               = le_remote_simDir->text();
        app_new.remote_dataSyncType         = cb_remote_dataSyncType->currentText();

        qDebug() << "Remote : "<< endl;
        qDebug() << "\t app.user            :  " << app_new.remote_user << endl;
        qDebug() << "\t app.host            :  " << app_new.remote_host << endl;
        qDebug() << "\t app.executableRemote:  " << app_new.remote_executable << endl;
        qDebug() << "\t app.maxRemoteThreads:  " << app_new.remote_maxThreads << endl;
        qDebug() << "\t app.remoteSimDir    :  " << app_new.remote_simDir << endl;
        qDebug() << "\t app.syncing         :  " << app_new.remote_dataSyncType << endl << endl;

        config::setApplication(app_new);

        close();
    }
//    if(checkValues())
//    {
//        // TODO - write to settings.conf-file
//        config::application app = config::getApplication();
//        app.outputDir = le_outputDir->text();
//        app.maxLocalJobs = sb_maxLocalJobs->value();
//        app.maxLocalThreads = sb_maxLocalThreads->value();
//        app.maxLocalTime = sb_maxLocalTime->value();
//        app.user = le_user->text();
//        app.host = le_host->text();
//        app.remoteSimDir = le_remoteSimDir->text();
//        app.executableRemote = le_executableRemote->text();
//        app.maxRemoteThreads = sb_maxRemoteThreads->value();
//        app.syncing = cb_syncing->currentText();
//        config::setApplication(app);
//
//        close();
//    }
    else
    {QMessageBox::information((QWidget*)0, "Error", "OutputDir doesn't exists or a file with the same name exists!\nSettings aren't saved!\n");}
}

//---------------------------------------------------------------------------------------------------

bool settingsDialog::checkValues()
{
    if(QDir(le_general_outputDir->text()).exists())
    {
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------

void settingsDialog::openFileDialogOutputDir()
{
    QString tmp = QFileDialog::getExistingDirectory(this, tr("Select folder for saving simulation-output"), le_general_outputDir->text());
    if(tmp != "")
    {
        le_general_outputDir->setText(tmp);
    }
}

//---------------------------------------------------------------------------------------------------


void settingsDialog::openFileDialogExecutable()
{
    QString tmp = QFileDialog::getOpenFileName(this, tr("Select Morpheus executable"), le_local_executable->text());

    if(tmp != "")
    {
        le_local_executable->setText(tmp);
    }
}

void settingsDialog::openFileDialogGnuPlotExecutable()
{
    QString tmp = QFileDialog::getOpenFileName(this, tr("Select GnuPlot executable"), le_local_executable->text());

    if(tmp != "")
    {
        le_local_GnuPlot_executable->setText(tmp);
    }
}
void settingsDialog::openFileDialogFFmpegExecutable()
{
    QString tmp = QFileDialog::getOpenFileName(this, tr("Select FFmpeg executable"), le_local_executable->text());

    if(tmp != "")
    {
        le_local_FFmpeg_executable->setText(tmp);
    }
}
//---------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------

void settingsDialog::checkLocal()
{
    QString executable = le_local_executable->text().trimmed();
	if (executable.isEmpty()) {
		QMessageBox::information((QWidget*)0, "Error", "Executable of simulator \""+ executable +"\" does not exist!\n");
        return;
    }
    executable = config::getPathToExecutable(executable);
	if (executable.isEmpty()) {
		QMessageBox::information((QWidget*)0, "Error", "Executable of simulator \""+ executable +"\" does not exist!\n");
        return;
    }

	
    QProcess p;
	QStringList arguments;
    p.start(executable,arguments);
    if(p.waitForFinished(60000)){
        cout << "Executable exists..." << endl;
    }else{
        QMessageBox::information((QWidget*)0, "Error", "Executable of simulator \""+ executable +"\" does not exist!\n");
        return;
    }

    QString version_revision("");
	arguments << "--version";
    p.start(executable,arguments);
    if(p.waitForFinished(60000))
    {
         version_revision += p.readAll();
    }else{
        QMessageBox::information((QWidget*)0, "Error", "Simulator does not return version number!\n"+p.errorString());
        return;
    }
	version_revision = version_revision.replace("\n",", ");
	
	arguments = QStringList("--revision");
    p.start(executable,arguments);
    if(p.waitForFinished(60000)){
        version_revision += p.readAll();
    }else{
        QMessageBox::information((QWidget*)0, "Error", "Simulator does not return revision number!\n"+p.errorString());
        return;
    }

	arguments = QStringList("--gnuplot-version");
	if (le_local_GnuPlot_executable->isEnabled() && ! le_local_GnuPlot_executable->text().isEmpty()) {
		arguments << "--gnuplot-path";
		arguments << le_local_GnuPlot_executable->text();
	}
    p.start(executable,arguments);
    if(p.waitForFinished(60000)){
        version_revision += p.readAll();
		version_revision.resize(min(200,version_revision.size()));
    }else{
        QMessageBox::information((QWidget*)0, "Error", "Gnuplot does not return revision number!\n"+p.errorString());
        return;
    }

    if(version_revision.startsWith("Version"))
    {
        lb_localSimStatus->setPixmap(QThemedIcon("dialog-apply" ,style()->standardIcon(QStyle::SP_DialogApplyButton)).pixmap(24));
        lb_localSimRev->setText(version_revision.trimmed());
    }
    else
    {
        lb_localSimStatus->setPixmap(QThemedIcon("dialog-cancel" ,style()->standardIcon(QStyle::SP_DialogCancelButton)).pixmap(24));
        lb_localSimRev->setText("unknown version/revision");
    }


    //* === FFMPEG ===
    if( le_local_FFmpeg_executable->isEnabled() ){
        QString executable_ffmpeg = le_local_FFmpeg_executable->text().trimmed();
        if (executable_ffmpeg.isEmpty()) {
            QMessageBox::information((QWidget*)0, "Error", "FFmpeg executable \""+ executable_ffmpeg +"\" does not exist!\n");
            return;
        }
        executable = config::getPathToExecutable(executable_ffmpeg);
        if (executable.isEmpty()) {
            QMessageBox::information((QWidget*)0, "Error", "FFmpeg executable \""+ executable_ffmpeg +"\" does not exist!\n");
            return;
        }
        arguments.clear();
        arguments << "--version";
        qDebug() << "Testing FFMpeg:" << executable << arguments;
        p.start(executable,arguments);
        QString FFmpeg_version;
        if(p.waitForFinished(60000)){
            FFmpeg_version += p.readAll();
            FFmpeg_version.resize(min(200,FFmpeg_version.size()));
            qDebug() << "FFmpeg executable exists..." << endl << FFmpeg_version;

        }else{
            QMessageBox::information((QWidget*)0, "Error", "FFmpeg executable \""+ executable_ffmpeg +"\" does not exist!\n");
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------

void settingsDialog::checkRemote()
{
    QMessageBox msgBox;

    QString username = le_remote_user->text();
    QString resource  = le_remote_host->text();
    msgBox.setText("The ssh connection requires a password-less authentification (using 'ssh-keygen').\n\n"\
                   "To avoid problems, make sure that\n  ssh "+username+"@"+resource+"\n can log in from the command line.");
    msgBox.setInformativeText("Do you to test connection now?");
    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    if( msgBox.exec() == QMessageBox::Cancel )
        return;

    QString executable = le_remote_executable->text();
	if (le_remote_GnuPlot_executable->isEnabled() && ! le_remote_GnuPlot_executable->text().isEmpty()) {
		executable = QString("\"%1 --gnuplot-path %2\"").arg(executable,le_remote_GnuPlot_executable->text());
	}
    sshProxy ssh;

    if (ssh.checkConnection(resource,username))
    {
        lb_remoteConnectionStatus->setPixmap(QThemedIcon("dialog-apply" ,style()->standardIcon(QStyle::SP_DialogApplyButton)).pixmap(24));
		lb_remoteSimStatus->setPixmap(QThemedIcon("dialog-apply" ,style()->standardIcon(QStyle::SP_DialogApplyButton)).pixmap(24));
	}
//         QString version("");
//         cout << "COMMAND: " << "processProxy.sh -a version -e " << executable.toStdString() << endl;
//         if ( ! ssh.exec(QString("processProxy.sh -a version -e " + executable), &version) ) {
//             cout << "response: " << version.toStdString() << endl;
//             cout << ssh.getLastError().toStdString();
//         }
//         QString revision("");
//         cout << "COMMAND: " << "processProxy.sh -a revision -e " << executable.toStdString() << endl;
//         if ( ! ssh.exec(QString("processProxy.sh -a revision -e " + executable) , &revision) ) {
//             cout << "response: " << revision.toStdString() << endl;
//             cout << ssh.getLastError().toStdString();
//         }
//
//         QString gnuplot_version("");
//         cout << "COMMAND: " << "processProxy.sh -a gnuplot-version -e " << executable.toStdString() << endl;
//         if ( ! ssh.exec(QString("processProxy.sh -a gnuplot-version -e " + executable) , &gnuplot_version) ) {
//             cout << "response: " << gnuplot_version.toStdString() << endl;
//             cout << ssh.getLastError().toStdString();
//         }
//
//         QString version_revision = version+", "+revision +"\n"+gnuplot_version;
//
//
//         cout << "ANSWER: " << version_revision.toStdString()  << endl << endl;
//         if(version_revision.startsWith("Version") || version_revision.startsWith("Revision"))
//         {
//             lb_remoteSimStatus->setPixmap(QThemedIcon("dialog-apply" ,style()->standardIcon(QStyle::SP_DialogApplyButton)).pixmap(24));
// // 			version_revision.resize(min(200, version_revision.size()));
//             lb_remoteSimRev->setText(version_revision.trimmed());
//         }
//         else
//         {
//             lb_remoteSimStatus->setPixmap(QThemedIcon("dialog-cancel" ,style()->standardIcon(QStyle::SP_DialogCancelButton)).pixmap(24));
//             lb_remoteSimRev->setText("Error");
//             cout<<"Cannot get version_revision of remote simulator!: '"<< version_revision.toStdString() << "'!" << endl;
//         }
//     }
    else
    {
		lb_remoteConnectionStatus->setPixmap(QThemedIcon("dialog-cancel" ,style()->standardIcon(QStyle::SP_DialogCancelButton)).pixmap(24));
		lb_remoteSimStatus->setPixmap(QThemedIcon("dialog-cancel" ,style()->standardIcon(QStyle::SP_DialogCancelButton)).pixmap(24));
		lb_remoteSimRev->setText(ssh.getLastError());
	}
}
