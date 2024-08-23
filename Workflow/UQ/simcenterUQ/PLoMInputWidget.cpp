/* *****************************************************************************
Copyright (c) 2016-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS 
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, 
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */

// Written: fmckenna, kuanshi

#include <PLoMInputWidget.h>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QLabel>
#include <QValidator>
#include <QJsonObject>
#include <QPushButton>
#include <QFileDialog>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>

#include <iostream>
#include <fstream>
#include <regex>
#include <iterator>
#include <string>
#include <sstream>
//#include <InputWidgetParameters.h>
//#include <InputWidgetEDP.h>
//#include <InputWidgetFEM.h>
#include <QButtonGroup>
#include <QRadioButton>
#include <QStackedWidget>
#include <QJsonDocument>
#include <QJsonArray>
#include <QComboBox>
#include <QDebug>

#include "SC_DoubleLineEdit.h"
#include "SC_FileEdit.h"
#include "SC_CheckBox.h"
#include "SimCenterIntensityMeasureWidget.h"
#include "SimCenterIntensityMeasureCombo.h"
#include "SimCenterUnitsCombo.h"

PLoMInputWidget::PLoMInputWidget(QWidget *parent)
    : UQ_Method(parent)
{
    auto layout = new QVBoxLayout();
    int wid = 0; // widget id

    //First we need to add type radio buttons
    m_typeButtonsGroup = new QButtonGroup(this);
    QRadioButton* rawDataRadioButton = new QRadioButton(tr("Raw Data"));
    QRadioButton* preTrainRadioButton = new QRadioButton(tr("Pre-trained Model"));
    m_typeButtonsGroup->addButton(rawDataRadioButton, 0);
    m_typeButtonsGroup->addButton(preTrainRadioButton, 1);
    QWidget* typeGroupBox = new QWidget(this);
    typeGroupBox->setContentsMargins(0,0,0,0);
    typeGroupBox->setStyleSheet("QGroupBox { font-weight: normal;}");
    QHBoxLayout* typeLayout = new QHBoxLayout(typeGroupBox);
    typeGroupBox->setLayout(typeLayout);
    typeLayout->addWidget(rawDataRadioButton);
    typeLayout->addWidget(preTrainRadioButton);
    typeLayout->addStretch();
    layout->addWidget(typeGroupBox,wid++);

    // input data widget
    rawDataGroup = new QWidget(this);
    rawDataGroup->setMaximumWidth(800);
    QGridLayout* rawDataLayout = new QGridLayout(rawDataGroup);
    rawDataGroup->setLayout(rawDataLayout);
    preTrainGroup = new QWidget(this);
    preTrainGroup->setMaximumWidth(800);
    QGridLayout* preTrainLayout = new QGridLayout(preTrainGroup);
    preTrainGroup->setLayout(preTrainLayout);
    // Create Input LineEdit
    inpFileDir = new QLineEdit();
    QPushButton *chooseInpFile = new QPushButton("Choose");
    connect(chooseInpFile, &QPushButton::clicked, this, [=](){
        inpFileDir->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"", "All files (*.*)"));
        this->parseInputDataForRV(inpFileDir->text());
    });
    inpFileDir->setReadOnly(true);
    inpFileDir->setMaximumWidth(600);
    rawDataLayout->addWidget(new QLabel("Training Data File: Input"),0,0);
    rawDataLayout->addWidget(inpFileDir,0,1,1,3);
    rawDataLayout->addWidget(chooseInpFile,0,4);
    // Create Output LineEdit
    outFileDir = new QLineEdit();
    outFileDir->setMaximumWidth(600);
    chooseOutFile = new QPushButton("Choose");
    connect(chooseOutFile, &QPushButton::clicked, this, [=](){
        outFileDir->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"", "All files (*.*)"));
        this->parseOutputDataForQoI(outFileDir->text());
    });
    outFileDir->setReadOnly(true);
    rawDataLayout->addWidget(new QLabel("Training Data File: Output"),1,0);
    rawDataLayout->addWidget(outFileDir,1,1,1,3);
    rawDataLayout->addWidget(chooseOutFile,1,4);
    errMSG=new QLabel("Unrecognized file format");
    errMSG->setStyleSheet({"color: red"});
    rawDataLayout->addWidget(errMSG,2,1);
    errMSG->hide();

    inpFileDir2 = new QLineEdit();
    QPushButton *chooseInpFile2 = new QPushButton("Choose");
    connect(chooseInpFile2, &QPushButton::clicked, this, [=](){
        inpFileDir2->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"", "h5 files (*.h5)"));
        this->parsePretrainedModelForRVQoI(inpFileDir2->text());
    });
    inpFileDir2->setReadOnly(true);
    preTrainLayout->addWidget(new QLabel("Training Data File: Pretrained Model"),0,0);
    preTrainLayout->addWidget(inpFileDir2,0,1,1,3);
    preTrainLayout->addWidget(chooseInpFile2,0,4);

    //We will add stacked widget to switch between raw data and trained model
    m_stackedWidgets = new QStackedWidget(this);
    m_stackedWidgets->addWidget(rawDataGroup);
    m_stackedWidgets->addWidget(preTrainGroup);
    m_typeButtonsGroup->button(0)->setChecked(true);
    m_stackedWidgets->setCurrentIndex(0);

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(m_typeButtonsGroup, &QButtonGroup::idReleased, [this](int id)
    {
        if(id == 0) {
            m_typeButtonsGroup->button(0)->setChecked(true);
            m_stackedWidgets->setCurrentIndex(0);
            preTrained = false;
        }
        else if (id == 1) {
            m_typeButtonsGroup->button(1)->setChecked(true);
            m_stackedWidgets->setCurrentIndex(1);
            preTrained = true;
        }
    });
#else
    connect(m_typeButtonsGroup, QOverload<int>::of(&QButtonGroup::buttonReleased),this, [this](int id)
    {
        if(id == 0) {
            m_typeButtonsGroup->button(0)->setChecked(true);
            m_stackedWidgets->setCurrentIndex(0);
            preTrained = false;
        }
        else if (id == 1) {
            m_typeButtonsGroup->button(1)->setChecked(true);
            m_stackedWidgets->setCurrentIndex(1);
            preTrained = true;
        }
    });
