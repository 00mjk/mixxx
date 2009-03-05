/***************************************************************************
                          wnumber.cpp  -  description
                             -------------------
    begin                : Wed Jun 18 2003
    copyright            : (C) 2003 by Tue & Ken Haste Andersen
    email                : haste@diku.dk
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "wnumber.h"
#include "wskincolor.h"
#include <math.h>
#include <qfont.h>
//Added by qt3to4:
#include <QLabel>

WNumber::WNumber(QWidget * parent) : WWidget(parent)
{
    m_pLabel = new QLabel(parent);
    m_qsText = "";
    m_dConstFactor = 0.;
}

WNumber::~WNumber()
{
    delete m_pLabel;
}

void WNumber::setup(QDomNode node)
{
    WWidget::setup(node);

    // Number of digits
    setNumDigits(selectNodeInt(node, "NumberOfDigits"));

    // Colors
    QPalette palette = this->palette(); //we have to copy out the palette to edit it since it's const (probably for threadsafety)
    
    if(!WWidget::selectNode(node, "BgColor").isNull()) {
        m_qBgColor.setNamedColor(WWidget::selectNodeQString(node, "BgColor"));
        //m_pLabel->setPaletteBackgroundColor(WSkinColor::getCorrectColor(m_qBgColor));
        palette.setColor(this->backgroundRole(), WSkinColor::getCorrectColor(m_qBgColor));
        m_pLabel->setAutoFillBackground(true);
    }
    m_qFgColor.setNamedColor(WWidget::selectNodeQString(node, "FgColor"));
    //m_pLabel->setPaletteForegroundColor(WSkinColor::getCorrectColor(m_qFgColor));
    palette.setColor(this->foregroundRole(), WSkinColor::getCorrectColor(m_qFgColor));
    
    setPalette(palette);
    
    // Text
    if (!selectNode(node, "Text").isNull())
        m_qsText = selectNodeQString(node, "Text");

    // Size //XXX this code is repeated, it should be in a parsing class
    QString size = selectNodeQString(node, "Size");
    int x = size.left(size.indexOf(",")).toInt();
    int y = size.mid(size.indexOf(",")+1).toInt();
    setFixedSize(x,y);

    // FWI: Begin of font size patch                                                    // FWI
    int fontsize = 9;
    if (!selectNode(node, "FontSize").isNull())
        fontsize = selectNodeQString(node, "FontSize").toInt();
    m_pLabel->setFont( QFont("Helvetica",fontsize,QFont::Normal) );
    // FWI: End of font size patch

    // Alignment
    if (!selectNode(node, "Alignment").isNull())
    {
        if (selectNodeQString(node, "Alignment")=="right")
            m_pLabel->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
        // FWI: Begin of font alignment patch
        else if (selectNodeQString(node, "Alignment")=="center")
            m_pLabel->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
        // FWI: End of font alignment patch
    }

    // Constant factor
    if (!selectNode(node, "ConstFactor").isNull())
    {
        m_dConstFactor = selectNodeQString(node, "ConstFactor").toDouble();
        setValue(0.);
    }

    QString pos = selectNodeQString(node, "Pos");
    int px = pos.left(pos.indexOf(",")).toInt();
    int py = pos.mid(pos.indexOf(",")+1).toInt();
    move(px,py);
    m_pLabel->show();
}

void WNumber::setFixedSize(int x,int y)
{
    WWidget::setFixedSize(x,y);
    m_pLabel->setFixedSize(x,y);
}

void WNumber::move(int x, int y)
{
    WWidget::move(x,y);
    m_pLabel->move(x,y);
}

void WNumber::setNumDigits(int n)
{
    m_iNoDigits = n;
}

void WNumber::setValue(double dValue)
{
    double v = dValue+m_dConstFactor;
    int d1 = (int)floor((v-floor(v))*10.);
    int d2 = (int)floor((v-floor(v))*100.)%10;

    m_pLabel->setText(QString(m_qsText).append("%1.%2%3").arg((int)v,3,10).arg(d1,1,10).arg(d2,1,10));
}

void WNumber::setConstFactor(double c)
{
    m_dConstFactor = c;
}
