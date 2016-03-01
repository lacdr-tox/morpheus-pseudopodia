#include "qverticalpushbutton.h"

QVerticalPushButton::QVerticalPushButton(QWidget *parent) :
    QPushButton(parent)
{
}

QVerticalPushButton::QVerticalPushButton(QIcon i, QString text, QWidget *parent) :
    QPushButton(i, text, parent)
{
}

QVerticalPushButton::QVerticalPushButton( QString text, QWidget *parent) :
    QPushButton(text, parent)
{
}


void QVerticalPushButton::computeMargins(int* hMargin, int* vMargin) const {
    *hMargin = 3;
    *vMargin = 8;
}

void QVerticalPushButton::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    // Paint bevel..
    if (underMouse() || isChecked())
        style()->drawPrimitive(QStyle::PE_PanelButtonTool, &opt, &painter);
    int hMargin, vMargin;
    computeMargins(&hMargin, &vMargin);

    QPixmap pixmap = icon().pixmap(20);
    int iconRoom = pixmap.height() + 2*vMargin;
    int textRoom = height() - iconRoom - vMargin;
    QString t = painter.fontMetrics().elidedText(text(), Qt::ElideRight, textRoom);
    if (t == QLatin1String("...") || t == QChar(0x2026))
        t.clear();

    QRect iconArea = QRect(0, vMargin + textRoom, width(), iconRoom);
    QRect labelArea = QRect(0, vMargin, width(), textRoom);

    style()->drawItemPixmap(&painter, iconArea, Qt::AlignCenter | Qt::AlignVCenter, pixmap);

    QRect labelPaintArea = labelArea;

    // If we're vertical, we paint to a simple 0,0 origin rect,
    // and get the transformations to get us in the right place
    labelPaintArea = QRect(0, 0, labelArea.height(), labelArea.width());
    QTransform tr;
    tr.translate(labelArea.x(), labelPaintArea.width() + labelArea.y());
    tr.rotate(-90);

    painter.setTransform(tr);
    style()->drawItemText(&painter, labelPaintArea, Qt::AlignLeading | Qt::AlignVCenter, palette(), true, t, QPalette::ButtonText);
}

QSize QVerticalPushButton::minimumSizeHint() const
{
     return computeSizeHint(false);
}


QSize QVerticalPushButton::sizeHint() const
{
     return computeSizeHint(true);
}

QSize QVerticalPushButton::computeSizeHint(bool withText) const
{
    // Compute as horizontal first, then flip around if need be.
    QStyleOptionToolButton opt;
    initStyleOption(&opt);

    int hMargin, vMargin;
    computeMargins(&hMargin, &vMargin);

    // Compute interior size, starting from pixmap..
    QSize size;
    QPixmap iconPix = icon().pixmap(20);
    size = iconPix.size();

    // Always include text height in computation, to avoid resizing the minor direction
    // when expanding text..
    QSize textSize = fontMetrics().size(Qt::TextShowMnemonic, text());
    size.setWidth(qMax(size.width(), textSize.height()));

    // Pick margins for major/minor direction, depending on orientation
    size.setWidth (size.width()  + 2*hMargin);
    size.setHeight(size.height() + 2*vMargin);

       // Add enough room for the text, and an extra major margin.
    if (withText)
        size.setHeight(size.height() + textSize.width() + vMargin);
    return size;
}

void QVerticalPushButton::initStyleOption(QStyleOptionToolButton* opt) const
{
    opt->initFrom(this);
    // Setup icon..
    if (!icon().isNull()) {
        opt->iconSize = icon().pixmap(20).size();
        opt->icon     = icon();
    }

    // Should we draw text?
    opt->text = text();

    if (underMouse())
        opt->state |= QStyle::State_AutoRaise | QStyle::State_MouseOver | QStyle::State_Raised;

    if (isChecked())
        opt->state |= QStyle::State_Sunken | QStyle::State_On;

    opt->font = font();
    opt->toolButtonStyle = Qt::ToolButtonTextBesideIcon;
    opt->subControls = QStyle::SC_ToolButton;
}


