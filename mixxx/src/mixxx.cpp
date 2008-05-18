/***************************************************************************
                          mixxx.cpp  -  description
                             -------------------
    begin                : Mon Feb 18 09:48:17 CET 2002
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

#include <QtDebug>
#include <QtCore>
#include <QtGui>

#include "wknob.h"
#include "wslider.h"
#include "wpushbutton.h"
#include "woverview.h"
#include "mixxx.h"
#include "controlnull.h"
#include "readerextractwave.h"
#include "controlpotmeter.h"
#include "controlobjectthreadmain.h"
#include "reader.h"
#include "enginebuffer.h"
#include "enginevumeter.h"
#include "track.h"
#include "trackcollection.h"
#include "trackinfoobject.h"
#include "mixxxsocketserver.h"
#include "mixxxmenuplaylists.h"
#include "wavesummary.h"
#include "bpmdetector.h"
#include "log.h"
#include "dlgabout.h"

#include "playerproxy.h"
#include "soundmanager.h"
#include "defs_urls.h"
#include "defs_audiofiles.h"
#include "recording/defs_recording.h"

#ifdef __IPOD__
#include "wtracktableview.h"
#include "gpod/itdb.h"
#include "track.h"
#endif

#ifdef __C_METRICS__
#include <cmetrics.h>
#include "defs_mixxxcmetrics.h"
#endif


extern "C" void crashDlg()
{
    QMessageBox::critical(0, "Mixxx", "Mixxx has encountered a serious error and needs to close.");
}

MixxxApp::MixxxApp(QApplication * a, struct CmdlineArgs args, QSplashScreen * pSplash)
{
    app = a;

    #include ".mixxx_version.svn" // #define BUILD_REV = "<svn rev number>"
    QString buildRevision = "";
    #ifdef BUILD_REV
      buildRevision = BUILD_REV;
    #endif
    #include ".mixxx_flags.svn" // #define BUILD_FLAGS = "<flags>"
    QString buildFlags = "";
    #ifdef BUILD_FLAGS
      buildFlags = BUILD_FLAGS;
    #endif
    if (buildRevision.trimmed().length() > 0)
        if (buildFlags.trimmed().length() > 0) buildRevision = "(svn " + buildRevision + "; built on: " + __DATE__ + " @ " + __TIME__ + "; flags: " + buildFlags.trimmed() + ") ";
        else buildRevision = "(svn " + buildRevision + "; built on: " + __DATE__ + " @ " + __TIME__ + ") ";

    qDebug() << "Mixxx" << VERSION << buildRevision << "is starting...";
    setWindowTitle(tr("Mixxx " VERSION));
#ifdef __MACX__
    setWindowIcon(QIcon(":icon.svg"));
#else
    setWindowIcon(QIcon(":icon.svg"));
    //setWindowIcon(QIcon(":iconsmall.png")); //This is a smaller 16x16 icon, looks cleaner...
#endif

    //Reset pointer to players
    //player = 0;
    soundmanager = 0;
    m_pTrack = 0;
    prefDlg = 0;

    // Read the config file from home directory
    config = new ConfigObject<ConfigValue>(QDir::homePath().append("/").append(SETTINGS_FILE));
    QString qConfigPath = config->getConfigPath();

#ifdef __C_METRICS__
	//Initialize Case Metrics if User is OK with that
	int fuserAgreeToDataCollection = false;
	if(config->getValueString(ConfigKey("[User Experience]","AgreedToUserExperienceProgram")) == QString("yes"))
		fuserAgreeToDataCollection = true;
	else if(config->getValueString(ConfigKey("[User Experience]","AgreedToUserExperienceProgram")) != QString("no"))
	{
		while(1)
		{
			fuserAgreeToDataCollection = QMessageBox::question(this, "Mixxx", "Mixxx's development is driven by community feedback.  At your discretion, Mixxx can automatically send data on your user experience back to the developers. Would you like to help us make Mixxx better by enabling this feature?", "Yes", "No", "Privacy Policy", 0, -1);
			if(fuserAgreeToDataCollection == 0)
			{
				fuserAgreeToDataCollection = true;
				config->set(ConfigKey("[User Experience]","AgreedToUserExperienceProgram"), ConfigValue(QString("yes")));
				break;
			}
			else if(fuserAgreeToDataCollection == 1)
			{
				fuserAgreeToDataCollection = false;
				config->set(ConfigKey("[User Experience]","AgreedToUserExperienceProgram"), ConfigValue(QString("no")));
				break;
			}
			else
			{
				//show privacy policy
				QMessageBox::information(this, "Mixxx: Privacy Policy", "Mixxx's development is driven by community feedback.  In order to help improve future versions Mixxx will with your permission collect information on your hardware and usage of Mixxx.  This information will primarily be used to fix bugs, improve features, and determine the system requirements of later versions.  Additionally this information may be used in aggregate for statistical purposes.\n\nThe hardware information will include:\n\t- CPU model and features\n\t- Total/Available Amount of RAM\n\t- Available disk space\n\t- OS version\n\nYour usage information will include:\n\t- Settings/Preferences\n\t- Internal errors\n\t- Internal debugging messages\n\t- Performance statistics (average latency, CPU usage)\n\nThis information will not be used to personally identify you, contact you, advertise to you, or otherwise bother you in any way.\n");
			}
		}
	}
    //If the user agrees, attempt to load the user ID from the config file
    const char *pstzUID;
    bool fFreePstzUID = false;
    if(fuserAgreeToDataCollection)
    {
        if(config->getValueString(ConfigKey("[User Experience]", "UID")) == QString(""))
        {
            pstzUID = cm_generate_userid();
            fFreePstzUID = true;
            if(pstzUID != NULL)
            {
                config->set(ConfigKey("[User Experience]", "UID"), ConfigValue(QString(pstzUID)));
            }
        }
        else
        {
            pstzUID = config->getValueString(ConfigKey("[User Experience]", "UID")).ascii();
        }
    }

	cm_init(100,20, fuserAgreeToDataCollection, MIXXCMETRICS_RELEASE_ID, pstzUID);

    if(fFreePstzUID)
        free((void*)pstzUID);

    cm_set_crash_dlg(crashDlg);
	cm_writemsg_ascii(MIXXXCMETRICS_VERSION,
	                  VERSION);
#endif

    // Store the path in the config database
    config->set(ConfigKey("[Config]","Path"), ConfigValue(qConfigPath));

    // Instantiate a ControlObject, and set static parent widget
    control = new ControlNull();


    if (pSplash)
        pSplash->showMessage("Initializing control devices...",Qt::AlignLeft|Qt::AlignBottom);

    // Read keyboard configuration and set kdbConfig object in WWidget
    kbdconfig = new ConfigObject<ConfigValueKbd>(QString(qConfigPath).append("keyboard/").append("Standard.kbd.cfg"));
    WWidget::setKeyboardConfig(kbdconfig);


    if (pSplash)
        pSplash->showMessage("Setting up sound engine...",Qt::AlignLeft|Qt::AlignBottom);

    // Sample rate used by Player object
    ControlObject * sr = new ControlObject(ConfigKey("[Master]","samplerate"));
    sr->set(44100.);

    ControlObject * latency = new ControlObject(ConfigKey("[Master]","latency"));
    /* avoid unused warning*/
    latency = 0;

    // Master rate
    new ControlPotmeter(ConfigKey("[Master]","rate"),-1.,1.);

    // Init buffers/readers
    buffer1 = new EngineBuffer("[Channel1]", config);
    buffer2 = new EngineBuffer("[Channel2]", config);
    buffer1->setOtherEngineBuffer(buffer2);
    buffer2->setOtherEngineBuffer(buffer1);

    // Starting channels:
    channel1 = new EngineChannel("[Channel1]");
    channel2 = new EngineChannel("[Channel2]");

    // Starting the master (mixing of the channels and effects):
    master = new EngineMaster(config, buffer1, buffer2, channel1, channel2, "[Master]");

    // Initialize player device
    //Player::setMaster(master);
    //player = new PlayerProxy(config);

    soundmanager = new SoundManager(config, master);
    soundmanager->queryDevices();

    if (pSplash)
        pSplash->showMessage("Loading skin...",Qt::AlignLeft|Qt::AlignBottom);

    // Find path of skin
    QString qSkinPath = getSkinPath();

    // Get Music dir
    QDir dir(config->getValueString(ConfigKey("[Playlist]","Directory")));
    if ((config->getValueString(ConfigKey("[Playlist]","Directory")).length()<1) || (!dir.exists()))
    {
        QString fd = QFileDialog::getExistingDirectory(this, "Choose music library directory");
        if (fd != "")
        {
            config->set(ConfigKey("[Playlist]","Directory"), fd);
            config->Save();
        }
    }
    // Needed for Search class and Simple skin
    new ControlPotmeter(ConfigKey("[Channel1]","virtualplayposition"),0.,1.);

    // Needed for updating widgets with track duration info
    new ControlObject(ConfigKey("[Channel1]","duration"));
    new ControlObject(ConfigKey("[Channel2]","duration"));

    // Initialize widgets
    bool bVisualsWaveform = true;
    if (config->getValueString(ConfigKey("[Controls]","Visuals")).toInt()==1)
        bVisualsWaveform = false;

    // Use frame as container for view, needed for fullscreen display
    frame = new QFrame;
    setCentralWidget(frame);
    //move(10,10);
    // Call inits to invoke all other construction parts

    view=new MixxxView(frame, kbdconfig, bVisualsWaveform, qSkinPath, config);

    if (bVisualsWaveform && !view->activeWaveform())
    {
        config->set(ConfigKey("[Controls]","Visuals"), ConfigValue(1));
        QMessageBox * mb = new QMessageBox(this);
        mb->setWindowTitle(QString("Wavform displays"));
        mb->setIcon(QMessageBox::Information);
        mb->setText("OpenGL cannot be initialized, which means that\nthe waveform displays won't work. A simple\nmode will be used instead where you can still\nuse the mouse to change speed.");
        mb->show();
    }

    // Tell EngineBuffer to notify the visuals if they are WVisualWaveform
    if (view->activeWaveform())
    {
        buffer1->setVisual((WVisualWaveform *)view->m_pVisualCh1);
        buffer2->setVisual((WVisualWaveform *)view->m_pVisualCh2);

        // Dynamic zoom on visuals
        if (view->m_bZoom)
        {
            ControlObject::connectControls(ConfigKey("[Channel1]", "rate"), ConfigKey("[Channel1]", "VisualLengthScale-marks"));
            ControlObject::connectControls(ConfigKey("[Channel1]", "rate"), ConfigKey("[Channel1]", "VisualLengthScale-signal"));
            ControlObject::connectControls(ConfigKey("[Channel2]", "rate"), ConfigKey("[Channel2]", "VisualLengthScale-marks"));
            ControlObject::connectControls(ConfigKey("[Channel2]", "rate"), ConfigKey("[Channel2]", "VisualLengthScale-signal"));
        }
    }

    // Verify path for xml track file.
    QFile trackfile(config->getValueString(ConfigKey("[Playlist]","Listfile")));
    if ((config->getValueString(ConfigKey("[Playlist]","Listfile")).length()<1) || (!trackfile.exists()))
    {
        config->set(ConfigKey("[Playlist]","Listfile"), QDir::homePath().append("/").append(TRACK_FILE));
        config->Save();
    }

    // Initialize wavefrom summary generation
    m_pWaveSummary = new WaveSummary(config);

    // Intialize default BPM system values
    if(config->getValueString(ConfigKey("[BPM]","BPMRangeStart")).length()<1)
    {
        config->set(ConfigKey("[BPM]","BPMRangeStart"),ConfigValue(65));
    }

    if(config->getValueString(ConfigKey("[BPM]","BPMRangeEnd")).length()<1)
    {
        config->set(ConfigKey("[BPM]","BPMRangeEnd"),ConfigValue(135));
    }

    if(config->getValueString(ConfigKey("[BPM]","AnalyzeEntireSong")).length()<1)
    {
        config->set(ConfigKey("[BPM]","AnalyzeEntireSong"),ConfigValue(1));
    }

    // Initialize Bpm detection queue
    m_pBpmDetector = new BpmDetector(config);

    if (pSplash)
        pSplash->showMessage("Loading song database...",Qt::AlignLeft|Qt::AlignBottom);

    // Initialize track object:
    m_pTrack = new Track(config->getValueString(ConfigKey("[Playlist]","Listfile")), view, config, buffer1, buffer2, m_pWaveSummary, m_pBpmDetector);

    //WTreeItem::setTrack(m_pTrack);
    // Set up drag and drop to player visuals
    if (view->m_pVisualCh1)
        connect(view->m_pVisualCh1, SIGNAL(trackDropped(QString)), m_pTrack, SLOT(slotLoadPlayer1(QString)));
    if (view->m_pVisualCh2)
        connect(view->m_pVisualCh2, SIGNAL(trackDropped(QString)), m_pTrack, SLOT(slotLoadPlayer2(QString)));

    // Ensure that visual receive updates when new tracks are loaded
    if (view->m_pVisualCh1)
        connect(m_pTrack, SIGNAL(newTrackPlayer1(TrackInfoObject *)), view->m_pVisualCh1, SLOT(slotNewTrack()));
    if (view->m_pVisualCh2)
        connect(m_pTrack, SIGNAL(newTrackPlayer2(TrackInfoObject *)), view->m_pVisualCh2, SLOT(slotNewTrack()));



    // Setup state of End of track controls from config database
    ControlObject::getControl(ConfigKey("[Channel1]","TrackEndMode"))->queueFromThread(config->getValueString(ConfigKey("[Controls]","TrackEndModeCh1")).toDouble());
    ControlObject::getControl(ConfigKey("[Channel2]","TrackEndMode"))->queueFromThread(config->getValueString(ConfigKey("[Controls]","TrackEndModeCh2")).toDouble());

    // Initialize preference dialog
    prefDlg = new DlgPreferences(this, view, soundmanager, m_pTrack, config);
    prefDlg->setHidden(true);

