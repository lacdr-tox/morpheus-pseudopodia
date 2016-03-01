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

#ifndef IMAGETABLE_H
#define IMAGETABLE_H

#include <QtGui>
#include "fstream"
#include "sstream"
#include "config.h"

/*!
*/
class WaitThread: public QThread {
public:
	static void msleep(int ms)
	{
		QThread::msleep(ms);
	}
};

class ImageTableDialog : public QDialog
{
Q_OBJECT

public:
    ImageTableDialog(QString currdir, QWidget* parent = 0); /*!< Creates a widget with a textedit. */
    ~ImageTableDialog();

private:

    QString currentdir;
    QString currentfilename;
    QDialog* dialog;
    QPushButton *html;
    QPushButton *tex;
    QPushButton *pdf;
    QPushButton *cancel;

    QLabel* l_filename;
    QLabel* l_columns;
    QLabel* l_columns_info;
    QLabel* l_latex;

    QLineEdit* le_filename;
    QPushButton *bt_filename;
    QSpinBox* sb_columns;
    QCheckBox* cb_latex;


    struct JobInfo{
        QString path; // path to folder (relative to sweep dir)
        QString imagefile; // absolute path to image file
        QString imagefile_relative; // relative path to image file
        QString basename;
        QString extension;
        vector<double> parameters;
    };
    vector<ImageTableDialog::JobInfo> sweepInfo;

    QString getParamString( vector<double> p);
    void readSweepSummary( QString image_filename );

private slots:
    void openFileDialog();
    bool makeHtml();
    bool makeHtml_noPython();
    bool makeTex();
    void makePDF();
    void cancelDialog();
};

#endif // IMAGETABLE_H
