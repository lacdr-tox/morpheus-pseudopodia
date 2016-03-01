#include "attrcontroller.h"

//konstruktor des delegates
attrController::attrController(QObject *parent, AbstractAttribute* attr, bool range) : QItemDelegate(parent), val(parent)
{
    this->attr = attr;
	this->is_range = range;
	setValidator();
}

void attrController::setAttribute ( AbstractAttribute* attribute)
{
	this->attr = attribute;
	setValidator();
}


bool attrController::eventFilter ( QObject * editor, QEvent * event ) {
   if (event->type() == QEvent::KeyRelease || event->type() == QEvent::FocusOut || event->type() == QEvent::KeyPress ) {
        if ( ! attr->getType()->is_enum || is_range) {
            QLineEdit *box = static_cast<QLineEdit*>(editor);
            QString value = box->text();
            int pos;
            bool valid = (val.validate(value,pos) == QValidator::Acceptable);

            bool leaving = false;
//             if (event->type() == QEvent::FocusOut)
//                  leaving = true;
            if (event->type() == QEvent::KeyPress) {
                QKeyEvent* key_ev = (QKeyEvent*) event;
                if (key_ev->key() == Qt::Key_Enter || key_ev->key() == Qt::Key_Return || key_ev->key() == Qt::Key_Tab || key_ev->key() == Qt::Key_Backtab)
                    leaving = true;
            }
            if (event->type() == QEvent::KeyRelease) {
                if (valid) {
                box->setStyleSheet("");
                }
                else {
                    box->setStyleSheet("background: #FFD0D0;");
                }
            }
            if (leaving && !valid) {
                event->ignore();
                box->setFocus(Qt::OtherFocusReason);
                return true;
            }
        }
    }
    return QItemDelegate::eventFilter(editor, event);
}

//------------------------------------------------------------------------------

void attrController::setValidator()
{
	if (!attr)
		return;
    pattern = attr->getPattern();
	QRegExp reg;
	if (is_range) {
		pattern.remove('^');
		pattern.remove('$');
		QString range_regex;
		// The list pattern ..
		range_regex = QString("(:?") + pattern + ";)*(:?" +pattern + ")";
		
		if(attr->getType()->base_type == "xs:double" || attr->getType()->base_type == "xs:integer" || attr->getType()->base_type == "xs:decimal")
		{
			// The linear range pattern ...
			range_regex += "|(:?" + pattern + "):(:?"+ pattern +"|#\\d+(:?lin|log)?):(:?" + pattern + ")";

		}
		pattern = range_regex;
	}
	reg.setPattern(pattern);
	val.setRegExp(reg);
}

//------------------------------------------------------------------------------
 bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
 {
     return s1.toLower() < s2.toLower();
 }
//funktion welche aufgerufen wird,wenn ein objekt der tabelle angeklickt wird
//muss nicht per connect verbunden werden, da es als delegierter für die einzelne zelle in der tabelle wirkt
QWidget *attrController::createEditor(QWidget *parent, const QStyleOptionViewItem & option , const QModelIndex &/* index */) const
{
    QWidget* current_editor;
    //je nach dem welchen typ die zelle ist wird das passende objekt zur verwaltung der daten angelegt und aufgerufen
    if (attr->getType()->is_enum  && ! is_range) {
        // Enum Type
        QComboBox *editor = new QComboBox(parent);
        if ( XSD::dynamicTypeRefs.contains(attr->getType()->name) ) {
			QMap<QString,QString> symbol_names = attr->getModelDescr().getSymbolNames(attr->getType()->name);
			QStringList symbols = symbol_names.keys();
			qSort(symbols.begin(),symbols.end(),caseInsensitiveLessThan);
			for (int i=0; i<symbols.size(); i++ ){
				if ( symbol_names[symbols[i]].isEmpty() ) {
					editor->addItem( symbols[i], symbols[i] );
				}
				else {
					editor->addItem( symbols[i] + " - " + symbol_names[symbols[i]], symbols[i] );
				}
			}
        }
        else {
            editor->addItems( attr->getEnumValues() );
        }
        current_editor = editor;
    }
#ifndef Q_OS_WIN32
    else if ( attr->getType()->name == "cpmSystemPath" || attr->getType()->name == "cpmSystemFile") {
        QFileDialog* system_dialog = new QFileDialog(parent);
		system_dialog->setDirectory(QDesktopServices::storageLocation(QDesktopServices::HomeLocation));
        if (attr->getType()->name == "cpmSystemPath" )
            system_dialog->setFileMode( QFileDialog::DirectoryOnly );
        current_editor = system_dialog;
    }
#endif
    else {
        // String like Type
        current_editor = new QLineEdit(parent);
		current_editor->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred));
	}
    return current_editor;
}

