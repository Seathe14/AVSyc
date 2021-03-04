#ifndef BASEEDITOR_H
#define BASEEDITOR_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTableWidgetItem>
#include <QLineEdit>
#include <QClipboard>
#include <QToolTip>
#include <QTime>
#include <QTimer>
#include <windows.h>
#include "ReadWrite.h"
#include <SHA256.h>
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

    void on_tableWidget_itemChanged(QTableWidgetItem *item);

    void on_tableWidget_itemClicked(QTableWidgetItem *item);

    void on_tableWidget_itemDoubleClicked(QTableWidgetItem *item);

    void update();

private:
    Ui::BaseEditor *ui;
    QTimer* timer;
    qint64 m_tooltipTimerStart;
    HANDLE hFile;
    void Save();
    void Load();
    void setHash();
};
#endif // BASEEDITOR_H