#ifdef __LADSPA__
    ladspaDlg = new DlgLADSPA(this);
    ladspaDlg->setHidden(true);
#endif

    // Try open player device If that fails, the preference panel is opened.
    //if (!player->open())
    //    prefDlg->setHidden(false);

    if (soundmanager->setupDevices() != 0)
    {

#ifdef __C_METRICS__
	    cm_writemsg_ascii(MIXXXCMETRICS_FAILED_TO_OPEN_SNDDEVICE_AT_STARTUP,
	                      "Mixxx failed to open audio device(s) on startup.");
#endif

        QMessageBox::warning(this, tr("Mixxx"),
                                   tr("Failed to open your audio device(s).\n"
                                      "Please verify your selection in the preferences."),
                                   QMessageBox::Ok,
                                   QMessageBox::Ok);
         prefDlg->show();
    }

    //setFocusPolicy(QWidget::StrongFocus);
    //grabKeyboard();

    // Load tracks in args.qlMusicFiles (command line arguments) into player 1 and 2:
#ifndef QT3_SUPPORT
    if (args.qlMusicFiles.count()>1)
        m_pTrack->slotLoadPlayer1((*args.qlMusicFiles.at(1)));
    if (args.qlMusicFiles.count()>2)
        m_pTrack->slotLoadPlayer2((*args.qlMusicFiles.at(2)));
