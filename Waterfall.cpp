/* -*- c++ -*- */
/* + + +   This Software is released under the "Simplified BSD License"  + + +
 * Copyright 2010 Moe Wheatley. All rights reserved.
 * Copyright 2011-2013 Alexandru Csete OZ9AEC
 * Copyright 2018 Gonzalo José Carracedo Carballal - Minimal modifications for integration
 * Copyright 2023-2024 Sultan Qasim Khan - Abstract class for OpenGL and QPainter waterfalls
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice, this list of
 *       conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright notice, this list
 *       of conditions and the following disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those of the
 * authors and should not be interpreted as representing official policies, either expressed
 * or implied, of Moe Wheatley.
 */
#include <cmath>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QFont>
#include <QPainter>
#include <QtGlobal>
#include <QToolTip>
#include <QDebug>

#include "Waterfall.h"
#include "SuWidgetsHelpers.h"

///////////////////////////// Waterfall ////////////////////////////////////////
Waterfall::Waterfall(QWidget *parent) : AbstractWaterfall(parent)
{
  // default waterfall color scheme
  for (int i = 0; i < 256; i++)
  {
    // level 0: black background
    if (i < 20)
      m_ColorTbl[i].setRgb(0, 0, 0);
    // level 1: black -> blue
    else if ((i >= 20) && (i < 70))
      m_ColorTbl[i].setRgb(0, 0, 140*(i-20)/50);
    // level 2: blue -> light-blue / greenish
    else if ((i >= 70) && (i < 100))
      m_ColorTbl[i].setRgb(60*(i-70)/30, 125*(i-70)/30, 115*(i-70)/30 + 140);
    // level 3: light blue -> yellow
    else if ((i >= 100) && (i < 150))
      m_ColorTbl[i].setRgb(195*(i-100)/50 + 60, 130*(i-100)/50 + 125, 255-(255*(i-100)/50));
    // level 4: yellow -> red
    else if ((i >= 150) && (i < 250))
      m_ColorTbl[i].setRgb(255, 255-255*(i-150)/100, 0);
    // level 5: red -> white
    else if (i >= 250)
      m_ColorTbl[i].setRgb(255, 255*(i-250)/5, 255*(i-250)/5);
  }

  for (int i = 0; i < 256; i++)
    m_UintColorTbl[i] = qRgb(
        m_ColorTbl[i].red(),
        m_ColorTbl[i].green(),
        m_ColorTbl[i].blue());

  memset(m_wfbuf, 255, MAX_SCREENSIZE);
}

Waterfall::~Waterfall()
{
}

void Waterfall::setPalette(const QColor *table)
{
  unsigned int i;

  for (i = 0; i < 256; ++i) {
    this->m_ColorTbl[i] = table[i];
    this->m_UintColorTbl[i] = qRgb(
        table[i].red(),
        table[i].green(),
        table[i].blue());
  }

  this->update();
}

void Waterfall::clearWaterfall()
{
  m_WaterfallImage.fill(Qt::black);
  memset(m_wfbuf, 255, MAX_SCREENSIZE);
}

/**
 * @brief Save waterfall to a graphics file
 * @param filename
 * @return TRUE if the save successful, FALSE if an erorr occurred.
 *
 * We assume that frequency strings are up to date
 */