#endif

    layout->addWidget(m_stackedWidgets,wid++);

    // create widget for new sample ratio
    newSampleRatioWidget = new QWidget();
    newSampleRatioWidget->setMaximumWidth(400);
    QGridLayout* nsrLayout = new QGridLayout(newSampleRatioWidget);
    newSampleRatioWidget->setLayout(nsrLayout);
    ratioNewSamples = new QLineEdit();
    ratioNewSamples->setText(tr("5"));
    ratioNewSamples->setValidator(new QIntValidator);
    ratioNewSamples->setToolTip("The ratio between the number of new realizations and the size of original sample. \nIf \"0\" is given, the PLoM model is trained without new predictions");
    ratioNewSamples->setMaximumWidth(150);
    QLabel *newSNR = new QLabel("New Sample Number Ratio");
    nsrLayout->addWidget(newSNR, 0, 0);
    nsrLayout->addWidget(ratioNewSamples, 0, 1);
    layout->addWidget(newSampleRatioWidget, wid++);

    // Create Advanced options
    // advanced option widget
    QWidget *advOptGroup = new QWidget(this);
    QVBoxLayout* advOptLayout = new QVBoxLayout(advOptGroup);
    advOptGroup->setLayout(advOptLayout);
    int widd = 0;

    QHBoxLayout *advComboLayout = new QHBoxLayout();
    theAdvancedCheckBox = new QCheckBox();
    theAdvancedTitle=new QLabel("Advanced Options");
    theAdvancedTitle->setStyleSheet("font-weight: bold; color: gray");
    //advOptLayout->addWidget(theAdvancedTitle, widd, 1, Qt::AlignBottom);
    //advOptLayout->addWidget(theAdvancedCheckBox, widd, 0, Qt::AlignBottom);
    advComboLayout->addWidget(theAdvancedTitle);
    advComboLayout->addWidget(theAdvancedCheckBox);
    /***
    // create advanced combobox
    theAdvancedComboBox = new QComboBox();
    theAdvancedComboBox->addItem(tr("General"));
    theAdvancedComboBox->addItem(tr("KDE"));
    theAdvancedComboBox->addItem(tr("Constraints"));
    theAdvancedComboBox->setCurrentIndex(0);
    theAdvancedComboBox->setVisible(false);
    advComboLayout->addWidget(theAdvancedComboBox);
    ***/
    advComboLayout->addStretch();
    advOptLayout->addLayout(advComboLayout, widd++);

    // division line
    lineA = new QFrame;
    lineA->setFrameShape(QFrame::HLine);
    lineA->setFrameShadow(QFrame::Sunken);
    lineA->setMaximumWidth(300);
    advOptLayout->addWidget(lineA);
    lineA->setVisible(false);    

    // tab widget for adv. options
    advComboWidget = new QTabWidget(this);
    advOptLayout->addWidget(advComboWidget);
    advComboWidget->setVisible(false);
    advComboWidget->setStyleSheet("QTabBar {font-size: 10pt}");
    advComboWidget->setContentsMargins(0, 0, 0, 0);
    // adv. opt. general widget
    advGeneralWidget = new QWidget();
    //advGeneralWidget->setMaximumWidth(800);
    QGridLayout* advGeneralLayout = new QGridLayout(advGeneralWidget);
    advGeneralWidget->setLayout(advGeneralLayout);
    // Log transform
    theLogtLabel=new QLabel("Log-space Transform");
    theLogtLabel2=new QLabel("     (only if data always positive)");
    theLogtCheckBox = new QCheckBox();
    advGeneralLayout->addWidget(theLogtLabel, 0, 0);
    advGeneralLayout->addWidget(theLogtLabel2, 0, 1,1,-1,Qt::AlignLeft);
    advGeneralLayout->addWidget(theLogtCheckBox, 0, 1);
    theLogtLabel->setVisible(false);
    theLogtLabel2->setVisible(false);
    theLogtCheckBox->setVisible(false);
    // random seed
    randomSeed = new QLineEdit();
    randomSeed->setText(tr("10"));
    randomSeed->setValidator(new QIntValidator);
    randomSeed->setToolTip("Random Seed Number");
    randomSeed->setMaximumWidth(150);
    newRandomSeed = new QLabel("Random Seed");
    advGeneralLayout->addWidget(newRandomSeed, 1, 0);
    advGeneralLayout->addWidget(randomSeed, 1, 1);
    randomSeed->setVisible(false);
    newRandomSeed->setVisible(false);
    // pca tolerance
    epsilonPCA = new QLineEdit();
    epsilonPCA->setText(tr("0.0001"));
    epsilonPCA->setValidator(new QDoubleValidator);
    epsilonPCA->setToolTip("PCA Tolerance");
    epsilonPCA->setMaximumWidth(150);
    newEpsilonPCA = new QLabel("PCA Tolerance");
    advGeneralLayout->addWidget(newEpsilonPCA, 2, 0);
    advGeneralLayout->addWidget(epsilonPCA, 2, 1);
    epsilonPCA->setVisible(false);
    newEpsilonPCA->setVisible(false);
    advGeneralLayout->setColumnStretch(4,1);
    advGeneralLayout->setRowStretch(3,1);
    //
    advComboWidget->addTab(advGeneralWidget, "General");

    //
    // adv. opt. kde widget
    //
    advKDEWidget = new QWidget();
    //advKDEWidget->setMaximumWidth(800);
    QGridLayout* advKDELayout = new QGridLayout(advKDEWidget);
    advKDEWidget->setLayout(advKDELayout);
    advComboWidget->addTab(advKDEWidget, "Kernel Density Estimation");

    // kde smooth factor

    smootherKDE = new SC_DoubleLineEdit("smootherKDE",25,"KDE Smooth Factor");
    smootherKDE->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    smootherKDE->setMaximumWidth(100);
    //smootherKDE->setMinimumWidth(100);
    smootherKDE_check = new SC_CheckBox("smootherKDE_Customize","Customize",false);
    smootherKDE_path_label = new QLabel("Python File Path");
    smootherKDE_path = new SC_FileEdit("smootherKDE_path");
    smootherKDE_path->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    smootherKDE_path_warning = new QLabel("Please upload the Constraint Files using the third tab");
    smootherKDE_path_warning->setObjectName(QString::fromUtf8("info"));// get style sheet
    newSmootherKDE = new QLabel("KDE Smooth Factor");
    advKDELayout->addWidget(newSmootherKDE, 0, 0);
    advKDELayout->addWidget(smootherKDE, 0, 1);
    advKDELayout->addWidget(smootherKDE_check, 0, 2);
    advKDELayout->addWidget(smootherKDE_path_label, 0, 3);
    advKDELayout->addWidget(smootherKDE_path, 0, 4);
    advKDELayout->addWidget(smootherKDE_path_warning, 0, 5,1,-1);
    connect(smootherKDE_check, &QCheckBox::toggled, this, [=](bool tog){ smootherKDE_path_label->setVisible(tog); smootherKDE_path->setVisible(tog); smootherKDE->setDisabled(tog);; smootherKDE_path_warning->setVisible(tog);});
    smootherKDE_path_label->hide();
    smootherKDE_path->hide();
    smootherKDE_path_warning->hide();

    // diff. maps

    theDMLabel=new QLabel("Diffusion Maps");
    theDMCheckBox = new QCheckBox();
    advKDELayout->addWidget(theDMLabel, 1, 0);
    advKDELayout->addWidget(theDMCheckBox, 1, 1);
    theDMCheckBox->setChecked(true);

    // diff. maps tolerance

    tolKDE = new SC_DoubleLineEdit("kdeTolerance",0.1,"Diffusion Maps Tolerance: ratio between the cut-off eigenvalue and the first eigenvalue.");
    tolKDE->setMaximumWidth(100);
    tolKDE->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    //tolKDE->setMinimumWidth(100);
    //newTolKDE = new QLabel("Diff. Maps Tolerance");
    tolKDE_check = new SC_CheckBox("tolKDE_Customize","Customize",false);
    tolKDE_path = new SC_FileEdit("tolKDE_path");
    tolKDE_path->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    tolKDE_path_label = new QLabel("Python File Path");
    tolKDE_path_warning = new QLabel("Please upload the Constraint Files using the third tab");
    tolKDE_path_warning->setObjectName(QString::fromUtf8("info"));// get style sheet

    advKDELayout->addWidget(new QLabel("Diff. Maps Tolerance"), 2, 0);
    advKDELayout->addWidget(tolKDE, 2, 1);
    advKDELayout->addWidget(tolKDE_check, 2, 2);
    advKDELayout->addWidget(tolKDE_path_label, 2, 3);
    advKDELayout->addWidget(tolKDE_path, 2, 4);
    advKDELayout->addWidget(tolKDE_path_warning, 2, 5);
    connect(tolKDE_check, &QCheckBox::toggled, this, [=](bool tog){ tolKDE_path_label->setVisible(tog); tolKDE_path->setVisible(tog); tolKDE->setDisabled(tog);tolKDE_path_warning->setVisible(tog);});
    tolKDE_path_label->hide();
    tolKDE_path->hide();
    tolKDE->setDisabled(false);
    tolKDE_check->setDisabled(false);
    connect(theDMCheckBox,SIGNAL(toggled(bool)),this,SLOT(setDiffMaps(bool)));
    advKDELayout->setColumnStretch(6,1);
    advKDELayout->setRowStretch(3, 1);
    tolKDE_path_warning->hide();

    //
    // adv. opt. constraints widget
    //

    advConstraintsWidget = new QWidget();
    //advConstraintsWidget->setMaximumWidth(800);
    advConstraintsLayout = new QGridLayout(advConstraintsWidget);
    advConstraintsWidget->setLayout(advConstraintsLayout);
    //
    theConstraintsButton = new QCheckBox();
    theConstraintsLabel2 = new QLabel();
    theConstraintsLabel2->setText("Add constratins");
    constraintsPath = new QLineEdit();
    chooseConstraints = new QPushButton(tr("Choose"));
    chooseConstraints->setMaximumWidth(150);
    connect(chooseConstraints, &QPushButton::clicked, this, [=](){
        constraintsPath->setText(QFileDialog::getOpenFileName(this,tr("Open File"),"", "All files (*.*)"));
    });
    constraintsPath->setReadOnly(true);
    theConstraintsLabel1 = new QLabel();
    theConstraintsLabel1->setText("Constraints file (.py)");
    advConstraintsLayout->addWidget(theConstraintsButton,0,1,Qt::AlignTop);
    advConstraintsLayout->addWidget(theConstraintsLabel2,0,0,Qt::AlignTop);
    advConstraintsLayout->addWidget(theConstraintsLabel1,1,0,Qt::AlignTop);
    advConstraintsLayout->addWidget(constraintsPath,1,1,1,2,Qt::AlignTop);
    advConstraintsLayout->addWidget(chooseConstraints,1,3,Qt::AlignTop);
    advConstraintsLayout->setColumnStretch(4,1);
    constraintsPath->setVisible(false);
    theConstraintsLabel1->setVisible(false);
    theConstraintsLabel2->setVisible(false);
    chooseConstraints->setVisible(false);
    theConstraintsButton->setVisible(false);
    constraintsPath->setDisabled(1);
    chooseConstraints->setDisabled(1);
    constraintsPath->setStyleSheet("background-color: lightgrey;border-color:grey");
    connect(theConstraintsButton,SIGNAL(toggled(bool)),this,SLOT(setConstraints(bool)));
    // iterations when applying constraints
    numIter = new QLineEdit();
    numIter->setText(tr("50"));
    numIter->setValidator(new QIntValidator);
    numIter->setToolTip("Iteration Number");
    numIter->setMaximumWidth(150);
    numIterLabel = new QLabel("Iteration Number");
    advConstraintsLayout->addWidget(numIterLabel, 2, 0);
    advConstraintsLayout->addWidget(numIter, 2, 1);
    numIter->setVisible(false);
    numIterLabel->setVisible(false);
    numIter->setDisabled(1);
    numIter->setStyleSheet("background-color: lightgrey;border-color:grey");
    // iteration tol
    tolIter = new QLineEdit();
    tolIter->setText(tr("0.02"));
    tolIter->setValidator(new QDoubleValidator);
    tolIter->setToolTip("Iteration Tolerance");
    tolIter->setMaximumWidth(150);
    tolIterLabel = new QLabel("Iteration Tolerance");
    advConstraintsLayout->addWidget(tolIterLabel, 3, 0);
    advConstraintsLayout->addWidget(tolIter, 3, 1);
    tolIter->setVisible(false);
    tolIterLabel->setVisible(false);
    tolIter->setDisabled(1);
    tolIter->setStyleSheet("background-color: lightgrey;border-color:grey");

    SIMConstraintLabel = new QLabel("SIM");
    EVTConstraintLabel = new QLabel("EVT");
    constraintNameLabel = new QLabel("Constraint Name");
    IMConstraintLabel = new QLabel("        IM");
    // An ugly way to change the space between gridlayout columns
    PeriodsConstraintLabel = new QLabel("                                                                         Periods");

    //
    advComboWidget->addTab(advConstraintsWidget, "Constraints");

    // create the stacked widgets
    /***
    adv_stackedWidgets = new QStackedWidget(this);
    adv_stackedWidgets->addWidget(advGeneralWidget);
    adv_stackedWidgets->addWidget(advKDEWidget);
    adv_stackedWidgets->addWidget(advConstraintsWidget);
    adv_stackedWidgets->setCurrentIndex(0);
    connect(theAdvancedComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int id)
    {
        adv_stackedWidgets->setCurrentIndex(id);
    });
    advOptLayout->addWidget(adv_stackedWidgets);
    ***/
    advOptLayout->addStretch();

    layout->addWidget(advOptGroup, wid++);
    layout->addStretch();

    //
    // Finish
    //

    //layout->setRowStretch(wid, 1);
    //layout->setColumnStretch(6, 1);
    this->setLayout(layout);

    outFileDir->setDisabled(0);
    chooseOutFile->setDisabled(0);
    //chooseOutFile->setStyleSheet("background-color: lightgrey;border-color:grey");

    connect(theAdvancedCheckBox,SIGNAL(toggled(bool)),this,SLOT(doAdvancedSetup(bool)));
}


