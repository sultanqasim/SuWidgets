//
//    TVDisplay.cpp: Description
//    Copyright (C) 2020 Gonzalo José Carracedo Carballal
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Lesser General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful, but
//    WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this program.  If not, see
//    <http://www.gnu.org/licenses/>
//

#include <QPainter>

#include <sigutils/tvproc.h>

#include <QResizeEvent>
#include "TVDisplay.h"

void
TVDisplay::setPicGeometry(int width, int height)
{
  this->picture = QImage(width, height, QImage::Format_ARGB32);
  this->picture.fill(this->background);
}

void
TVDisplay::putFrame(const struct sigutils_tv_frame_buffer *buffer)
{
  int i;
  int x = 0, y = 0, size;
  QRgb *scanLine;

  if (this->picture.width() != buffer->width
      || this->picture.height() != buffer->height)
    this->setPicGeometry(buffer->width, buffer->height);

  size = buffer->width * buffer->height;

  scanLine = reinterpret_cast<QRgb *>(this->picture.scanLine(0));

  for (i = 0; i < size; ++i) {
    scanLine[x++] = this->tvSampleToRgb(buffer->buffer[i]);

    if (x == buffer->width) {
      x = 0;
      scanLine = reinterpret_cast<QRgb *>(this->picture.scanLine(++y));
    }
  }

  this->dirty = true;
}

void
TVDisplay::putLine(int line, const SUFLOAT *data, int size)
{
  int i;
  QRgb *scanLine;

  if (!this->havePicGeometry())
    return;

  if (line < 0 || line >= this->picture.height())
    return;

  if (size > this->picture.width())
    size = this->picture.width();

  scanLine = reinterpret_cast<QRgb *>(this->picture.scanLine(line));

  for (i = 0; i < size; ++i)
    scanLine[i] = this->tvSampleToRgb(data[i]);

  for (i = size; i < this->picture.width(); ++i)
    scanLine[i] = this->colors[0];

  this->dirty = true;
}

void
TVDisplay::draw(void)
{
  if (!this->size().isValid())
    return;

  if (this->geometry != this->size()) {
    this->geometry = this->size();
    this->dirty = true;
  }

  if (this->dirty) {
    if (!this->havePicGeometry()) {
      if (this->contentPixmap.size() != this->geometry)
        this->contentPixmap = QPixmap(this->geometry);
      this->contentPixmap.fill(this->background);
    } else {
      this->contentPixmap =
        QPixmap::fromImage(this->picture).scaled(
          this->width(),
            this->height(),
            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    this->dirty = false;
  }
}

QSize
TVDisplay::sizeHint(void) const
{
  QSize s = this->size();

  if (this->parentWidget() != nullptr)
    s = this->parentWidget()->size();

  if (s.width() != 0)
    s.setHeight(static_cast<int>(s.width() / this->aspect));
  else if (s.height() != 0)
    s.setWidth(static_cast<int>(s.height() * this->aspect));
  else
    s = QSize(640, 480);

  return s;
}

void
TVDisplay::resizeEvent(QResizeEvent *event)
{
  if (event->size().width() > 0
      && event->size().height() > 0) {
    int width, height;

    this->requestedGeometry = event->size();
    event->accept();
    width = std::min(
          event->size().width(),
          static_cast<int>(event->size().height() * this->aspect));
    height = std::min(
          event->size().height(),
          static_cast<int>(event->size().width() / this->aspect));

    this->resize(width, height);
    this->invalidate();
  }
}


void
TVDisplay::paint(void)
{
  QPainter painter(this);
  qreal rx = .5 * this->width();
  qreal ry = .5 * this->height();

  painter.translate(QPointF(rx, ry));
  painter.scale(
        this->hFlip ? -this->pZoom : this->pZoom,
        this->vFlip ? -this->pZoom : this->pZoom);

  if (this->angle < 0 || this->angle > 0)
    painter.rotate(this->angle);

  painter.drawPixmap(
        static_cast<int>(-rx),
        static_cast<int>(-ry),
        this->contentPixmap);
}


TVDisplay::TVDisplay(QWidget *parent) : ThrottleableWidget(parent)
{
  this->contentPixmap = QPixmap(0, 0);
  this->picture = QImage(0, 0, QImage::Format_ARGB32);

  this->setBackgroundColor(TVDISPLAY_DEFAULT_BACKGROUND_COLOR);
  this->setForegroundColor(TVDISPLAY_DEFAULT_FOREGROUND_COLOR);

  this->invalidate();
}
