//
// C++ Interface: woverview
//
// Description:
//
//
// Author: Tue Haste Andersen <haste@diku.dk>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef WOVERVIEW_H
#define WOVERVIEW_H

#include <wwidget.h>
#include <qcolor.h>
#include <q3valuelist.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QMouseEvent>
#include <Q3MemArray>
#include <QPixmap>
/**
Waveform overview display

@author Tue Haste Andersen
*/


class WOverview : public WWidget
{
    Q_OBJECT
public:
    WOverview(QWidget *pParent=0, const char *pName=0);
    ~WOverview();
    void setup(QDomNode node);
    void setData(Q3MemArray<char> *pWaveformSummary, Q3ValueList<long> *pSegmentation, long liSampleDuration);
    void mouseMoveEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void paintEvent(QPaintEvent *);
    void repaint();
    QColor getMarkerColor();
    QColor getSignalColor();
public slots:
    void setValue(double);
    void setVirtualPos(double);
protected:

    /** Pointer to array containing waveform summary */
    Q3MemArray<char> *m_pWaveformSummary;
    /** Pointer to list of segmentation points */
    Q3ValueList<long> *m_pSegmentation;
    /** Duration of current track in samples */
    int m_liSampleDuration;
    /** True if slider is dragged. Only used when m_bEventWhileDrag is false */
    bool m_bDrag;
    /** Internal storage of slider position in pixels */
    int m_iPos, m_iVirtualPos, m_iStartMousePos;
    /** Pointer to screen buffer */
    QPixmap *m_pScreenBuffer;
    QColor m_qColorMarker;
    QColor m_qColorSignal;
};

#endif
