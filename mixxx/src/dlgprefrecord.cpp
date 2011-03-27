/***************************************************************************
                          dlgprefrecord.cpp  -  description
                             -------------------
    begin                : Thu Jun 19 2007
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

#include <QtCore>
#include <QtGui>
#include "dlgprefrecord.h"
#include "recording/defs_recording.h"
#include "controlobject.h"
#include "controlobjectthreadmain.h"
#include "recording/encoder.h"

DlgPrefRecord::DlgPrefRecord(QWidget * parent, ConfigObject<ConfigValue> * _config) : QWidget(parent), Ui::DlgPrefRecordDlg()
{
    config = _config;
    confirmOverwrite = false;

    setupUi(this);

    recordControl = new ControlObjectThreadMain(ControlObject::getControl(ConfigKey("[Master]", "Record"))); //See RECORD_* #defines in defs_recording.h


    //Fill up encoding list
    comboBoxEncoding->addItem(ENCODING_WAVE);
#ifdef SF_FORMAT_FLAC
    comboBoxEncoding->addItem(ENCODING_FLAC);
#endif
    comboBoxEncoding->addItem(ENCODING_AIFF);
#ifdef __SHOUTCAST__
    comboBoxEncoding->addItem(ENCODING_MP3);
    comboBoxEncoding->addItem(ENCODING_OGG);
#endif

    int encodingIndex = comboBoxEncoding->findText(config->getValueString(ConfigKey("[Recording]","Encoding")));
    if (encodingIndex >= 0)
        comboBoxEncoding->setCurrentIndex(encodingIndex);
    else //Invalid, so set default and save
    {
        comboBoxEncoding->setCurrentIndex(0);
        config->set(ConfigKey(RECORDING_PREF_KEY, "Encoding"), ConfigValue(comboBoxEncoding->currentText()));
    }

    //Connections

    connect(comboBoxEncoding, SIGNAL(activated(int)),   this,   SLOT(slotRecordPathChange()));
    connect(SliderQuality,    SIGNAL(valueChanged(int)), this,  SLOT(slotSliderQuality()));
    connect(SliderQuality,    SIGNAL(sliderMoved(int)), this,   SLOT(slotSliderQuality()));
    connect(SliderQuality,    SIGNAL(sliderReleased()), this,   SLOT(slotSliderQuality()));

    slotApply();
    recordControl->slotSet(RECORD_OFF); //make sure a corrupt config file won't cause us to record constantly
}

void DlgPrefRecord::slotSliderQuality()
{
    updateTextQuality();
    QString encodingType = comboBoxEncoding->currentText();
    if (encodingType == "OGG")
    {
        config->set(ConfigKey(RECORDING_PREF_KEY, "OGG_Quality"), ConfigValue(SliderQuality->value()));
    }
    else if (encodingType == "MP3")
    {
        config->set(ConfigKey(RECORDING_PREF_KEY, "MP3_Quality"), ConfigValue(SliderQuality->value()));
    }
}

int DlgPrefRecord::getSliderQualityVal()
{

    /* Commented by Tobias Rafreider
     * We always use the bitrate to denote the quality since it is more common to the users
     */
    return Encoder::convertToBitrate(SliderQuality->value());

}

void DlgPrefRecord::updateTextQuality()
{
    int quality = getSliderQualityVal();
    QString encodingType = comboBoxEncoding->currentText();

    TextQuality->setText(QString( QString::number(quality) + tr("kbps")));


}

void DlgPrefRecord::slotEncoding()
{
    //set defaults
    groupBoxQuality->setEnabled(true);
    config->set(ConfigKey(RECORDING_PREF_KEY, "Encoding"), ConfigValue(comboBoxEncoding->currentText()));
    if (comboBoxEncoding->currentText() == ENCODING_WAVE ||
        comboBoxEncoding->currentText() == ENCODING_FLAC ||
        comboBoxEncoding->currentText() == ENCODING_AIFF)
    {
        groupBoxQuality->setEnabled(false);
    }
    else if (comboBoxEncoding->currentText() == ENCODING_OGG)
    {
        int value = config->getValueString(ConfigKey(RECORDING_PREF_KEY, "OGG_Quality")).toInt();
        //if value == 0 then a default value of 128kbps is proposed.
        if(!value)
            value = 6; // 128kbps

        SliderQuality->setValue(value);
    }
    else if (comboBoxEncoding->currentText() == ENCODING_MP3)
    {
        int value = config->getValueString(ConfigKey(RECORDING_PREF_KEY, "MP3_Quality")).toInt();
        //if value == 0 then a default value of 128kbps is proposed.
        if(!value)
            value = 6; // 128kbps

        SliderQuality->setValue(value);
    }
    else
        qDebug() << "Invalid recording encoding type in" << __FILE__ << "on line:" << __LINE__;
}

void DlgPrefRecord::setMetaData()
{
    config->set(ConfigKey(RECORDING_PREF_KEY, "Title"), ConfigValue(LineEditTitle->text()));
    config->set(ConfigKey(RECORDING_PREF_KEY, "Author"), ConfigValue(LineEditAuthor->text()));
    config->set(ConfigKey(RECORDING_PREF_KEY, "Album"), ConfigValue(LineEditAlbum->text()));
}

void DlgPrefRecord::loadMetaData()
{
    LineEditTitle->setText( config->getValueString(ConfigKey(RECORDING_PREF_KEY, "Title")));
    LineEditAuthor->setText( config->getValueString(ConfigKey(RECORDING_PREF_KEY, "Author")));
    LineEditAlbum->setText( config->getValueString(ConfigKey(RECORDING_PREF_KEY, "Album")));
}

DlgPrefRecord::~DlgPrefRecord()
{

}
void DlgPrefRecord::slotRecordPathChange()
{
    confirmOverwrite = false;
    slotApply();
}

//This function updates/refreshes the contents of this dialog
void DlgPrefRecord::slotUpdate()
{
    int encodingIndex = comboBoxEncoding->findText(config->getValueString(ConfigKey("[Recording]","Encoding")));
    if (encodingIndex >= 0)
        comboBoxEncoding->setCurrentIndex(encodingIndex);
    else //Invalid, so set default and save
    {
        comboBoxEncoding->setCurrentIndex(0);
        config->set(ConfigKey(RECORDING_PREF_KEY, "Encoding"), ConfigValue(comboBoxEncoding->currentText()));
    }
    loadMetaData();
}

void DlgPrefRecord::slotApply()
{
    setMetaData();

    slotEncoding();
}
