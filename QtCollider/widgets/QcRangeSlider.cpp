/************************************************************************
*
* Copyright 2010-2012 Jakob Leben (jakob.leben@gmail.com)
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

#include "QcRangeSlider.h"
#include "../QcWidgetFactory.h"
#include "../style/routines.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QPainter>

#define HND 10

QC_DECLARE_QWIDGET_FACTORY(QcRangeSlider);

QcRangeSlider::QcRangeSlider() :
  QtCollider::Style::Client(this),
  _lo( 0.f ),
  _hi( 1.f ),
  _step( 0.01f ),
  mouseMode( None )
{
  setFocusPolicy( Qt::StrongFocus );
  setOrientation( Qt::Vertical );
}

void QcRangeSlider::setOrientation( Qt::Orientation o )
{
  _ort = o;

  if( _ort == Qt::Horizontal ) {
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  }
  else {
    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Expanding );
  }

  updateGeometry();
  update();
}

void QcRangeSlider::setLoValue( float val )
{
  val = qMax( 0.f, qMin( 1.f, val ) );
  if( val <= _hi ) {
    _lo = val;
  }
  else {
    _lo = _hi;
    _hi = val;
  }
  update();
}

void QcRangeSlider::setHiValue( float val )
{
  val = qMax( 0.f, qMin( 1.f, val ) );
  if( val >= _lo ) {
    _hi = val;
  }
  else {
    _hi = _lo;
    _lo = val;
  }
  update();
}

void QcRangeSlider::setRange( double val, double range )
{
  range = qBound( 0.0, range, 1.0 );
  _lo = qBound( 0.0, val, 1.0 - range);
  _hi = _lo + range;
  update();
}

QSize QcRangeSlider::sizeHint() const
{
  return ( _ort == Qt::Horizontal ? QSize( 150, 20 ) : QSize( 20, 150 ) );
}

QSize QcRangeSlider::minimumSizeHint() const
{
  return ( _ort == Qt::Horizontal ? QSize( 30, 20 ) : QSize( 20, 30 ) );
}

void QcRangeSlider::increment( double factor )
{
  moveBy( factor * _step );
}

void QcRangeSlider::decrement( double factor )
{
  moveBy( -factor * _step );
}

void QcRangeSlider::increment()
{
  float step = _step;
  modifyStep( &step );
  moveBy( step );
}

void QcRangeSlider::decrement()
{
  float step = _step;
  modifyStep( &step );
  moveBy( -step );
}

QRect QcRangeSlider::thumbRect()
{
  using namespace QtCollider::Style;
  QRect contRect( sunkenContentsRect(rect()) );
  int hnd2 = HND * 2;
  if( _ort == Qt::Horizontal ) {
    double valRange = contRect.width() - hnd2;
    double left = _lo * valRange;
    double right = _hi * valRange; right += hnd2;
    return QRect( left + contRect.x(), contRect.y(), right - left, contRect.height() );
  }
  else {
    double valRange = contRect.height() - hnd2;
    int up = (1.0 - _hi) * valRange;
    int down = (1.0 - _lo) * valRange; down += hnd2;
    return QRect( contRect.x(), contRect.y() + up, contRect.width(), down - up );
  }
}

double QcRangeSlider::valueFromPos( const QPoint& pos )
{
  using namespace QtCollider::Style;

  bool horiz = _ort == Qt::Horizontal;

  QSize margins = horiz ? QSize(HND*2, 0) : QSize(0, HND*2);
  QRect valBounds = marginsRect( sunkenContentsRect(rect()), margins );

  return horiz ? xValue( pos.x(), valBounds ) : yValue( pos.y(), valBounds );
}

void QcRangeSlider::moveBy( float dif )
{
  if( _lo + dif < 0.f ) {
    _hi += 0.f - _lo;
    _lo = 0.f;
  }
  else if( _hi + dif > 1.f ) {
    _lo += 1.f - _hi;
    _hi = 1.f;
  }
  else {
    _lo += dif;
    _hi += dif;
  }
  update();
}

void QcRangeSlider::mouseMoveEvent ( QMouseEvent * e )
{
  using namespace QtCollider::Style;

  if( !e->buttons() ) return;

  if( mouseMode == SetHi || mouseMode == SetLo )
  {
    float val = valueFromPos( e->pos() );
    if( mouseMode == SetLo ) {
      if( val > _hi ) mouseMode = SetHi;
      setLoValue( val );
    }
    else if( mouseMode == SetHi ) {
      if( val < _lo ) mouseMode = SetLo;
      setHiValue( val );
    }
  }
  else if( mouseMode != None )
  {
    QPoint pt = e->pos() - dragOrigin;
    QRect contRect( sunkenContentsRect(rect()) );

    double dif;
    if( _ort == Qt::Horizontal ) {
      double range = contRect.width() - HND*2;
      dif = range > 0 ?  pt.x() / range : 0.0;
    }
    else {
      double range = contRect.height() - HND*2;
      dif = range > 0 ? - pt.y() / range : 0.0;
    }

    if( dif != 0.0 ) {
      switch( mouseMode ) {
        case Move:
          setRange( dragVal + dif, dragRange ); break;
        case MoveHi:
          setHiValue( qMax(dragVal + dif, (double)_lo) ); break;
        case MoveLo:
          setLoValue( qMin(dragVal + dif, (double)_hi) ); break;
      }
    }
  }

  Q_EMIT( action() );
}

void QcRangeSlider::mousePressEvent ( QMouseEvent * e )
{
  using namespace QtCollider::Style;

  if( e->modifiers() & Qt::ShiftModifier ) {
    _lo = _hi = qBound(0.0, valueFromPos( e->pos() ), 1.0);
    mouseMode = SetHi;
    Q_EMIT( action() );
  }
  else {
    QRect thumb( thumbRect() );

    int len, pos;

    if( _ort == Qt::Horizontal ) {
      len = thumb.width();
      pos = e->pos().x() - thumb.left();
    }
    else {
      len = thumb.height();
      pos = thumb.top() + len - e->pos().y();
    }

    if( pos < 0 || pos > len ) return;

    dragOrigin = e->pos();

    if( pos < HND ) {
      mouseMode = MoveLo;
      dragVal = _lo;
    }
    else if( pos >= len - HND ) {
      mouseMode = MoveHi;
      dragVal = _hi;
    }
    else {
      mouseMode = Move;
      dragVal = _lo;
      dragRange = _hi - _lo;
    }
  }

  update();
}

void QcRangeSlider::mouseReleaseEvent ( QMouseEvent * e )
{
  Q_UNUSED(e);
  mouseMode = None;
}

void QcRangeSlider::keyPressEvent ( QKeyEvent *e )
{
  switch( e->key() ) {
    case Qt::Key_Up:
    case Qt::Key_Right:
      increment(); break;
    case Qt::Key_Down:
    case Qt::Key_Left:
      decrement(); break;
    case Qt::Key_A:
      _lo = 0.f; _hi = 1.f; update(); break;
    case Qt::Key_N:
      _lo = 0.f; _hi = 0.f; update(); break;
    case Qt::Key_X:
      _lo = 1.f; _hi = 1.f; update(); break;
    case Qt::Key_C:
      _lo = 0.5; _hi = 0.5; update(); break;
    default:
      return QWidget::keyPressEvent( e );
  }
  Q_EMIT( action() );
}

inline void drawMarker( QPainter *p, const QPalette &plt, Qt::Orientation ort, bool first, const QPoint & pt, int len )
{
  p->save();
  p->setRenderHint( QPainter::Antialiasing, false );

  QPen pen(plt.color(QPalette::ButtonText));
  pen.setCapStyle(Qt::FlatCap);

  bool vert = ort == Qt::Vertical;

  if(vert) {
    pen.setWidth(1); p->setPen(pen);
    p->drawPoint( QPoint( pt.x() + len * 0.5, first ? pt.y() - 5 : pt.y() + 5 ) );

    pen.setWidth(2); p->setPen(pen);
    QLine line( pt, QPoint(pt.x() + len, pt.y()) );
    p->drawLine(line);

    line.translate(0,1);
    pen.setColor(plt.color(QPalette::Light));
    pen.setWidth(1);
    p->setPen(pen);
    p->drawLine(line);
  } else {
    pen.setWidth(1); p->setPen(pen);
    p->drawPoint( QPoint( first ? pt.x() - 5 : pt.x() + 5, pt.y() + len * 0.5 ) );

    pen.setWidth(2); p->setPen(pen);
    QLine line( pt, QPoint(pt.x(), pt.y() + len));
    p->drawLine(line);

    line.translate(1,0);
    pen.setColor(plt.color(QPalette::Light));
    pen.setWidth(1);
    p->setPen(pen);
    p->drawLine(line);
  }
  p->restore();
}

void QcRangeSlider::paintEvent ( QPaintEvent *e )
{
  using namespace QtCollider::Style;

  Q_UNUSED(e);

  QPainter p(this);
  p.setRenderHint( QPainter::Antialiasing, true );

  QPalette plt = palette();

  RoundRect frame(rect(), 3);
  QColor baseColor( grooveColor() );
  drawSunken( &p, plt, frame, baseColor, hasFocus() ? focusColor() : QColor() );

  QRect contRect( sunkenContentsRect(rect()) );

  QRect hndRect;
  QRect valRect;
  int HND2 = HND * 2;
  bool horiz = _ort == Qt::Horizontal;

  if( horiz ) {
    double valRange = contRect.width() - HND2;
    int lo = _lo * valRange;
    int hi = _hi * valRange;
    valRect = QRect( lo + HND + contRect.x(), contRect.y(), hi - lo, contRect.height() );
    hndRect = valRect.adjusted( -HND, 0, HND, 0 );
  }
  else {
    double valRange = contRect.height() - HND2;
    int hi = _hi * valRange;
    int lo = _lo * valRange;
    valRect = QRect( contRect.x(), contRect.y() + contRect.height() - hi - HND, contRect.width(), hi - lo );
    hndRect = valRect.adjusted( 0, -HND, 0, HND );
  }

  // handle

  RoundRect handle(hndRect, 2);
  drawRaised( &p, plt, handle, plt.color(QPalette::Button).lighter(105) );

  // markers

  if( horiz ) {
    int mark_len = hndRect.height() - 4;

    QPoint pt( hndRect.left() + HND, hndRect.top() + 2 );
    drawMarker( &p, plt, _ort, true, pt, mark_len );

    pt.setX( hndRect.right() - HND + 1 );
    drawMarker( &p, plt, _ort, false, pt, mark_len );
  } else {
    int mark_len = hndRect.width() - 4;
    QPoint pt( hndRect.left() + 2, hndRect.top() + HND );
    drawMarker( &p, plt, _ort, true, pt, mark_len );

    pt.setY( hndRect.bottom() - HND + 1 );
    drawMarker( &p, plt, _ort, false, pt, mark_len );
  }

  // value region

  if( horiz ? valRect.width() > 4 : valRect.height() > 4 ) {
    p.setRenderHint( QPainter::Antialiasing, false );

    QColor c( plt.color(QPalette::ButtonText) );
    c.setAlpha(50);

    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawRect(valRect.adjusted( 2, 2, -2, -2 ));
  }
}
