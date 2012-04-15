/***************************************************************************
 *   Copyright (C) 2003 by Matthias H. Hennig                              *
 *   hennig@cn.stir.ac.uk                                                  *
 *   Copyright (C) 2005 by Bernd Porr                                      *
 *   BerndPorr@f2s.com                                                     *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "physio_psth.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPrinter>
#include <QPrintDialog>
#include <QFileDialog>
#include <QTextStream>

class PrintFilter: public QwtPlotPrintFilter
{
public:
  PrintFilter() {}

  virtual QFont font(const QFont &f, Item, int) const
  {
    QFont f2 = f;
    f2.setPointSize(f.pointSize() * 0.75);
    return f2;
  }
};

MainWindow::MainWindow( QWidget *parent ) :
    QWidget(parent),
    adChannel(4),
    psthLength(1000),
    psthBinw(20),
    spikeThres(2500),
    psthOn(0),
    psthNumTrials(10),
    psthRecordMode(0),
    spikeDetected(false),
    time(0),
    beep(255),
    quiet(0),
    playSound(0)
{
  // initialize comedi
  const char *filename = "/dev/comedi0";

  /* open the device */
  if( (dev = comedi_open(filename)) == 0 )
  {
    comedi_perror(filename);
    exit(1);
  }

  numChannels = comedi_get_n_channels(dev, COMEDI_SUB_DEVICE);

  chanlist = new unsigned[numChannels];

  /* Set up channel list */
  for( int i=0; i<numChannels; i++ )
    chanlist[i] = CR_PACK(i, COMEDI_RANGE_ID, AREF_GROUND);

  //[FIX]: i think this one is expressed in nanoseconds 1e9
  int ret = comedi_get_cmd_generic_timed( dev,
                                          COMEDI_SUB_DEVICE,
                                          &comediCommand,
                                          numChannels,
                                          (int)(1e9/(SAMPLING_RATE/numChannels)) );

  if(ret < 0)
  {
    printf("comedi_get_cmd_generic_timed failed\n");
    exit(-1);
  }

  /* Modify parts of the command */
  comediCommand.chanlist = chanlist;
  comediCommand.stop_src = TRIG_NONE;
  comediCommand.stop_arg = 0;

  /* comedi_command_test() tests a command to see if the
   * trigger sources and arguments are valid for the subdevice.
   * If a trigger source is invalid, it will be logically ANDed
   * with valid values (trigger sources are actually bitmasks),
   * which may or may not result in a valid trigger source.
   * If an argument is invalid, it will be adjusted to the
   * nearest valid value.  In this way, for many commands, you
   * can test it multiple times until it passes.  Typically,
   * if you can't get a valid command in two tests, the original
   * command wasn't specified very well. */
  ret = comedi_command_test(dev, &comediCommand);

  if(ret < 0)
  {
    comedi_perror("comedi_command_test");
    exit(-1);
  }

  fprintf(stderr, "first test returned %d\n", ret);

  ret = comedi_command_test(dev, &comediCommand);
  if(ret < 0)
  {
    comedi_perror("comedi_command_test");
    exit(-1);
  }

  fprintf(stderr, "second test returned %d\n", ret);

  if(ret != 0)
  {
    fprintf(stderr,"Error preparing command\n");
    exit(-1);
  }

  /* start the command */
  ret = comedi_command(dev, &comediCommand);
  if(ret < 0)
  {
    comedi_perror("comedi_command");
    exit(1);
  }

  int subdev_flags = comedi_get_subdevice_flags(dev, COMEDI_SUB_DEVICE);

  if( (sigmaBoard = subdev_flags & SDF_LSAMPL) )
    readSize = sizeof(lsampl_t) * numChannels;
  else
    readSize = sizeof(sampl_t) * numChannels;

  //  Initialize data for plots
  for(int i=0; i<MAX_PSTH_LENGTH; i++)
  {
    xData[i] = i;     // time axis
    yData[i] = 0;
    timeData[i] = double(i)*psthBinw; // psth time axis
    spikeCountData[i] = 0;
    psthData[i] = 0;
  }

  // the gui, straight forward QT/Qwt
  resize(640,420);
  QHBoxLayout *mainLayout = new QHBoxLayout( this );

  QVBoxLayout *controlLayout = new QVBoxLayout;
  mainLayout->addLayout(controlLayout);

  QVBoxLayout *plotLayout = new QVBoxLayout;
  plotLayout->addStrut(400);
  mainLayout->addLayout(plotLayout);

  // two plots
  RawDataPlot = new DataPlot(xData, yData, psthLength, this);
  plotLayout->addWidget(RawDataPlot);
  RawDataPlot->show();

  plotLayout->addSpacing(20);

  MyPsthPlot = new PsthPlot(timeData, psthData, psthLength/psthBinw, this);
  plotLayout->addWidget(MyPsthPlot);
  MyPsthPlot->show();

  /*---- Buttons ----*/

  // AD group
  QGroupBox   *ADcounterGroup = new QGroupBox( "A/D Channel", this );
  QVBoxLayout *ADcounterLayout = new QVBoxLayout;

  ADcounterGroup->setLayout(ADcounterLayout);
  ADcounterGroup->setAlignment(Qt::AlignJustify);
  ADcounterGroup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
  controlLayout->addWidget( ADcounterGroup );

  QwtCounter *cntChannel = new QwtCounter(ADcounterGroup);
  cntChannel->setRange(0, numChannels-1, 1);
  cntChannel->setValue(adChannel);
  ADcounterLayout->addWidget(cntChannel);
  connect(cntChannel, SIGNAL(valueChanged(double)), SLOT(slotSetChannel(double)));

  QPushButton *toggleSound = new QPushButton(ADcounterGroup);
  toggleSound->setText("Sound on");
  toggleSound->setCheckable(true);
  ADcounterLayout->addWidget(toggleSound);
  connect(toggleSound, SIGNAL(clicked()), SLOT(slotSoundToggle()));


  // psth functions
  QGroupBox   *PSTHfunGroup  = new QGroupBox( "PSTH functions", this );
  QVBoxLayout *PSTHfunLayout = new QVBoxLayout;

  PSTHfunGroup->setLayout(PSTHfunLayout);
  PSTHfunGroup->setAlignment(Qt::AlignJustify);
  PSTHfunGroup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
  controlLayout->addWidget( PSTHfunGroup );

  averagePsth = new QPushButton(PSTHfunGroup);
  averagePsth->setText("linear average");
  averagePsth->setCheckable(true);
  PSTHfunLayout->addWidget(averagePsth);
  connect( averagePsth, SIGNAL(clicked(bool)), SLOT(slotAveragePsth(bool)) );

  triggerPsth = new QPushButton(PSTHfunGroup);
  triggerPsth->setText("PSTH on");
  triggerPsth->setCheckable(true);
  PSTHfunLayout->addWidget(triggerPsth);
  connect(triggerPsth, SIGNAL(clicked()), SLOT(slotTriggerPsth()));

  QPushButton *clearPsth = new QPushButton(PSTHfunGroup);
  clearPsth->setText("clear PSTH");
  PSTHfunLayout->addWidget(clearPsth);
  connect(clearPsth, SIGNAL(clicked()), SLOT(slotClearPsth()));

  QPushButton *savePsth = new QPushButton(PSTHfunGroup);
  savePsth->setText("save PSTH");
  PSTHfunLayout->addWidget(savePsth);
  connect(savePsth, SIGNAL(clicked()), SLOT(slotSavePsth()));

  QPushButton *printPsth = new QPushButton(PSTHfunGroup);
  printPsth->setText("print PSTH");
  PSTHfunLayout->addWidget(printPsth);
  connect(printPsth, SIGNAL(clicked()), SLOT(slotPrint()));


  // psth params
  QGroupBox   *PSTHcounterGroup = new QGroupBox( "PSTH parameters", this );
  QVBoxLayout *PSTHcounterLayout = new QVBoxLayout;

  PSTHcounterGroup->setLayout(PSTHcounterLayout);
  PSTHcounterGroup->setAlignment(Qt::AlignJustify);
  PSTHcounterGroup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
  controlLayout->addWidget( PSTHcounterGroup );

  QLabel *psthLengthLabel = new QLabel("PSTH Length", PSTHcounterGroup);
  PSTHcounterLayout->addWidget(psthLengthLabel);

  QwtCounter *cntSLength = new QwtCounter(PSTHcounterGroup);
  cntSLength->setNumButtons(2);
  cntSLength->setIncSteps(QwtCounter::Button1, 10);
  cntSLength->setIncSteps(QwtCounter::Button2, 100);
  cntSLength->setRange(1, MAX_PSTH_LENGTH, 1);
  cntSLength->setValue(psthLength);
  PSTHcounterLayout->addWidget(cntSLength);
  connect(cntSLength, SIGNAL(valueChanged(double)), SLOT(slotSetPsthLength(double)));

  QLabel *binwidthLabel = new QLabel("PSTH Binwidth", PSTHcounterGroup);
  PSTHcounterLayout->addWidget(binwidthLabel);

  cntBinw = new QwtCounter(PSTHcounterGroup);
  cntBinw->setNumButtons(2);
  cntBinw->setIncSteps(QwtCounter::Button1, 1);
  cntBinw->setIncSteps(QwtCounter::Button2, 10);
  cntBinw->setRange(1, 100, 1);
  cntBinw->setValue(psthBinw);
  PSTHcounterLayout->addWidget(cntBinw);
  connect(cntBinw, SIGNAL(valueChanged(double)), SLOT(slotSetPsthBinw(double)));

  QLabel *thresholdLabel = new QLabel("Spike Threshold", PSTHcounterGroup);
  PSTHcounterLayout->addWidget(thresholdLabel);

  cntSpikeT = new QwtCounter(PSTHcounterGroup);
  cntSpikeT->setNumButtons(3);
  cntSpikeT->setIncSteps(QwtCounter::Button1, 1);
  cntSpikeT->setIncSteps(QwtCounter::Button2, 10);
  cntSpikeT->setIncSteps(QwtCounter::Button3, 100);
  cntSpikeT->setRange(0, 4095, 1);
  cntSpikeT->setValue(spikeThres);
  PSTHcounterLayout->addWidget(cntSpikeT);
  connect(cntSpikeT, SIGNAL(valueChanged(double)), SLOT(slotSetSpikeThres(double)));


  // psth recording
  QGroupBox   *PSTHrecGroup  = new QGroupBox( "PSTH recording", this );
  QVBoxLayout *PSTHrecLayout = new QVBoxLayout;

  PSTHrecGroup->setLayout(PSTHrecLayout);
  PSTHrecGroup->setAlignment(Qt::AlignJustify);
  PSTHrecGroup->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
  controlLayout->addWidget( PSTHrecGroup );

  QLabel *numTrialsLabel = new QLabel("Number of Trials", PSTHrecGroup);
  PSTHrecLayout->addWidget(numTrialsLabel);

  QwtCounter *cntTrials = new QwtCounter(PSTHrecGroup);
  cntTrials->setNumButtons(3);
  cntTrials->setIncSteps(QwtCounter::Button1, 1);
  cntTrials->setIncSteps(QwtCounter::Button2, 10);
  cntTrials->setIncSteps(QwtCounter::Button3, 100);
  cntTrials->setRange(0, 9000, 1);
  cntTrials->setValue(psthNumTrials);
  PSTHrecLayout->addWidget(cntTrials);
  connect(cntTrials, SIGNAL(valueChanged(double)), SLOT(slotSetNumTrials(double)));

  QPushButton *startRecPsthb = new QPushButton(PSTHrecGroup);
  startRecPsthb->setText("start Recording");
  PSTHrecLayout->addWidget(startRecPsthb);
  connect(startRecPsthb, SIGNAL(clicked()), SLOT(slotStartPsthRec()));


  // Generate timer event every 50ms
  (void)startTimer(50);

}

