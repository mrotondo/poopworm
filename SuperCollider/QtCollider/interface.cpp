/************************************************************************
*
* Copyright 2011 Jakob Leben (jakob.leben@gmail.com)
*
* This file is part of SuperCollider Qt GUI.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
************************************************************************/

#include "QtCollider.h"
#include "QcApplication.h"
#include "Common.h"

#include <QTimer>

QC_PUBLIC
void QtCollider::init() {
  if( !QApplication::instance() ) {
    qcDebugMsg( 1, "Initializing QtCollider" );
    #ifdef Q_OS_MAC
      QApplication::setAttribute( Qt::AA_MacPluginApplication, true );
    #endif
    static int qcArgc = 1;
    static char qcArg0[] = "";
    static char *qcArgv[1];
    qcArgv[0] = qcArg0;
    QcApplication *qcApp = new QcApplication( qcArgc, qcArgv );
    qcApp->setQuitOnLastWindowClosed( false );
  }
}
