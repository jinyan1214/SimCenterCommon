#include "FronteraMachine.h"
#include <SC_IntLineEdit.h>

#include <QGridLayout>
#include <QLabel>
#include <QJsonObject>
#include <QDebug>


FronteraMachine::FronteraMachine()
  :TapisMachine()
{

  //
  // create widgets, setting min and max values
  //

  numCPU = new SC_IntLineEdit(QString("nodeCount"), 1, 1, 2048);
  numCPU->setText("1");

  numProcessors = new SC_IntLineEdit(QString("coresPerNode"),1, 1, 56);
  numProcessors->setText("56");

  runTime = new SC_IntLineEdit(QString("maxMinutes"),20, 1,1440);
  runTime->setText("20");

  //  runtimeLineEdit->setToolTip(tr("Run time limit on running Job (minutes). Job will be stopped if while running it exceeds this"));
  
  //
  // add widgets to a QGrid Layout
  //
  
  QGridLayout *theLayout = new QGridLayout(this);

  theLayout->addWidget(new QLabel("Num Node:"), 0,0);
  theLayout->addWidget(numCPU, 0,1);          
  theLayout->addWidget(new QLabel("Num Processors Per Node:"), 1,0);
  theLayout->addWidget(numProcessors, 1,1);        
  theLayout->addWidget(new QLabel("Max Run Time (minutes):"),2,0);
  theLayout->addWidget(runTime,2,1);

  this->setLayout(theLayout);
}

FronteraMachine::~FronteraMachine()
{
  qDebug() << "FronteraMachine::Destructor";
}

bool
FronteraMachine::outputToJSON(QJsonObject &job)
{
    numCPU->outputToJSON(job);
    numProcessors->outputToJSON(job);
    runTime->outputToJSON(job);
    
    int ramPerNodeMB = 128000;    
    job["memoryMB"]= ramPerNodeMB;

    // figure out queue
    int nodeCount = numCPU->text().toInt();
    QString queue = "small";
    if (nodeCount > 2) {
      queue = "normal";
    } else if (nodeCount > 512) {
      queue = "large";
    }

    job["execSystemId"]=QString("frontera");    
    job["execSystemLogicalQueue"]=queue;
    return true;
}

int FronteraMachine::setNumTasks(int numTasks) {
  int cpuCount = numCPU->getInt();
  int numP = numProcessors->getInt();
  int minTasks = cpuCount * numP;

  if (minTasks > numTasks) {
    numP = numTasks/cpuCount;
    if (numP > 56)
      numP = 56;
    numProcessors->setText(QString::number(numP));
  }
  return 0;
}
