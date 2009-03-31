/***************************************************************************
                          midiobject.h  -  description
                             -------------------
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

#ifndef MIDIOBJECT_H
#define MIDIOBJECT_H

#include <Q3PtrVector>
#include <QtCore>
#include "defs.h"
#include "configobject.h"
#include "midimessage.h" //MOC doesn't like incomplete type definitions in signals. :(

#ifdef __MIDISCRIPT__
class MidiScriptEngine;     // Forward declaration
#endif

class MidiMapping;     // Forward declaration
class ControlObject;
class QWidget;
class DlgPrefMidiBindings;
class DlgPrefMidiDevice;

// These enums are used in the decoding of the status message into voice categories
typedef enum {
    NOTE_OFF         = 0x80,
    NOTE_ON          = 0x90,
    AFTERTOUCH       = 0xA0,
    CTRL_CHANGE      = 0xB0,
    PROG_CHANGE      = 0xC0,
    CHANNEL_PRESSURE = 0xD0,
    PITCH_WHEEL      = 0xE0
} MidiCategory;


class MidiObject : public QThread
{
  Q_OBJECT

public:
    MidiObject();
    virtual ~MidiObject();
    void setMidiConfig(ConfigObject<ConfigValueMidi> *pMidiConfig);
    void reopen(QString device);
    virtual void devOpen(QString) = 0;
    virtual void updateDeviceList() {};
    virtual void devClose() = 0;
    /** Delete MIDIMapping & stop script engine */
    void shutdown();
    void add(ControlObject* c);
    void remove(ControlObject* c);
    /** Returns a list of available devices */
    virtual QStringList *getDeviceList();
    /** Returns the name of the current open device */
    QString getOpenDevice();
    /** Returns a list of available configurations. Takes as input the directory path
      * containing the configuration files */
    QStringList *getConfigList(QString path);

    // Stuff for sending messages to control the device
    Q_INVOKABLE void sendShortMsg(unsigned char status, unsigned char byte1, unsigned char byte2, QString device);
    virtual void sendShortMsg(unsigned int word);
    virtual void sendSysexMsg(unsigned char data[], unsigned int length);
    Q_INVOKABLE void sendSysexMsg(QList<int> data, unsigned int length);

    bool getMidiLearnStatus();

#ifdef __MIDISCRIPT__
    void restartScriptEngine();
    MidiScriptEngine *getMidiScriptEngine();
#endif

    MidiMapping *getMidiMapping();

public slots:
    void enableMidiLearn();
    void disableMidiLearn();
    void slotScriptEngineReady();

signals:
    void devicesChanged();
    void midiEvent(MidiMessage message);

protected:
#ifdef __MIDISCRIPT__
    void run();
#endif
    void stop();
    void receive(MidiCategory category, char channel, char control, char value, QString device);

    bool requestStop;
    bool m_bMidiLearn;

    int fd, count, size, no;
    Q3PtrVector<ControlObject> controlList;

    /** Name of the currently opened MIDI device. Ignore the other device strings.
        This is a temp hack to deal with the weak multiple device support in MidiObject-based
        classes. */
    QString m_deviceName;

    /** List of available midi devices */
    QStringList devices;
    /** Name of current open devices */
    QStringList openDevices;
    /** List of available midi configurations. Initialized upon call to getConfigList() */
    QStringList configs;

#ifdef __MIDISCRIPT__
    MidiScriptEngine *m_pScriptEngine;
    QMutex m_scriptEngineInitializedMutex;
    QWaitCondition m_scriptEngineInitializedCondition;
#endif

    /** Device MIDI mapping object */
    MidiMapping *m_pMidiMapping;    //FIXME: should this be a pointer, won't compile without the *
};

void abortRead(int);

#endif
