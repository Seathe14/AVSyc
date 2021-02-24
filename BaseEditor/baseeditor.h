#ifndef BASEEDITOR_H
#define BASEEDITOR_H

#include <QMainWindow>
#include <QFileDialog>
#include <windows.h>
#include "ReadWrite.h"
QT_BEGIN_NAMESPACE
namespace Ui { class BaseEditor; }
QT_END_NAMESPACE

class BaseEditor : public QMainWindow
{
    Q_OBJECT

public:
    BaseEditor(QWidget *parent = nullptr);
    ~BaseEditor();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::BaseEditor *ui;
    HANDLE hFile;
    void Save();
    void Load();
};
#endif // BASEEDITOR_H
