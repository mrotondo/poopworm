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

#ifndef QC_SIGNALSPY_H
#define QC_SIGNALSPY_H

#include "Common.h"
#include "QObjectProxy.h"

#include <PyrKernel.h>
#include <PyrSymbol.h>
#include <VMGlobals.h>
#include <SCBase.h>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QMetaObject>
#include <QMetaMethod>
#include <QVariant>

class QcSignalSpy: public QObject
{

public:

QcSignalSpy( QObjectProxy *proxy, const char *sigName,
              Qt::ConnectionType conType = Qt::QueuedConnection )
: QObject( proxy ), _proxy( proxy ), _sigId( -1 )
{

  Q_ASSERT( sigName );

  const QMetaObject *mo = _proxy->object()->metaObject();

  QByteArray signal = QMetaObject::normalizedSignature( sigName );
  int sigId = mo->indexOfSignal( signal );

  if( sigId < 0 ) {
    qcErrorMsg( QString("No such signal: '%1'").arg(signal.constData()) );
    return;
  }

  int slotId = QObject::staticMetaObject.methodCount();

  if( !QMetaObject::connect( _proxy->object(), sigId, this, slotId,
                            conType, 0) )
  {
    qcErrorMsg( "QMetaObject::connect returned false. Unable to connect." );
    return;
  }

  QMetaMethod mm = mo->method( sigId );

  QList<QByteArray> params = mm.parameterTypes();

  for( int i = 0; i < params.count(); ++i ) {
    int type = QMetaType::type( params.at(i).constData() );
    if( type == QMetaType::Void )
      qcErrorMsg( QString("QObject:connect: Don't know how to handle '%1', "
                          "use qRegisterMetaType to register it.")
                  .arg(params.at(i).constData()) );
    _argTypes << type;
  }

  _sigId = sigId;
}

int qt_metacall( QMetaObject::Call call, int methodId, void **argData )
{
  methodId = QObject::qt_metacall( call, methodId, argData );

  if( methodId < 0 )
    return methodId;

  if( call == QMetaObject::InvokeMetaMethod ) {
    Q_ASSERT( methodId == 0 );

    QList<QVariant> args;
#if QT_VERSION >= 0x040700
    args.reserve( _argTypes.count() );
#endif

    for (int i = 0; i < _argTypes.count(); ++i) {
      QMetaType::Type type = static_cast<QMetaType::Type>(_argTypes.at(i));
      args << QVariant( type, argData[i + 1] );
    }

    react( args );

    methodId = -1;
  }

  return methodId;
}

inline int indexOfSignal () const { return _sigId; }

inline bool isValid () const { return _sigId > 0; }

void destroy () {
  int slotId = QObject::staticMetaObject.methodCount();
  QMetaObject::disconnect( _proxy->object(), _sigId, this, slotId );
  _sigId = -1;
  deleteLater();
}

protected:

  virtual void react( const QList<QVariant> args ) = 0;

  QObjectProxy *_proxy;
  int _sigId;
  QList<int> _argTypes;
};

class QcMethodSignalHandler : public QcSignalSpy
{
public:
  QcMethodSignalHandler( QObjectProxy *proxy, const char *sigName, PyrSymbol *handler,
                          Qt::ConnectionType conType = Qt::QueuedConnection )
  : QcSignalSpy( proxy, sigName, conType ), _handler( handler )
  { }

  inline PyrSymbol *method() { return _handler; }

protected:

  virtual void react( const QList<QVariant> args ) {

    qcDebugMsg( 1, QString("SIGNAL: '%1' handled by method '%2'")
                  .arg( _proxy->object() ?
                        _proxy->object()->metaObject()->method( _sigId ).signature() :
                        "unknown" )
                  .arg(_handler->name)
              );

    _proxy->invokeScMethod( _handler, args );
  }

  PyrSymbol * _handler;
};

class QcFunctionSignalHandler : public QcSignalSpy
{
public:
  QcFunctionSignalHandler( QObjectProxy *proxy, const char *sigName, PyrObject *handler,
                          Qt::ConnectionType conType = Qt::QueuedConnection )
  : QcSignalSpy( proxy, sigName, conType ), _handler( handler )
  { }

  inline PyrObject *function() { return _handler; }

protected:

  virtual void react( const QList<QVariant> args ) {

    qcDebugMsg( 1, QString("SIGNAL: '%1' handled by a Function")
                  .arg( _proxy->object() ?
                        _proxy->object()->metaObject()->method( _sigId ).signature() :
                        "unknown" )
              );

    // FIXME, reuse QObjectProxy::invokeScMethod. Here a custom implementation just
    // because Slot does not support a Function.

    QtCollider::lockLang();

    if( _proxy->scObject && this->isValid() ) {
      qcDebugMsg(1, QString("SC FUNCTION CALL [+++] ") );

      VMGlobals *g = gMainVMGlobals;
      g->canCallOS = true;
      ++g->sp;  SetObject(g->sp, _proxy->scObject);
      ++g->sp;  SetObject(g->sp, _handler);
      Q_FOREACH( QVariant var, args ) {
        ++g->sp;
        if( QtCollider::Slot::setVariant( g->sp, var ) )
          SetNil( g->sp );
      }
      runInterpreter(g, QtCollider::s_doFunction, args.size() + 2);
      g->canCallOS = false;

      qcDebugMsg(1, QString("SC FUNCTION CALL [---] ") );
    }

    QtCollider::unlockLang();
  }

  PyrObject * _handler;
};


#endif //QC_SIGNALSPY_H
