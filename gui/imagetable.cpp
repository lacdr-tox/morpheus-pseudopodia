#include "imagetable.h"

/*
 TODO: checkbox to select relative or absolute path
*/

ImageTableDialog::ImageTableDialog(QString currdir, QWidget* parent )
{
    currentdir = currdir;
    currentfilename = "";
    QGridLayout* grid = new QGridLayout();

    dialog = new QDialog(parent, Qt::Dialog);
    dialog->setModal(true);
    dialog->setWindowTitle("Create Result Table");
    dialog->setLayout(grid);

    l_filename = new QLabel(dialog);
    l_filename->setText("Filename: ");
    le_filename = new QLineEdit(dialog);
    le_filename->setText( "image or movie file" );

    l_columns = new QLabel(dialog);
    l_columns->setText("Columns: ");
    l_columns_info = new QLabel(dialog);
    l_columns_info->setText("0 = best guess");

    bt_filename = new QPushButton(QThemedIcon("document-open", style()->standardIcon(QStyle::SP_DialogOpenButton)), "Browse...", dialog);

    sb_columns = new QSpinBox(dialog);
    sb_columns->setMinimum(0);
    sb_columns->setValue(0);

//    l_latex = new QLabel(dialog);
//    l_latex->setText("pdflatex?");
//    cb_latex = new QCheckBox();
//    latex = (latex==false? false : true);
//    cb_latex->setChecked(latex);

    cancel = new QPushButton("Cancel");
    html  = new QPushButton("Generate");
    html->setDefault(true);
    tex   = new QPushButton("Generate .tex");
    pdf   = new QPushButton("Generate .pdf");

    grid->addWidget(l_filename, 0, 0, 1, 1);
    grid->addWidget(le_filename, 0, 1, 1, 1);
    grid->addWidget(bt_filename, 0, 2, 1, 1);

    grid->addWidget(l_columns, 1, 0, 1, 1);
    grid->addWidget(sb_columns, 1, 1, 1, 1);
    grid->addWidget(l_columns_info, 1, 2, 1, 1);

//    grid->addWidget(l_latex, 2, 0, 1, 1);
//    grid->addWidget(cb_latex, 2, 1, 1, 1);

    grid->addWidget(cancel, 2, 1, 1, 1);
    grid->addWidget(html,   2, 2, 1, 1);
    //grid->addWidget(tex,   2, 2, 1, 1);
    //grid->addWidget(pdf,   2, 3, 1, 1);

    connect(bt_filename, SIGNAL(clicked(bool)), this, SLOT(openFileDialog()));
    connect(html, SIGNAL(clicked(bool)), this, SLOT(makeHtml_noPython()));
    connect(tex, SIGNAL(clicked(bool)), this, SLOT(makeTex()));
    connect(pdf, SIGNAL(clicked(bool)), this, SLOT(makePDF()));
    connect(cancel, SIGNAL(clicked(bool)), this, SLOT(cancelDialog()));

    dialog->open();
}

ImageTableDialog::~ImageTableDialog(){}

QString ImageTableDialog::getParamString( vector<double> p){

    QString s = QString::number(p[0]);

    for(int i=1; i<p.size(); i++){
        s += ", " + QString::number(p[i]);
    }
    return s;
}

