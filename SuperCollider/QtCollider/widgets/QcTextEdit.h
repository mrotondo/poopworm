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

#ifndef QC_TEXT_EDIT
#define QC_TEXT_EDIT

#include "../QcHelper.h"

#include <QTextEdit>

class QcTextEdit : public QTextEdit, QcHelper
{
  Q_OBJECT
  Q_PROPERTY( QString document READ documentFilename WRITE setDocument );
  Q_PROPERTY( int selectionStart READ selectionStart );
  Q_PROPERTY( int selectionSize READ selectionSize );
  Q_PROPERTY( QString selectedString READ selectedString );
  Q_PROPERTY( QFont textFont READ dummyFont WRITE setTextFont );
  Q_PROPERTY( QColor textColor READ dummyColor WRITE setTextColor );
  Q_PROPERTY( VariantList rangeColor
              READ dummyVariantList WRITE setRangeColor );
  Q_PROPERTY( VariantList rangeFont
              READ dummyVariantList WRITE setRangeFont );
  Q_PROPERTY( VariantList rangeText
              READ dummyVariantList WRITE setRangeText );
  private:
    QString documentFilename() const;
    void setDocument( const QString & );
    int selectionStart();
    int selectionSize();
    QString selectedString();
    void setTextFont( const QFont & );
    void setTextColor( const QColor & );
    void setRangeColor( const VariantList & );
    void setRangeFont( const VariantList & );
    void setRangeText( const VariantList & );

    QString _document;
};

#endif
