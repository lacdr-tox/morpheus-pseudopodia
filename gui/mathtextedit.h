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

#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QtGui>
#include "equationhighlighter.h"
// #include <iostream>

using namespace std;

/*!
This TextEdit is used to highlight and match pairs of parentheses, if the cursor stands in the left or right of one.
*/
class mathTextEdit : public QTextEdit
{
Q_OBJECT

public:
    mathTextEdit(QWidget* parent); /*!< Creates a new textedit which highlights parentheses if the cursor stands next to them. */
// 	QSize minimumSizeHint () const override;
// 	QSize sizeHint () const override;

private:
	equationHighlighter highlighter;


signals:
    void emitParentheses(int pos); /*!< Signal is send when the cursor stands next to a bracket. */
public slots:
	void adjustMinSize();
private slots:
    void matchParentheses(); /*!< Highlights the bracket next to the current cursor position and the correspondending bracket. */
};

#endif // MYTEXTEDIT_H