#else
    if (args.qlMusicFiles.count()>1)
        m_pTrack->slotLoadPlayer1(args.qlMusicFiles.at(1));
    if (args.qlMusicFiles.count()>2)
        m_pTrack->slotLoadPlayer2(args.qlMusicFiles.at(2));
#endif

    // Set up socket interface
//#ifndef __WIN__
//    new MixxxSocketServer(m_pTrack);
//#endif

    // Initialize visualization of temporal effects
    channel1->setVisual(buffer1);
    channel2->setVisual(buffer2);

    // Dynamic scaling of temporal effect curves
    if (view->m_bZoom)
    {
        ControlObject::connectControls(ConfigKey("[Channel1]", "rate"), ConfigKey("[Channel1]", "VisualLengthScale-temporal"));
        ControlObject::connectControls(ConfigKey("[Channel2]", "rate"), ConfigKey("[Channel2]", "VisualLengthScale-temporal"));
    }

#ifdef __SCRIPT__
    scriptEng = new ScriptEngine(this, m_pTrack);
#endif

    initActions();
    initMenuBar();

    // Check direct rendering
    if (bVisualsWaveform)
        view->checkDirectRendering();

    //Install an event filter to catch certain QT events, such as tooltips.
    //This allows us to turn off tooltips.
    app->installEventFilter(this); //The eventfilter is located in this Mixxx class as a callback.

    // Initialize the log if a log file name was given on the command line
    Log *pLog = 0;
    if (args.qLogFileName.length()>0)
        pLog = new Log(args.qLogFileName, m_pTrack);

    //If we were told to start in fullscreen mode on the command-line, then turn on fullscreen mode.
    if (args.bStartInFullscreen)
        slotOptionsFullScreen(true);
#ifdef __C_METRICS__
	cm_writemsg_ascii(MIXXXCMETRICS_MIXXX_CONSTRUCTOR_COMPLETE, "Mixxx constructor complete.");
#endif
}

