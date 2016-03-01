#include "mathtextedit.h"

mathTextEdit::mathTextEdit(QWidget* parent) : QTextEdit(parent)
{
    //this->setFont( QFont( "DejaVu Sans Mono" ) );
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    this->setFont( font );
    this->setLineWrapMode(QTextEdit::NoWrap);

    QObject::connect(this, SIGNAL(cursorPositionChanged()), SLOT(matchParentheses()));
}

void mathTextEdit::matchParentheses()
{
    QTextCharFormat format;
    format.setBackground(Qt::white);

    QTextCursor cur = textCursor();
    cur.setPosition(0, QTextCursor::MoveAnchor);
    cur.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
    blockSignals(true);
    cur.setCharFormat(format);
    blockSignals(false);

    int posr = textCursor().position();
    int posl = posr;
    QString str = toPlainText();
    QChar c = str[posr];

    if(c == '(')
    {
        emit emitParentheses(posr);
        int count = 0;

        while(posr < str.size())
        {
            posr++;
            if(str[posr] == '(')
            {count++;}
            if(str[posr] == ')')
            {
                if(count == 0)
                {
                    emit emitParentheses(posr);
                    break;
                }
                else
                {count--;}
            }
        }

    }

    if(posl-1 >= 0)
    {
        c = str[posl-1];
        if(c == ')')
        {
            emit emitParentheses(posl-1);
            int count = 0;

            while(posl-1 > 0)
            {
                posl--;
                if(str.at(posl-1) == ')')
                {count++;}
                if(str.at(posl-1) == '(')
                {
                    if(count == 0)
                    {
                        emit emitParentheses(posl-1);
                        break;
                    }
                    else
                    {count--;}
                }
            }
        }
    }
}