PLoMInputWidget::~PLoMInputWidget()
{

}


// SLOT function
void PLoMInputWidget::doAdvancedSetup(bool tog)
{
    if (tog) {
        theAdvancedTitle->setStyleSheet("font-weight: bold; color: black");
    } else {
        theAdvancedTitle->setStyleSheet("font-weight: bold; color: gray");
        theLogtCheckBox->setChecked(false);
    }

    //theAdvancedComboBox->setVisible(tog);
    advComboWidget->setVisible(tog);
    lineA->setVisible(tog);
    theLogtCheckBox->setVisible(tog);
    theLogtLabel->setVisible(tog);
    theLogtLabel2->setVisible(tog);
    theDMCheckBox->setVisible(tog);
    theDMLabel->setVisible(tog);
    epsilonPCA->setVisible(tog);
    newEpsilonPCA->setVisible(tog);
    smootherKDE->setVisible(tog);
    newSmootherKDE->setVisible(tog);
    tolKDE->setVisible(tog);
    //newTolKDE->setVisible(tog);
    randomSeed->setVisible(tog);
    newRandomSeed->setVisible(tog);
    constraintsPath->setVisible(tog);
    theConstraintsLabel1->setVisible(tog);
    theConstraintsLabel2->setVisible(tog);
    chooseConstraints->setVisible(tog);
    theConstraintsButton->setVisible(tog);
    numIter->setVisible(tog);
    numIterLabel->setVisible(tog);
    tolIter->setVisible(tog);
    tolIterLabel->setVisible(tog);
    for (int i = 0; i < constraintsLabelList.length(); i++){
        constraintsLabelList.at(i)->setVisible(tog);
        EVTButtonList.at(i)->setVisible(tog);
        SIMButtonList.at(i)->setVisible(tog);
    }
}


