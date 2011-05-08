/************************************************************************
*
* Copyright 2010 Jakob Leben (jakob.leben@gmail.com)
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

#include "QcApplication.h"

#include <PyrLexer.h>
#include <VMGlobals.h>

#include <QThread>
#include <QFileOpenEvent>

QcApplication * QcApplication::_instance = 0;
QMutex QcApplication::_mutex;

QcApplication::QcApplication( int & argc, char ** argv )
: QApplication( argc, argv )
{
  _mutex.lock();
  _instance = this;
  _mutex.unlock();
  qRegisterMetaType<VariantList>();
}

QcApplication::~QcApplication()
{
  _mutex.lock();
  _instance = 0;
  _mutex.unlock();
}

void QcApplication::postSyncEvent( QcSyncEvent *e, QObject *rcv )
{
  // WARNING: If the current thread is other than receiver's, this
  // method will block until an event loop is started for the receiver's
  // thread (e.g. a QApplication is started)

  if( QThread::currentThread() == rcv->thread() ) {
    sendEvent( rcv, e );
    delete e;
  }
  else {
    QMutex mutex;
    QWaitCondition cond;

    e->_cond = &cond;
    e->_mutex = &mutex;

    mutex.lock();
    postEvent( rcv, e );
    cond.wait( &mutex );
    mutex.unlock();
  }
}

void QcApplication::postSyncEvent( QcSyncEvent *e, EventHandlerFn handler )
{
  bool sameThread;

  _mutex.lock();
  if( !_instance ) {
    // Can not use QcApplication's thread as QcApplication is not instantiated
    _mutex.unlock();
    return;
  }
  sameThread = QThread::currentThread() == _instance->thread();
  _mutex.unlock();

  if( sameThread ) {
    (*handler)(e);
    delete(e);
  }
  else {
    // NOTE:
    // Despite locking QcApplication's mutex for the time of event processing
    // this method can be called recursively from event processing, because that
    // implies that the current thread will be the same as QcApplication's, so
    // this branch will not be entered the second time
    _mutex.lock();

    QMutex mutex;
    QWaitCondition cond;

    e->_handler = handler;
    e->_cond = &cond;
    e->_mutex = &mutex;

    mutex.lock();
    postEvent( _instance, e );
    cond.wait( &mutex );
    mutex.unlock();

    _mutex.unlock();
  }
}

bool QcApplication::event( QEvent *e )
{
  if( e->type() == QEvent::FileOpen ) {

    // open the file dragged onto the application icon on Mac

    QFileOpenEvent *fe = static_cast<QFileOpenEvent*>(e);

    QtCollider::lockLang();
    gMainVMGlobals->canCallOS = true;

    QString cmdLine = QString("Document.open(\"%1\")").arg(fe->file());
    char *method = strdup( "interpretPrintCmdLine" );
    interpretCmdLine( cmdLine.toStdString().c_str(), cmdLine.size(), method );
    free(method);

    gMainVMGlobals->canCallOS = false;
    QtCollider::unlockLang();

    return true;
  }

  return QApplication::event( e );
}

void QcApplication::customEvent( QEvent *e )
{
  // FIXME properly check event type
  QcSyncEvent *qce = static_cast<QcSyncEvent*>(e);
  if( qce->_handler ) {
    (*qce->_handler) ( qce );
  }
}