MainWindow::~MainWindow()
{
  delete[] chanlist;
}

void MainWindow::slotPrint()
{
  // print the PSTH plot
  QPrinter printer;

  QString docName = MyPsthPlot->title().text();
  if ( docName.isEmpty() )
  {
    docName.replace (QRegExp (QString::fromLatin1 ("\n")), tr (" -- "));
    printer.setDocName (docName);
  }

  printer.setOrientation(QPrinter::Landscape);

  QPrintDialog dialog(&printer, this);

  if( dialog.exec() == QDialog::Accepted )
    MyPsthPlot->print(printer, PrintFilter());
}

void MainWindow::slotSoundToggle()
{
  if(playSound == 0)
    playSound = 1;
  else
    playSound = 0;
}

void MainWindow::slotStartPsthRec()
{
  for(int i=0; i<psthLength/psthBinw; i++) {
    psthData[i] = 0;
    spikeCountData[i] = 0;
  }
  psthRecordMode = 1;
  psthActTrial = 0;
  psthOn = 1;
  time = 0;
  MyPsthPlot->startDisplay();
}

void MainWindow::slotSetNumTrials(double n)
{
  psthNumTrials = (int) n;
}

void MainWindow::slotSavePsth()
{
  QString fileName = QFileDialog::getSaveFileName();

  if( !fileName.isNull() )
  {
    QFile file(fileName);

    if( file.open(QIODevice::WriteOnly | QFile::Truncate) )
    {
      QTextStream out(&file);

      for(int i=0; i<psthLength/psthBinw; i++)
        out << timeData[i] << "\t" << psthData[i] << "\n";

      file.close();
    }
    else
    {
      // TODO: warning box
    }
  }
}

