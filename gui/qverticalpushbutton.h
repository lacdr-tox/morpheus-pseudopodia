//////
//
// This file is part of the modelling and simulation framework 'Morpheus',
// and is made available under the terms of the BSD 3-clause license (see LICENSE
// file that comes with the distribution or https://opensource.org/licenses/BSD-3-Clause).
//
// Authors:  Joern Starruss and Walter de Back
// Copyright 2009-2016, Technische Universität Dresden, Germany
//
//////

#ifndef QVERTICALPUSHBUTTON_H
#define QVERTICALPUSHBUTTON_H

#include <QPushButton>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionToolButton>
#include <QDebug>


class QVerticalPushButton : public QPushButton
{
    Q_OBJECT
public:
    explicit QVerticalPushButton(QWidget *parent = 0);
    QVerticalPushButton(QIcon i, QString text, QWidget *parent = 0);
    QVerticalPushButton( QString text, QWidget *parent = 0);

    QSize sizeHint() const;
    QSize minimumSizeHint() const;
private:
    void paintEvent(QPaintEvent*);
    void computeMargins(int* hMargin, int* vMargin) const;
    QSize computeSizeHint(bool withText) const;
    void initStyleOption(QStyleOptionToolButton* opt) const;
signals:

public slots:

};

#endif // QVERTICALPUSHBUTTON_H
