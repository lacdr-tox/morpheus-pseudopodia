#ifndef DROPDOWNMULTISELECT_H
#define DROPDOWNMULTISELECT_H

#include <QtWidgets>

class CheckBoxList: public QComboBox
{
    Q_OBJECT

public:
	CheckBoxList(QWidget *widget = nullptr);
	~CheckBoxList() override;
	bool eventFilter(QObject *object, QEvent *event) override;
	void paintEvent(QPaintEvent *) override;
	void setData(QStringList data);
	QString currentText() const;
	QVariant currentData(int role = Qt::UserRole) const;
signals:
	void currentTextChanged(QStringList newList); 
	
private slots:
	void updateText();
private: 
    QStringList _current;
    QString m_DisplayText;
};

class CheckBoxListDelegate : public QAbstractItemDelegate
{
public:
	static const auto DataRole = Qt::UserRole;
	
	CheckBoxListDelegate(QObject* parent);
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex &index) const override;
	QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem & option ,const QModelIndex & index ) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
private:
	QStyleOptionButton default_opt;
};

#endif // DROPDOWNMULTISELECT_H
