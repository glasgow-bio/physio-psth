/***************************************************************************
 *   Copyright (C) 2003 by Matthias H. Hennig                              *
 *   hennig@cn.stir.ac.uk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef DATAPLOT_H
#define DATAPLOT_H

#include <qwt/qwt_plot.h>
#include <qwt/qwt_plot_curve.h>

/// this plot shows the raw input data (spikes or membrane potential)
class DataPlot : public QwtPlot
{
public:

  DataPlot(const double *xData, const double *yData, int length, QWidget *parent = 0);
  void setPsthLength(int length);
  
private:
  const double *xData, *yData;

  // number of data points
  int psthLength;
  // curve object
  QwtPlotCurve *dataCurve;
};

#endif
