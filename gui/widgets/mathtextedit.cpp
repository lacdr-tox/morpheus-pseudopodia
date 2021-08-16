#include "mathtextedit.h"

mathTextEdit::mathTextEdit(QWidget* parent) : QTextEdit(parent), highlighter(this)
{
	//this->setFont( QFont( "DejaVu Sans Mono" ) );
	QFont font("Monospace");
	font.setStyleHint(QFont::TypeWriter);
	font.setPointSize(this->font().pointSize());
	this->setFont( font );
	this->setLineWrapMode(QTextEdit::NoWrap);
	setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	setMinimumWidth(120);

	QObject::connect(this, SIGNAL(cursorPositionChanged()), SLOT(matchParentheses()));
	QObject::connect(this, SIGNAL(textChanged()),SLOT(adjustMinSize()),Qt::QueuedConnection);
	QObject::connect(this, SIGNAL(emitParentheses(int)), &highlighter,SLOT(highlightParentheses(int)));
}

void mathTextEdit::adjustMinSize()
{
// 	qDebug() << "pt " << currentFont().pointSize() << " px "<< QFontMetrics(currentFont()).height() << "+" << QFontMetrics(currentFont()).leading() << " bd " << this->frameWidth() << " sc " <<this->horizontalScrollBar()->height() ;
	QFontMetrics(currentFont()).height();
	int rowcount = toPlainText().count("\n")+1;
	int min_height = 2*this->frameWidth() + 8 + min(6,rowcount)*(QFontMetrics(currentFont()).lineSpacing());
	if ( this->horizontalScrollBar()->isVisible()) {
		min_height += this->horizontalScrollBar()->height();
	}
	setMinimumHeight(min_height);
	QSize s = size();
	if (size().height()<min_height) {
		s.setHeight(min_height);
		resize(s);
	}
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