bool Waterfall::saveWaterfall(const QString & filename) const
{
  QBrush          axis_brush(QColor(0x00, 0x00, 0x00, 0x70), Qt::SolidPattern);
  QPixmap         pixmap = QPixmap::fromImage(m_WaterfallImage);
  QPainter        painter(&pixmap);
  QRect           rect;
  QDateTime       tt;
  QFont           font("sans-serif");
  QFontMetrics    font_metrics(font);
  float           pixperdiv;
  int             x, y, w, h;
  int             hxa, wya = 85;
  int             i;

  w = pixmap.width();
  h = pixmap.height();
  hxa = font_metrics.height() + 5;    // height of X axis
  y = h - hxa;
  pixperdiv = (float) w / (float) m_HorDivs;

  painter.setBrush(axis_brush);
  painter.setPen(QColor(0x0, 0x0, 0x0, 0x70));
  painter.drawRect(0, y, w, hxa);
  painter.drawRect(0, 0, wya, h - hxa - 1);
  painter.setFont(font);
  painter.setPen(QColor(0xFF, 0xFF, 0xFF, 0xFF));

  // skip last frequency entry
  for (i = 2; i < m_HorDivs - 1; i++)
  {
    // frequency tick marks
    x = (int)((float)i * pixperdiv);
    painter.drawLine(x, y, x, y + 5);

    // frequency strings
    x = (int)((float)i * pixperdiv - pixperdiv / 2.0);
    rect.setRect(x, y, (int)pixperdiv, hxa);
    painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
  }
  rect.setRect(w - pixperdiv - 10, y, pixperdiv, hxa);
  painter.drawText(rect, Qt::AlignRight|Qt::AlignBottom, tr("MHz"));

  quint64 msec;
  int tdivs = h / 70 + 1;
  pixperdiv = (float) h / (float) tdivs;
  tt.setTimeSpec(Qt::OffsetFromUTC);
  for (i = 1; i < tdivs; i++)
  {
    y = (int)((float)i * pixperdiv);
    if (msec_per_wfline > 0)
      msec =  tlast_wf_ms - y * msec_per_wfline;
    else
      msec =  tlast_wf_ms - y * 1000 / fft_rate;

    tt.setMSecsSinceEpoch(msec);
    rect.setRect(0, y - font_metrics.height(), wya - 5, font_metrics.height());
    painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("yyyy.MM.dd"));
    painter.drawLine(wya - 5, y, wya, y);
    rect.setRect(0, y, wya - 5, font_metrics.height());
    painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, tt.toString("hh:mm:ss"));
  }

  return pixmap.save(filename, 0, -1);
}

// Called by QT when screen needs to be redrawn
void Waterfall::paintEvent(QPaintEvent *)
{
  QPainter painter(this);
  qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64  EndFreq = StartFreq + m_Span;

  int y = m_Percent2DScreen * m_Size.height() / 100;

  painter.drawPixmap(0, 0, m_2DPixmap);
  painter.drawImage(0, y, m_WaterfallImage);

  // Draw named channel cutoffs
  if (m_channelsEnabled) {
    for (auto i = m_channelSet.find(StartFreq); i != m_channelSet.cend(); ++i) {
      auto p = i.value();
      int x_fCenter = xFromFreq(p->frequency);
      int x_fMin = xFromFreq(p->frequency + p->lowFreqCut);
      int x_fMax = xFromFreq(p->frequency + p->highFreqCut);

      if (EndFreq < p->frequency + p->lowFreqCut)
        break;

      WFHelpers::drawChannelCutoff(
          painter,
          y,
          x_fMin,
          x_fMax,
          x_fCenter,
          p->markerColor,
          p->cutOffColor,
          !p->bandLike);
    }
  }

  if (m_FilterBoxEnabled)
    this->drawFilterCutoff(painter, y);

  if (m_TimeStampsEnabled) {
    paintTimeStamps(
        painter,
        QRect(2, y, this->width(), this->height()));
  }
}

