/***************************************************************************
                          dlgprefeq.cpp  -  description
                             -------------------
    begin                : Thu Jun 7 2007
    copyright            : (C) 2007 by John Sully
    email                : jsully@scs.ryerson.ca
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "dlgprefeq.h"
#include "engine/enginefilteriir.h"
#include <qlineedit.h>
#include <qwidget.h>
#include <qslider.h>
#include <qlabel.h>
#include <qstring.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qgraphicsscene.h>

#include <assert.h>

#define CONFIG_KEY "[Mixer Profile]"

DlgPrefEQ::DlgPrefEQ(QWidget *pParent, ConfigObject<ConfigValue> *pConfig)
  : QWidget(pParent)
  , Ui::DlgPrefEQDlg()
#ifndef __LOFI__
  , m_COTLoFreq(ControlObject::getControl(ConfigKey(CONFIG_KEY, "LoEQFrequency")))
  , m_COTHiFreq(ControlObject::getControl(ConfigKey(CONFIG_KEY, "HiEQFrequency")))
  , m_COTLoFi(ControlObject::getControl(ConfigKey(CONFIG_KEY, "LoFiEQs")))
#endif
{
    m_pConfig = pConfig;

    setupUi(this);

    // Connection
#ifndef __LOFI__
    connect(SliderHiEQ, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateHiEQ()));
    connect(SliderHiEQ, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateHiEQ()));
    connect(SliderHiEQ, SIGNAL(sliderReleased()), this, SLOT(slotUpdateHiEQ()));

    connect(SliderLoEQ, SIGNAL(valueChanged(int)), this, SLOT(slotUpdateLoEQ()));
    connect(SliderLoEQ, SIGNAL(sliderMoved(int)), this, SLOT(slotUpdateLoEQ()));
    connect(SliderLoEQ, SIGNAL(sliderReleased()), this, SLOT(slotUpdateLoEQ()));

    connect(CheckBoxLoFi, SIGNAL(stateChanged(int)), this, SLOT(slotLoFiChanged()));
#else
    CheckBoxLoFi->setChecked(true);
    slotLoFiChanged();
    CheckBoxLoFi->setEnabled(false);
#endif
    connect(PushButtonReset, SIGNAL(clicked(bool)), this, SLOT(reset()));

    m_lowEqFreq = 0;
    m_highEqFreq = 0;

    loadSettings();
}

DlgPrefEQ::~DlgPrefEQ()
{
}

void DlgPrefEQ::loadSettings()
{
    if (m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "HiEQFrequency")) == QString("")) {
        // apparently we don't have any settings, set defaults
        CheckBoxLoFi->setChecked(true);
        setDefaultShelves();
    }
    SliderHiEQ->setValue(
        getSliderPosition(m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "HiEQFrequency")).toInt()));
    SliderLoEQ->setValue(
        getSliderPosition(m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "LoEQFrequency")).toInt()));

    if (m_pConfig->getValueString(ConfigKey(CONFIG_KEY, "LoFiEQs")) == QString("yes")) {
        CheckBoxLoFi->setChecked(true);
    } else {
        CheckBoxLoFi->setChecked(false);
    }

    slotUpdate();
    slotApply();
}

void DlgPrefEQ::setDefaultShelves()
{
    m_pConfig->set(ConfigKey(CONFIG_KEY, "HiEQFrequency"), ConfigValue(2500));
    m_pConfig->set(ConfigKey(CONFIG_KEY, "LoEQFrequency"), ConfigValue(250));
}

/** Resets settings, leaves LOFI box checked asis.
 */
void DlgPrefEQ::reset() {
    setDefaultShelves();
    loadSettings();
}

void DlgPrefEQ::slotLoFiChanged()
{
    GroupBoxHiEQ->setEnabled(!CheckBoxLoFi->isChecked());
    GroupBoxLoEQ->setEnabled(!CheckBoxLoFi->isChecked());
    if(CheckBoxLoFi->isChecked()) {
        m_pConfig->set(ConfigKey(CONFIG_KEY, "LoFiEQs"), ConfigValue(QString("yes")));
    } else {
        m_pConfig->set(ConfigKey(CONFIG_KEY, "LoFiEQs"), ConfigValue(QString("no")));
    }
    slotApply();
}

void DlgPrefEQ::slotUpdateHiEQ()
{
    if (SliderHiEQ->value() < SliderLoEQ->value())
    {
        SliderHiEQ->setValue(SliderLoEQ->value());
    }
    m_highEqFreq = getEqFreq(SliderHiEQ->value());
    validate_levels();
    if (m_highEqFreq < 1000) {
        TextHiEQ->setText( QString("%1 Hz").arg(m_highEqFreq));
    } else {
        TextHiEQ->setText( QString("%1 Khz").arg(m_highEqFreq / 1000.));
    }
    m_pConfig->set(ConfigKey(CONFIG_KEY, "HiEQFrequency"), ConfigValue(m_highEqFreq));

    slotApply();
}

void DlgPrefEQ::slotUpdateLoEQ()
{
    if (SliderLoEQ->value() > SliderHiEQ->value())
    {
        SliderLoEQ->setValue(SliderHiEQ->value());
    }
    m_lowEqFreq = getEqFreq(SliderLoEQ->value());
    validate_levels();
    if (m_lowEqFreq < 1000) {
        TextLoEQ->setText(QString("%1 Hz").arg(m_lowEqFreq));
    } else {
        TextLoEQ->setText(QString("%1 Khz").arg(m_lowEqFreq / 1000.));
    }
    m_pConfig->set(ConfigKey(CONFIG_KEY, "LoEQFrequency"), ConfigValue(m_lowEqFreq));

    slotApply();
}

int DlgPrefEQ::getSliderPosition(int eqFreq)
{
    if(eqFreq >= 20050) {
        return 480;
    }
    double dsliderPos = pow(eqFreq, 1./4.);
    dsliderPos *= 40;
    return dsliderPos;
}


void DlgPrefEQ::slotApply()
{
#ifndef __LOFI__
    m_COTLoFreq.slotSet(m_lowEqFreq);
    m_COTHiFreq.slotSet(m_highEqFreq);
    m_COTLoFi.slotSet(CheckBoxLoFi->isChecked());
#endif
}

void DlgPrefEQ::slotUpdate()
{
    slotUpdateLoEQ();
    slotUpdateHiEQ();
    slotLoFiChanged();
}

int DlgPrefEQ::getEqFreq(int sliderVal)
{
    if(sliderVal == 480) {
        return 20050; //normalize maximum to match label
    } else {
        double dsliderVal = (double) sliderVal / 40;
        double result = (dsliderVal * dsliderVal * dsliderVal * dsliderVal);
        return (int) result;
    }
}

void DlgPrefEQ::validate_levels() {
    if (m_lowEqFreq == m_highEqFreq) {
        // magic numbers: 16 is the low, 20050 is the high
        if (m_lowEqFreq == 16) {
            ++m_highEqFreq;
        } else if (m_highEqFreq == 20050) {
            --m_lowEqFreq;
        } else {
            ++m_highEqFreq;
        }
    }
}
