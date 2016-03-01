#include "xmlpreview.h"

XMLTextDialog::XMLTextDialog(QString text, QWidget* parent ) : QDialog(parent)
{
    QVBoxLayout *lay = new QVBoxLayout();

    xml_view = new QTextEdit(this);
    xml_view->setText(text);
    xml_view->setReadOnly(true);


    highlighter = new XMLHighlighter(xml_view);

    QPalette pal;
    pal.setColor(QPalette::Base, QColor(239, 235, 231, 255));
    xml_view->setPalette(pal);
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    xml_view->setFont( font );
    //xml_view->setFont( QFont( "DejaVu Sans Mono" ) );
    xml_view->setLineWrapMode( QTextEdit::NoWrap );

    xml_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    lay->addWidget(xml_view);

    p_close = new QPushButton("Close");
    connect(p_close, SIGNAL(clicked()), this, SLOT(close()));
    lay->addWidget(p_close);
    this->setMinimumWidth(700);
    this->setMinimumHeight(400);
    this->setLayout(lay);

}

XMLTextDialog::~XMLTextDialog()
{
    delete highlighter;
}
