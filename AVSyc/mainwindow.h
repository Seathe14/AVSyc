#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTextCodec>
#include "FileDialog.h"
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
    void on_browseButton_clicked();

    void on_startScanButton_clicked();

    void on_browseButton_2_clicked();

private:
    Ui::MainWindow *ui;
    HANDLE hPipe;
	LPCTSTR lpszPipeName = TEXT("\\\\.\\pipe\\IPCPipe");
    HANDLE hEvent;

private:
    void connectPipe();
    void scan(bool scheduled);
signals:
	void output(const QString& toAppend);
};
#endif // MAINWINDOW_H
