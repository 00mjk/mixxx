/***************************************************************************
                          dlgprefmidibindings.cpp  -  description
                             -------------------
    begin                : Sat Jun 21 2008
    copyright            : (C) 2008 by Tom Care
    email                : psyc0de@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <QtGui>
#include <QDebug>
#include "midiinputmappingtablemodel.h"
#include "midioutputmappingtablemodel.h"
#include "midichanneldelegate.h"
#include "midistatusdelegate.h"
#include "midinodelegate.h"
#include "midioptiondelegate.h"
#include "controlgroupdelegate.h"
#include "controlvaluedelegate.h"
#include "dlgprefmidibindings.h"
#include "widget/wwidget.h"
#include "configobject.h"
#include "midimapping.h"

#ifdef __MIDISCRIPT__
#include "script/midiscriptengine.h"
#endif

const QStringList options = (QStringList() << "Normal" << "Script-Binding" << "Invert" << "Rot64" << "Rot64Inv"
        << "Rot64Fast" << "Diff" << "Button" << "Switch" << "HercJog"
        << "Spread64" << "SelectKnob");

QStringList controKeyOptionChoices;

const QStringList outputTypeChoices = (QStringList() << "light");

DlgPrefMidiBindings::DlgPrefMidiBindings(QWidget *parent, MidiObject &midi, QString deviceName,
										 ConfigObject<ConfigValue> *pConfig) :
							QWidget(parent), Ui::DlgPrefMidiBindingsDlg(), m_rMidi(midi) {
    setupUi(this);
    m_pConfig = pConfig;
    m_deviceName = deviceName;

    m_pDlgMidiLearning = NULL;

    labelDeviceName->setText(m_deviceName);

    //Tell the input mapping table widget which data model it should be viewing
    //(note that m_pInputMappingTableView is defined in the .ui file!)
    m_pInputMappingTableView->setModel((QAbstractItemModel*)m_rMidi.getMidiMapping()->getMidiInputMappingTableModel());

    m_pInputMappingTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pInputMappingTableView->setSelectionMode(QAbstractItemView::ContiguousSelection); //The model won't like ExtendedSelection, probably.
    m_pInputMappingTableView->verticalHeader()->hide();

    //Set up "delete" as a shortcut key to remove a row for the MIDI input table.
    m_deleteMIDIInputRowAction = new QAction(m_pInputMappingTableView);
    /*m_deleteMIDIInputRowAction->setShortcut(QKeySequence::Delete);
    m_deleteMIDIInputRowAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(m_deleteMIDIInputRowAction, SIGNAL(triggered()), this, SLOT(slotRemoveInputBinding()));
    */
    //The above shortcut doesn't work yet, not quite sure why. -- Albert Feb 1 / 2009

    //Set up the cool item delegates for the input mapping table
    m_pMidiChannelDelegate = new MidiChannelDelegate();
    m_pMidiStatusDelegate = new MidiStatusDelegate();
    m_pMidiNoDelegate = new MidiNoDelegate();
    m_pMidiOptionDelegate = new MidiOptionDelegate();
    m_pControlGroupDelegate = new ControlGroupDelegate();
    m_pControlValueDelegate = new ControlValueDelegate();
    m_pInputMappingTableView->setItemDelegateForColumn(MIDIINPUTTABLEINDEX_MIDISTATUS, m_pMidiStatusDelegate);
    m_pInputMappingTableView->setItemDelegateForColumn(MIDIINPUTTABLEINDEX_MIDICHANNEL, m_pMidiChannelDelegate);
    m_pInputMappingTableView->setItemDelegateForColumn(MIDIINPUTTABLEINDEX_MIDINO, m_pMidiNoDelegate);
    m_pInputMappingTableView->setItemDelegateForColumn(MIDIINPUTTABLEINDEX_CONTROLOBJECTGROUP, m_pControlGroupDelegate);
    m_pInputMappingTableView->setItemDelegateForColumn(MIDIINPUTTABLEINDEX_CONTROLOBJECTVALUE, m_pControlValueDelegate);
    m_pInputMappingTableView->setItemDelegateForColumn(MIDIINPUTTABLEINDEX_MIDIOPTION, m_pMidiOptionDelegate);
    
    //Tell the output mapping table widget which data model it should be viewing 
    //(note that m_pOutputMappingTableView is defined in the .ui file!)
    m_pOutputMappingTableView->setModel((QAbstractItemModel*)m_rMidi.getMidiMapping()->getMidiOutputMappingTableModel());
    m_pOutputMappingTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_pOutputMappingTableView->setSelectionMode(QAbstractItemView::ContiguousSelection);
    m_pOutputMappingTableView->verticalHeader()->hide();

    //Set up the cool item delegates for the output mapping table
    m_pOutputMappingTableView->setItemDelegateForColumn(MIDIOUTPUTTABLEINDEX_MIDISTATUS, m_pMidiStatusDelegate);
    m_pOutputMappingTableView->setItemDelegateForColumn(MIDIOUTPUTTABLEINDEX_MIDICHANNEL, m_pMidiChannelDelegate);
    m_pOutputMappingTableView->setItemDelegateForColumn(MIDIOUTPUTTABLEINDEX_MIDINO, m_pMidiNoDelegate);
    //TODO: We need different delegates for the output table's CO group/value columns because we only list real input
    //      controls, and for output we'd want to list a different set with stuff like "VUMeter" and other output controls.
    //m_pOutputMappingTableView->setItemDelegateForColumn(MIDIOUTPUTTABLEINDEX_CONTROLOBJECTGROUP, m_pControlGroupDelegate);
    //m_pOutputMappingTableView->setItemDelegateForColumn(MIDIOUTPUTTABLEINDEX_CONTROLOBJECTVALUE, m_pControlValueDelegate);

    // Connect buttons to slots
    connect(btnExportXML, SIGNAL(clicked()), this, SLOT(slotExportXML()));

    //Input bindings
    connect(btnMidiLearnWizard, SIGNAL(clicked()), this, SLOT(slotShowMidiLearnDialog()));
    connect(btnClearAllInputBindings, SIGNAL(clicked()), this, SLOT(slotClearAllInputBindings()));
    connect(btnRemoveInputBinding, SIGNAL(clicked()), this, SLOT(slotRemoveInputBinding()));
    connect(btnAddInputBinding, SIGNAL(clicked()), this, SLOT(slotAddInputBinding()));

    //Output bindings
    connect(btnClearAllOutputBindings, SIGNAL(clicked()), this, SLOT(slotClearAllOutputBindings()));
    connect(btnRemoveOutputBinding, SIGNAL(clicked()), this, SLOT(slotRemoveOutputBinding()));
    connect(btnAddOutputBinding, SIGNAL(clicked()), this, SLOT(slotAddOutputBinding()));

    //Connect the activate button. One day this will be replaced with an "Enabled" checkbox.
    connect(btnActivateDevice, SIGNAL(clicked()), this, SLOT(slotEnableDevice()));
    
    connect(comboBoxPreset, SIGNAL(activated(const QString&)), this, SLOT(slotLoadMidiMapping(const QString&)));
    
    //Load the list of presets into the presets combobox.
    enumeratePresets();
}

