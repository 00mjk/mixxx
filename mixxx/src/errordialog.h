/***************************************************************************
                          errordialog.h  -  description
                             -------------------
    begin                : Fri Feb 20 2009
    copyright            : (C) 2009 by Sean M. Pappalardo
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

#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QObject>

/**
  * Class used to allow all threads to display message boxes on error conditions
  *
  *@author Sean M. Pappalardo
  */

class ErrorDialog : public QObject {
   Q_OBJECT
public:
    ErrorDialog();
    ~ErrorDialog();
    void requestErrorDialog(int type, QString message);

signals:
    /** Threads call this to emit a signal to display the requested message box */
    void showErrorDialog(int type, QString message);
    
private:
    bool m_continue;
private slots:
    /** Actually displays the box */
    void errorDialog(int type, QString message);
};

#endif
