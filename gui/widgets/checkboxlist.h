#ifndef DROPDOWNMULTISELECT_H
#define DROPDOWNMULTISELECT_H

#include <QtWidgets>

class CheckBoxList: public QComboBox
{
    Q_OBJECT

public:
    CheckBoxList(QWidget *widget = nullptr);
    virtual ~CheckBoxList() override;
    bool eventFilter(QObject *object, QEvent *event) override;
    virtual void paintEvent(QPaintEvent *) override;
    void SetDisplayText(QString text);
    QString GetDisplayText() const;
    void hidePopup() override;
    QString	currentText() const;
    QVariant currentData(int role = Qt::UserRole) const;

private:
    QStringList _current;
    QString m_DisplayText;
};

class CheckBoxListDelegate : public QAbstractItemDelegate
{
public:
    CheckBoxListDelegate(QObject* parent): QAbstractItemDelegate(parent){}
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex &index) const override;
    QWidget *createEditor(QWidget *parent,const QStyleOptionViewItem & option ,const QModelIndex & index ) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
};

#endif // DROPDOWNMULTISELECT_H
