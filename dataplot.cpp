/***************************************************************************
 *   Copyright (C) 2003 by Matthias H. Hennig                              *
 *   hennig@cn.stir.ac.uk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "dataplot.h"

DataPlot::DataPlot(const double *xData, const double *yData, int length, QWidget *parent) :
    QwtPlot(parent),
    xData(xData),
    yData(yData)
{
  setTitle("Raw Data");
  setAxisTitle(QwtPlot::xBottom, "Time/ms");
  setAxisTitle(QwtPlot::yLeft, "A/D Value");
  //setAxisScale(QwtPlot::yLeft, 2000, 5000);

  // Insert new curve for raw data
  dataCurve = new QwtPlotCurve("Raw Data");
  dataCurve->setPen( QPen(Qt::red, 2) );
  dataCurve->setRawData(xData, yData, length);
  dataCurve->attach(this);

  //long mY = insertLineMarker("", QwtPlot::yLeft);
  //setMarkerYPos(mY, 0.0);
}

void DataPlot::setPsthLength(int length)
{
  psthLength = length;
  dataCurve->setRawData(xData, yData, psthLength);
  replot();
}