void MainWindow::slotClearPsth()
{
  time = 0;
  for(int i=0; i<psthLength/psthBinw; i++) {
    psthData[i] = 0;
    spikeCountData[i] = 0;
  }
  spikeDetected = false;
  psthActTrial = 0;
  MyPsthPlot->replot();
}

void MainWindow::slotTriggerPsth()
{
  if(psthOn == 0)
    {
      MyPsthPlot->startDisplay();
      psthOn = 1;
      psthActTrial = 0;
      time = 0;
    }
  else
    {
      MyPsthPlot->stopDisplay();
      psthOn = 0;
      psthActTrial = 0;
      spikeDetected = false;
    }
}

void MainWindow::slotSetChannel(double c)
{
  adChannel = (int)c;
  //RawDataPlot->setChannel(adChannel);
  spikeDetected = false;
}

void MainWindow::slotSetPsthLength(double l)
{
  psthLength = (int)l;

  for(int i=0; i<psthLength/psthBinw; i++) {
    psthData[i] = 0;
    spikeCountData[i] = 0;
    timeData[i] = double(i)*psthBinw;
  }
  spikeDetected = false;
  psthActTrial = 0;
  time = 0;

  RawDataPlot->setPsthLength((int) l);
  MyPsthPlot->setPsthLength(psthLength/psthBinw);
}