MixxxApp::~MixxxApp()
{
    QTime qTime;
    qTime.start();

    qDebug() << "Destroying MixxxApp";

// Moved this up to insulate macros you've worked hard on from being lost in
// a segfault that happens sometimes somewhere below here
#ifdef __SCRIPT__
    scriptEng->saveMacros();
    delete scriptEng;
#endif

#ifdef __IPOD__
    if (m_pTrack->m_qIPodPlaylist.getSongNum()) {
      qDebug() << "Dispose of iPod track collection";
      m_pTrack->m_qIPodPlaylist.clear();
    }
#endif

    qDebug() << "Write track xml, " << qTime.elapsed();
    m_pTrack->writeXML(config->getValueString(ConfigKey("[Playlist]","Listfile")));

    //qDebug() << "close player, " << qTime.elapsed();
    //player->close();
    //qDebug() << "player->close() done";

    qDebug() << "close soundmanager" << qTime.elapsed();
    soundmanager->closeDevices();
    qDebug() << "soundmanager->close() done";

    // Save state of End of track controls in config database
    config->set(ConfigKey("[Controls]","TrackEndModeCh1"), ConfigValue((int)ControlObject::getControl(ConfigKey("[Channel1]","TrackEndMode"))->get()));
    config->set(ConfigKey("[Controls]","TrackEndModeCh2"), ConfigValue((int)ControlObject::getControl(ConfigKey("[Channel2]","TrackEndMode"))->get()));

    //qDebug() << "delete player, " << qTime.elapsed();
    //delete player;
    qDebug() << "delete soundmanager, " << qTime.elapsed();
    delete soundmanager;
    qDebug() << "delete master, " << qTime.elapsed();
    delete master;
    qDebug() << "delete channel1, " << qTime.elapsed();
    delete channel1;
    qDebug() << "delete channel2, " << qTime.elapsed();
    delete channel2;

    qDebug() << "delete buffer1, " << qTime.elapsed();
    delete buffer1;
    qDebug() << "delete buffer2, " << qTime.elapsed();
    delete buffer2;

//    qDebug() << "delete prefDlg";
//    delete m_pControlEngine;
//    qDebug() << "delete midi";
//    qDebug() << "delete midiconfig";

    qDebug() << "delete view, " << qTime.elapsed();
    delete view;

    qDebug() << "delete tracks, " << qTime.elapsed();
    delete m_pTrack;

    delete prefDlg;

    //   delete m_pBpmDetector;
    //   delete m_pWaveSummary;

    qDebug() << "save config, " << qTime.elapsed();
    config->Save();
    qDebug() << "delete config, " << qTime.elapsed();
    delete config;

    delete frame;
#ifdef __C_METRICS__
	cm_writemsg_ascii(MIXXXCMETRICS_MIXXX_DESTRUCTOR_COMPLETE, "Mixxx deconstructor complete.");
	cm_close(10);
#endif
#ifdef __WIN__
    _exit(0);
#endif
}

