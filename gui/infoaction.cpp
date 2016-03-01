#include "infoaction.h"

infoAction::infoAction(QString n, QString s, QObject *o) : QAction(n, o) {
    setToolTip(s.remove(s.size()-1, 1));
    connect(this, SIGNAL(hovered()), SLOT(showInfo()));
}

//-----------------------------------------------------------------------------

void infoAction::showInfo() {
    QToolTip::showText(parentWidget()->cursor().pos(), toolTip());
}
