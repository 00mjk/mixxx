
#ifndef WAVEFORMVIEWERFACTORY_H
#define WAVEFORMVIEWERFACTORY_H

#include <QList>
#include <QDomNode>

#include "configobject.h"
#include "widget/wwidget.h"

class WVisualSimple;
class WWaveformViewer;
class WGLWaveformViewer;

enum WaveformViewerType {
    WAVEFORM_SIMPLE=0,
    WAVEFORM_WIDGET,
    WAVEFORM_GL,
    WAVEFORM_INVALID
};


class WaveformViewerFactory {
private:
    /* shouldn't be accessible */
    WaveformViewerFactory() {};
    ~WaveformViewerFactory() {};
    static QList<QObject*> m_viewers;
    static QList<WVisualSimple*> m_simpleViewers;
    static QList<WWaveformViewer*> m_visualViewers;
    static QList<WGLWaveformViewer*> m_visualGLViewers;

public:
    static WaveformViewerType createWaveformViewer(const char* group, QWidget *pParent, ConfigObject<ConfigValue> *pConfig, QObject **target);
    static void destroyWaveformViewer(QObject *pWaveformViewer);
    static WaveformViewerType getWaveformViewerType(QObject *pWaveformViewer);
    
};

#endif