DlgPrefMidiBindings::~DlgPrefMidiBindings() {
    //delete m_pMidiConfig;
    delete m_pMidiChannelDelegate;
    delete m_pMidiNoDelegate;
    delete m_pMidiStatusDelegate;

    delete m_deleteMIDIInputRowAction;
}

void DlgPrefMidiBindings::enumeratePresets()
{
    comboBoxPreset->clear();
    
    //Insert a dummy "..." item at the top to try to make it less confusing.
    //(For example, we don't want "Akai MPD24" showing up as the default item
    // when a user has their controller plugged in)
    comboBoxPreset->addItem("...");
    
    QString midiDirPath = m_pConfig->getConfigPath().append("midi/");
    QDirIterator it(midiDirPath, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        it.next(); //Advance iterator. We get the filename from the next line. (It's a bit weird.)
        QString curMapping = it.fileName();
        if (curMapping.endsWith(MIDI_MAPPING_EXTENSION)) //blah, thanks for nothing Qt
        {
            curMapping.chop(QString(MIDI_MAPPING_EXTENSION).length()); //chop off the .midi.xml
            comboBoxPreset->addItem(curMapping);
        }
    }
}

/* loadPreset(QString)
 * Asks MidiMapping to load a set of MIDI bindings from an XML file
 */
void DlgPrefMidiBindings::loadPreset(QString path) {
    m_rMidi.getMidiMapping()->loadPreset(path);
}


/* slotUpdate()
 * Called when the dialog is displayed.
 */