bool ImageTableDialog::makeHtml_noPython( ){

    qDebug() << "1. ImageTableDialog::makeHtml_noPython";

    // get paths, params and imagefilenames from sweep summary and user-selected filename
    readSweepSummary( le_filename->text() );

    if( sweepInfo.size() == 0 ){
        QMessageBox::critical(this, tr("Error: images not found"),
                                        tr("Images were not found.\n"
                                           "Please select another image file."),
                                        QMessageBox::Ok,
                                        QMessageBox::Ok);
        return false;
    }

    qDebug() << "2. ImageTableDialog::makeHtml_noPython";

    //  Determine number of columns and rows
    int numcols = sb_columns->value();
    qDebug() << "spinbox = " << numcols << "\tsweepInfo.size()"<< sweepInfo.size();
    if( numcols == 0 ){
        numcols = int(ceil( sqrt( float(sweepInfo.size()) ) ));
    }
    int numrows = 0;
    if( numrows == 0 ){
        numrows = int(ceil( float(sweepInfo.size()) / float(numcols) ));
    }

    qDebug() << "3. Rows " << numrows << " x Cols " << numcols;

    // Define css
    QString css(
            "<style>                             \n"
            " body {                             \n"
            "     background-color: white;       \n"
            " }                                  \n"
            " h1 {                               \n"
            "     color: maroon;                 \n"
            "     margin-left: 40px;             \n"
            " }                                  \n"
            " table {                            \n"
            "     margin: 0 auto;                \n"
            " }                                  \n"
            " figure {                           \n"
            "     #display: inline-block;        \n"
            "     #border: 1px dotted gray;      \n"
            "     margin: 10px;                  \n"
            " }                                  \n"
            " figure img {                       \n"
            "     text-align: center;            \n"
            "     vertical-align: bottom;        \n"
            "     margin-left: auto;             \n"
            "     margin-right: auto;            \n"
            " }                                  \n"
            " figure figcaption {                \n"
            "     display: block;                \n"
            "     border: 0px dotted blue;       \n"
            "     text-align: center;            \n"
            "     font-family: helvetica;        \n"
            "     font-size: 20px;               \n"
            " }                                  \n"
            " </style>                       \n\n");

    // Define HTML table
    QString table("<table style=\"width:80%\" border=0>\n");

    for(int r=0; r<numrows; r++){
        table += "<tr>\n";
        for(int c=0; c<numcols; c++){
            int job = (numcols*r)+c;
            qDebug() << "Job: " << job;
            if( job >= sweepInfo.size() )
                continue;
            JobInfo jobinfo = sweepInfo[job];
            table += "\t<td style=\"text-align: center;\">\n";
            table += "\t\t<figure>\n";
            table += "\t\t\t<figcaption>" + getParamString(jobinfo.parameters) + "</figcaption>\n";

            if( jobinfo.extension == "mp4" || jobinfo.extension == "webm" || jobinfo.extension == "ogg" )
                table += "\t\t\t<video title=\""+jobinfo.imagefile_relative+"\" width=\"250px\" id=\"video_"+jobinfo.imagefile_relative
                         +"\" src=\""+jobinfo.imagefile_relative+"\" type='video/"
                         +jobinfo.extension+"' controls autoplay loop>Your browser does not yet support the HTML5 video tag.\n\t\t\t</video>\n";
            else
                table += "\t\t\t<img title=\""+jobinfo.imagefile_relative+"\" src=\""+jobinfo.imagefile_relative+"\" width=\"100%\"/>\n";
            table += "\t\t</figure>\n";
            table += "\t</td>\n";
        }
        table += "</tr>\n";
    }
    table += "</table>\n";

    qDebug() << css;
    qDebug() << table;
    qDebug() << "4. Css and Table written" << numcols;

    // Write HTML table to file
    QString fn = currentdir+"/table_"+sweepInfo[0].basename+".html";
    QFile fout(fn);
    fout.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outstream(&fout);
    outstream << css;
    outstream << table;
    fout.close();

    QFileInfo tableFile ( fn );
    if (tableFile.exists() && tableFile.isFile()) {
        qDebug() << "HTML table file exist:" << tableFile.absoluteFilePath();
    }else{
        qDebug() << "HTML table file does not exist:" << tableFile.absoluteFilePath();
    }

    qDebug() << "5. Written to file" << numcols;

    dialog->done(0);

    // Open HTML file in webbrowser
    int result = QDesktopServices::openUrl("file://"+tableFile.absoluteFilePath());
    if(result){
        qDebug() << "openURL return true";
    }
    else{
        qDebug() << "openURL return false";
    }
    qDebug() << "6. Opened in browser" << numcols;

    return true;
}

bool ImageTableDialog::makeHtml( ){

    if( (config::getPathToExecutable( "python" )).isEmpty() ){
            QString message;
            message =  "Python is required for this feature!";
            QMessageBox::warning(this, "Python not found", message);
    }

    QString command, cmdlatex;
    QString executable = "morphImageTable.py";
    QString columnss = QString::number( sb_columns->value() );

    if( config::getPathToExecutable( executable ).isEmpty() ){
            QString message;
            message =  executable + " is required for this feature!";
            QMessageBox::warning(this, executable+ " not found", message);
    }

    if( !(command = config::getPathToExecutable( executable )).isEmpty() ){

            qDebug() << command <<  QString("--sweepfile") << QString(currentdir+"/"+"sweep_summary.txt") << QString("--columns") << columnss << QString("--filename") << le_filename->text() << QString("--format") << QString("html")<< "\n";
            QProcess::startDetached(command, QStringList() << QString("--sweepfile") << QString(currentdir+"/"+"sweep_summary.txt") << QString("--columns") << columnss << QString("--filename") << le_filename->text() << QString("--format") << QString("html"), currentdir);

        }
    else{
            QString message;
            message =  "Unable to find " + executable + " !";
            QMessageBox::warning(this, "Not found", message);
        }

    dialog->done(0);
    return true;
}

