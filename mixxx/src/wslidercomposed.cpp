/***************************************************************************
                          wslidercomposed.cpp  -  description
                             -------------------
    begin                : Tue Jun 25 2002
    copyright            : (C) 2002 by Tue & Ken Haste Andersen
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

#include "wslidercomposed.h"
#include <qpixmap.h>
#include <qpainter.h>
//Added by qt3to4:
#include <QMouseEvent>
#include <QPaintEvent>
#include "defs.h"
#include "wpixmapstore.h"

WSliderComposed::WSliderComposed(QWidget *parent, const char *name ) : WWidget(parent,name)
{
    m_pSlider = 0;
    m_pHandle = 0;
    m_pDoubleBuffer = 0;
    m_bHorizontal = false;
    m_bEventWhileDrag = true;
    m_bDrag = false;

    // Set default values
    m_iSliderLength=0;
    m_iHandleLength=0;

    m_fValue = 63.5; // FWI: Fixed slider default positon
}

WSliderComposed::~WSliderComposed()
{
    unsetPixmaps();
}

void WSliderComposed::setup(QDomNode node)
{
    // Setup position
    WWidget::setup(node);

    // Setup pixmaps
    QString pathSlider = getPath(selectNodeQString(node, "Slider"));
    QString pathHandle = getPath(selectNodeQString(node, "Handle"));
    QString pathHorizontal = selectNodeQString(node, "Horizontal");
    bool h = false;
    if (pathHorizontal.contains("true",false))
        h = true;
    setPixmaps(h, pathSlider, pathHandle);

    if (!selectNode(node, "EventWhileDrag").isNull())
        if (selectNodeQString(node, "EventWhileDrag").contains("no"))
            m_bEventWhileDrag = false;
}


void WSliderComposed::setPixmaps(bool bHorizontal, const QString &filenameSlider, const QString &filenameHandle)
{
    m_bHorizontal = bHorizontal;
    unsetPixmaps();
    m_pSlider = WPixmapStore::getPixmap(filenameSlider);
    if (!m_pSlider)
        qDebug("WSliderComposed: Error loading slider pixmap: %s",filenameSlider.latin1());
    m_pHandle = WPixmapStore::getPixmap(filenameHandle);
    if (!m_pHandle)
        qDebug("WSliderComposed: Error loading handle pixmap: %s",filenameHandle.latin1());
    m_pDoubleBuffer = new QPixmap(m_pSlider->size());
    
    if (m_bHorizontal)
    {
        m_iSliderLength = m_pSlider->width();
        m_iHandleLength = m_pHandle->width();
    }
    else
    {
        m_iSliderLength = m_pSlider->height();
        m_iHandleLength = m_pHandle->height();
    }

    // Set size of widget, using size of slider pixmap
    if (m_pSlider)
        setFixedSize(m_pSlider->size());
    
    setValue(m_fValue);

    repaint();
}

void WSliderComposed::unsetPixmaps()
{
    if (m_pSlider)
        WPixmapStore::deletePixmap(m_pSlider);
    if (m_pHandle)
        WPixmapStore::deletePixmap(m_pHandle);
    if (m_pDoubleBuffer)
        WPixmapStore::deletePixmap(m_pDoubleBuffer);
    m_pSlider = 0;
    m_pHandle = 0;
    m_pDoubleBuffer = 0;
}

void WSliderComposed::mouseMoveEvent(QMouseEvent *e)
{
    if (m_bHorizontal)
        m_iPos = e->x()-m_iHandleLength/2;
    else
        m_iPos = e->y()-m_iHandleLength/2;

    //qDebug("start %i, pos %i",m_iStartPos, m_iPos);
    m_iPos = m_iStartHandlePos + (m_iPos-m_iStartMousePos);

    if (m_iPos>(m_iSliderLength-m_iHandleLength))
        m_iPos = m_iSliderLength-m_iHandleLength;
    else if (m_iPos<0)
        m_iPos = 0;

    // value ranges from 0 to 127
    m_fValue = (double)m_iPos*(127./(double)(m_iSliderLength-m_iHandleLength));
    if (!m_bHorizontal)
        m_fValue = 127.-m_fValue;

    // Emit valueChanged signal
    if (m_bEventWhileDrag)
    {
        if (e->button()==Qt::RightButton)
            emit(valueChangedRightUp(m_fValue));
        else
            emit(valueChangedLeftUp(m_fValue));
    }

    // Update display
    update();
}

void WSliderComposed::mouseReleaseEvent(QMouseEvent *e)
{
    if (!m_bEventWhileDrag)
    {
        mouseMoveEvent(e);

        if (e->button()==Qt::RightButton)
            emit(valueChangedRightUp(m_fValue));
        else
            emit(valueChangedLeftUp(m_fValue));

        m_bDrag = false;
    }
}

void WSliderComposed::mousePressEvent(QMouseEvent *e)
{
    if (!m_bEventWhileDrag)
    {
        m_iStartMousePos = 0;
        m_iStartHandlePos = 0;
        mouseMoveEvent(e);
        m_bDrag = true;
    }
    else
    {
        if (e->button() == Qt::RightButton)
            reset();
        else
        {
            if (m_bHorizontal)
                m_iStartMousePos = e->x()-m_iHandleLength/2;
            else
                m_iStartMousePos = e->y()-m_iHandleLength/2;

            m_iStartHandlePos = m_iPos;
        }
    }
}

void WSliderComposed::paintEvent(QPaintEvent *)
{
    if (m_pSlider && m_pHandle)
    {
        int posx;
        int posy;
        if (m_bHorizontal)
        {
            posx = m_iPos;
            posy = 0;
        }
        else
        {
            posx = 0;
            posy = m_iPos;
        }

        // Draw slider followed by handle to double buffer
        bitBlt(m_pDoubleBuffer, 0, 0, m_pSlider);
        bitBlt(m_pDoubleBuffer, posx, posy, m_pHandle);

        // Draw double buffer to screen
        bitBlt(this, 0, 0, m_pDoubleBuffer);
    }
}

void WSliderComposed::setValue(double fValue)
{
    if (!m_bDrag)
    {
        // Set value without emitting a valueChanged signal, and force display update
        m_fValue = fValue;

        // Calculate handle position
        if (!m_bHorizontal)
            fValue = 127-fValue;
        m_iPos = (int)((fValue/127.)*(double)(m_iSliderLength-m_iHandleLength));

	if (m_iPos>(m_iSliderLength-m_iHandleLength))
	    m_iPos = m_iSliderLength-m_iHandleLength;
	else if (m_iPos<0)
	    m_iPos = 0;

        repaint();
    }
}

void WSliderComposed::reset()
{
    setValue(63.5); // FWI: Fixed slider default Position
    emit(valueChangedLeftUp(m_fValue));
    emit(valueChangedLeftDown(m_fValue));
}

