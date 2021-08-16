#include "equationhighlighter.h"

equationHighlighter::equationHighlighter(QTextEdit *edit) : QSyntaxHighlighter(edit)
{this->edit = edit;}

void equationHighlighter::highlightBlock(const QString &text)
{
    //****************************//
    QTextCharFormat format;
    format.setForeground(QColor(0,100,180));
    highlightPattern(text,"\\b[^\\W\\d]+[\\w\\.]*\\b", format);

    //****************************//
    //format.setForeground(Qt::darkCyan);
    //format.setFontWeight(QFont::Bold);
    //highlightPattern(text,"(\\(|\\))", format);

    //****************************//
    format.setForeground(Qt::darkRed);
//     format.setFontWeight(QFont::Bold);
    //if([condition], [then], [else]), and, or, xor, sin, cos, tan, asin, acos, atan, sinh, cosh, tanh, asinh, acosh, atanh, log2, log10, ln, exp, sqrt, sign, rint, abs, min, max, sum, avg, mod
    QString pattern = "((\\bif\\b)|(\\band\\b)|(\\bor\\b)|(\\bxor\\b)|(\\bsin\\b)|(\\bcos\\b)|(\\btan\\b)|(\\basin\\b)|(\\bacos\\b)|(\\batan\\b)|(\\bsinh\\b)|(\\bcosh\\b)|(\\btanh\\b)|(\\basinh\\b)|(\\bacosh\\b)|(\\batanh\\b)|(\\blog2\\b)|(\\blog10\\b)|(\\bln\\b)|(\\bexp\\b)|(\\bsqrt\\b)|(\\bsign\\b)|(\\brint\\b)|(\\babs\\b)|(\\bmin\\b)|(\\bmax\\b)|(\\bsum\\b)|(\\bavg\\b)|(\\bmod\\b)|(\\brand_uni\\b)|(\\brand_norm\\b)|(\\brand_gamma\\b)|(\\brand_bool\\b))";
    highlightPattern(text, pattern, format);
}

void equationHighlighter::highlightPattern(const QString &text, QString pattern, QTextCharFormat format){

    QRegExp expression(pattern);

    int index = text.indexOf(expression);
    while (index >= 0)
    {
        int length = expression.matchedLength();
        setFormat(index, length, format);
        index = text.indexOf(expression, index + length);
    }
}

void equationHighlighter::highlightParentheses(int pos)
{
    QTextCharFormat format;
    format.setBackground(Qt::yellow);
    format.setFontWeight(QFont::Bold);

    QTextCursor cur = edit->textCursor();
    cur.setPosition(pos, QTextCursor::MoveAnchor);
    cur.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor);
    edit->blockSignals(true);
    cur.setCharFormat(format);
    edit->blockSignals(false);
}
