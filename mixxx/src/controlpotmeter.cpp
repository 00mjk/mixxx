/***************************************************************************
                          controlpotmeter.cpp  -  description
                             -------------------
    begin                : Wed Feb 20 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "controlpushbutton.h"
#include "controlpotmeter.h"

/* -------- ------------------------------------------------------
   Purpose: Creates a new potmeter
   Input:   n - name
            midino - number of the midi controller.
            master - pointer to the control to which the potmeter is
                     attached. This control is acknowledged when the
                     potmeter is changed.
            midicontroller - pointer to the midi controller.
   -------- ------------------------------------------------------ */
ControlPotmeter::ControlPotmeter(ConfigKey key, double dMinValue, double dMaxValue) : ControlObject(key)
{
    setRange(dMinValue,dMaxValue);
    setStep(m_dValueRange/10.f);

    ControlPushButton * p;
    p = new ControlPushButton(ConfigKey(key.group, QString(key.item)+"_up"));
    connect(p, SIGNAL(valueChanged(double)), this, SLOT(incValue(double)));
    p = new ControlPushButton(ConfigKey(key.group, QString(key.item)+"_down"));
    connect(p, SIGNAL(valueChanged(double)), this, SLOT(decValue(double)));
}

ControlPotmeter::~ControlPotmeter()
{
}

double ControlPotmeter::getMin()
{
    return m_dMinValue;
}

double ControlPotmeter::getMax()
{
    return m_dMaxValue;
}

void ControlPotmeter::setStep(double dValue)
{
    m_dStep = dValue;
}

void ControlPotmeter::setRange(double dMinValue, double dMaxValue)
{
    m_dMinValue = dMinValue;
    m_dMaxValue = dMaxValue;
    m_dValueRange = m_dMaxValue-m_dMinValue;
    m_dValue = m_dMinValue + 0.5*m_dValueRange;
    //qDebug() << "" << this << ", min " << m_dMinValue << ", max " << m_dMaxValue << ", range " << m_dValueRange << ", val " << m_dValue;
}

double ControlPotmeter::getValueToWidget(double dValue)
{
    double out = (dValue-m_dMinValue)/m_dValueRange;
    return (out < 0.5) ? out*128. : out*126. + 1.;
}

double ControlPotmeter::GetMidiValue()
{
    double out = (m_dValue-m_dMinValue)/m_dValueRange;
    return (out < 0.5) ? out*128. : out*126. + 1.;
}

double ControlPotmeter::getValueFromWidget(double dValue)
{
    double out = (dValue < 64) ? dValue / 128. : (dValue-1) / 126.;
    return m_dMinValue + out * m_dValueRange;
}

void ControlPotmeter::setValueFromThread(double dValue)
{
    if (dValue>m_dMaxValue)
        m_dValue = m_dMaxValue;
    else if (dValue<m_dMinValue)
        m_dValue = m_dMinValue;
    else
        m_dValue = dValue;
    emit(valueChanged(m_dValue));
}

void ControlPotmeter::setValueFromEngine(double dValue)
{
    if (dValue>m_dMaxValue)
        m_dValue = m_dMaxValue;
    else if (dValue<m_dMinValue)
        m_dValue = m_dMinValue;
    else
        m_dValue = dValue;
    emit(valueChangedFromEngine(m_dValue));
}

void ControlPotmeter::setValueFromMidi(MidiCategory, double v)
{
    double out = (v < 64) ? v / 128. : (v-1) / 126.;
    m_dValue = m_dMinValue + out*m_dValueRange;
    emit(valueChanged(m_dValue));
}

void ControlPotmeter::incValue(double keypos)
{
    if (keypos>0)
    {
        if (m_dValue+m_dStep>m_dMaxValue)
            m_dValue = m_dMaxValue;
        else
            m_dValue += m_dStep;
        emit(valueChanged(m_dValue));

        // incValue will be activated by assosiated _up or _down ControlObject, and thus it is safe to update all proxies.
        updateProxies(0);
    }
}

void ControlPotmeter::decValue(double keypos)
{
    if (keypos>0)
    {
        if (m_dValue-m_dStep<m_dMinValue)
            m_dValue = m_dMinValue;
        else
            m_dValue -= m_dStep;
        emit(valueChanged(m_dValue));

        // incValue will be activated by assosiated _up or _down ControlObject, and thus it is safe to update all proxies.
        updateProxies(0);
    }
}

