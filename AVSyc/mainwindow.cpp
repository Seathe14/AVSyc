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
}



void MainWindow::scan()
{
	Writeint8_t(hPipe, codes::codes::PATH);
	std::u16string toSend = ui->pathLineEdit->text().toStdU16String();
	WriteU16String(hPipe, toSend);
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
	QThread* scThread = QThread::create(&MainWindow::scan, this);	
	scThread->start();
}