void PLoMInputWidget::setOutputDir(bool tog)
{
    if (tog) {
        outFileDir->setDisabled(0);
        chooseOutFile->setDisabled(0);
        chooseOutFile->setStyleSheet("color: white");
        //theFemWidget->setFEMforGP("GPdata");
        parseInputDataForRV(inpFileDir->text());
        parseOutputDataForQoI(outFileDir->text());
    } else {
        outFileDir->setDisabled(1);
        chooseOutFile->setDisabled(1);
        chooseOutFile->setStyleSheet("background-color: lightgrey;border-color:grey");
        //theEdpWidget->setGPQoINames(QStringList("") );
        outFileDir->setText(QString("") );
        //theFemWidget->setFEMforGP("GPmodel");
        //theFemWidget->femProgramChanged("OpenSees");
        //theEdpWidget->setGPQoINames(QStringList({}) );// remove GP RVs
        //theParameters->setGPVarNamesAndValues(QStringList({}));// remove GP RVs
    }
}

void PLoMInputWidget::setConstraints(bool tog)
{
    if (tog) {
        constraintsPath->setDisabled(0);
        chooseConstraints->setDisabled(0);
        constraintsPath->setStyleSheet("background-color: white");
        numIter->setStyleSheet("background-color: white");
        tolIter->setStyleSheet("background-color: white");
        numIter->setDisabled(0);
        tolIter->setDisabled(0);
        for (int i = 0; i < constraintsLabelList.length(); i++){
            constraintsLabelList.at(i)->setDisabled(0);
            SIMButtonList.at(i)->setDisabled(0);
            EVTButtonList.at(i)->setDisabled(0);
        }
    } else {
        constraintsPath->setDisabled(1);
        chooseConstraints->setDisabled(1);
        constraintsPath->setStyleSheet("background-color: lightgrey;border-color:grey");
        numIter->setStyleSheet("background-color: lightgrey;border-color:grey");
        tolIter->setStyleSheet("background-color: lightgrey;border-color:grey");
        numIter->setDisabled(1);
        tolIter->setDisabled(1);
        for (int i = 0; i < constraintsLabelList.length(); i++){
            constraintsLabelList.at(i)->setDisabled(1);
            SIMButtonList.at(i)->setDisabled(1);
            EVTButtonList.at(i)->setDisabled(1);
        }
    }
}

