/***************************************************************************
                             midimapping.h
                           MIDI Mapping Class
                           -------------------
    begin                : Sat Jan 17 2009
    copyright            : (C) 2009 Sean M. Pappalardo
    email                : pegasus@c64.org

***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef MIDIMAPPING_H
#define MIDIMAPPING_H

#include "midiobject.h"
#include "midimessage.h"
#include "mixxxcontrol.h"
#include "midiinputmapping.h"
#include "midioutputmapping.h"
#include <QTableWidget>

#ifdef __MIDISCRIPT__
#include "script/midiscriptengine.h"
#endif

//Forward declarations
class MidiInputMappingTableModel;
class MidiOutputMappingTableModel;

#define BINDINGS_PATH QDir::homePath().append("/").append(SETTINGS_PATH).append("MixxxMIDIBindings.xml")
#define MIDI_MAPPING_EXTENSION ".midi.xml"

class MidiMapping : public QObject
{
    Q_OBJECT

    public:
    /** Constructor also loads & applies the default XML MIDI mapping file */
    MidiMapping(MidiObject &midi_object);
    ~MidiMapping();

    void loadInitialPreset();
    void loadPreset(QString path);
    void loadPreset(QDomElement root);

    MidiInputMapping* getInputMapping();

    void savePreset(QString path = BINDINGS_PATH);
    void applyPreset();
    void clearPreset();
    void buildDomElement();

    void addControl(QDomElement& control, QString device);
    void addOutput(QDomElement& output, QString device);
    void addMidiScriptInfo(QDomElement &scriptFile, QString device); //Sucks

    bool addInputControl(MidiStatusByte midiStatus, int midiNo, int midiChannel,
                         QString controlObjectGroup, QString controlObjectKey,
                         MidiOption midiOption);
    bool addInputControl(MidiMessage message, MixxxControl control);
    void removeInputMapping(MidiStatusByte midiStatus, int midiNo, int midiChannel);
    MidiInputMappingTableModel* getMidiInputMappingTableModel();
    MidiOutputMappingTableModel* getMidiOutputMappingTableModel();
    //MixxxControl* getInputMixxxControl(MidiMessage command);

    double ComputeValue(MidiOption midioption, double _prevmidivalue, double _newmidivalue);

    // MIDI Input Mapping Modifiers
    int numInputMidiMessages();
    bool isInputIndexValid(int index);
    bool isMidiMessageMapped(MidiMessage command);
    MidiMessage getInputMidiMessage(int index);
    MixxxControl getInputMixxxControl(int index);
    MixxxControl getInputMixxxControl(MidiMessage command);
    void setInputMidiMapping(MidiMessage command, MixxxControl control);
    void clearInputMidiMapping(int index);
    void clearInputMidiMapping(MidiMessage command);
    void clearInputMidiMapping(int index, int count);

    // MIDI Output Mapping Modifiers
    int numOutputMixxxControls();
    bool isOutputIndexValid(int index);
    bool isMixxxControlMapped(MixxxControl control);
    MixxxControl getOutputMixxxControl(int index);
    MidiMessage getOutputMidiMessage(int index);
    MidiMessage getOutputMidiMessage(MixxxControl control);
    void setOutputMidiMapping(MixxxControl control, MidiMessage command);
    void clearOutputMidiMapping(int index);
    void clearOutputMidiMapping(MixxxControl control);
    void clearOutputMidiMapping(int index, int count);

public slots:
    void finishMidiLearn(MidiMessage message);
    void beginMidiLearn(MixxxControl control);
    void cancelMidiLearn();

signals:
    void inputMappingChanged();
    void inputMappingChanged(int startIndex, int endIndex);
    void outputMappingChanged();
    void outputMappingChanged(int startIndex, int endIndex);
    void midiLearningStarted();
    void midiLearningFinished(MidiMessage);
    void midiLearningFinished();

private:
#ifdef __MIDISCRIPT__
    /** Adds a script file name and function prefix to the list to be loaded */
    void addScriptFile(QString filename, QString functionprefix);

    QList<QString> m_pScriptFileNames;
    QList<QString> m_pScriptFunctionPrefixes;
    MidiScriptEngine *m_pScriptEngine;
#endif
    QDomElement m_Bindings;
    MidiObject &m_rMidiObject;
    MidiInputMapping m_inputMapping;
    MidiOutputMapping m_outputMapping;
    MidiInputMappingTableModel* m_pMidiInputMappingTableModel;
    MidiOutputMappingTableModel* m_pMidiOutputMappingTableModel;
    MixxxControl m_controlToLearn;
};

#endif
