#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "AVSyc.h"
#include "ReadWrite.h"
#include <thread>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	//To consider: Admin rights -> service, service -> no admin rights(doesn't work if AVSyc requires admin rights). Should AVSyc be always with admin rights?
	std::thread t1(&MainWindow::connectPipe,this);
	ui->dateTimeEdit->setMinimumDate(QDate::currentDate());
	ui->dateTimeEdit->setMinimumDateTime(QDateTime::currentDateTime());
	SC_HANDLE sc = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
#ifndef NDEBUG
	SC_HANDLE os = OpenService(sc, L"AVSycDbg", SERVICE_START);
#else
	SC_HANDLE os = OpenService(sc, L"AVSyc", SERVICE_START);
#endif
	StartService(os, 0, NULL);

	CloseServiceHandle(sc);
	CloseServiceHandle(os);
	
	t1.detach();
	connect(this, &MainWindow::output, ui->resultTextEdit, &QTextEdit::insertPlainText);
}
MainWindow::~MainWindow()
{
    DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
    delete ui;
}
void MainWindow::connectPipe()
{
	hPipe = CreateFile(lpszPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	while (hPipe == INVALID_HANDLE_VALUE)
	{
		hPipe = CreateFile(lpszPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		Sleep(1);
	}
	//hEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"StatEvent");
}



void MainWindow::scan(bool scheduled)
{
	if (!scheduled)
	{
		Writeint8_t(hPipe, codes::codes::PATH);
		std::u16string toSend = ui->pathLineEdit->text().toStdU16String();
		WriteU16String(hPipe, toSend);
	}
	else
	{
		qint64 qTime = ui->dateTimeEdit->dateTime().toSecsSinceEpoch() / 60;
		Writeint8_t(hPipe, codes::codes::INT64);
		Writeint64_t(hPipe, qTime);
		std::u16string toSend = ui->pathLineEdit_2->text().toStdU16String();
		WriteU16String(hPipe, toSend);
	}
	WaitForSingleObject(hEvent,INFINITE);
	//ui->resultTextEdit->setText("");

	int numofParts = Readuint32_t(hPipe);
	for (int i = 0; i < numofParts; i++)
	{
		std::u16string stat = ReadU16String(hPipe);
		if (stat != u"")
			output(QDir::fromNativeSeparators(QString::fromStdU16String(stat)));
	}
}


void MainWindow::on_browseButton_clicked()
{
	FileDialog* fd = new FileDialog(nullptr);
	fd->show();
	if (fd->exec())
	{
		QString directory = fd->selectedFiles()[0];
		if (directory != "")
		{
			directory.replace(QString("//"), QString("/"));
			ui->pathLineEdit->setText(directory);
		}
	}
}


void MainWindow::on_startScanButton_clicked()
{
	ui->resultTextEdit->setText("");
	QThread* scThread = QThread::create(&MainWindow::scan, this,0);	
	scThread->start();
}

void MainWindow::on_browseButton_2_clicked()
{
	QThread* scThread = QThread::create(&MainWindow::scan, this, 1);
	ui->pathLineEdit_2->setText(ui->pathLineEdit->text());
	scThread->start();
// 	ui->dateTimeEdit->dateTime().setOffsetFromUtc(0);
// 	qint64 qTime = ui->dateTimeEdit->dateTime().toSecsSinceEpoch() / 60;
// 	QDateTime secTo;
// 	secTo = secTo.currentDateTime();
// 	//qint64 secondsTo = secTo.secsTo(ui->dateTimeEdit->dateTime());
// 	Writeint8_t(hPipe, codes::codes::INT64);
// 	Writeint64_t(hPipe,qTime);
// 	std::u16string toSend = ui->pathLineEdit->text().toStdU16String();
// 	WriteU16String(hPipe, toSend);
// 	int numofParts = Readuint32_t(hPipe);
// 	for (int i = 0; i < numofParts; i++)
// 	{
// 		std::u16string stat = ReadU16String(hPipe);
// 		if (stat != u"")
// 			output(QDir::fromNativeSeparators(QString::fromStdU16String(stat)));
// 	}
}
