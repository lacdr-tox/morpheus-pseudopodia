#include "moviedialog.h"

/*
 TODO: checkbox to select relative or absolute path
*/

MovieDialog::MovieDialog( QString currdir, QWidget* parent, bool sweep )
{
    currentdir = currdir;
    isSweep = sweep;
    QGridLayout* grid = new QGridLayout();
    qsrand(QTime::currentTime().msec());

    dialog = new QDialog(parent, Qt::Dialog);
    dialog->setModal(true);
    dialog->setWindowTitle("Create Movie");
    dialog->setLayout(grid);

    l_filestring = new QLabel(dialog);
    l_filestring->setText("Input: ");
    l_filestring_info = new QLabel(dialog);
    l_filestring_info->setText("Use * as wildcard");
    l_framerate = new QLabel(dialog);
    l_framerate->setText("Framerate: ");
    l_framerate_info = new QLabel(dialog);
    l_framerate_info->setText("1 slow - 100 fast");
    l_quality = new QLabel(dialog);
    l_quality->setText("Quality: ");
    l_quality_info = new QLabel(dialog);
    l_quality_info->setText("0 worst - 10 best");
    l_outputfn = new QLabel(dialog);
    l_outputfn->setText("Output: ");

    le_filestring = new QLineEdit(dialog);
    le_filestring->setText( "plot*.png" );
    le_outputfn = new QLineEdit(dialog);
    le_outputfn->setText( "movie.mp4" );

    //bt_filenames = new QPushButton(QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), "Browse...", dialog);

    sb_framerate = new QSpinBox(dialog);
    sb_framerate->setMinimum(1);
    sb_framerate->setMaximum(100);
    sb_framerate->setValue(24);

    sb_quality = new QSpinBox(dialog);
    sb_quality->setMinimum(0);
    sb_quality->setMaximum(10);
    sb_quality->setValue(8);


    cancel = new QPushButton("Cancel");
    cancel->setDefault(false);
    generate  = new QPushButton("Create");
    generate->setDefault(true);

    grid->addWidget(l_filestring, 0, 0, 1, 1);
    grid->addWidget(le_filestring, 0, 1, 1, 1);
    grid->addWidget(l_filestring_info, 0, 2, 1, 1);

    //grid->addWidget(bt_filenames, 0, 2, 1, 1);

    grid->addWidget(l_framerate, 1, 0, 1, 1);
    grid->addWidget(sb_framerate, 1, 1, 1, 1);
    grid->addWidget(l_framerate_info, 1, 2, 1, 1);

    grid->addWidget(l_quality, 2, 0, 1, 1);
    grid->addWidget(sb_quality, 2, 1, 1, 1);
    grid->addWidget(l_quality_info, 2, 2, 1, 1);

    grid->addWidget(l_outputfn, 3, 0, 1, 1);
    grid->addWidget(le_outputfn, 3, 1, 1, 1);
    //grid->addWidget(l_outputfn_info, 3, 2, 1, 1);

    grid->addWidget(cancel, 5, 1, 1, 1);
    grid->addWidget(generate,   5, 2, 1, 1);


    //connect(le_filestring, SIGNAL(textEdited(QString)), this, SLOT(filestringEdited(QString)));
    //connect(bt_filenames, SIGNAL(clicked(bool)), this, SLOT(openFileDialog()));
    connect(cancel, SIGNAL(clicked(bool)), this, SLOT(cancelDialog()));
    connect(generate, SIGNAL(clicked(bool)), this, SLOT(generateMovie()));

    QString executable = "ffmpeg";
    if ( ! config::getApplication().local_FFmpeg_executable.isEmpty() ) {
        executable = config::getApplication().local_FFmpeg_executable;
    }
    QString cmd;
    if( !(cmd = config::getPathToExecutable( executable )).isEmpty() ){
        dialog->open();
    }
    else{
        QString message;
        message =  "This feature requires 'FFmpeg', which was not found.\nPlease install ffmpeg and/or specify its location in Settings/local.";
                   //+(!config::getApplication().local_FFmpeg_executable.isEmpty()?"\n("+config::getApplication().local_FFmpeg_executable+" was not found)"+"");
        QMessageBox::warning(this, "FFmeg not found", message);
        return;
    }

}