void MainWindow::slotSetPsthBinw(double b)
{
  psthBinw = (int)b;
  for(int i=0; i<psthLength/psthBinw; i++) {
    psthData[i] = 0;
    spikeCountData[i] = 0;
    timeData[i] = double(i)*psthBinw;
  }
  spikeDetected = false;
  psthActTrial = 0;
  time = 0;
  MyPsthPlot->setPsthLength(psthLength/psthBinw);
}

void MainWindow::slotSetSpikeThres(double t)
{
  spikeThres = (int)t;
  //RawDataPlot->setSpikeThres(spikeThres);
  spikeDetected = false;
}

void MainWindow::slotAveragePsth(bool checked)
{
  if( checked )
  {
    cntBinw->setEnabled(false);
    cntSpikeT->setEnabled(false);
    MyPsthPlot->setYaxisLabel("Averaged Data");
    triggerPsth->setText("Averaging on");
    psthBinw = 1;
    cntBinw->setValue(psthBinw);
  }
  else
  {
    cntBinw->setEnabled(true);
    cntSpikeT->setEnabled(true);
    MyPsthPlot->setYaxisLabel("Spikes/s");
    triggerPsth->setText("PSTH on");
  }
}

void MainWindow::timerEvent(QTimerEvent *)
{
  unsigned char buffer[readSize];

  while( comedi_get_buffer_contents(dev, COMEDI_SUB_DEVICE) > 0 )
  {
    if( read(comedi_fileno(dev), buffer, readSize) == 0 )
    {
      printf("Error: end of Aquisition\n");
      exit(1);
    }

    memmove( yData, yData+1, (psthLength - 1) * sizeof(yData[0]) );

    double &yNew = yData[psthLength-1];

    if( sigmaBoard )
      yNew = ((lsampl_t *)buffer)[adChannel];
    else
      yNew = ((sampl_t *)buffer)[adChannel];


    int trialIndex = time % psthLength;

    if( averagePsth->isChecked() && psthOn )
    {
      spikeCountData[trialIndex] += yNew;

      psthData[trialIndex] = spikeCountData[trialIndex] / (time/psthLength + 1);
    }
    else if( !spikeDetected && yNew>spikeThres )
    {
      if(playSound != 0)
      {
        // click once
        // crashes when artsd is running
        sounddev = fopen("/dev/dsp","w");
        fwrite(&beep, 1, 1, sounddev);
        fwrite(&quiet, 1, 1, sounddev);
        fclose(sounddev);
      }
      if(psthOn)
      {
        int psthIndex = trialIndex / psthBinw;

        spikeCountData[psthIndex] += 1;

        psthData[psthIndex] = ( spikeCountData[psthIndex]*1000 ) / ( psthBinw * (time/psthLength + 1) );

        spikeDetected = true;
      }
    }
    else if( yNew < spikeThres )
    {
      spikeDetected = false;
    }
    
    if( trialIndex == 0 )
      psthActTrial += 1;
    
    if( psthRecordMode && psthActTrial == psthNumTrials )
    {
      psthRecordMode = 0;
      psthOn = 0;
      MyPsthPlot->stopDisplay();
    }

    ++time;
  }
  
  RawDataPlot->replot();
}