// Called to draw an overlay bitmap containing grid and text that
// does not need to be recreated every fft data update.
void Waterfall::drawOverlay()
{
  if (m_OverlayPixmap.isNull())
    return;

  int     w = m_OverlayPixmap.width();
  int     h = m_OverlayPixmap.height();
  int     x,y;
  int     bandY = 0;
  float   pixperdiv;
  float   adjoffset;
  float   unitStepSize;
  float   minUnitAdj;
  QRect   rect;
  QFontMetrics    metrics(m_Font);
  QPainter        painter(&m_OverlayPixmap);

  painter.setFont(m_Font);

  // solid background
  painter.setBrush(Qt::SolidPattern);
  painter.fillRect(0, 0, w, h, m_FftBgColor);

#define HOR_MARGIN 5
#define VER_MARGIN 5

  // X and Y axis areas
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  int tw = metrics.horizontalAdvance("XXXX");
#else
  int tw = metrics.width("XXXX");
#endif // QT_VERSION_CHECK

  m_YAxisWidth = tw + 2 * HOR_MARGIN;
  m_XAxisYCenter = h - metrics.height()/2;
  int xAxisHeight = metrics.height() + 2 * VER_MARGIN;
  int xAxisTop = h - xAxisHeight;
  int fLabelTop = xAxisTop + VER_MARGIN;

  if (m_CenterLineEnabled)
  {
    x = xFromFreq(m_CenterFreq - m_tentativeCenterFreq);
    if (x > 0 && x < w)
    {
      painter.setPen(m_FftCenterAxisColor);
      painter.drawLine(x, 0, x, xAxisTop);
    }
  }

  // Frequency grid
  qint64  StartFreq = m_CenterFreq + m_FftCenter - m_Span / 2;
  qint64  EndFreq = StartFreq + m_Span;
  QString label;
  label.setNum(float(EndFreq / m_FreqUnits), 'f', m_FreqDigits);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  tw = metrics.horizontalAdvance(label) + metrics.horizontalAdvance("O");
#else
  tw = metrics.width(label) + metrics.width("O");
#endif // QT_VERSION_CHECK

  calcDivSize(StartFreq, EndFreq,
      qMin(w/tw, HORZ_DIVS_MAX),
      m_StartFreqAdj, m_FreqPerDiv, m_HorDivs);

  pixperdiv = (float)w * (float) m_FreqPerDiv / (float) m_Span;
  adjoffset = pixperdiv * float (m_StartFreqAdj - StartFreq) / (float) m_FreqPerDiv;

  painter.setPen(QPen(m_FftAxesColor, 1, Qt::DotLine));
  for (int i = 0; i <= m_HorDivs; i++)
  {
    x = (int)((float)i * pixperdiv + adjoffset);
    if (x > m_YAxisWidth)
      painter.drawLine(x, 0, x, xAxisTop);
  }

  // draw frequency values (x axis)
  makeFrequencyStrs();
  painter.setPen(m_FftTextColor);
  for (int i = 0; i <= m_HorDivs; i++)
  {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int tw = metrics.horizontalAdvance(m_HDivText[i]);
#else
    int tw = metrics.width(m_HDivText[i]);
#endif // QT_VERSION_CHECK
    x = (int)((float)i*pixperdiv + adjoffset);
    if (x > m_YAxisWidth)
    {
      rect.setRect(x - tw/2, fLabelTop, tw, metrics.height());
      painter.drawText(rect, Qt::AlignHCenter|Qt::AlignBottom, m_HDivText[i]);
    }
  }

  // Draw frequency allocation tables
  if (this->m_ShowFATs) {
    int count = 0;
    for (auto fat : this->m_FATs)
      if (fat.second != nullptr) {
        FrequencyBandIterator p = fat.second->find(StartFreq);

        while (p != fat.second->cbegin() && p->second.max > StartFreq)
          --p;

        for (; p != fat.second->cend() && p->second.min < EndFreq; ++p) {
          int x0 = xFromFreq(p->second.min);
          int x1 = xFromFreq(p->second.max);
          bool leftborder = true;
          bool rightborder = true;
          int tw, boxw;


          if (x0 < m_YAxisWidth) {
            leftborder = false;
            x0 = m_YAxisWidth;
          }

          if (x1 >= w) {
            rightborder = false;
            x1 = w - 1;
          }

          if (x1 < m_YAxisWidth)
            continue;

          boxw = x1 - x0;

          painter.setBrush(QBrush(p->second.color));
          painter.setPen(p->second.color);

          painter.drawRect(
              x0,
              count * metrics.height(),
              x1 - x0 + 1,
              metrics.height());

          if (leftborder)
            painter.drawLine(
                x0,
                count * metrics.height(),
                x0,
                h);

          if (rightborder)
            painter.drawLine(
                x1,
                count * metrics.height(),
                x1,
                h);

          label = metrics.elidedText(
              QString::fromStdString(p->second.primary),
              Qt::ElideRight,
              boxw);

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
          tw = metrics.horizontalAdvance(label);
#else
          tw = metrics.width(label);
#endif // QT_VERSION_CHECK

          if (tw < boxw) {
            painter.setPen(m_FftTextColor);
            rect.setRect(
                x0 + (x1 - x0) / 2 - tw / 2,
                count * metrics.height(),
                tw,
                metrics.height());
            painter.drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, label);
          }
        }

        ++count;
      }

    bandY = count + metrics.height() / 2;
  }

  // Level grid
  qint64 minUnitAdj64 = 0;
  qint64 unitDivSize = 0;
  qint64 unitSign    = m_dBPerUnit < 0 ? -1 : 1;
  qint64 pandMinUnit = unitSign * static_cast<qint64>(toDisplayUnits(m_PandMindB));
  qint64 pandMaxUnit = unitSign * static_cast<qint64>(toDisplayUnits(m_PandMaxdB));

  calcDivSize(pandMinUnit, pandMaxUnit,
      qMax(h/m_VdivDelta, VERT_DIVS_MIN), minUnitAdj64, unitDivSize,
      m_VerDivs);

  unitStepSize = (float) unitDivSize;
  minUnitAdj = minUnitAdj64;

  pixperdiv = (float) h * (float) unitStepSize / (pandMaxUnit - pandMinUnit);
  adjoffset = (float) h * (minUnitAdj - pandMinUnit) / (pandMaxUnit - pandMinUnit);

