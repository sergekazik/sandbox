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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onStateChange(QCheckBox *cb, int position)
{
    unsigned int mask = (1 << position);
    if (cb->isChecked()) {
        this->mDevClassVal |= mask;
    } else {
        this->mDevClassVal &= ~mask;
    }
    char sVal[16];
    sprintf(sVal, "0x%08X", mDevClassVal);
    ui->label_2->setText(QString::fromUtf8(sVal));
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

