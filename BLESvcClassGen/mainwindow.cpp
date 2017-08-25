#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <stdio.h>
#include "stdlib.h"
#include "string.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    mDevClassVal(0),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->renderDevClassVal_label();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::renderDevClassVal_label()
{
    char sVal[16];
    sprintf(sVal, "0x%06X", mDevClassVal);
    ui->label_2->setText(QString::fromUtf8(sVal));
}

void MainWindow::onStateChange(QCheckBox *cb, int position)
{
    unsigned int mask = (1 << position);
    if (cb->isChecked()) {
        this->mDevClassVal |= mask;
    } else {
        this->mDevClassVal &= ~mask;
    }

    this->renderDevClassVal_label();
}


#define DECLARE_CHECKBOX(_num, _bit) void MainWindow::on_checkBox_##_num##_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_##_num, _bit); }
//void MainWindow::on_checkBox_1_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_1, 13); }
//void MainWindow::on_checkBox_2_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_2, 16); }
//void MainWindow::on_checkBox_3_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_3, 17); }
//void MainWindow::on_checkBox_4_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_4, 18); }
//void MainWindow::on_checkBox_5_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_5, 19); }
//void MainWindow::on_checkBox_6_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_6, 20); }
//void MainWindow::on_checkBox_7_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_7, 21); }
//void MainWindow::on_checkBox_8_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_8, 22); }
//void MainWindow::on_checkBox_9_stateChanged(int arg1) {(void) arg1; this->onStateChange(ui->checkBox_9, 23); }
DECLARE_CHECKBOX(1, 13)
DECLARE_CHECKBOX(2, 16)
DECLARE_CHECKBOX(3, 17)
DECLARE_CHECKBOX(4, 18)
DECLARE_CHECKBOX(5, 19)
DECLARE_CHECKBOX(6, 20)
DECLARE_CHECKBOX(7, 21)
DECLARE_CHECKBOX(8, 22)
DECLARE_CHECKBOX(9, 23)

/* Major Device Classes (bits 8-12) value range: [0..31] not a bitmask
12	11	10	9	8	Major Device Class
----------------------------------------------------------------------
0	0	0	0	0	Miscellaneous [Ref #2]
0	0	0	0	1	Computer (desktop, notebook, PDA, organizer, ... )
0	0	0	1	0	Phone (cellular, cordless, pay phone, modem, ...)
0	0	0	1	1	LAN /Network Access point
0	0	1	0	0	Audio/Video (headset, speaker, stereo, video display, VCR, ...
0	0	1	0	1	Peripheral (mouse, joystick, keyboard, ... )
0	0	1	1	0	Imaging (printer, scanner, camera, display, ...)
0	0	1	1	1	Wearable
0	1	0	0	0	Toy
0	1	0	0	1	Health
1	1	1	1	1	Uncategorized: device code not specified
--------------------------------------------------*/
#define DECLARE_RADIOBOX(_num, _val) void MainWindow::on_radioButton_##_num##_toggled(bool checked) {   \
    if (checked) {  \
        unsigned int val = 0x1F << 8; \
        mDevClassVal &= ~val; /* clean 5 bits starting from 8th */  \
        mDevClassVal |= (_val << 8); \
        renderDevClassVal_label();  \
    }}


DECLARE_RADIOBOX(10, 0)
DECLARE_RADIOBOX(11, 1)
DECLARE_RADIOBOX(12, 2)
DECLARE_RADIOBOX(13, 3)
DECLARE_RADIOBOX(14, 4)
DECLARE_RADIOBOX(15, 5)
DECLARE_RADIOBOX(16, 6)
DECLARE_RADIOBOX(17, 7)
DECLARE_RADIOBOX(18, 8)
DECLARE_RADIOBOX(19, 9)
DECLARE_RADIOBOX(20, 31)


/* Minor Device Class field - Audio/Video Major Class
7	6	5	4	3	2	Minor Device Class bit no. of CoD
----------------------------------------------------------------------
0	0	0	0	0	0	Uncategorized, code not assigned
0	0	0	0	0	1	Wearable Headset Device
0	0	0	0	1	0	Hands-free Device
0	0	0	0	1	1	(Reserved)
0	0	0	1	0	0	Microphone
0	0	0	1	0	1	Loudspeaker
0	0	0	1	1	0	Headphones
0	0	0	1	1	1	Portable Audio
0	0	1	0	0	0	Car audio
0	0	1	0	0	1	Set-top box
0	0	1	0	1	0	HiFi Audio Device
0	0	1	0	1	1	VCR
0	0	1	1	0	0	Video Camera
0	0	1	1	0	1	Camcorder
0	0	1	1	1	0	Video Monitor
0	0	1	1	1	1	Video Display and Loudspeaker
0	1	0	0	0	0	Video Conferencing
0	1	0	0	0	1	(Reserved)
0	1	0	0	1	0	Gaming/Toy
X	X	X	X	X	X	All other values reserved
--------------------------------------------------*/