/** initializes all QActions of the application */
void MixxxApp::initActions()
{
    fileLoadSongPlayer1 = new QAction(tr("&Load Song (Player 1)..."), this);
    fileLoadSongPlayer1->setShortcut(tr("Ctrl+O"));
    fileLoadSongPlayer1->setShortcutContext(Qt::ApplicationShortcut);

    fileLoadSongPlayer2 = new QAction(tr("&Load Song (Player 2)..."), this);
    fileLoadSongPlayer2->setShortcut(tr("Ctrl+Shift+O"));
    fileLoadSongPlayer2->setShortcutContext(Qt::ApplicationShortcut);

    fileQuit = new QAction(tr("E&xit"), this);
    fileQuit->setShortcut(tr("Ctrl+Q"));
    fileQuit->setShortcutContext(Qt::ApplicationShortcut);

    libraryRescan = new QAction(tr("&Rescan Library"), this);

    playlistsNew = new QAction(tr("Add &new playlist"), this);
    playlistsNew->setShortcut(tr("Ctrl+N"));
    playlistsNew->setShortcutContext(Qt::ApplicationShortcut);

    playlistsImport = new QAction(tr("&Import playlist"), this);
    playlistsImport->setShortcut(tr("Ctrl+I"));
    playlistsImport->setShortcutContext(Qt::ApplicationShortcut);

#ifdef __IPOD__
    iPodToggle = new QAction(tr("iPod &Active"), this);
    iPodToggle->setShortcut(tr("Ctrl+A"));
    iPodToggle->setShortcutContext(Qt::ApplicationShortcut);
    iPodToggle->setCheckable(true);
    connect(iPodToggle, SIGNAL(toggled(bool)), this, SLOT(slotiPodToggle(bool)));
#endif

    optionsBeatMark = new QAction(tr("&Audio Beat Marks"), this);

    optionsFullScreen = new QAction(tr("&Full Screen"), this);
    optionsFullScreen->setShortcut(tr("Esc"));
    optionsFullScreen->setShortcutContext(Qt::ApplicationShortcut);
    //QShortcut * shortcut = new QShortcut(QKeySequence(tr("Esc")),  this);
    //    connect(shortcut, SIGNAL(activated()), this, SLOT(slotQuitFullScreen()));

    optionsPreferences = new QAction(tr("&Preferences"), this);
    optionsPreferences->setShortcut(tr("Ctrl+P"));
    optionsPreferences->setShortcutContext(Qt::ApplicationShortcut);

    helpAboutApp = new QAction(tr("&About..."), this);
    helpSupport = new QAction(tr("&Community Support..."), this);
#ifdef __VINYLCONTROL__
    optionsVinylControl = new QAction(tr("Enable &Vinyl Control"), this);
    optionsVinylControl->setShortcut(tr("Ctrl+Y"));
    optionsVinylControl->setShortcutContext(Qt::ApplicationShortcut);
#endif

    optionsRecord = new QAction(tr("&Record Mix"), this);
    //optionsRecord->setShortcut(tr("Ctrl+R"));
    optionsRecord->setShortcutContext(Qt::ApplicationShortcut);

#ifdef __SCRIPT__
    macroStudio = new QAction(tr("Show Studio"), this);
#endif
#ifdef __LADSPA__
    ladspaShow = new QAction(tr("Show LADSPA window"), this);
#endif


    fileLoadSongPlayer1->setStatusTip(tr("Opens a song in player 1"));
    fileLoadSongPlayer1->setWhatsThis(tr("Open\n\nOpens a song in player 1"));
    connect(fileLoadSongPlayer1, SIGNAL(activated()), this, SLOT(slotFileLoadSongPlayer1()));

    fileLoadSongPlayer2->setStatusTip(tr("Opens a song in player 2"));
    fileLoadSongPlayer2->setWhatsThis(tr("Open\n\nOpens a song in player 2"));
    connect(fileLoadSongPlayer2, SIGNAL(activated()), this, SLOT(slotFileLoadSongPlayer2()));

    fileQuit->setStatusTip(tr("Quits the application"));
    fileQuit->setWhatsThis(tr("Exit\n\nQuits the application"));
    connect(fileQuit, SIGNAL(activated()), this, SLOT(slotFileQuit()));

    libraryRescan->setStatusTip(tr("Rescans the song library"));
    libraryRescan->setWhatsThis(tr("Rescan library\n\nRescans the song library"));
    libraryRescan->setCheckable(false);
    connect(libraryRescan, SIGNAL(activated()), m_pTrack, SLOT(slotScanLibrary()));

    playlistsNew->setStatusTip(tr("Create a new playlist"));
    playlistsNew->setWhatsThis(tr("New playlist\n\nCreate a new playlist"));
    connect(playlistsNew, SIGNAL(activated()), m_pTrack, SLOT(slotNewPlaylist()));

    playlistsImport->setStatusTip(tr("Import playlist"));
    playlistsImport->setWhatsThis(tr("Import playlist"));
    connect(playlistsImport, SIGNAL(activated()), m_pTrack, SLOT(slotImportPlaylist()));

    optionsBeatMark->setCheckable(false);
    optionsBeatMark->setChecked(false);
    optionsBeatMark->setStatusTip(tr("Audio Beat Marks"));
    optionsBeatMark->setWhatsThis(tr("Audio Beat Marks\nMark beats by audio clicks"));
    connect(optionsBeatMark, SIGNAL(toggled(bool)), this, SLOT(slotOptionsBeatMark(bool)));

#ifdef __VINYLCONTROL__
    //Either check or uncheck the vinyl control menu item depending on what it was saved as.
    optionsVinylControl->setCheckable(true);
    if ((bool)config->getValueString(ConfigKey("[VinylControl]","Enabled")).toInt() == true)
        optionsVinylControl->setChecked(true);
    else
        optionsVinylControl->setChecked(false);
    optionsVinylControl->setStatusTip(tr("Activate Vinyl Control"));
    optionsVinylControl->setWhatsThis(tr("Use timecoded vinyls on external turntables to control Mixxx"));
    connect(optionsVinylControl, SIGNAL(toggled(bool)), this, SLOT(slotOptionsVinylControl(bool)));
#endif

    optionsRecord->setCheckable(true);
    optionsRecord->setStatusTip(tr("Start Recording your Mix"));
    optionsRecord->setWhatsThis(tr("Record your mix to a file"));
    connect(optionsRecord, SIGNAL(toggled(bool)), this, SLOT(slotOptionsRecord(bool)));

    optionsFullScreen->setCheckable(true);
    optionsFullScreen->setChecked(false);
    optionsFullScreen->setStatusTip(tr("Full Screen"));
    optionsFullScreen->setWhatsThis(tr("Display Mixxx using the full screen"));
    connect(optionsFullScreen, SIGNAL(toggled(bool)), this, SLOT(slotOptionsFullScreen(bool)));

    optionsPreferences->setStatusTip(tr("Preferences"));
    optionsPreferences->setWhatsThis(tr("Preferences\nPlayback and MIDI preferences"));
    connect(optionsPreferences, SIGNAL(activated()), this, SLOT(slotOptionsPreferences()));

    helpSupport->setStatusTip(tr("Support..."));
    helpSupport->setWhatsThis(tr("Support\n\nGet help with Mixxx"));
    connect(helpSupport, SIGNAL(activated()), this, SLOT(slotHelpSupport()));

    helpAboutApp->setStatusTip(tr("About the application"));
    helpAboutApp->setWhatsThis(tr("About\n\nAbout the application"));
    connect(helpAboutApp, SIGNAL(activated()), this, SLOT(slotHelpAbout()));

#ifdef __SCRIPT__
    macroStudio->setStatusTip(tr("Shows the macro studio window"));
    macroStudio->setWhatsThis(tr("Show Studio\n\nMakes the macro studio visible"));
     connect(macroStudio, SIGNAL(activated()), scriptEng->getStudio(), SLOT(showStudio()));
#endif
#ifdef __LADSPA__
    ladspaShow->setStatusTip(tr("Shows the LADSPA window"));
    ladspaShow->setWhatsThis(tr("Show LADSPA window\n\nMakes the LADSPA window visible"));
    connect(ladspaShow, SIGNAL(activated()), this, SLOT(slotLadspa()));
#endif
}

