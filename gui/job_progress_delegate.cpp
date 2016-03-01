#include "job_progress_delegate.h"

JobProgressDelegate::JobProgressDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

void JobProgressDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
//    if (index.column() == 2) {
        QVariantList data = index.data(Qt::DisplayRole).toList();
        if ( ! data.isEmpty()) {
            QStyleOptionProgressBar progressBarOption;
            progressBarOption.rect = option.rect;
            progressBarOption.minimum = 0;
            progressBarOption.maximum = 100;
            progressBarOption.progress = data[0].toInt();
            progressBarOption.text = data[1].toString();
            progressBarOption.textVisible = true;
            if (data[2].toInt() ) {
                 progressBarOption.palette.setColor(QPalette::Highlight,QColor(Qt::magenta));
                 progressBarOption.palette.setColor(QPalette::Window,QColor::fromRgb(180,160,180));
            }

            QApplication::style()->drawControl(QStyle::CE_ProgressBar,
                                               &progressBarOption, painter);
        }
        else
            QStyledItemDelegate::paint(painter, option, index);
//    } else
//        QStyledItemDelegate::paint(painter, option, index);
}
