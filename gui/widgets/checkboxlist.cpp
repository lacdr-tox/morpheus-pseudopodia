#include "checkboxlist.h"

//min-width:10em;
CheckBoxList::CheckBoxList(QWidget *widget ) : QComboBox(widget), m_DisplayText("")
{
	// set delegate items view
	view()->setItemDelegate(new CheckBoxListDelegate(this));
	//view()->setStyleSheet("  padding: 15px; ");
	// Enable editing on items view
	view()->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::DoubleClicked);
	// set "CheckBoxList::eventFilter" as event filter for items view
	view()->viewport()->installEventFilter(this);
	// it just cool to have it as defualt ;)
	view()->setAlternatingRowColors(true);
	view()->setCurrentIndex(QModelIndex());
	connect(model(), &QAbstractItemModel::dataChanged, this, &CheckBoxList::updateText);
	connect(model(), &QAbstractItemModel::rowsInserted, this, &CheckBoxList::updateText);
	connect(model(), &QAbstractItemModel::rowsRemoved, this, &CheckBoxList::updateText);
}

CheckBoxList::~CheckBoxList(){}

bool CheckBoxList::eventFilter(QObject *object, QEvent *event)
{
	// Make sure the first event is not current, such that the editor opens on hover
	if(event->type() == QEvent::Show && object==view()->viewport()) {
		view()->setCurrentIndex(rootModelIndex());
	}
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
    if(m_DisplayText.isEmpty())
        opt.currentText = "....";
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
	model()->blockSignals(true);
	for(int y = 0; y < model()->rowCount(); ++y)
    {
		auto idx = model()->index(y,0);
		if(data.contains(model()->data(idx,Qt::DisplayRole).toString())) {
			model()->setData(idx, true, Qt::UserRole);
		}
		else {
			model()->setData(idx, false,Qt::UserRole);
		}
    }
    model()->blockSignals(false);
    updateText();
}

void CheckBoxList::updateText()
{
	QStringList value;
	for(int y = 0; y < model()->rowCount(); ++y)
	{
		auto idx = model()->index(y,0);
		if (idx.data(Qt::UserRole).toBool()) {
			value.append( idx.data(Qt::DisplayRole).toString());
		}
	}
    
	if (_current == value) return;
	
	_current = value;
	m_DisplayText = _current.join(", ");
	repaint();
	emit currentTextChanged(_current);
}

QString CheckBoxList::currentText() const
{
    return _current.join(", ");
}

QVariant CheckBoxList::currentData(int role) const
{
	if(role == Qt::UserRole || role == Qt::EditRole) {
		return _current;
	}
	else if(role == Qt::DisplayRole){
		return m_DisplayText;
	}
	else {
		return QComboBox::currentData(role);
	}
}


CheckBoxListDelegate::CheckBoxListDelegate(QObject* parent) : QAbstractItemDelegate(parent) {
	QCheckBox box;
	default_opt.initFrom(&box);
}

void CheckBoxListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,const QModelIndex &index) const
{
	// Draw CheckBox views as visualisation
    bool value = index.data(DataRole).toBool();
    QString text = index.data(Qt::DisplayRole).toString();
    const QStyle *style = QApplication::style();
    QStyleOptionButton opt = default_opt;
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
	edit->setFocusPolicy(Qt::StrongFocus);
	auto model = const_cast<QAbstractItemModel*>(index.model());
	connect(edit, &QCheckBox::stateChanged, [=](bool state) {
		model->setData(index, state, DataRole);
	});
	return edit;
}

void CheckBoxListDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    QCheckBox* edit = static_cast<QCheckBox*>(editor);
    edit->setText(index.data(Qt::DisplayRole).toString());
    edit->setChecked(index.data(DataRole).toBool());
}

void CheckBoxListDelegate::updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    editor->setGeometry(option.rect);
}