void PLoMInputWidget::setDiffMaps(bool tog)
{
    if (tog) {
        tolKDE->setDisabled(0);
        tolKDE_check->setDisabled(0);
        tolKDE_check->setStyleSheet("font-color: white");
    } else {
        tolKDE_check->setDisabled(1);
        tolKDE_check->setStyleSheet("font-color: lightgrey;border-color:grey");
        if (tolKDE_check->isChecked()) {
            tolKDE_check->setChecked(false); // uncheck
            //emit tolKDE_check->stateChanged(true); // uncheck
        }
        tolKDE->setDisabled(1);
    }
}

bool
PLoMInputWidget::outputToJSON(QJsonObject &jsonObj){

    bool result = true;

    if (m_typeButtonsGroup->button(0)->isChecked()) {
        jsonObj["preTrained"] = false;
        jsonObj["inpFile"]=inpFileDir->text();
        jsonObj["outFile"]=outFileDir->text();
    } else {
        jsonObj["preTrained"] = true;
        jsonObj["inpFile2"] = inpFileDir2->text();
    }


    jsonObj["outputData"]=true;

    jsonObj["newSampleRatio"]=ratioNewSamples->text().toInt();

    jsonObj["advancedOpt"]=theAdvancedCheckBox->isChecked();
    if (theAdvancedCheckBox->isChecked())
    {
        jsonObj["logTransform"]=theLogtCheckBox->isChecked();
        jsonObj["diffusionMaps"] = theDMCheckBox->isChecked();
        jsonObj["randomSeed"] = randomSeed->text().toInt();
        jsonObj["epsilonPCA"] = epsilonPCA->text().toDouble();
        //jsonObj["smootherKDE"] = smootherKDE->text().toDouble();
        smootherKDE->outputToJSON(jsonObj);
        smootherKDE_path->outputToJSON(jsonObj);
        smootherKDE_check->outputToJSON(jsonObj);
        //jsonObj["kdeTolerance"] = tolKDE->text().toDouble();
        tolKDE->outputToJSON(jsonObj);
        tolKDE_path->outputToJSON(jsonObj);
        tolKDE_check->outputToJSON(jsonObj);
        jsonObj["constraints"]= theConstraintsButton->isChecked();
        if (theConstraintsButton->isChecked()) {
            jsonObj["constraintsFile"] = constraintsPath->text();
            jsonObj["numIter"] = numIter->text().toInt();
            jsonObj["tolIter"] = tolIter->text().toDouble();
            //If raw data is input
            if (m_typeButtonsGroup->button(0)->isChecked()) {
                QJsonArray EVTConstraints;
                QJsonArray SIMConstraints;
                QJsonObject EVTConstraintObj;

                QJsonObject PSDConstraintObj;
                QJsonArray PSDPeriodsArray;
                bool includePSD = false;

                QJsonObject PSVConstraintObj;
                QJsonArray PSVPeriodsArray;
                bool includePSV = false;

                QJsonObject PSAConstraintObj;
                QJsonArray PSAPeriodsArray;
                bool includePSA = false;

                QJsonObject SaRatioConstraintObj;
                QJsonArray SaRatioPeriodsArray;
                bool includeSaRatio = false;

                for (int i = 0; i < buttonGroupList.length(); i++){
                    if (buttonGroupList.at(i)->button(0)->isChecked()){ // EVT
                        QJsonObject obj;
                        if(!imUnitComboList.at(i)->outputToJSON(obj)){
                            errorMessage(QString("The EVT constraint ") + inputRVnames.at(i) + QString(" is not properly defined."));
                        }
                        EVTConstraintObj[inputRVnames.at(i)] = obj;
//                        // If it is one of PSA, PSV, PSD or SaRatio
//                        if (obj.contains("PSA")) {
//                            PSAPeriodsArray.append(obj["Period"]);
//                            includePSA = true;
//                        } else if (obj.contains("PSV")){
//                            PSVPeriodsArray.append(obj["Period"]);
//                            includePSV = true;
//                        } else if (obj.contains("PSD")){
//                            PSDPeriodsArray.append(obj["Period"]);
//                            includePSD = true;
//                        } else if (obj.contains("sa")){
//                            SaRatioPeriodsArray.append(obj["Period"]);
//                            includeSaRatio = true;
//                        } else{
//                            // If not
//                            for (auto it = obj.begin(); it != obj.end(); ++it) {
//                                EVTConstraintObj.insert(it.key(), it.value());
//                            }
//                        }
                    } else if (buttonGroupList.at(i)->button(1)->isChecked()){
                        SIMConstraints.append(inputRVnames.at(i));
                    }
                }
//                if(includePSD){
//                    PSDConstraintObj["Unit"] = "=in";
//                    PSDConstraintObj["Periods"] = PSDPeriodsArray;
//                    EVTConstraintObj["Sa"] =  QJsonObject{{"PSA", PSDConstraintObj}}
//                }
//                PSAConstraintObj["Unit"] = "g";
//                PSAConstraintObj["Periods"] = PSAPeriodsArray;

//                PSVConstraintObj["Unit"] = "=in/sec";
//                PSVConstraintObj["Periods"] = PSVPeriodsArray;

                jsonObj["EVTConstraints"] = EVTConstraintObj;
                jsonObj["SIMConstraints"] = SIMConstraints;

            }
        }
    }
    jsonObj["parallelExecution"]=false;

    return result;    
}