void ImageTableDialog::readSweepSummary( QString image_filename ){


    QDir dir(currentdir);
    QFile file(currentdir+"/"+"sweep_data.csv");
    if(!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, "error", file.errorString());
    }
    QTextStream in(&file);

    while(!in.atEnd()){
        // read line
        QString line = in.readLine();
        qDebug() << "Line: " << line;
        // skip commented line
        if(line.startsWith("#") || line.size() == 0)
            continue;

        // tokenize line
        QStringList fields = line.split("\t");
        qDebug() << "Tokens: " << fields;

        ImageTableDialog::JobInfo jobInfo;

        // path of folder (relative to Sweep folder)
        jobInfo.path = fields.at(0);

        // absolute path of image file
        QString imagepath = currentdir+"/../"+jobInfo.path+"/"+image_filename;
        qDebug() << currentdir << " || " << imagepath << endl;
        QFileInfo imageFile ( imagepath );
        if (imageFile.exists() && imageFile.isFile()) {
            jobInfo.imagefile = imageFile.absoluteFilePath();
            jobInfo.imagefile_relative = dir.relativeFilePath(jobInfo.imagefile);
            jobInfo.extension = imageFile.suffix();
            jobInfo.basename = imageFile.baseName();
            //qDebug() << "Absolute: " << jobInfo.imagefile << "\nRelative: " << jobInfo.imagefile_relative;
        } else {
            qDebug() << "ImageTable: '" << imagepath << "' does not exist!";
            continue;
        }

        // get parameters
        for(int t=1; t<fields.size(); t++){
            jobInfo.parameters.push_back( fields[t].toDouble() );
        }

        // put jobinfo in sweepinfo
        sweepInfo.push_back( jobInfo );
    }
    if( sweepInfo.size() == 0 )
    qDebug() << "ImageTableDialog::readSweepSummary: Error: No jobs found in sweep_summary.txt";

}

bool ImageTableDialog::makeTex( ){

    if( (config::getPathToExecutable( "python" )).isEmpty() ){
            QString message;
            message =  "Python is required for this feature!";
            QMessageBox::warning(this, "Python not found", message);
    }

    QString command, cmdlatex;
    QString executable = "morphImageTable.py";
    QString columnss = QString::number( sb_columns->value() );

    if( config::getPathToExecutable( executable ).isEmpty() ){
            QString message;
            message =  executable + " is required for this feature!";
            QMessageBox::warning(this, executable+ " not found", message);
    }
/*    QFileInfo fi(executable);
    bool file_executable = fi.isExecutable();
    if( !file_executable ){
            QString message;
            message =  executable + " is not executable !";
            QMessageBox::warning(this, "Cannot execute", message);
            dialog->done(0);
            return false;
      }
*/

    if( !(command = config::getPathToExecutable( executable )).isEmpty() ){

            //qDebug() << command << QString("--sweepfile") << QString(currentdir+"/"+"sweep_summary.txt") <<  QString("--filename") << filename <<  QString("--format") << QString("tex") << "\n";
            QProcess::startDetached(command, QStringList() << QString("--sweepfile") << QString(currentdir+"/"+"sweep_summary.txt") <<  QString("--filename") << le_filename->text() << QString("--columns") << columnss << QString("--format") << QString("tex") , currentdir);

        }
    else{
            QString message;
            message =  "Unable to find " + executable + " !";
            QMessageBox::warning(this, "Not found", message);
        }

    dialog->done(0);
    return true;
}

void ImageTableDialog::makePDF( ){

    QString executable_latex = "pdflatex";
    QFileInfo fi(currentdir + "/" + "table.tex");

	if( !makeTex() ){
         return;
    }

    WaitThread* wt = new WaitThread();
	wt->msleep(1000);

    bool file_exists = QFile(fi.absoluteFilePath()).exists();
    while( !file_exists ){
            wt->msleep(100);
            fi.refresh();
            file_exists = QFile(fi.absoluteFilePath()).exists();
     }

    QString cmdlatex;
    if( !(cmdlatex = config::getPathToExecutable( executable_latex )).isEmpty() ){
            QProcess::startDetached(cmdlatex, QStringList() << QString("table.tex"), currentdir);
    }
    else{
            QString message;
            message =  "Unable to find 'pdflatex' !";
            QMessageBox::warning(this, "Not found", message);
        }

    dialog->done(0);
}

void ImageTableDialog::openFileDialog(){

    qDebug() << " openFileDialog: PATH  = " << le_filename->text()  << endl;
    QString tmp = QFileDialog::getOpenFileName(this,
                                               tr("Select image or movie file for table"),
                                               QString(currentdir + "/" + currentfilename ),
                                               tr("Image or movie file (*.png *.jpg *.tif *.gif *.mp4 *.webm *.ogg)"));
    if(tmp != ""){
            QFileInfo fi(tmp);
            QString name = fi.fileName();
            le_filename->setText(name);
    }
    currentfilename = le_filename->text();
}

void ImageTableDialog::cancelDialog(){
    dialog->done(0);
}
