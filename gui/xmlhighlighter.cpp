#include "xmlhighlighter.h"

XMLHighlighter::XMLHighlighter(QTextEdit *edit) : QSyntaxHighlighter(edit)
{}

void XMLHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat myClassFormat;
    myClassFormat.setForeground(Qt::darkMagenta);

    QString pattern = "<(\\S(?!>|\\s))+\\S|/?>";
    QRegExp expression(pattern);

    int index = text.indexOf(expression);
    while (index >= 0)
    {
        int length = expression.matchedLength();
        setFormat(index, length, myClassFormat);
        index = text.indexOf(expression, index + length);
    }

    myClassFormat.setForeground(Qt::blue);

    pattern = "=\".*\"";
    expression.setPattern(pattern);

    index = text.indexOf(expression);
    while (index >= 0)
    {
        int length = expression.matchedLength();
        setFormat(index, length, myClassFormat);
        index = text.indexOf(expression, index + length);
    }

    myClassFormat.setForeground(Qt::red);

    pattern = "\\S+=";
    expression.setPattern(pattern);

    index = text.indexOf(expression);
    while (index >= 0)
    {
        int length = expression.matchedLength();
        setFormat(index, length, myClassFormat);
        index = text.indexOf(expression, index + length);
    }
}