int PLoMInputWidget::parseInputDataForRV(QString name1){

    // countColumn and save headers to inputRVnames
    // get number of columns
    std::ifstream inFile(name1.toStdString());
    // read lines of input searching for pset using regular expression
    std::string line;
    errMSG->hide();
    int numberOfColumns_pre = -100;
    bool header = true;
    while (std::getline(inFile, line)) {
        int  numberOfColumns=1;
        bool previousWasSpace=false;
        //for(int i=0; i<line.size(); i++){
        if (header){
            header = false;
            // Clear the inputRVnames
            inputRVnames.clear();
            // Remove existing constraints labals
            for(int i = 0; i < constraintsLabelList.length(); i++){
                advConstraintsLayout->removeWidget(constraintsLabelList.at(i));
                delete constraintsLabelList.at(i);
            }
            constraintsLabelList.clear();
            // Remove existing SIMButtonList
            for(int i = 0; i < SIMButtonList.length(); i++){
                advConstraintsLayout->removeWidget(SIMButtonList.at(i));
                delete SIMButtonList.at(i);
            }
            SIMButtonList.clear();

            // Remove existing EVTButtonList
            for(int i = 0; i < EVTButtonList.length(); i++){
                advConstraintsLayout->removeWidget(EVTButtonList.at(i));
                delete EVTButtonList.at(i);
            }
            EVTButtonList.clear();

            // Remove existing radio button grouplist
            for(int i = 0; i < buttonGroupList.length(); i++){
                delete buttonGroupList.at(i);
            }
            buttonGroupList.clear();

            // Remove the IM combo lists
            for(int i = 0; i < imComboList.length(); i++){
                delete imComboList.at(i);
            }
            imComboList.clear();
            for(int i = 0; i < unitComboList.length(); i++){
                delete unitComboList.at(i);
            }
            unitComboList.clear();
            for(int i = 0; i < imUnitComboList.length(); i++){
                delete imUnitComboList.at(i);
            }
            imUnitComboList.clear();


            // Remove the EVT and SIM labels
            advConstraintsLayout->removeWidget(EVTConstraintLabel);
            advConstraintsLayout->removeWidget(SIMConstraintLabel);
            advConstraintsLayout->removeWidget(IMConstraintLabel);
            advConstraintsLayout->removeWidget(PeriodsConstraintLabel);
            advConstraintsLayout->removeWidget(constraintNameLabel);

            // Parse the hearder line and add them to lists
            inputRVnames = parseLineCSV(QString(line.c_str()));
            foreach (QString rv, inputRVnames) {
                QRadioButton* SIMButton = new QRadioButton(this);
                QRadioButton* EVTButton = new QRadioButton(this);
                QButtonGroup* buttonGroup = new QButtonGroup(this);

                SIMButtonList.append(SIMButton);
                EVTButtonList.append(EVTButton);
                buttonGroup->addButton(SIMButton, 0);
                buttonGroup->addButton(EVTButton, 1);
                buttonGroupList.append(buttonGroup);
                constraintsLabelList.append(new QLabel(rv));

                // Connect radio group to slot to show IM combo widget
                connect(buttonGroup, SIGNAL(buttonClicked(int)), this, SLOT(onRadioButtonGroupClicked(int)));

                // create a im combo widget
                SimCenterIntensityMeasureCombo *imCombo = new SimCenterIntensityMeasureCombo(SimCenterEQ::IntensityMeasure::ALL,rv);
                QString unitName = "Unit"+ rv;
                SimCenterUnitsCombo *unitCombo = new SimCenterUnitsCombo(SimCenter::Unit::ALL,unitName);
                SimCenterIM *imUnitCombo = new SimCenterIM(imCombo, unitCombo);
                imUnitCombo->setCurrentIMtoPSA(); // Default Sa
                imUnitCombo->setLabelVisible(false);
                imUnitCombo->removeRadioButton();
                imComboList.append(imCombo);
                unitComboList.append(unitCombo);
                imUnitComboList.append(imUnitCombo);

            }
        }
        for(size_t i=0; i<line.size(); i++){
            if(line[i] == '%' || line[i] == '#'){ // ignore header
                numberOfColumns = numberOfColumns_pre;
                break;
            }
            if(line[i] == ' ' || line[i] == '\t' || line[i] == ','){
                if(!previousWasSpace)
                    numberOfColumns++;
                previousWasSpace = true;
            } else {
                previousWasSpace = false;
            }
        }
        if(previousWasSpace)// when there is a blank space at the end of each row
            numberOfColumns--;

        if (numberOfColumns_pre==-100)  // to pass header
        {
            numberOfColumns_pre=numberOfColumns;
            continue;
        }
        if (numberOfColumns != numberOfColumns_pre)// Send an error
        {
            errMSG->show();
            numberOfColumns_pre=0;
            break;
        }
    }
    // close file
    inFile.close();

    // Original parseInputDataForRV
    double numberOfColumns=numberOfColumns_pre;

    QStringList varNamesAndValues;
    for (int i=0;i<numberOfColumns;i++) {
        varNamesAndValues.append(QString("RV_column%1").arg(i+1));
        varNamesAndValues.append("nan");
    }
    //theParameters->setGPVarNamesAndValues(varNamesAndValues);
    numSamples=0;


    // Add parsed RV to constraints:
    advConstraintsLayout->addWidget(EVTConstraintLabel, 4, 0);
    advConstraintsLayout->addWidget(SIMConstraintLabel, 4, 1);
    advConstraintsLayout->addWidget(constraintNameLabel, 4, 2);
    advConstraintsLayout->addWidget(IMConstraintLabel, 4, 3);
    advConstraintsLayout->addWidget(PeriodsConstraintLabel, 4, 4);

//    QWidget* IMlabels = new QWidget();
//    QGridLayout* imUnitLayout = new QGridLayout();
//    imUnitLayout->addWidget(new QLabel("IM"),0,1);
//    imUnitLayout->addWidget(new QLabel(tr("Periods")),0,3);
//    imUnitLayout->setColumnStretch(0, 1);
//    imUnitLayout->setColumnStretch(1, 1);
//    imUnitLayout->setColumnStretch(3, 1);
//    IMlabels->setLayout(imUnitLayout);
//    advConstraintsLayout->addWidget(IMlabels, 4, 3);


    IMConstraintLabel->setVisible(false);
    PeriodsConstraintLabel->setVisible(false);
    for (int i = 0; i < inputRVnames.length(); i++){
        advConstraintsLayout->addWidget(SIMButtonList.at(i), 5+i, 0);
        advConstraintsLayout->addWidget(EVTButtonList.at(i), 5+i, 1);
        advConstraintsLayout->addWidget(constraintsLabelList.at(i), 5+i, 2);
//        advConstraintsLayout->addWidget(imUnitComboList.at(i), 5+i, 3, 1, 3);
        advConstraintsLayout->addWidget(imUnitComboList.at(i), 5+i, 3, 1, 2);
        imUnitComboList.at(i)->setVisible(false);

        if (theAdvancedCheckBox->isChecked()){
            SIMButtonList.at(i)->setVisible(true); // Only visible if adv opt checked
            EVTButtonList.at(i)->setVisible(true);
            constraintsLabelList.at(i)->setVisible(true);
        } else {
            SIMButtonList.at(i)->setVisible(false); // Only visible if adv opt checked
            EVTButtonList.at(i)->setVisible(false);
            constraintsLabelList.at(i)->setVisible(false);
        }

        if (theConstraintsButton->isChecked()){
            SIMButtonList.at(i)->setDisabled(0); // Only enabled if add constraint checked
            EVTButtonList.at(i)->setDisabled(0);
        } else {
            SIMButtonList.at(i)->setDisabled(1); // Only enabled if add constraint checked
            EVTButtonList.at(i)->setDisabled(1);
        }
    }
//    advConstraintsLayout->setColumnStretch(0, 1); // Column 2 will have a stretch factor of 1
//    advConstraintsLayout->setColumnStretch(1, 1);
//    advConstraintsLayout->setColumnStretch(2, 1);
    advConstraintsLayout->setColumnStretch(3, 3);
    advConstraintsLayout->setColumnStretch(4, 1);
//    advConstraintsLayout->setColumnStretch(4, 1);
    return 0;
}

