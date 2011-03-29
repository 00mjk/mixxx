// loopingcontrol.h
// Created on Sep 23, 2008
// Author: asantoni, rryan

#ifndef LOOPINGCONTROL_H
#define LOOPINGCONTROL_H

#include <QObject>

#include "configobject.h"
#include "engine/enginecontrol.h"

class ControlPushButton;
class ControlObject;

class LoopingControl : public EngineControl {
    Q_OBJECT
  public:
    LoopingControl(const char * _group, ConfigObject<ConfigValue> * _config);
    virtual ~LoopingControl();

    // process() updates the internal state of the LoopingControl to reflect the
    // correct current sample. If a loop should be taken LoopingControl returns
    // the sample that should be seeked to. Otherwise it returns currentSample.
    double process(const double dRate,
                   const double currentSample,
                   const double totalSamples,
                   const int iBufferSize);

    // nextTrigger returns the sample at which the engine will be triggered to
    // take a loop, given the value of currentSample and dRate.
    double nextTrigger(const double dRate,
                       const double currentSample,
                       const double totalSamples,
                       const int iBufferSize);

    // getTrigger returns the sample that the engine will next be triggered to
    // loop to, given the value of currentSample and dRate.
    double getTrigger(const double dRate,
                      const double currentSample,
                      const double totalSamples,
                      const int iBufferSize);

    // hintReader will add to hintList hints both the loop in and loop out
    // sample, if set.
    void hintReader(QList<Hint>& hintList);

    void notifySeek(double dNewPlaypos);

  public slots:
    void slotLoopIn(double);
    void slotLoopOut(double);
    void slotReloopExit(double);
    void slotLoopStartPos(double);
    void slotLoopEndPos(double);
    void slotLoopScale(double);
    void slotLoopDouble(double);
    void slotLoopHalve(double);

  private:
    void setLoopingEnabled(bool enabled);

    ControlObject* m_pCOLoopStartPosition;
    ControlObject* m_pCOLoopEndPosition;
    ControlObject* m_pCOLoopEnabled;
    ControlPushButton* m_pLoopInButton;
    ControlPushButton* m_pLoopOutButton;
    ControlPushButton* m_pReloopExitButton;
    ControlObject* m_pCOLoopScale;
    ControlPushButton* m_pLoopHalveButton;
    ControlPushButton* m_pLoopDoubleButton;

    bool m_bLoopingEnabled;
    int m_iLoopEndSample;
    int m_iLoopStartSample;
    int m_iCurrentSample;
};

#endif /* LOOPINGCONTROL_H */