void DlgPrefMidiBindings::slotUpdate() {

    //Check if the device that this dialog is for is already enabled...
    if (m_rMidi.getOpenDevice() == m_deviceName)
    {
        btnActivateDevice->setEnabled(false); //Disable activate button
        toolBox->setEnabled(true); //Enable MIDI in/out toolbox.
        groupBoxPresets->setEnabled(true); //Enable presets group box.
    }
    else {
        btnActivateDevice->setEnabled(true); //Enable activate button
        toolBox->setEnabled(false); //Disable MIDI in/out toolbox.
        groupBoxPresets->setEnabled(false); //Disable presets group box.
    }
}

/* slotApply()
 * Called when the OK button is pressed.
 */
void DlgPrefMidiBindings::slotApply() {
    /* User has pressed OK, so write the controls to the DOM, reload the MIDI
     * bindings, and save the default XML file. */
    m_rMidi.getMidiMapping()->savePreset();   // use default bindings path
    m_rMidi.getMidiMapping()->applyPreset();
    m_rMidi.disableMidiLearn();
}

void DlgPrefMidiBindings::slotShowMidiLearnDialog() {
    //Note that DlgMidiLearning is set to delete itself on
    //close using the Qt::WA_DeleteOnClose attribute (so this "new" doesn't leak memory)
    m_pDlgMidiLearning = new DlgMidiLearning(this, m_rMidi.getMidiMapping());
    m_pDlgMidiLearning->show();
}

/* slotImportXML()
 * Prompts the user for an XML preset and loads it.
 */
void DlgPrefMidiBindings::slotLoadMidiMapping(const QString &name) {
    
    if (name == "...")
        return;
    
    //Ask for confirmation if the MIDI tables aren't empty...
    MidiMapping* mapping = m_rMidi.getMidiMapping();
    if (mapping->numInputMidiMessages() > 0 ||
        mapping->numOutputMixxxControls() > 0)
    {
         QMessageBox::StandardButton result = QMessageBox::question(this, 
                tr("Overwrite existing mapping?"), 
                tr("Are you sure you'd like to load the " + name + " mapping?\n"
                   "This will overwrite your existing MIDI mapping."),  
                   QMessageBox::Yes | QMessageBox::No);
                              
         if (result == QMessageBox::No) {
            //Select the "..." item again in the combobox.
            comboBoxPreset->setCurrentIndex(0);
            return;                     
         }
    }
    
    QString filename = m_pConfig->getConfigPath().append("midi/") + name + MIDI_MAPPING_EXTENSION;
    if (!filename.isNull()) {
        loadPreset(filename);
        m_rMidi.getMidiMapping()->applyPreset();
    }
    m_pInputMappingTableView->update();
    
    //Select the "..." item again in the combobox.
    comboBoxPreset->setCurrentIndex(0);
}

/* slotExportXML()
 * Prompts the user for an XML preset and saves it.
 */
void DlgPrefMidiBindings::slotExportXML() {
    QString fileName = QFileDialog::getSaveFileName(this,
            "Export Mixxx MIDI Bindings", m_pConfig->getConfigPath().append("midi/"),
            "Preset Files (*.midi.xml)");
    if (!fileName.isNull()) m_rMidi.getMidiMapping()->savePreset(fileName);
}

void DlgPrefMidiBindings::slotEnableDevice()
{
	//Just tell MidiObject to close the old device and open this device
	m_rMidi.devClose();
	m_rMidi.devOpen(m_deviceName);
	m_pConfig->set(ConfigKey("[Midi]","Device"), m_deviceName);
	btnActivateDevice->setEnabled(false);
	toolBox->setEnabled(true); //Enable MIDI in/out toolbox.
	groupBoxPresets->setEnabled(true); //Enable presets group box.
	
	//TODO: Should probably check if devOpen() actually succeeded.
}

