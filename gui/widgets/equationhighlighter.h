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

#ifndef EQUATIONHIGHLIGHTER_H
#define EQUATIONHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QtGui>
#include <QTextEdit>

using namespace std;

/*!
This class highlights specific blocks in the text of QTextedit.<br>
Also it highlights the pairs of parentheses next to the current cursor.
*/
class equationHighlighter : public QSyntaxHighlighter
{
Q_OBJECT

public:
    equationHighlighter(QTextEdit*); /*!< Creates a syntaxhighlighter on the given textedit. */
    void highlightBlock(const QString &text); /*!< Highlights the given text inside the textedit. */
private:
    QTextEdit *edit; /*!< Textedit which holds the text. */
    void highlightPattern(const QString &text, QString pattern, QTextCharFormat format);
    /*!<
      Highlights all textblocks inside the given text, which match the given pattern.
      This function uses the given format.
      \param text Text in which matching blocks should be highlighted.
      \param pattern Specifies which blocks should be highlighted.
      \param format Declares how the matching blocks should be highlighted.
      */

public slots:
    void highlightParentheses(int pos); /*!< Highlights the parentheses beside the given position. */
};

#endif // EQUATIONHIGHLIGHTER_H
