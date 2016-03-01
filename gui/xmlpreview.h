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

#ifndef XMLPREVIEW_H
#define XMLPREVIEW_H

#include <QtGui>
#include "xmlhighlighter.h"

/*!
This class provides a widget with a QTextEdit, whose text will be highlighted like a xml for better reading.
*/
class XMLTextDialog : public QDialog
{
public:
    XMLTextDialog(QString text, QWidget* parent = 0); /*!< Creates a widget with a textedit. */
    ~XMLTextDialog();

private:
    QTextEdit *xml_view; /*!< Textedit which holds the highlighted text. */
    QPushButton *p_close;
    XMLHighlighter *highlighter; /*!< Highlighter which highlights the text. */
};

#endif // XMLPREVIEW_H