void DlgPrefMidiBindings::slotAddInputBinding() 
{
    bool ok = true;
    QString controlGroup = QInputDialog::getItem(this, tr("Select Control Group"), tr("Select Control Group"), 
                                                ControlGroupDelegate::getControlGroups(), 0, false,  &ok);
    if (!ok) return;
    
    QStringList controlValues;
    if (controlGroup == CONTROLGROUP_CHANNEL1_STRING ||
        controlGroup == CONTROLGROUP_CHANNEL2_STRING) {
        controlValues = ControlValueDelegate::getChannelControlValues();
    }
    else if (controlGroup == CONTROLGROUP_MASTER_STRING)
    {
        controlValues = ControlValueDelegate::getMasterControlValues();
    }
    else if (controlGroup == CONTROLGROUP_PLAYLIST_STRING)
    {
        controlValues = ControlValueDelegate::getPlaylistControlValues();
    }
    else
    {
        qDebug() << "Unhandled ControlGroup in " << __FILE__;
    }
    
        
    QString controlValue = QInputDialog::getItem(this, tr("Select Control"), tr("Select Control"), 
                                                 controlValues, 0, false,  &ok);
    if (!ok) return;


    MixxxControl mixxxControl(controlGroup, controlValue);
    MidiMessage message;

    while (m_rMidi.getMidiMapping()->isMidiMessageMapped(message))
    {
        message.setMidiNo(message.getMidiNo() + 1);
    }
    m_rMidi.getMidiMapping()->setInputMidiMapping(message, mixxxControl);
}

void DlgPrefMidiBindings::slotRemoveInputBinding()
{
	QModelIndexList selectedIndices = m_pInputMappingTableView->selectionModel()->selectedRows();
	if (selectedIndices.size() > 0)
	{
		MidiInputMappingTableModel* tableModel = dynamic_cast<MidiInputMappingTableModel*>(m_pInputMappingTableView->model());
		if (tableModel) {
		
			QModelIndex curIndex;
			//The model indices are sorted so that we remove the rows from the table
            //in ascending order. This is necessary because if row A is above row B in
            //the table, and you remove row A, the model index for row B will change.
            //Sorting the indices first means we don't have to worry about this.
            qSort(selectedIndices);

            //Going through the model indices in descending order (see above comment for explanation).
			QListIterator<QModelIndex> it(selectedIndices);
			it.toBack();
			while (it.hasPrevious())
			{
				curIndex = it.previous();
				tableModel->removeRow(curIndex.row());
			}
		}
	}
}

void DlgPrefMidiBindings::slotClearAllInputBindings() {
    if (QMessageBox::warning(this, "Clear Input Bindings",
            "Are you sure you want to clear all bindings?",
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok)
        return;

    //Remove all the rows from the data model (ie. the MIDI mapping).
    MidiInputMappingTableModel* tableModel = dynamic_cast<MidiInputMappingTableModel*>(m_pInputMappingTableView->model());
    if (tableModel) {
        tableModel->removeRows(0, tableModel->rowCount());
    }
}


void DlgPrefMidiBindings::slotAddOutputBinding() {
    qDebug() << "STUB: DlgPrefMidiBindings::slotAddOutputBinding()";

    m_rMidi.getMidiMapping()->setOutputMidiMapping(MixxxControl(), MidiMessage());
}

void DlgPrefMidiBindings::slotRemoveOutputBinding()
{
	QModelIndexList selectedIndices = m_pOutputMappingTableView->selectionModel()->selectedRows();
	if (selectedIndices.size() > 0)
	{
		MidiOutputMappingTableModel* tableModel =
		                    dynamic_cast<MidiOutputMappingTableModel*>(m_pOutputMappingTableView->model());
		if (tableModel) {
			QModelIndex curIndex;
			//The model indices are sorted so that we remove the rows from the table
            //in ascending order. This is necessary because if row A is above row B in
            //the table, and you remove row A, the model index for row B will change.
            //Sorting the indices first means we don't have to worry about this.
            //qSort(selectedIndices);

            //Going through the model indices in descending order (see above comment for explanation).
			QListIterator<QModelIndex> it(selectedIndices);
			it.toBack();
			while (it.hasPrevious())
			{
				curIndex = it.previous();
				qDebug() << "Dlg: removing row" << curIndex.row();
				tableModel->removeRow(curIndex.row());
			}
		}
	}
}

void DlgPrefMidiBindings::slotClearAllOutputBindings() {
    if (QMessageBox::warning(this, "Clear Output Bindings",
            "Are you sure you want to clear all output bindings?",
            QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Ok)
        return;

    //Remove all the rows from the data model (ie. the MIDI mapping).
    MidiOutputMappingTableModel* tableModel = dynamic_cast<MidiOutputMappingTableModel*>(m_pOutputMappingTableView->model());
    if (tableModel) {
        tableModel->removeRows(0, tableModel->rowCount());
    }
}
