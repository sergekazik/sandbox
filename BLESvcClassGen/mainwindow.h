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

private:
    unsigned int mDevClassVal;

    Ui::MainWindow *ui;
    void onStateChange(QCheckBox *cb, int position);
};

#endif // MAINWINDOW_H
