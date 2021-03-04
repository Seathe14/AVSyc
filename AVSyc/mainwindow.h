#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTextCodec>
#include <windows.h>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_lineEdit_returnPressed();
    void on_lineEdit_2_returnPressed();
    void on_lineEdit_3_returnPressed();
    void on_lineEdit_4_returnPressed();
    void on_lineEdit_5_returnPressed();
	void on_lineEdit_6_returnPressed();
	void on_lineEdit_7_returnPressed();
	void on_lineEdit_8_returnPressed();
	void on_lineEdit_9_returnPressed();
    void on_lineEdit_9_textChanged(const QString& text);
    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    HANDLE hPipe;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");

private:
    void connectPipe();
};
#endif // MAINWINDOW_H
