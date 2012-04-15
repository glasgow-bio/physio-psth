/***************************************************************************
 *   Copyright (C) 2003 by Matthias H. Hennig                              *
 *   hennig@cn.stir.ac.uk                                                  *
 *   Copyright (C) 2005 by Bernd Porr                                      *
 *   BerndPorr@f2s.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef PHYSIO_PSTH_H
#define PHYSIO_PSTH_H

#include <QWidget>
#include <QPushButton>

#include <comedilib.h>
#include <qwt/qwt_counter.h>

#include "psthplot.h"
#include "dataplot.h"


// maximal length of the PSTH (for memory alloctaion)
#define MAX_PSTH_LENGTH 5000

#define SAMPLING_RATE 8000 // 8kHz

#define FILTER_FREQU 50 // filter out 50Hz noise

#define COMEDI_SUB_DEVICE  0
#define COMEDI_RANGE_ID    0    /* +/- 4V */


class MainWindow : public QWidget
{
  Q_OBJECT

  // show the raw serai data here
  DataPlot *RawDataPlot;
  // here the PSTH will be shown
  PsthPlot *MyPsthPlot;

  // channel number for the seari device
  int adChannel;
  // length of the PSTH, this is the length on one trial
  int psthLength;
  // bin width for the PSTH
  int psthBinw;
  // threshold for a spike
  int spikeThres;

  // boo, activate/deactivate the psth plot
  int psthOn;

  // for the PSTH 'record mode':
  // numer of repetitions
  int psthNumTrials;
  // bool, indicates active recording mode
  int psthRecordMode;
  // count trials while recording
  int psthActTrial;

  // bool, set when a spike is detected and the activity has not
  // gone back to resting potential
  bool spikeDetected;

  // data
  double xData[MAX_PSTH_LENGTH], yData[MAX_PSTH_LENGTH];
  // PSTH, t is time, p is spike count, psth is spikes/sec
  double timeData[MAX_PSTH_LENGTH], spikeCountData[MAX_PSTH_LENGTH], psthData[MAX_PSTH_LENGTH];

  // serai file desc
  int usbFd;

  // time counter
  long int time;

  // click
  FILE *sounddev;
  char beep;
  char quiet;
  int playSound;

  comedi_cmd comediCommand;

  /**
   * file descriptor for /dev/comedi0
   **/
  comedi_t *dev;
  size_t   readSize;
  bool     sigmaBoard;

  int numChannels;
  unsigned *chanlist;

  QPushButton *averagePsth;
  QwtCounter *cntBinw;
  QwtCounter *cntSpikeT;
  QPushButton *triggerPsth;

private slots:

  // actions:
  void slotPrint();
  void slotClearPsth();
  void slotTriggerPsth();
  void slotSetChannel(double c);
  void slotSetPsthLength(double l);
  void slotSetPsthBinw(double b);
  void slotSetSpikeThres(double t);
  void slotSavePsth();
  void slotSetNumTrials(double);
  void slotStartPsthRec();
  void slotSoundToggle();
  void slotAveragePsth(bool checked);

protected:

  /// timer to read out the data
  virtual void timerEvent(QTimerEvent *e);

public:

  MainWindow( QWidget *parent=0 );
  ~MainWindow();

};

#endif