#ifdef PLOTTER_DEBUG
  qDebug() << "minDb =" << m_PandMindB << "maxDb =" << m_PandMaxdB
    << "mindbadj =" << mindbadj << "dbstepsize =" << dbstepsize
    << "pixperdiv =" << pixperdiv << "adjoffset =" << adjoffset;
#endif

  painter.setPen(QPen(m_FftAxesColor, 1, Qt::DotLine));
  for (int i = 0; i <= m_VerDivs; i++)
  {
    y = h - (int)((float) i * pixperdiv + adjoffset);
    if (y < h - xAxisHeight)
      painter.drawLine(m_YAxisWidth, y, w, y);
  }

#ifdef WATERFALL_BOOKMARKS_SUPPORT
  if (m_BookmarksEnabled && this->m_BookmarkSource != nullptr)
  {
    m_BookmarkTags.clear();
    static const QFontMetrics fm(painter.font());
    static const int fontHeight = fm.ascent() + 1;
    static const int slant = 5;
    static const int levelHeight = fontHeight + 5;
    static const int nLevels = 10;

    QList<BookmarkInfo> bookmarks =
      this->m_BookmarkSource->getBookmarksInRange(
          m_CenterFreq + m_FftCenter - m_Span / 2,
          m_CenterFreq + m_FftCenter + m_Span / 2);
    int tagEnd[nLevels] = {0};
    for (int i = 0; i < bookmarks.size(); i++)
    {
      x = xFromFreq(bookmarks[i].frequency);

#if defined(_WIN16) || defined(_WIN32) || defined(_WIN64)
      int nameWidth = fm.width(bookmarks[i].name);
#else
      int nameWidth = fm.boundingRect(bookmarks[i].name).width();
#endif

      int level = 0;
      int yMin = static_cast<int>(this->m_FATs.size()) * metrics.height();
      while(level < nLevels && tagEnd[level] > x)
        level++;

      if(level == nLevels)
        level = 0;


      tagEnd[level] = x + nameWidth + slant - 1;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
      BookmarkInfo bookmark = bookmarks[i];
      m_BookmarkTags.append(
          qMakePair<QRect, BookmarkInfo>(
            QRect(x, yMin + level * levelHeight, nameWidth + slant, fontHeight),
            std::move(bookmark))); // Be more Cobol every day
#else
      m_BookmarkTags.append(
          qMakePair<QRect, BookmarkInfo>(
            QRect(x, yMin + level * levelHeight, nameWidth + slant, fontHeight),
            bookmarks[i]));
#endif // QT_VERSION
      QColor color = QColor(bookmarks[i].color);
      color.setAlpha(0x60);
      // Vertical line
      painter.setPen(QPen(color, 1, Qt::DashLine));
      painter.drawLine(x, yMin + level * levelHeight + fontHeight + slant, x, xAxisTop);

      // Horizontal line
      painter.setPen(QPen(color, 1, Qt::SolidLine));
      painter.drawLine(x + slant, yMin + level * levelHeight + fontHeight,
          x + nameWidth + slant - 1,
          yMin + level * levelHeight + fontHeight);
      // Diagonal line
      painter.drawLine(x + 1, yMin + level * levelHeight + fontHeight + slant - 1,
          x + slant - 1, yMin + level * levelHeight + fontHeight + 1);

      color.setAlpha(0xFF);
      painter.setPen(QPen(color, 2, Qt::SolidLine));
      painter.drawText(x + slant, yMin + level * levelHeight, nameWidth,
          fontHeight, Qt::AlignVCenter | Qt::AlignHCenter,
          bookmarks[i].name);
    }
  }
