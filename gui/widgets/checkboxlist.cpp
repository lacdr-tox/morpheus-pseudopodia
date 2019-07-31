#include "checkboxlist.h"

//min-width:10em;
     CheckBoxList::CheckBoxList(QWidget *widget ) : QComboBox(widget),m_DisplayText("")
     {
    // set delegate items view
    view()->setItemDelegate(new CheckBoxListDelegate(this));
    //view()->setStyleSheet("  padding: 15px; ");
    // Enable editing on items view
    view()->setEditTriggers(QAbstractItemView::CurrentChanged);

    // set "CheckBoxList::eventFilter" as event filter for items view
    view()->viewport()->installEventFilter(this);


    // it just cool to have it as defualt ;)
    view()->setAlternatingRowColors(true);
     }

CheckBoxList::~CheckBoxList(){}

void CheckBoxList::hidePopup()
{
    QComboBox::hidePopup();
    updateText();
}

bool CheckBoxList::eventFilter(QObject *object, QEvent *event)
{
    // don't close items view after we release the mouse button
    // by simple eating MouseButtonRelease in viewport of items view
    if(event->type() == QEvent::MouseButtonRelease && object==view()->viewport())
    {
        return true;
    }
    return QComboBox::eventFilter(object,event);
}

void CheckBoxList::paintEvent(QPaintEvent *)
{
    QStylePainter painter(this);
    painter.setPen(palette().color(QPalette::Text));

    // draw the combobox frame, focusrect and selected etc.
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    // if no display text been set , use "..." as default
    if(m_DisplayText.isNull())
        opt.currentText = "......";
    else
        opt.currentText = m_DisplayText;
    painter.drawComplexControl(QStyle::CC_ComboBox, opt);

    // draw the icon and text
    painter.drawControl(QStyle::CE_ComboBoxLabel, opt);
}

void CheckBoxList::setData(QStringList data)
{
	for(auto& elem:data)
	{
		elem = elem.trimmed();
	}
	for(int y = 0; y < model()->rowCount(); ++y)
    {
		auto idx = model()->index(y,0);
		if(data.contains(model()->data(idx,Qt::DisplayRole).toString()))
		{
			model()->setData(idx, true,Qt::UserRole);
			data.removeAll(model()->data(idx,Qt::DisplayRole).toString());
		}else{
			model()->setData(idx, false,Qt::UserRole);
		}
    }
    if(!data.empty())
	{
		qDebug()<<"CheckBoxList::SetData keys: "<<data<<" have not been found!";
	}
    updateText();
}

void CheckBoxList::updateText()
{
	QStringList value;
    for(int y = 0; y < model()->rowCount(); ++y)
    {
        if(model()->data(model()->index(y,0),Qt::UserRole).toBool())
        {
            value.append(model()->data(model()->index(y,0),Qt::DisplayRole).toString());
        }
    }
    
	if (_current == value) return;
	
	_current = value;
	m_DisplayText = _current.join(", ");
	emit currentTextChanged(_current);
}

QString	CheckBoxList::currentText() const
{
    return _current.join(", ");
}

QVariant CheckBoxList::currentData(int role) const
{
    if(role == Qt::UserRole || role == Qt::EditRole)
    {
        return _current;
    }else if(role == Qt::DisplayRole){
		return m_DisplayText;
	}else{
        return false;
    }
}

void CheckBoxListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const
{
    bool value = index.data(Qt::UserRole).toBool();
    QString text = index.data(Qt::DisplayRole).toString();
    const QStyle *style = QApplication::style();
    QStyleOptionButton opt;
    opt.state |= value ? QStyle::State_On : QStyle::State_Off;
    opt.state |= QStyle::State_Enabled;
    opt.text = text;
    opt.rect = option.rect;

    // draw item data as CheckBox
    style->drawControl(QStyle::CE_CheckBox,&opt,painter);
}

QSize CheckBoxListDelegate::sizeHint(const QStyleOptionViewItem & option, const QModelIndex &index) const
{
    return QSize(90,30);
}

QWidget* CheckBoxListDelegate::createEditor(QWidget *parent,const QStyleOptionViewItem & option ,const QModelIndex & index ) const
{
    QCheckBox* edit = new QCheckBox(parent);
    // this->connect(edit,SIGNAL(stateChanged()),this,SLOT(setModelData()));
    connect(edit, &QPushButton::clicked, [=] {setModelData(edit,const_cast<QAbstractItemModel*>(index.model()),index);});
    return edit;
}

void CheckBoxListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QCheckBox* edit = static_cast<QCheckBox*>(editor);
    edit->setText(index.data(Qt::DisplayRole).toString());
    edit->setChecked(index.data(Qt::UserRole).toBool());
}

void CheckBoxListDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const
{
    QCheckBox *myEditor = static_cast<QCheckBox*>(editor);
    bool value = myEditor->isChecked();

    //set model data
    QMap<int,QVariant> data;
    data.insert(Qt::DisplayRole,myEditor->text());
    data.insert(Qt::UserRole,value);
    model->setItemData(index,data);
}

void CheckBoxListDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    editor->setGeometry(option.rect);
}
