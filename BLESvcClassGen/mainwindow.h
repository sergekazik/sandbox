#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCheckBox>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_checkBox_1_stateChanged(int arg1);
    void on_checkBox_2_stateChanged(int arg1);
    void on_checkBox_3_stateChanged(int arg1);
    void on_checkBox_4_stateChanged(int arg1);
    void on_checkBox_5_stateChanged(int arg1);
    void on_checkBox_6_stateChanged(int arg1);
    void on_checkBox_7_stateChanged(int arg1);
    void on_checkBox_8_stateChanged(int arg1);
    void on_checkBox_9_stateChanged(int arg1);

    void on_radioButton_10_toggled(bool checked);
    void on_radioButton_11_toggled(bool checked);
    void on_radioButton_12_toggled(bool checked);
    void on_radioButton_13_toggled(bool checked);
    void on_radioButton_14_toggled(bool checked);
    void on_radioButton_15_toggled(bool checked);
    void on_radioButton_16_toggled(bool checked);
    void on_radioButton_17_toggled(bool checked);
    void on_radioButton_18_toggled(bool checked);
    void on_radioButton_19_toggled(bool checked);
    void on_radioButton_20_toggled(bool checked);

private:
    unsigned int mDevClassVal;

    Ui::MainWindow *ui;
    void onStateChange(QCheckBox *cb, int position);
    void renderDevClassVal_label();
};

#endif // MAINWINDOW_H