#endif // WATERFALL_BOOKMARKS_SUPPORT

  // draw amplitude values (y axis)
  int unit;
  int unitWidth;

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
  m_YAxisWidth = metrics.horizontalAdvance("-160 ");
  unitWidth    = metrics.horizontalAdvance(m_unitName);
#else
  m_YAxisWidth = metrics.width("-160 ");
  unitWidth    = metrics.width(m_unitName);
#endif // QT_VERSION_CHECK

  if (unitWidth > m_YAxisWidth)
    m_YAxisWidth = unitWidth;

  painter.setPen(m_FftTextColor);
  int th = metrics.height();
  for (int i = 0; i < m_VerDivs; i++)
  {
    y = h - (int)((float) i * pixperdiv + adjoffset);

    if (y < h -xAxisHeight)
    {
      unit = minUnitAdj + unitStepSize * i;
      rect.setRect(HOR_MARGIN, y - th / 2, m_YAxisWidth, th);
      painter.drawText(
          rect,
          Qt::AlignRight|Qt::AlignVCenter,
          QString::number(unitSign * unit));
    }
  }

  // Draw unit name on top left corner
  rect.setRect(HOR_MARGIN, 0, unitWidth, th);
  painter.drawText(rect, Qt::AlignRight|Qt::AlignVCenter, m_unitName);

  // Draw named channel (boxes)
  if (m_channelsEnabled) {
    for (auto i = m_channelSet.find(StartFreq - m_Span); i != m_channelSet.cend(); ++i) {
      auto p = i.value();
      int x_fCenter = xFromFreq(p->frequency);
      int x_fMin = xFromFreq(p->frequency + p->lowFreqCut);
      int x_fMax = xFromFreq(p->frequency + p->highFreqCut);

      if (p->frequency + p->highFreqCut < StartFreq)
        continue;

      if (EndFreq < p->frequency + p->lowFreqCut)
        break;


      if (p->bandLike) {
        WFHelpers::drawChannelBox(
            painter,
            h,
            x_fMin,
            x_fMax,
            x_fCenter,
            p->boxColor,
            p->markerColor,
            p->name,
            p->markerColor,
            0,
            bandY + p->nestLevel * metrics.height());
      } else {
        WFHelpers::drawChannelBox(
            painter,
            h,
            x_fMin,
            x_fMax,
            x_fCenter,
            p->boxColor,
            p->markerColor,
            p->name,
            QColor(),
            -1,
            bandY + p->nestLevel * metrics.height());
      }
    }
  }

  // Draw demod filter box
  if (m_FilterBoxEnabled)
    this->drawFilterBox(painter, h);


  // Draw info text (if enabled)
  if (!m_infoText.isEmpty()) {
    int flags = Qt::AlignRight |  Qt::AlignTop | Qt::TextWordWrap;
    QRectF pixRect = m_OverlayPixmap.rect();

    pixRect.setWidth(pixRect.width() - 10);

    QRectF rect = painter.boundingRect(
        pixRect,
        flags,
        m_infoText);

    rect.setX(pixRect.width() - rect.width());
    rect.setY(0);

    painter.setPen(QPen(m_infoTextColor, 2, Qt::SolidLine));
    painter.drawText(rect, flags, m_infoText);
  }

  if (!m_Running)
  {
    // if not running so is no data updates to draw to screen
    // copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);

    // trigger a new paintEvent
    update();
  }

  painter.end();
}