void MixxxApp::initMenuBar()
{
    // MENUBAR
    fileMenu=new QMenu("&File");
    optionsMenu=new QMenu("&Options");
    libraryMenu=new QMenu("&Library");
    viewMenu=new QMenu("&View");
    helpMenu=new QMenu("&Help");
#ifdef __SCRIPT__
    macroMenu=new QMenu("&Macro");
#endif
#ifdef __LADSPA__
    ladspaMenu=new QMenu("LADSPA");
#endif

    // menuBar entry fileMenu
    fileMenu->addAction(fileLoadSongPlayer1);
    fileMenu->addAction(fileLoadSongPlayer2);
    fileMenu->addSeparator();
    fileMenu->addAction(fileQuit);

    // menuBar entry optionsMenu
    //optionsMenu->setCheckable(true);
    //  optionsBeatMark->addTo(optionsMenu);
#ifdef __VINYLCONTROL__
    optionsMenu->addAction(optionsVinylControl);
#endif
    optionsMenu->addAction(optionsRecord);
    optionsMenu->addAction(optionsFullScreen);
    optionsMenu->addSeparator();
    optionsMenu->addAction(optionsPreferences);

    //    libraryMenu->setCheckable(true);
    libraryMenu->addAction(libraryRescan);
    libraryMenu->addSeparator();
    libraryMenu->addAction(playlistsNew);
    libraryMenu->addAction(playlistsImport);

#ifdef __IPOD__
    libraryMenu->addSeparator();
    libraryMenu->addAction(iPodToggle);
    connect(libraryMenu, SIGNAL(aboutToShow()), this, SLOT(slotlibraryMenuAboutToShow()));
#endif

    // menuBar entry viewMenu
    //viewMenu->setCheckable(true);

    // menuBar entry helpMenu
    helpMenu->addAction(helpSupport);
    helpMenu->addSeparator();
    helpMenu->addAction(helpAboutApp);


#ifdef __SCRIPT__
    macroMenu->addAction(macroStudio);
#endif
#ifdef __LADSPA__
    ladspaMenu->addAction(ladspaShow);
#endif

    menuBar()->addMenu(fileMenu);
    menuBar()->addMenu(libraryMenu);
    menuBar()->addMenu(optionsMenu);

    //    menuBar()->addMenu(viewMenu);
#ifdef __SCRIPT__
    menuBar()->addMenu(macroMenu);
#endif
#ifdef __LADSPA__
    menuBar()->addMenu(ladspaMenu);
#endif
    menuBar()->addSeparator();
    menuBar()->addMenu(helpMenu);
    //new MixxxMenuPlaylists(libraryMenu, m_pTrack);

}


void MixxxApp::slotiPodToggle(bool toggle) {
#ifdef __IPOD__
// iPod stuff
  QString iPodMountPoint = config->getValueString(ConfigKey("[iPod]","MountPoint"));
  bool iPodAvailable = !iPodMountPoint.isEmpty() &&
                       QDir( iPodMountPoint + "/iPod_Control").exists();
  bool iPodActivated = iPodAvailable && toggle;

  iPodToggle->setEnabled(iPodAvailable);

  if (iPodAvailable && iPodActivated && view->m_pComboBox->findData(TABLE_MODE_IPOD) == -1 ) {
    view->m_pComboBox->addItem( "iPod", TABLE_MODE_IPOD );
    // Activate IPod model

    Itdb_iTunesDB *itdb;
    itdb = itdb_parse (iPodMountPoint, NULL);
    if (itdb == NULL) {
      qDebug() << "Error reading iPod database\n";
      return;
    }
    GList *it;
    int count = 0;
    m_pTrack->m_qIPodPlaylist.clear();

    for (it = itdb->tracks; it != NULL; it = it->next) {
       count++;
       Itdb_Track *song;
       song = (Itdb_Track *)it->data;

       if (song->movie_flag) { qDebug() << "Movies/Videos not supported." << song->title << QString(song->ipod_path).replace(':','/'); continue; }
       if (song->unk220) { qDebug() << "Protected media not supported." << song->title << QString(song->ipod_path).replace(':','/'); continue; }

       QFileInfo file(iPodMountPoint + QString(song->ipod_path).replace(':','/'));
       TrackInfoObject* pTrack = new TrackInfoObject(file.absolutePath(), file.fileName(), m_pBpmDetector );
       pTrack->setBpm(song->BPM);
       pTrack->setBpmConfirm(song->BPM != 0);  //    void setBeatFirst(float); ??
//       pTrack->setHeaderParsed(true);
       pTrack->setComment(song->comment);
       pTrack->setType(file.suffix());
       pTrack->setBitrate(song->bitrate);
       pTrack->setSampleRate(song->samplerate);
       pTrack->setDuration(song->tracklen/1000);
       pTrack->setTitle(song->title);
       pTrack->setArtist(song->artist);
       // song->rating // user rating
       // song->volume and song->soundcheck -- track level normalization / gain info as determined by iTunes
       m_pTrack->m_qIPodPlaylist.addTrack(pTrack);
    }
    itdb_free (itdb);

    qDebug() << "iPod playlist has" << m_pTrack->m_qIPodPlaylist.getSongNum() << "of"<< count <<"songs on the iPod.";

    view->m_pComboBox->setCurrentIndex( view->m_pComboBox->findData(TABLE_MODE_IPOD) );
    m_pTrack->slotActivatePlaylist( view->m_pComboBox->findData(TABLE_MODE_IPOD) );
    m_pTrack->resizeColumnsForLibraryMode();

  } else if (view->m_pComboBox->findData(TABLE_MODE_IPOD) != -1 ) {
    view->m_pComboBox->setCurrentIndex( view->m_pComboBox->findData(TABLE_MODE_LIBRARY) );
    m_pTrack->slotActivatePlaylist( view->m_pComboBox->findData(TABLE_MODE_LIBRARY) );

    view->m_pComboBox->removeItem( view->m_pComboBox->findData(TABLE_MODE_IPOD) );
    // Empty iPod model m_qIPodPlaylist
    m_pTrack->m_qIPodPlaylist.clear();

  }
#endif
}


void MixxxApp::slotlibraryMenuAboutToShow(){

#ifdef __IPOD__
  QString iPodMountPoint = config->getValueString(ConfigKey("[iPod]","MountPoint"));
  bool iPodAvailable = !iPodMountPoint.isEmpty() &&
                       QDir( iPodMountPoint + "/iPod_Control").exists();
  iPodToggle->setEnabled(iPodAvailable);

#endif
}

bool MixxxApp::queryExit()
{
    int exit=QMessageBox::information(this, tr("Quit..."),
                                      tr("Do your really want to quit?"),
                                      QMessageBox::Ok, QMessageBox::Cancel);

    if (exit==1)
    {
    }
    else
    {
    };

    return (exit==1);
}

