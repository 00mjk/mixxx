// loopingcontrol.cpp
// Created on Sep 23, 2008
// Author: asantoni, rryan

#include <QtDebug>
#include <QObject>

#include "controlobject.h"
#include "configobject.h"
#include "controlpushbutton.h"
#include "cachingreader.h"
#include "engine/loopingcontrol.h"
#include "engine/enginecontrol.h"
#include "mathstuff.h"

LoopingControl::LoopingControl(const char * _group,
                               ConfigObject<ConfigValue> * _config)
        : EngineControl(_group, _config) {

    m_bLoopingEnabled = false;
    m_iLoopStartSample = kNoTrigger;
    m_iLoopEndSample = kNoTrigger;

    //Create loop-in, loop-out, and reloop/exit ControlObjects
    m_pLoopInButton = new ControlPushButton(ConfigKey(_group, "loop_in"));
    connect(m_pLoopInButton, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopIn(double)),
            Qt::DirectConnection);
    m_pLoopInButton->set(0);

    m_pLoopOutButton = new ControlPushButton(ConfigKey(_group, "loop_out"));
    connect(m_pLoopOutButton, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopOut(double)),
            Qt::DirectConnection);
    m_pLoopOutButton->set(0);

    m_pReloopExitButton = new ControlPushButton(ConfigKey(_group, "reloop_exit"));
    connect(m_pReloopExitButton, SIGNAL(valueChanged(double)),
            this, SLOT(slotReloopExit(double)),
            Qt::DirectConnection);
    m_pReloopExitButton->set(0);


    m_pCOLoopEnabled = new ControlObject(ConfigKey(_group, "loop_enabled"));
    m_pCOLoopEnabled->set(0.0f);

    m_pCOLoopStartPosition =
            new ControlObject(ConfigKey(_group, "loop_start_position"));
    m_pCOLoopStartPosition->set(kNoTrigger);
    connect(m_pCOLoopStartPosition, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopStartPos(double)),
            Qt::DirectConnection);

    m_pCOLoopEndPosition =
            new ControlObject(ConfigKey(_group, "loop_end_position"));
    m_pCOLoopEndPosition->set(kNoTrigger);
    connect(m_pCOLoopEndPosition, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopEndPos(double)),
            Qt::DirectConnection);

    m_pCOLoopScale = new ControlObject(ConfigKey(_group, "loop_scale"));
    connect(m_pCOLoopScale, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopScale(double)));
    m_pLoopHalveButton = new ControlPushButton(ConfigKey(_group, "loop_halve"));
    connect(m_pLoopHalveButton, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopHalve(double)));
    m_pLoopDoubleButton = new ControlPushButton(ConfigKey(_group, "loop_double"));
    connect(m_pLoopDoubleButton, SIGNAL(valueChanged(double)),
            this, SLOT(slotLoopDouble(double)));


}

LoopingControl::~LoopingControl() {
}

void LoopingControl::slotLoopScale(double scale) {
    int loop_length = m_iLoopEndSample - m_iLoopStartSample;
    loop_length *= scale;
    m_iLoopEndSample = m_iLoopStartSample + loop_length;

    if (m_iLoopEndSample % 2 != 0) {
        m_iLoopEndSample--;
    }

    // Don't allow 0 samples loop, so one can still manipulate it
    if (m_iLoopEndSample == m_iLoopStartSample){
        m_iLoopEndSample = m_iLoopStartSample + 2;
    }

    // Update CO for loop end marker
    m_pCOLoopEndPosition->set(m_iLoopEndSample);
}

void LoopingControl::slotLoopHalve(double v) {
    if (v > 0.0) {
        slotLoopScale(0.5);
    }
}

void LoopingControl::slotLoopDouble(double v) {
    if (v > 0.0f) {
        slotLoopScale(2.0);
    }
}

double LoopingControl::process(const double dRate,
                               const double currentSample,
                               const double totalSamples,
                               const int iBufferSize) {
    m_iCurrentSample = currentSample;
    if (!even(m_iCurrentSample))
        m_iCurrentSample--;

    bool reverse = dRate < 0;

    double retval = kNoTrigger;
    if(m_bLoopingEnabled &&
       m_iLoopStartSample != kNoTrigger &&
       m_iLoopEndSample != kNoTrigger) {
        bool outsideLoop = (currentSample >= m_iLoopEndSample ||
                            currentSample <= m_iLoopStartSample);
        if (outsideLoop) {
            retval = reverse ? m_iLoopEndSample : m_iLoopStartSample;
        }
    }

    return retval;
}

double LoopingControl::nextTrigger(const double dRate,
                                   const double currentSample,
                                   const double totalSamples,
                                   const int iBufferSize) {
    bool bReverse = dRate < 0;

    if(m_bLoopingEnabled) {
        if (bReverse)
            return m_iLoopStartSample;
        else
            return m_iLoopEndSample;
    }
    return kNoTrigger;
}