// Called to update spectrum data for displaying on the screen
void Waterfall::draw(bool everything)
{
  int     i, n;
  int     w;
  int     h;
  int     xmin, xmax;
  qint64 limit = ((qint64)m_SampleFreq + m_Span) / 2 - 1;
  bool valid = m_fftDataSize > 0;

  if (m_DrawOverlay) {
    drawOverlay();
    m_DrawOverlay = false;
  }

  QPoint LineBuf[MAX_SCREENSIZE];

  // get/draw the waterfall
  w = m_WaterfallImage.width();
  h = m_WaterfallImage.height();

  // no need to draw if pixmap is invisible
  if (w != 0 && h != 0 && valid && everything)
  {
    quint64     tnow_ms = SCAST(quint64, m_lastFft.toMSecsSinceEpoch());

    // get scaled FFT data
    n = qMin(w, MAX_SCREENSIZE);
    getScreenIntegerFFTData(255, n, m_WfMaxdB, m_WfMindB,
        qBound(
          -limit,
          m_tentativeCenterFreq + m_FftCenter,
          limit) - (qint64)m_Span/2,
        qBound(
          -limit,
          m_tentativeCenterFreq + m_FftCenter,
          limit) + (qint64)m_Span/2,
        m_wfData, m_fftbuf,
        &xmin, &xmax);

    if (msec_per_wfline > 0)
    {
      // not in "auto" mode, so accumulate waterfall data
      for (i = 0; i < n; i++)
      {
        // average
        m_wfbuf[i] = (m_wfbuf[i] + m_fftbuf[i]) / 2;

        // peak (0..255 where 255 is min)
        if (m_fftbuf[i] < m_wfbuf[i])
          m_wfbuf[i] = m_fftbuf[i];
      }
    }

    // is it time to update waterfall?
    if (tnow_ms < tlast_wf_ms || tnow_ms - tlast_wf_ms >= msec_per_wfline)
    {
      int line_count = (tnow_ms - tlast_wf_ms) / msec_per_wfline;
      if (line_count >= 1 && line_count <= h && line_count <= 20) {
        tlast_wf_ms += msec_per_wfline * line_count;
      } else {
        line_count = 1;
        tlast_wf_ms = tnow_ms;
      }

      // move current data down required amounte (must do before attaching a QPainter object)
      // m_WaterfallImage.scroll(0, line_count, 0, 0, w, h);

      memmove(
          m_WaterfallImage.scanLine(line_count),
          m_WaterfallImage.scanLine(0),
          static_cast<size_t>(w) * (static_cast<size_t>(h) - line_count)
          * sizeof(uint32_t));

      uint32_t *scanLineData =
        reinterpret_cast<uint32_t *>(m_WaterfallImage.scanLine(0));

      memset(
          scanLineData,
          0,
          static_cast<unsigned>(xmin) * sizeof(uint32_t));

      memset(
          scanLineData + xmax,
          0,
          static_cast<unsigned>(w - xmax) * sizeof(uint32_t));

      if (msec_per_wfline > 0) {
        // user set time span
        for (i = xmin; i < xmax; i++) {
          scanLineData[i] = m_UintColorTbl[255 - m_wfbuf[i]];
          m_wfbuf[i] = 255;
        }
      } else {
        for (i = xmin; i < xmax; i++)
          scanLineData[i] = m_UintColorTbl[255 - m_fftbuf[i]];
      }

      // copy as needed onto extra lines
      for (int j = 1; j < line_count; j++) {
        uint32_t *subseqScanLineData =
          reinterpret_cast<uint32_t *>(m_WaterfallImage.scanLine(j));
        memcpy(subseqScanLineData, scanLineData,
            static_cast<size_t>(w) * sizeof(uint32_t));
      }

      this->m_TimeStampCounter += line_count;
    }
  }

  // get/draw the 2D spectrum
  w = m_2DPixmap.width();
  h = m_2DPixmap.height();

  if (w != 0 && h != 0 && valid)
  {
    // first copy into 2Dbitmap the overlay bitmap.
    m_2DPixmap = m_OverlayPixmap.copy(0,0,w,h);

    QPainter painter2(&m_2DPixmap);

    // workaround for "fixed" line drawing since Qt 5
    // see http://stackoverflow.com/questions/16990326
#if QT_VERSION >= 0x050000
    painter2.translate(0.5, 0.5);
#endif

    // get new scaled fft data
    getScreenIntegerFFTData(h, qMin(w, MAX_SCREENSIZE),
        m_PandMaxdB, m_PandMindB,
        qBound(
          -limit,
          m_tentativeCenterFreq + m_FftCenter,
          limit) - (qint64)m_Span/2,
        qBound(
          -limit,
          m_tentativeCenterFreq + m_FftCenter,
          limit) + (qint64)m_Span/2,
        m_fftData, m_fftbuf,
        &xmin, &xmax);

    // draw the pandapter
    painter2.setPen(m_FftColor);
    n = xmax - xmin;
    for (i = 0; i < n; i++)
    {
      LineBuf[i].setX(i + xmin);
      LineBuf[i].setY(m_fftbuf[i + xmin]);
    }

    if (m_FftFill)
    {
      painter2.setBrush(QBrush(m_FftFillCol, Qt::SolidPattern));
      if (n < MAX_SCREENSIZE-2)
      {
        LineBuf[n].setX(xmax-1);
        LineBuf[n].setY(h);
        LineBuf[n+1].setX(xmin);
        LineBuf[n+1].setY(h);
        painter2.drawPolygon(LineBuf, n+2);
      }
      else
      {
        LineBuf[MAX_SCREENSIZE-2].setX(xmax-1);
        LineBuf[MAX_SCREENSIZE-2].setY(h);
        LineBuf[MAX_SCREENSIZE-1].setX(xmin);
        LineBuf[MAX_SCREENSIZE-1].setY(h);
        painter2.drawPolygon(LineBuf, n);
      }
    }
    else
    {
      painter2.drawPolyline(LineBuf, n);
    }

    // Peak detection
    if (m_PeakDetection > 0)
    {
      m_Peaks.clear();

      float   mean = 0;
      float   sum_of_sq = 0;
      for (i = 0; i < n; i++)
      {
        mean += m_fftbuf[i + xmin];
        sum_of_sq += m_fftbuf[i + xmin] * m_fftbuf[i + xmin];
      }
      mean /= n;
      float stdev= sqrt(sum_of_sq / n - mean * mean );

      int lastPeak = -1;
      for (i = 0; i < n; i++)
      {
        //m_PeakDetection times the std over the mean or better than current peak
        float d = (lastPeak == -1) ? (mean - m_PeakDetection * stdev) :
          m_fftbuf[lastPeak + xmin];

        if (m_fftbuf[i + xmin] < d)
          lastPeak=i;

        if (lastPeak != -1 &&
            (i - lastPeak > PEAK_H_TOLERANCE || i == n-1))
        {
          m_Peaks.insert(lastPeak + xmin, m_fftbuf[lastPeak + xmin]);
          painter2.drawEllipse(lastPeak + xmin - 5,
              m_fftbuf[lastPeak + xmin] - 5, 10, 10);
          lastPeak = -1;
        }
      }
    }

    // Peak hold
    if (m_PeakHoldActive)
    {
      for (i = 0; i < n; i++)
      {
        if(!m_PeakHoldValid || m_fftbuf[i] < m_fftPeakHoldBuf[i])
          m_fftPeakHoldBuf[i] = m_fftbuf[i];

        LineBuf[i].setX(i + xmin);
        LineBuf[i].setY(m_fftPeakHoldBuf[i + xmin]);
      }
      painter2.setPen(m_PeakHoldColor);
      painter2.drawPolyline(LineBuf, n);

      m_PeakHoldValid = true;
    }

    painter2.end();

  }

  // trigger a new paintEvent
  update();
}