//------------------------------------------------------------------------------

//funktion mit der der wert des delegates initialisiert wird (wert aus zelle wird übernommen)
 void attrController::setEditorData(QWidget *editor, const QModelIndex &index) const
 {
     if (attr->getType()->is_enum && ! is_range) {
        QString value = attr->get();
//        index.model()->data(index, Qt::EditRole).toString();
        QComboBox *box = static_cast<QComboBox*>(editor);
        int ind;
        if ( XSD::dynamicTypeRefs.contains(attr->getType()->name) )
            ind = box->findData(value);
        else
            ind = box->findText(value);

        box->setCurrentIndex(ind);
        box->adjustSize();
     }
#ifndef Q_OS_WIN32
    else if ( attr->getType()->name == "cpmSystemPath" || attr->getType()->name == "cpmSystemFile") {
        if ( ! attr->get().isEmpty() ) {
            QFileDialog* system_dialog = static_cast<QFileDialog*>(editor);
            if (attr->getType()->name == "cpmSystemPath" ) {
                system_dialog->setDirectory(attr->get());
            }
            else if (attr->getType()->name == "cpmSystemFile") {
                system_dialog->selectFile(attr->get());
            }
        }
    }
#endif
     else {
		QString value;
		if (is_range) {
			value = index.model()->data(index, Qt::EditRole).toString();
		}
        else
			value = attr->get();

		QLineEdit *box = static_cast<QLineEdit*>(editor);
        box->setText(value);
		QSize s = box->size();
		s.setWidth(250);
		box->resize(s);
     }
 }

//------------------------------------------------------------------------------



 //funktion mit der der wert der zelle geändert wird, wenn im objekt des delegates sich der wert ändert
 void attrController::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
 {
    QString value;

    if (attr->getType()->is_enum && ! is_range)
    {
        QComboBox *box = static_cast<QComboBox*>(editor);

        if ( XSD::dynamicTypeRefs.contains(attr->getType()->name) )
            value = box->itemData(box->currentIndex()).toString();
        else
            value = box->currentText();
        if (attr->set(value))  {
            // create value for the view
            if (value.isEmpty())
                model->setData(index, "...", Qt::EditRole);
            else
                model->setData(index, value, Qt::EditRole);
        }
    }
#ifndef Q_OS_WIN32
    else if ( attr->getType()->name == "cpmSystemPath" || attr->getType()->name == "cpmSystemFile") {
        QFileDialog* system_dialog = static_cast<QFileDialog*>(editor);

        if (attr->getType()->name == "cpmSystemPath" ) {
            value=system_dialog->directory().canonicalPath();
        }
        else if (attr->getType()->name == "cpmSystemFile" &&  ! system_dialog->selectedFiles().empty() ) {
            value=system_dialog->selectedFiles().first();
        }

        if (value.isEmpty())
			return;

        if (attr->set(value)) {
            if (value.isEmpty())
                model->setData(index, "...", Qt::EditRole);
            else
                model->setData(index, value, Qt::EditRole);
        }
    }
#endif
    else {
        QLineEdit *box = static_cast<QLineEdit*>(editor);
        value = box->text();
        int pos;
        if (val.validate(value,pos) != QValidator::Acceptable) {
            qDebug() << "AttrController[" << attr->getName() << "]:: invalid value " << value;
			qDebug() << "AttrController[" << attr->getName() << "]:: pattern is " << pattern;
            setEditorData(editor,index);
            return;
        }
        else {
			if (is_range) {
				model->setData(index, value, Qt::EditRole);
			}
            else if (attr->set(value))  {
                // create value for the view
                model->setData(index, value, Qt::EditRole);
            }
        }
    }
//    cout << "attrController::setModelData:   setting value " << value.toStdString() << endl;
    // write the real value into the attribute

 }


//------------------------------------------------------------------------------

 void attrController::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &/* index */) const
 {
     editor->setGeometry(option.rect);
 }