void MixxxApp::slotFileLoadSongPlayer1()
{
    ControlObject* play = ControlObject::getControl(ConfigKey("[Channel1]", "play"));

    if (play->get() == 1.)
    {
        int ret = QMessageBox::warning(this, tr("Mixxx"),
                                        tr("Player 1 is currently playing a song.\n"
                                          "Are you sure you want to load a new song?"),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);

        if (ret != QMessageBox::Yes)
            return;
    }

    QString s = QFileDialog::getOpenFileName(this, tr("Load Song into Player 1"), config->getValueString(ConfigKey("[Playlist]","Directory")), QString("Audio (%1)").arg(MIXXX_SUPPORTED_AUDIO_FILETYPES));
    if (!(s == QString::null)) {
        TrackInfoObject * pTrack = m_pTrack->getTrackCollection()->getTrack(s);
        if (pTrack)
            m_pTrack->slotLoadPlayer1(pTrack);
    }
}

void MixxxApp::slotFileLoadSongPlayer2()
{
    ControlObject* play = ControlObject::getControl(ConfigKey("[Channel2]", "play"));

    if (play->get() == 1.)
    {
        int ret = QMessageBox::warning(this, tr("Mixxx"),
                                        tr("Player 2 is currently playing a song.\n"
                                          "Are you sure you want to load a new song?"),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::No);

        if (ret != QMessageBox::Yes)
            return;
    }

    QString s = QFileDialog::getOpenFileName(this, tr("Load Song into Player 2"), config->getValueString(ConfigKey("[Playlist]","Directory")), QString("Audio (%1)").arg(MIXXX_SUPPORTED_AUDIO_FILETYPES));
    if (!(s == QString::null)) {
        TrackInfoObject * pTrack = m_pTrack->getTrackCollection()->getTrack(s);
        if (pTrack)
            m_pTrack->slotLoadPlayer2(pTrack);
    }
}

void MixxxApp::slotFileQuit()
{
    qApp->quit();
}

void MixxxApp::slotOptionsBeatMark(bool)
{
// BEAT MARK STUFF
}

void MixxxApp::slotOptionsFullScreen(bool toggle)
{
    // Making a fullscreen window on linux and windows is harder than you could possibly imagine...
    if (toggle)
    {
#ifdef __LINUX__
         winpos = pos();
         // Can't set max to -1,-1 or 0,0 for unbounded?
         setMaximumSize(32767,32767);
#endif

        showFullScreen();
        //menuBar()->hide();
        // FWI: Begin of fullscreen patch
#ifdef __LINUX__
        // Crazy X window managers break this so I'm told by Qt docs
        //         int deskw = app->desktop()->width();
        //         int deskh = app->desktop()->height();

        //support for xinerama
        int deskw = app->desktop()->screenGeometry(frame).width();
        int deskh = app->desktop()->screenGeometry(frame).height();
#else
        int deskw = width();
        int deskh = height();
#endif
        view->move((deskw - view->width())/2, (deskh - view->height())/2);
        // FWI: End of fullscreen patch
    }
    else
    {
        // FWI: Begin of fullscreen patch
        view->move(0,0);
        menuBar()->show();
        showNormal();

#ifdef __LINUX__
        if (size().width() != view->width() ||
            size().height() != view->height() + menuBar()->height()) {
          setFixedSize(view->width(), view->height() + menuBar()->height());
        }
        move(winpos);
#endif

        // FWI: End of fullscreen patch
    }
}

void MixxxApp::slotOptionsPreferences()
{
    prefDlg->setHidden(false);
}

//Note: Can't #ifdef this because MOC doesn't catch it.
void MixxxApp::slotOptionsVinylControl(bool toggle)
{
#ifdef __VINYLCONTROL__
    //qDebug() << "slotOptionsVinylControl: toggle is " << (int)toggle;

    QString device1 = config->getValueString(ConfigKey("[VinylControl]","DeviceInputDeck1"));
    QString device2 = config->getValueString(ConfigKey("[VinylControl]","DeviceInputDeck2"));

    if (device1 == "" && device2 == "" && (toggle==true))
    {
        QMessageBox::warning(this, tr("Mixxx"),
                                   tr("No input device(s) select.\n"
                                      "Please select your soundcard(s) in vinyl control preferences."),
                                   QMessageBox::Ok,
                                   QMessageBox::Ok);
        prefDlg->show();
        prefDlg->showVinylControlPage();
        optionsVinylControl->setChecked(false);
    }
    else
    {
        config->set(ConfigKey("[VinylControl]","Enabled"), ConfigValue((int)toggle));
        ControlObject::getControl(ConfigKey("[VinylControl]", "Enabled"))->set((int)toggle);
    }
#endif
}

//Also can't ifdef this (MOC again)
void MixxxApp::slotOptionsRecord(bool toggle)
{
    ControlObjectThreadMain *recordingControl = new ControlObjectThreadMain(ControlObject::getControl(ConfigKey("[Master]", "Record")));
    QString recordPath = config->getValueString(ConfigKey("[Recording]","Path"));
    QString encodingType = config->getValueString(ConfigKey("[Recording]","Encoding"));
    QString encodingFileFilter = QString("Audio (*.%1)").arg(encodingType);
    bool proceedWithRecording = true;

    if (toggle == true)
    {
        //If there was no recording path set,
        if (recordPath == "")
        {
            QString selectedFile = QFileDialog::getSaveFileName(NULL, tr("Save Recording As..."),
                                                                recordPath,
                                                                encodingFileFilter);
            if (selectedFile.toLower() != "")
            {
                if(!selectedFile.toLower().endsWith("." + encodingType.toLower()))
                {
                    selectedFile.append("." + encodingType.toLower());
                }
                //Update the saved Path
                config->set(ConfigKey(RECORDING_PREF_KEY, "Path"), selectedFile);
            }
            else
                proceedWithRecording = false; //Empty filename, so don't record
        }
        else //If there was already a recording path set
        {
            //... and the file already exists, ask the user if they want to overwrite it.
            int result;
            if(QFile::exists(recordPath))
            {
                QFileInfo fi(recordPath);
                result = QMessageBox::question(this, tr("Mixxx Recording"), tr("The file %1 already exists. Would you like to overwrite it?\nSelecting \"No\" will abort the recording.").arg(fi.fileName()), QMessageBox::Yes | QMessageBox::No);
                if (result == QMessageBox::Yes) //If the user selected, "yes, overwrite the recording"...
                    proceedWithRecording = true;
                else
                    proceedWithRecording = false;
            }
        }

        if (proceedWithRecording == true)
        {
            qDebug() << "Setting record status: READY";
            recordingControl->slotSet(RECORD_READY);
        }
        else
        {
            optionsRecord->setChecked(false);
        }

    }
    else
    {
        qDebug() << "Setting record status: OFF";
        recordingControl->slotSet(RECORD_OFF);
    }

    delete recordingControl;
}