/**
 * Set new FFT data.
 * @param fftData Pointer to the new FFT data used on the pandapter.
 * @param wfData Pointer to the FFT data used in the waterfall.
 * @param size The FFT size.
 *
 * This method can be used to set different FFT data set for the pandapter and the
 * waterfall.
 */

void Waterfall::setNewFftData(
    float *fftData,
    float *wfData,
    int size,
    QDateTime const &t,
    bool looped)
{
  /** FIXME **/
  if (!m_Running)
    m_Running = true;

  if (looped) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText =
      m_lastFft.toLocalTime().toString("hh:mm:ss.zzz")
      + " - "
      + t.toLocalTime().toString("hh:mm:ss.zzz");
    ts.utcTimeStampText =
      m_lastFft.toUTC().toString("hh:mm:ss.zzzZ")
      + " - "
      + t.toUTC().toString("hh:mm:ss.zzzZ");
    ts.marker = true;

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  m_wfData = wfData;
  m_fftData = fftData;
  m_fftDataSize = size;
  m_lastFft = t;

  if (m_tentativeCenterFreq != 0) {
    m_tentativeCenterFreq = 0;
    m_DrawOverlay = true;
  }

  if (m_TimeStampCounter >= m_TimeStampSpacing) {
    TimeStamp ts;

    ts.counter = m_TimeStampCounter;
    ts.timeStampText = t.toLocalTime().toString("hh:mm:ss.zzz");
    ts.utcTimeStampText = t.toUTC().toString("hh:mm:ss.zzzZ");

    m_TimeStamps.push_front(ts);
    m_TimeStampCounter = 0;
  }

  draw();
}
