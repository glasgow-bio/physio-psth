/***************************************************************************
 *   Copyright (C) 2003 by Matthias H. Hennig                              *
 *   hennig@cn.stir.ac.uk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "psthplot.h"

#include <QTimerEvent>

PsthPlot::PsthPlot(double *xData, double *yData, int length, QWidget *parent) :
    QwtPlot(parent),
    xData(xData),
    yData(yData)
{
  // Assign a title
  setTitle("PSTH");
  setAxisTitle(QwtPlot::xBottom, "Time/ms");
  setAxisTitle(QwtPlot::yLeft, "Spikes/s");

  dataCurve = new QwtPlotCurve("PSTH");
  dataCurve->setRawSamples(xData, yData, length);
  dataCurve->attach(this);
  dataCurve->setPen( QPen(Qt::blue, 2) );
  dataCurve->setStyle(QwtPlotCurve::Steps);

  //long mY = insertLineMarker("", QwtPlot::yLeft);
  //setMarkerYPos(mY, 0.0);

  setAutoReplot(false);
}

void PsthPlot::setPsthLength(int length)
{
  dataCurve->setRawSamples(xData, yData, length);
}

void PsthPlot::startDisplay()
{
  currtimer=startTimer(150);
}

void PsthPlot::stopDisplay()
{
  killTimer(currtimer);
}

void PsthPlot::timerEvent(QTimerEvent *)
{
  replot();
}
