
#ifndef WGLWAVEFORMVIEWER_H
#define WGLWAVEFORMVIEWER_H

#include <qgl.h>
#include <QList>
#include <QEvent>
#include <QDateTime>
#include <QMutex>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTimerEvent>

#include "wwidget.h"
#include "defs.h"

class EngineBuffer;
class WaveformRenderer;
class TrackInfoObject;

class WGLWaveformViewer : public QGLWidget
{
    Q_OBJECT
public:
    WGLWaveformViewer(const char *group, QWidget *pParent=0, const QGLWidget *pShareWidget = 0, Qt::WFlags f = 0);
    ~WGLWaveformViewer();

    bool directRendering();
    void dragEnterEvent(QDragEnterEvent *event);
    void dropEvent(QDropEvent *event);
    void setup(QDomNode node);
    bool eventFilter(QObject *o, QEvent *e);
    //void resetColors();
    
public slots:
    void slotNewTrack(TrackInfoObject*);
    void setValue(double);
signals:
    void valueChangedLeftDown(double);
    void valueChangedRightDown(double);
    void trackDropped(QString filename);

protected:
    
    void initializeGL();
    void resizeGL(int, int);
    void paintGL();
    
    void timerEvent(QTimerEvent *);
    void paintEvent(QPaintEvent* event);

private:
    /** Used in mouse event handler */
    int m_iMouseStart;

    /** Timer id */
    int m_iTimerID;

    /** Waveform Renderer does all the work for us */
    WaveformRenderer *m_pWaveformRenderer;
    
    /** Colors */
    QColor colorBeat, colorSignal, colorHfc, colorMarker, colorFisheye, colorBack, colorCue;

    bool m_painting;
    QMutex m_paintMutex;

    const char *m_pGroup;
    EngineBuffer *m_pEngineBuffer;

};

#endif
