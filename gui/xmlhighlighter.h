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

#ifndef XMLHIGHLIGHTER_H
#define XMLHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QtGui>
#include <iostream>

using namespace std;

/*!
This class simply highlights all words in the given QTextEdit, which are xml-syntax conform.
*/
class XMLHighlighter : public QSyntaxHighlighter
{
public:
    XMLHighlighter(QTextEdit*); /*!< Sets a new syntaxhighlighter on the given textedit. */
    void highlightBlock(const QString &text); /*!< Highlights all textblocks inside 'text' which are xml-syntax conform.*/
};

#endif // XMLHIGHLIGHTER_H