void PLoMInputWidget::onRadioButtonGroupClicked(int EVTorSIM) {
    int numOfEVT = 0;
    for (int i = 0; i < buttonGroupList.length(); ++i) {
        int checkedID = buttonGroupList.at(i)->checkedId();
        if (checkedID==0){
            imUnitComboList.at(i)->setVisible(true);
            numOfEVT++;
        } else {
            imUnitComboList.at(i)->setVisible(false);
        }
    }
    if (numOfEVT>0) {
        IMConstraintLabel->setVisible(true);
        PeriodsConstraintLabel->setVisible(true);
    } else {
        IMConstraintLabel->setVisible(false);
        PeriodsConstraintLabel->setVisible(false);
    }

}

int PLoMInputWidget::parseOutputDataForQoI(QString name1){
    // get number of columns
    double numberOfColumns=countColumn(name1);
    QStringList qoiNames;
    for (int i=0;i<numberOfColumns;i++) {
        qoiNames.append(QString("QoI_column%1").arg(i+1));
    }
    //theEdpWidget->setGPQoINames(qoiNames);
    return 0;
}

int PLoMInputWidget::parsePretrainedModelForRVQoI(QString name1){

    // five tasks here:
    // 1. parse the JSON file of the pretrained model
    // 2. create RV
    // 3. create QoI
    // 4. check inpData file, inpFile.in
    // 5. check outFile file, outFile.in

    // look for the JSON file in the model directory
    QString fileName = name1;
    fileName.replace(".h5",".json");
    QFile jsonFile(fileName);
    if (!jsonFile.open(QFile::ReadOnly | QFile::Text)) {
        QString message = QString("Error: could not open file") + fileName;
        this->errorMessage(message);
        return 1;
    }
    // place contents of file into json object
    QString val;
    val=jsonFile.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject jsonObject = doc.object();
    // close file
    jsonFile.close();
    // check file contains valid object
    if (jsonObject.isEmpty()) {
        this->errorMessage("ERROR: file either empty or malformed JSON "+fileName);
        return 1;
    }
    this->statusMessage("Pretrained model JSON file loaded.");

    // create RV
    QJsonArray xLabels;
    if (jsonObject.contains("xlabels")) {
        xLabels = jsonObject["xlabels"].toArray();
    } else {
        this->errorMessage("ERROR: xlables are missing in "+fileName);
        return 1;
    }
    int numberOfColumns = xLabels.size();
    QStringList varNamesAndValues;
    for (int i=0;i<numberOfColumns;i++) {
        varNamesAndValues.append(xLabels[i].toString());
        varNamesAndValues.append("nan");
    }
    //theParameters->setGPVarNamesAndValues(varNamesAndValues);
    numSamples=0;
    this->statusMessage("RV created.");

    // create QoI
    QJsonArray yLabels;
    if (jsonObject.contains("ylabels")) {
        yLabels = jsonObject["ylabels"].toArray();
    } else {
        this->errorMessage("ERROR: ylables are missing in "+fileName);
        return 1;
    }
    numberOfColumns = yLabels.size();
    QStringList qoiNames;
    for (int i=0;i<numberOfColumns;i++) {
        qoiNames.append(yLabels[i].toString());
    }
    //theEdpWidget->setGPQoINames(qoiNames);
    this->statusMessage("QoI created.");

    // check inpFile
    QFileInfo fileInfo(fileName);
    QString path = fileInfo.absolutePath();
    QFile inpFile(path+QDir::separator()+"inpFile.in");
    if (!inpFile.open(QFile::ReadOnly | QFile::Text)) {
        QString message = QString("Error: could not open file") + inpFile.fileName();
        this->errorMessage(message);
        return 1;
    }
    inpFile.close();
    inpFileDir->setText(inpFile.fileName());

    // check outFile
    QFile outFile(path+QDir::separator()+"outFile.in");
    if (!outFile.open(QFile::ReadOnly | QFile::Text)) {
        QString message = QString("Error: could not open file") + outFile.fileName();
        this->errorMessage(message);
        return 1;
    }
    outFile.close();
    outFileDir->setText(outFile.fileName());
    this->statusMessage("Input data loaded.");

    // return
    return 0;
}

int PLoMInputWidget::countColumn(QString name1){
    // get number of columns
    std::ifstream inFile(name1.toStdString());
    // read lines of input searching for pset using regular expression
    std::string line;
    errMSG->hide();

    int numberOfColumns_pre = -100;
    while (getline(inFile, line)) {
        int  numberOfColumns=1;
        bool previousWasSpace=false;
        //for(int i=0; i<line.size(); i++){
        for(size_t i=0; i<line.size(); i++){
            if(line[i] == '%' || line[i] == '#'){ // ignore header
                numberOfColumns = numberOfColumns_pre;
                break;
            }
            if(line[i] == ' ' || line[i] == '\t' || line[i] == ','){
                if(!previousWasSpace)
                    numberOfColumns++;
                previousWasSpace = true;
            } else {
                previousWasSpace = false;
            }
        }
        if(previousWasSpace)// when there is a blank space at the end of each row
            numberOfColumns--;

        if (numberOfColumns_pre==-100)  // to pass header
        {
            numberOfColumns_pre=numberOfColumns;
            continue;
        }
        if (numberOfColumns != numberOfColumns_pre)// Send an error
        {
            errMSG->show();
            numberOfColumns_pre=0;
            break;
        }
    }
    // close file
    inFile.close();
    return numberOfColumns_pre;
}