MovieDialog::~MovieDialog(){}

void MovieDialog::generateMovie( ){

    filestring = le_filestring->text();

    QStringList tokens = filestring.split(".");
    QString extension = tokens.value( tokens.length() - 1 );
    if(extension!="png" && extension!="tif" && extension!="gif" && extension!="jpg"){
        QMessageBox::warning(this, "Error: no images", QString("Extension of '"+filestring+"' does not (exclusively) refer to images."));
        dialog->done(0);
        return;
    }

    if(isSweep){
        QProgressDialog *pd = new QProgressDialog("Reading sweep info...", "Cancel", 0, 100);
        pd->setAutoReset(true);
        pd->setAutoClose(true);
        pd->setValue(0);

        QStringList directories = readDirsFromSweepSummary();
        pd->setRange(0, directories.size());
        pd->setLabelText("Creating "+QString::number(directories.size())+" movies...");

        for(int i=0; i<directories.size(); i++){
            if(pd->wasCanceled())
                return;
            pd->setValue(i+1);
            pd->update();

            currentdirectory = currentdir + "/" + directories[i];
            qDebug() << "Sweep: currentdirectory = " << currentdirectory;
            makeMovie();
        }
    }
    else{
        currentdirectory = currentdir;
        makeMovie();
    }
}

void MovieDialog::makeMovie( ){

    this->setCursor(Qt::WaitCursor);

    QString executable = "ffmpeg";
    if ( ! config::getApplication().local_FFmpeg_executable.isEmpty() ) {
        executable = config::getApplication().local_FFmpeg_executable;
        qDebug() << "config::getApplication().local_FFmpeg_executable = " << config::getApplication().local_FFmpeg_executable;
    }
    filestring = le_filestring->text();

    /// GLOBBING (WILDCARD)
    // if filename contains wildcard
    if( filestring.contains('*', Qt::CaseSensitive) ){

        // get all files in directory with this wildcard
        QDir dir(currentdirectory);
        QStringList filters;
        filters << filestring;
        dir.setNameFilters(filters);
        dir.setSorting(QDir::Name);
        QFileInfoList fileinfoList = dir.entryInfoList();

        // create temporary directory with random name
        QString tmpdir = dir.tempPath();
        int maxint = 99999999;
        int minint = 0;
        int randint = (qrand() % ((maxint + 1) - minint) + minint);
        QString tempdir = tmpdir+"/morphmov_"+QString("%1").arg(randint,10,10,QChar('0'));
        dir.mkdir( tempdir );

        if( fileinfoList.size() == 0 ){
            QMessageBox::warning(this, "Creating movie failed",
                                QString("No files found with name \""+filestring+"\" in directory \""+currentdirectory+"\""));
            dialog->done(0);
            return;
        }

        // copy these files to /tmp/randomname
        qDebug() << "Copying images files to temporary directory";
        for(int i=0; i<fileinfoList.size(); i++){

            // - using incremental naming
            QString newfilename = filestring;
            newfilename.replace("*",QString("%1").arg(i, 6, 10, QChar('0')), Qt::CaseSensitive);
            QString newfilepath = tempdir+"/"+newfilename;

            // copy file to tempdir
            QFile::copy(fileinfoList[i].filePath(), newfilepath);

            //qDebug() << "OLD PATH: " << fileinfoList[i].filePath();
            //qDebug() << "NEW PATH: " << newfilepath;
        }

        // change directory
        // - change '*' wildcard to '%06d'
        //qDebug() << "OLD STRING: " << filestring;
        QString newstring = tempdir+"/"+filestring;
        newstring.replace("*","%06d", Qt::CaseSensitive);
        //qDebug() << "NEW STRING: " << newstring;
        filestring = newstring;

    }

    /// CREATE MOVIE (FFMPEG)

    int quality_0_10 = sb_quality->value();
    int quality_51_0 = int(51.0*(1.0-(float(quality_0_10)/10.0)));

    QStringList args;
    args << // overwrite without asking
            QString("-y")           <<
            // frame rate of input
            QString("-r")           << QString::number(sb_framerate->value()) <<
            // input files
            QString("-i")           << filestring <<
            // video codec: H.264
            QString("-vcodec")      << QString("libx264") <<
            // slow processing for better compression
            QString("-preset")      << QString("slow") <<
            // use YUV420p color space
            QString("-pix_fmt")     << QString("yuv420p") <<
            // scale image in case of width or height is not divisible by 2
            QString("-vf")          << QString("scale='trunc(iw/2)*2:trunc(ih/2)*2'") <<
            // set quality (constant rate factor, crf): 51=worst, 0=best
            QString("-crf")         << QString::number(quality_51_0)<<
            // output filename
            QString(le_outputfn->text());

    QString cmd;
    if( !(cmd = config::getPathToExecutable( executable )).isEmpty() ){

        /// Start without detachment yields error: "Process is already started"
        p.connect(&p, SIGNAL(readyReadStandardError()), this, SLOT(ffmpeg_readyReadStandardError()) );
        p.connect(&p, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(ffmpeg_finished(int,QProcess::ExitStatus)) );
        p.setWorkingDirectory(currentdirectory);
        p.start(cmd, args);
        if(p.waitForFinished(60000)){
            QString FFmpeg_report = p.readAll();
            FFmpeg_report.resize(min(200,FFmpeg_report.size()));
            qDebug() << "FFmpeg finished:" << endl << FFmpeg_report;
            if(!isSweep){
                QMessageBox::information((QWidget*)0, "Done", "Movie '"+le_outputfn->text()+"' created.\n"+ FFmpeg_report +"\n");

            }

        }else{
            QMessageBox::information((QWidget*)0, "Error", "FFmpeg executable \""+ executable +"\" does not exist!\n");
            return;
        }
        this->setCursor(Qt::ArrowCursor);

        //p.startDetached(cmd, args, currentdirectory);
    }
    else{
        QString message;
        message =  "Unable to find 'ffmpeg' !";
        QMessageBox::warning(this, "FFmeg Not found", message);
    }

    qDebug() << cmd << args.join(" ");
    dialog->done(0);
}