double LoopingControl::getTrigger(const double dRate,
                                  const double currentSample,
                                  const double totalSamples,
                                  const int iBufferSize) {
    bool bReverse = dRate < 0;

    if(m_bLoopingEnabled) {
        if (bReverse)
            return m_iLoopEndSample;
        else
            return m_iLoopStartSample;
    }
    return kNoTrigger;
}

void LoopingControl::hintReader(QList<Hint>& hintList) {
    Hint loop_hint;
    // If the loop is enabled, then this is high priority because we will loop
    // sometime potentially very soon! The current audio itself is priority 1,
    // but we will issue ourselves at priority 2.
    if (m_bLoopingEnabled) {
        // If we're looping, hint the loop in and loop out, in case we reverse
        // into it. We could save information from process to tell which
        // direction we're going in, but that this is much simpler, and hints
        // aren't that bad to make anyway.
        if (m_iLoopStartSample >= 0) {
            loop_hint.priority = 2;
            loop_hint.sample = m_iLoopStartSample;
            loop_hint.length = 0; // Let it issue the default length
            hintList.append(loop_hint);
        }
        if (m_iLoopEndSample >= 0) {
            loop_hint.priority = 10;
            loop_hint.sample = m_iLoopEndSample;
            loop_hint.length = -1; // Let it issue the default (backwards) length
            hintList.append(loop_hint);
        }
    } else {
        if (m_iLoopStartSample >= 0) {
            loop_hint.priority = 10;
            loop_hint.sample = m_iLoopStartSample;
            loop_hint.length = 0; // Let it issue the default length
            hintList.append(loop_hint);
        }
    }
}

void LoopingControl::slotLoopIn(double val) {
    if (val) {
        // set loop in position
        m_iLoopStartSample = m_iCurrentSample;
        m_pCOLoopStartPosition->set(m_iLoopStartSample);

        // Reset the loop out position if it is before the loop in so that loops
        // cannot be inverted.
        if (m_iLoopEndSample != -1 &&
            m_iLoopEndSample < m_iLoopStartSample) {
            m_iLoopEndSample = -1;
            m_pCOLoopEndPosition->set(kNoTrigger);
        }
    }
}

void LoopingControl::slotLoopOut(double val) {
    if (val) {

        // If the user is trying to set a loop-out before the loop in or without
        // having a loop-in, then ignore it.
        if (m_iLoopStartSample == -1 ||
            m_iCurrentSample < m_iLoopStartSample) {
            return;
        }

        //set loop out position and start looping
        m_iLoopEndSample = m_iCurrentSample;
        m_pCOLoopEndPosition->set(m_iLoopEndSample);

        if (m_iLoopStartSample != -1 &&
            m_iLoopEndSample != -1) {
            setLoopingEnabled(true);
        }
        //qDebug() << "set loop_out to " << m_iLoopStartSample;
    }
}

void LoopingControl::slotReloopExit(double val) {
    if (val) {
        // If we're looping, stop looping
        if (m_bLoopingEnabled) {
            setLoopingEnabled(false);
            //qDebug() << "reloop_exit looping off";
        } else {
            // If we're not looping, jump to the loop-in point and start looping
            if (m_iLoopStartSample != -1 && m_iLoopEndSample != -1 &&
                m_iLoopStartSample <= m_iLoopEndSample) {
                setLoopingEnabled(true);
            }
            //qDebug() << "reloop_exit looping on";
        }
    }
}

void LoopingControl::slotLoopStartPos(double pos) {
    int newpos = pos;
    if (newpos >= 0 && !even(newpos)) {
        newpos--;
    }
    if (pos == -1.0f) {
        setLoopingEnabled(false);
    }

    m_iLoopStartSample = newpos;

    if (m_iLoopEndSample != -1 &&
        m_iLoopEndSample < m_iLoopStartSample) {
        m_iLoopEndSample = -1;
        m_pCOLoopEndPosition->set(kNoTrigger);
        setLoopingEnabled(false);
    }
}

void LoopingControl::slotLoopEndPos(double pos) {
    int newpos = pos;

    if (newpos >= 0 && !even(newpos)) {
        newpos--;
    }

    // Reject if the loop-in is not set, or if the new position is before the
    // start point (but not -1).
    if (m_iLoopStartSample == -1 ||
        (newpos >= 0 && newpos < m_iLoopStartSample)) {
        return;
    }

    if (pos == -1.0f) {
        setLoopingEnabled(false);
    }
    m_iLoopEndSample = newpos;
}

void LoopingControl::notifySeek(double dNewPlaypos) {
    if (m_bLoopingEnabled) {
        Q_ASSERT(m_iLoopStartSample != -1);
        Q_ASSERT(m_iLoopEndSample != -1);
        if (dNewPlaypos < m_iLoopStartSample || dNewPlaypos > m_iLoopEndSample) {
            setLoopingEnabled(false);
        }
    }
}

void LoopingControl::setLoopingEnabled(bool enabled) {
    m_bLoopingEnabled = enabled;
    m_pCOLoopEnabled->set(enabled);
}