bool
PLoMInputWidget::inputFromJSON(QJsonObject &jsonObject){

    bool result = false;
    preTrained = false;
    if (jsonObject.contains("preTrained")) {
        preTrained = jsonObject["preTrained"].toBool();
        result = true;
    } else {
        return false;
    }
    if (preTrained) {
        if (jsonObject.contains("inpFile2")) {
            QString fileDir=jsonObject["inpFile2"].toString();
            inpFileDir2->setText(fileDir);
            result = true;
        } else {
            return false;
        }

    } else {
        if (jsonObject.contains("inpFile")) {
            QString fileDir=jsonObject["inpFile"].toString();
            inpFileDir->setText(fileDir);
            result = true;
        } else {
            return false;
        }

        if (jsonObject.contains("outputData")) {
          if (jsonObject["outputData"].toBool()) {
              QString fileDir=jsonObject["outFile"].toString();
              outFileDir->setText(fileDir);
              //theFemWidget->setFEMforGP("GPdata");
          }
          result = true;
        } else {
          return false;
        }
    }


    if (jsonObject.contains("newSampleRatio")) {
        int samples=jsonObject["newSampleRatio"].toInt();
        ratioNewSamples->setText(QString::number(samples));
    } else {
        result = false;
    }

  if (jsonObject.contains("advancedOpt")) {
      theAdvancedCheckBox->setChecked(jsonObject["advancedOpt"].toBool());
      if (jsonObject["advancedOpt"].toBool()) {
        theAdvancedCheckBox->setChecked(true);
        theLogtCheckBox->setChecked(jsonObject["logTransform"].toBool());
        theDMCheckBox->setChecked(jsonObject["diffusionMaps"].toBool());
        randomSeed->setText(QString::number(jsonObject["randomSeed"].toInt()));
        //smootherKDE->setText(QString::number(jsonObject["smootherKDE"].toDouble()));
        //tolKDE->setText(QString::number(jsonObject["kdeTolerance"].toDouble()));
        epsilonPCA->setText(QString::number(jsonObject["epsilonPCA"].toDouble()));
        theConstraintsButton->setChecked(jsonObject["constraints"].toBool());
        if (jsonObject["constraints"].toBool()) {
            constraintsPath->setText(jsonObject["constraintsFile"].toString());
            numIter->setText(QString::number(jsonObject["numIter"].toInt()));
            tolIter->setText(QString::number(jsonObject["tolIter"].toDouble()));
        }

        smootherKDE->inputFromJSON(jsonObject);
        tolKDE->inputFromJSON(jsonObject);

        // if fails skip.
        smootherKDE_path->inputFromJSON(jsonObject);
        smootherKDE_check->inputFromJSON(jsonObject);
        tolKDE_path->inputFromJSON(jsonObject);
        tolKDE_check->inputFromJSON(jsonObject);

      }
     result = true;
  } else {
     return false;
  }

  return result;
}

bool
PLoMInputWidget::copyFiles(QString &fileDir) {
    if (preTrained) {
        qDebug() << inpFileDir2->text();
        qDebug() << fileDir + QDir::separator() + "surrogatePLoM.h5";
        QFile::copy(inpFileDir2->text(), fileDir + QDir::separator() + "surrogatePLoM.h5");
        qDebug() << inpFileDir2->text().replace(".h5",".json");
        qDebug() << fileDir + QDir::separator() + "surrogatePLoM.json";
        QFile::copy(inpFileDir2->text().replace(".h5",".json"), fileDir + QDir::separator() + "surrogatePLoM.json");
        QFile::copy(inpFileDir->text(), fileDir + QDir::separator() + "inpFile.in");
        QFile::copy(outFileDir->text(), fileDir + QDir::separator() + "outFile.in");
    } else {
        QFile::copy(inpFileDir->text(), fileDir + QDir::separator() + "inpFile.in");
        QFile::copy(outFileDir->text(), fileDir + QDir::separator() + "outFile.in");
    }
    if (theConstraintsButton->isChecked()) {
        QFile::copy(constraintsPath->text(), fileDir + QDir::separator() + "plomConstraints.py");
    }

    //
    //
    //
    if (smootherKDE_check->isChecked()) {
        smootherKDE_path->copyFile(fileDir);
    }
    if (tolKDE_check->isChecked()) {
        tolKDE_path->copyFile(fileDir);
    }

    return true;
}

void
PLoMInputWidget::clear(void)
{

}

int
PLoMInputWidget::getNumberTasks()
{
  return numSamples;
}

QStringList PLoMInputWidget::parseLineCSV(const QString &csvString)
{
  QStringList fields;
  QString value;

  bool hasQuote = false;

  for (int i = 0; i < csvString.size(); ++i)
  {
        const QChar current = csvString.at(i);

        // Normal state
        if (hasQuote == false)
        {
        // Comma
        if (current == ',')
        {
            // Save field
            fields.append(value.trimmed());
            value.clear();
        }

        // Double-quote
        else if (current == '"')
        {
            hasQuote = true;
            value += current;
        }

        // Other character
        else
            value += current;
        }
        else if (hasQuote)
        {
        // Check for another double-quote
        if (current == '"')
        {
            if (i < csvString.size())
            {
                    // A double double-quote?
                    if (i+1 < csvString.size() && csvString.at(i+1) == '"')
                    {
                        value += '"';

                        // Skip a second quote character in a row
                        i++;
                    }
                    else
                    {
                        hasQuote = false;
                        value += '"';
                    }
            }
        }

        // Other character
        else
            value += current;
        }
  }

  if (!value.isEmpty())
        fields.append(value.trimmed());


  // Remove quotes and whitespace around quotes
  for (int i=0; i<fields.size(); ++i)
        if (fields[i].length()>=1 && fields[i].left(1)=='"')
        {
        fields[i]=fields[i].mid(1);
        if (fields[i].length()>=1 && fields[i].right(1)=='"')
            fields[i]=fields[i].left(fields[i].length()-1);
        }

  return fields;
}