//void MovieDialog::openFileDialog(){

//    qDebug() << " openFileDialog: PATH  = " << le_filenames->text()  << endl;
//    QStringList selected_files = QFileDialog::getOpenFileNames(this,
//                                               tr("Select multiple images"),
//                                               currentdir,
//                                               tr("Images (*.png *.jpg *.gif *.tif)"));

//    filenames = selected_files;
//    le_filenames->setText( filenames.join(";") );
//}

void MovieDialog::ffmpeg_readyReadStandardError(){

//    p.setReadChannel(QProcess::StandardError);
//    QTextStream stream(&p);
//    QStringList errmsg;
//    while (!stream.atEnd()) {
//        errmsg << stream.readLine();

//    }
//    QMessageBox::warning(this, "Creating movie failed", errmsg.join("\n"));
}

void MovieDialog::ffmpeg_finished(int exitCode,QProcess::ExitStatus status){

    p.setReadChannel(QProcess::StandardError);
    QTextStream stream(&p);
    QStringList errmsg;
    while (!stream.atEnd()) {
        errmsg << stream.readLine();
    }
    if(status == QProcess::CrashExit)
        QMessageBox::critical(this, "Creating movie failed", errmsg.join("\n"));
    else if(!isSweep){
    //    QMessageBox::information(this, "Done", QString("Movie '"+le_outputfn->text()+"' was created"));
    }

}
void MovieDialog::cancelDialog(){
    dialog->done(0);
}

void MovieDialog::filestringEdited( QString newtext ){
    filestring = newtext;
}

QStringList MovieDialog::readDirsFromSweepSummary(){

    QStringList directories;
    QFile file(currentdir+"/"+"sweep_summary.txt");
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "error", file.errorString());
    }
    QTextStream in(&file);

    while(!in.atEnd()){
        // read line
        QString line = in.readLine();

        // skip commented line
        if(line.startsWith("#") || line.size() == 0)
            continue;

        // tokenize line
        QStringList fields = line.split("\t");
        qDebug() << "Tokens: " << fields;

        // path of folder (relative to Sweep folder)
        directories << QString("../"+fields.at(0));
    }
    return directories;
}

