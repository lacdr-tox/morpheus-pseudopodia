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

#ifndef MOVIEDIALOG_H
#define MOVIEDIALOG_H

#include <QtGui>
#include "fstream"
#include "sstream"
#include "config.h"
//#include "imagetable.h"

/*!
*/

class MovieDialog : public QDialog
{
Q_OBJECT

public:
    MovieDialog(QString currdir, QWidget* parent = 0, bool sweep = false); /*!< Creates a widget with a textedit. */
    ~MovieDialog();

private:

    bool isSweep;

    QString currentdir;
    QString currentdirectory;
    QString filestring; // may include wildcard
    QDialog* dialog;
    QPushButton *cancel;
    QPushButton *generate;

    QLabel* l_filestring;
    QLabel* l_filestring_info;
    QLabel* l_framerate;
    QLabel* l_framerate_info;
    QLabel* l_quality;
    QLabel* l_quality_info;
    QLabel* l_outputfn;
    //QLabel* l_outputfn_info;

    QLineEdit* le_filestring;
    QLineEdit* le_outputfn;
    //QPushButton *bt_filenames;
    QSpinBox* sb_framerate;
    QSpinBox* sb_quality;
    QProcess p;

    void makeMovie();
    QStringList readDirsFromSweepSummary();

private slots:
    //void openFileDialog();
    void generateMovie();
    void cancelDialog();
    void filestringEdited(QString newtext);
    void ffmpeg_readyReadStandardError();
    void ffmpeg_finished(int,QProcess::ExitStatus);


};

#endif // MOVIEDIALOG_H