void MixxxApp::slotHelpAbout()
{

    DlgAbout *about = new DlgAbout(this);
    about->version_label->setText(VERSION);
    QString credits =
    "<p align=\"center\"><b>Mixxx 1.6.0 Development Team</b></p>"
"<p align=\"center\">"
"Adam Davison<br>"
"Albert Santoni<br>"
"Garth Dahlstrom<br>"
"Cedric Gestes<br>"
"John Sully<br>"
"Ben Wheeler<br>"
"Micah Lee<br>"
"Pawel Bartkiewicz<br>"
"Nathan Prado<br>"
"</p>"
"<p align=\"center\"><b>With contributions from:</b></p>"
"<p align=\"center\">"
"Mark Hills<br>"
"Martin Sakm&#225;r<br>"
"Alex Barker<br>"
"Dave Jarvis<br>"
"Thomas Baag<br>"
"Karlis Kalnins<br>"
"Amias Channer<br>"
"Sacha Berger<br>"
"Stefan Langhammer<br>"
"Andre Roth<br>"
"Frank Willascheck<br>"
"Robin Sheat<br>"
"Jeff Nelson<br>"
"Wesley Stessens<br>"
"Kevin Schaper<br>"
"J&aacute;n Jockusch<br>"
"Alex Markley<br>"
"Oriol Puigb&oacute;<br>"
"</p>"
"<p align=\"center\"><b>And special thanks to:</b></p>"
"<p align=\"center\">"
"Adam Bellinson<br>"
"Melanie Thielker<br>"
"Julien Rosener<br>"
"Pau Arum&iacute;<br>"
"David Garcia<br>"
"Seb Ruiz<br>"
"</p>"

"<p align=\"center\"><b>Past Contributors</b></p>"
"<p align=\"center\">"
"Tue Haste Andersen<br>"
"Ken Haste Andersen<br>"
"Ludek Hor&#225;cek<br>"
"Svein Magne Bang<br>"
"Kristoffer Jensen<br>"
"Ingo Kossyk<br>"
"Torben Hohn<br>"
"Peter Chang<br>"
"Mads Holm<br>"
"Lukas Zapletal<br>"
"Jeremie Zimmermann<br>"
"Gianluca Romanin<br>"
"Tim Jackson<br>"
"Jan Jockusch<br>"
"</p>";


    about->textBrowser->setHtml(credits);
    about->show();

}

void MixxxApp::slotHelpSupport()
{
    QUrl qSupportURL;
    qSupportURL.setUrl(MIXXX_SUPPORT_URL);
    QDesktopServices::openUrl(qSupportURL);
}

void MixxxApp::rebootMixxxView() {

    // Ok, so wierdly if you call setFixedSize with the same value twice, Qt breaks
    // So we check and if the size hasn't changed we don't make the call
    int oldh = view->height();
    int oldw = view->width();
    qDebug() << "Now in Rebootmixxview...";
    bool bVisualsWaveform = true;
    //TODO 0 or 1?
    if (config->getValueString(ConfigKey("[Controls]","Visuals")).toInt()==1)
        bVisualsWaveform = false;

    QString qSkinPath = getSkinPath();

    view->rebootGUI(frame, bVisualsWaveform, config, qSkinPath);
    qDebug() << "rebootgui DONE";
    if (oldw != view->width() || oldh != view->height() + menuBar()->height()) {
      setFixedSize(view->width(), view->height() + menuBar()->height());
    }
}

QString MixxxApp::getSkinPath() {
    QString qConfigPath = config->getConfigPath();

    QString qSkinPath(qConfigPath);
    qSkinPath.append("skins/");
    if (QDir(qSkinPath).exists())
    {
        // Is the skin listed in the config database there? If not, use default (outlineSmall) skin
        if ((config->getValueString(ConfigKey("[Config]","Skin")).length()>0 && QDir(QString(qSkinPath).append(config->getValueString(ConfigKey("[Config]","Skin")))).exists()))
            qSkinPath.append(config->getValueString(ConfigKey("[Config]","Skin")));
        else
        {
            config->set(ConfigKey("[Config]","Skin"), ConfigValue("outlineSmall"));
            config->Save();
            qSkinPath.append(config->getValueString(ConfigKey("[Config]","Skin")));
        }
    }
    else
        qCritical() << "Skin directory does not exist:" << qSkinPath;

    return qSkinPath;
}

/** Event filter to block certain events. For example, this function is used
  * to disable tooltips if the user specifies in the preferences that they
  * want them off. This is a callback function.
  */
bool MixxxApp::eventFilter(QObject *obj, QEvent *event)
{
    static int tooltips = config->getValueString(ConfigKey("[Controls]","Tooltips")).toInt();

    if (event->type() == QEvent::ToolTip) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (tooltips == 1)
            return false;
        else
            return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }

}

void MixxxApp::slotLadspa()
{
#ifdef __LADSPA__
    ladspaDlg->setHidden(false);
#endif
}
