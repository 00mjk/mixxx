/*
 * midistatusdelegate.cpp
 *
 *  Created on: 1-Feb-2009
 *      Author: alb
 */

#include <QtCore>
#include <QtGui>
#include "midimessage.h"
#include "midistatusdelegate.h"

#define MIDISTATUS_STRING_NOTE tr("Note/Key")
#define MIDISTATUS_STRING_CTRL tr("CC")
#define MIDISTATUS_STRING_PITCH tr("Pitch CC")


MidiStatusDelegate::MidiStatusDelegate(QObject *parent)
         : QItemDelegate(parent)
{
}

void MidiStatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if (index.data().canConvert<int>()) {
        int status = index.data().value<int>();
        
        //Throw away the channel bits (low nibble).
        status &= 0xF0;

        if (option.state & QStyle::State_Selected)
            painter->fillRect(option.rect, option.palette.highlight());

        QString text;
        if (status == MIDI_STATUS_NOTE_ON) //These come from the MidiStatusByte enum (midimessage.h)
            text = MIDISTATUS_STRING_NOTE;
        else if (status == MIDI_STATUS_CC)
            text = MIDISTATUS_STRING_CTRL;
        else if (status == MIDI_STATUS_PITCH_BEND)
            text = MIDISTATUS_STRING_PITCH;
        else
            text = tr("Unknown");

        painter->drawText(option.rect, text, QTextOption(Qt::AlignCenter));
        //Note that Qt::AlignCenter does both vertical and horizontal alignment.
    } else {
        QItemDelegate::paint(painter, option, index);
    }
}

QWidget *MidiStatusDelegate::createEditor(QWidget *parent,
        const QStyleOptionViewItem &/* option */,
        const QModelIndex &/* index */) const
{
    QComboBox *editor = new QComboBox(parent);
    editor->addItem(MIDISTATUS_STRING_NOTE);
    editor->addItem(MIDISTATUS_STRING_CTRL);
    editor->addItem(MIDISTATUS_STRING_PITCH);

    return editor;
}

void MidiStatusDelegate::setEditorData(QWidget *editor,
                                    const QModelIndex &index) const
{
    int status = index.model()->data(index, Qt::EditRole).toInt();
    int comboIdx = 0;
    
    //Throw away the channel bits (low nibble).
    status &= 0xF0;    
    
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    switch (status)
    {
        case MIDI_STATUS_NOTE_ON:
            comboIdx = 0;
            break;
        case MIDI_STATUS_CC:
            comboIdx = 1;
            break;
        case MIDI_STATUS_PITCH_BEND:
            comboIdx = 2;
            break;
    }
    comboBox->setCurrentIndex(comboIdx);
}

void MidiStatusDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                    const QModelIndex &index) const
{
    int midiStatus = 0;
    QComboBox *comboBox = static_cast<QComboBox*>(editor);
    //comboBox->interpretText();
    //Get the text from the combobox and turn it into a MidiMessage integer.
    QString text = comboBox->currentText();
    if (text == MIDISTATUS_STRING_NOTE) //These come from the MidiStatusByte enum (midimessage.h)
        midiStatus = MIDI_STATUS_NOTE_ON;
    else if (text == MIDISTATUS_STRING_CTRL)
        midiStatus = MIDI_STATUS_CC;
    else if (text == MIDISTATUS_STRING_PITCH)
        midiStatus = MIDI_STATUS_PITCH_BEND;

    model->setData(index, midiStatus, Qt::EditRole);
}

void MidiStatusDelegate::updateEditorGeometry(QWidget *editor,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &/* index */) const
{
    editor->setGeometry(option.rect);
}
