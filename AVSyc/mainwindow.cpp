#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <Operations.h>
//#include "AVSyc.h"
#include "ReadWrite.h"
#include <thread>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

	//To consider: Admin rights -> service, service -> no admin rights(doesn't work if AVSyc requires admin rights). Should AVSyc be always with admin rights?
	std::thread t1(&MainWindow::connectPipe,this);
	tmr = new QTimer();
	tmr->setInterval(1000);
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
	connect(this, &MainWindow::writeText, ui->resultTextEdit, &QTextEdit::setText);
	connect(this, &MainWindow::outputScheduled, ui->resultTextEdit_2, &QTextEdit::insertPlainText);
	connect(this, &MainWindow::writeTextScheduled, ui->resultTextEdit_2, &QTextEdit::setText);
	connect(this, &MainWindow::outputMonitor, ui->resultTextEdit_3, &QTextEdit::insertPlainText);
	connect(this, &MainWindow::writeTextMonitor, ui->resultTextEdit_3, &QTextEdit::setText);
	connect(this, &MainWindow::setScanStopButton, ui->stopButton, &QPushButton::setEnabled);
	connect(this, &MainWindow::setScanStartButton, ui->startScanButton, &QPushButton::setEnabled);
	connect(this, &MainWindow::setScheduleStopButton, ui->stopButton_2, &QPushButton::setEnabled);
	connect(this, &MainWindow::setScheduleSetButton, ui->setButton, &QPushButton::setEnabled);
	connect(this, &MainWindow::setScheduleCancelButton, ui->cancelSchedule, &QPushButton::setEnabled);
	connect(this, &MainWindow::setStartMonitoringButton, ui->monitorButton, &QPushButton::setEnabled);
	connect(this, &MainWindow::setCancelMonitoringButton, ui->cancelMonitorButton, &QPushButton::setEnabled);
	connect(tmr, SIGNAL(timeout()), this, SLOT(updateTime()));
	connect(this, &MainWindow::logAppend, ui->logTextEdit, &QTextEdit::append);
	tmr->start();

}
MainWindow::~MainWindow()
{
	toCancelMonitoring = true;
	Sleep(1000);
    DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);
	DisconnectNamedPipe(hPipeScheduled);
	CloseHandle(hPipeScheduled);
	DisconnectNamedPipe(hPipeMonitor);
	CloseHandle(hPipeMonitor);
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
	hPipeScheduled = CreateFile(lpszScPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	while (hPipeScheduled == INVALID_HANDLE_VALUE)
	{
		hPipeScheduled = CreateFile(lpszScPipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		Sleep(1);
	}
	hPipeMonitor = CreateFile(lpszPipeMonitorName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	while (hPipeMonitor == INVALID_HANDLE_VALUE)
	{
		hPipeMonitor = CreateFile(lpszPipeMonitorName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		Sleep(1);
	}
}


void MainWindow::scan(bool scheduled)
{
	Writeint8_t(hPipe, Operation::SCANPATH);
	std::u16string toSend = ui->pathLineEdit->text().toStdU16String();
	WriteU16String(hPipe, toSend);
	OperationResult oper = (OperationResult)Readint8_t(hPipe);
	OperationResult previousOper = oper;
	if (oper == OperationResult::WAITING)
		ui->scanStatusLabel->setText("Waiting");
	if (oper != OperationResult::SUCCESS)
	{
		//Sleep(300);
		while (oper != OperationResult::SUCCESS)
		{
			if (toStopScan)
			{
				Writeint8_t(hPipe, Operation::STOP);
				Sleep(500);
				break;
			}
			Writeint8_t(hPipe, Operation::GET_STATE);
			oper = (OperationResult)Readint8_t(hPipe);
			if (oper == OperationResult::RUNNING)
			{
				if (previousOper != oper)
					ui->scanStatusLabel->setText("Running");
				output(".");
			}
			Sleep(1000);
		}
		writeText("");
		Writeint8_t(hPipe, Operation::GET_STATISTICS);
		int numofParts = Readuint32_t(hPipe);
		for (int i = 0; i < numofParts; i++)
		{
			std::u16string stat = ReadU16String(hPipe);
			if (stat != u"")
				output(QDir::fromNativeSeparators(QString::fromStdU16String(stat)));
		}
		if (toStopScan)
		{
			output("Scan hasn't been finished");
			toStopScan = false;
		}
		else
		{
			QDateTime currTime(QDateTime::currentDateTime());
			logAppend(currTime.time().toString() + ": Finished scanning " + QString::fromStdU16String(toSend));
		}
		ui->scanStatusLabel->setText("");
		setScanStartButton(true);
		setScanStopButton(false);
	}
}


void MainWindow::startScheduling()
{
	QTime qtime(ui->dateTimeEdit->time().hour(), ui->dateTimeEdit->time().minute(), 0, 0);
	QDateTime qDateTime(ui->dateTimeEdit->date(), qtime);
	qint64 secondsSinceEpoch = qDateTime.toSecsSinceEpoch();//ui->dateTimeEdit->dateTime().toSecsSinceEpoch();
	Writeint8_t(hPipeScheduled, Operation::SCANSCHEDULED);
	Writeint64_t(hPipeScheduled, secondsSinceEpoch);
	std::u16string toSend = ui->pathLineEdit_2->text().toStdU16String();
	WriteU16String(hPipeScheduled, toSend);
	OperationResult oper = (OperationResult)Readint8_t(hPipeScheduled);
	OperationResult previousStatus = OperationResult::SCHEDULED;
	ui->scheduleStatusLabel->setText("Scheduled");
	if (oper != OperationResult::SUCCESS)
	{
		Sleep(300);
		while (oper != OperationResult::SUCCESS)
		{
			Writeint8_t(hPipeScheduled, Operation::GET_STATE);
			oper = (OperationResult)Readint8_t(hPipeScheduled);
			if (oper == OperationResult::RUNNING)
			{
				if (toStopScheduleScan)
				{
					Writeint8_t(hPipeScheduled, Operation::STOP);
					Sleep(500);
					break;
				}
				if (previousStatus != oper)
				{
					previousStatus = oper;
					setScheduleStopButton(true);
					QDateTime currTime(QDateTime::currentDateTime());
					logAppend(currTime.time().toString() + ": Started scheduled scanning " + QString::fromStdU16String(toSend));
					setScheduleCancelButton(false);
					//ui->logTextEdit->append(currTime.time().toString() + ": Started scheduled scanning " + QString::fromStdU16String(toSend));
					ui->scheduleStatusLabel->setText("Running");
				}
				//outputScheduled("Running");
				outputScheduled(".");

			}
			else if (oper == OperationResult::WAITING)
			{
				if (previousStatus != oper)
				{
					previousStatus = oper;
					ui->scheduleStatusLabel->setText("Waiting");
				}
			}
			else if (oper == OperationResult::SCHEDULED)
			{
				if (toCancelSchedule)
				{
					Writeint8_t(hPipeScheduled, Operation::CANCELSCHEDULE);
					ui->scheduleStatusLabel->setText("");
					toCancelSchedule = false;
					setScheduleCancelButton(false);
					QDateTime currTime(QDateTime::currentDateTime());
					logAppend(currTime.time().toString() + ": Schedule scan canceled");

					//outputScheduled("Schedule Canceled");
					return;
				}
				if (previousStatus != oper)
				{
					previousStatus = oper;
					ui->scheduleStatusLabel->setText("Scheduled");
				}
				//outputScheduled("Scheduled");
			}
			else if (oper == OperationResult::FAILED)
			{
				if (previousStatus != oper)
				{
					previousStatus = oper;
					ui->scheduleStatusLabel->setText("Failed");
				}
				writeTextScheduled("");
				setScheduleSetButton(true);
				outputScheduled("Failed, wrong time");
				return;
			}
			Sleep(1000);
		}
		if (previousStatus != oper)
		{
			previousStatus = oper;
			ui->scheduleStatusLabel->setText("");
		}
		writeTextScheduled("");
		ui->scheduleStatusLabel->setText("");
		Writeint8_t(hPipeScheduled, Operation::GET_STATISTICS);
		int numofParts = Readuint32_t(hPipeScheduled);
		for (int i = 0; i < numofParts; i++)
		{
			std::u16string stat = ReadU16String(hPipeScheduled);
			if (stat != u"")
				outputScheduled(QDir::fromNativeSeparators(QString::fromStdU16String(stat)));
		}
		if (toStopScheduleScan)
		{
			outputScheduled("Scan hasn't been finished");
			toStopScheduleScan = false;
		}
		else
		{
			QDateTime currTime(QDateTime::currentDateTime());
			logAppend(currTime.time().toString() + ": Finished scheduled scanning " + QString::fromStdU16String(toSend));
		}
		setScheduleSetButton(true);
		setScheduleCancelButton(false);
		setScheduleStopButton(false);
	}
}

void MainWindow::startMonitoring()
{
	Writeint8_t(hPipeMonitor, Operation::MONITOR);
	std::u16string toSend = ui->pathLineEdit_3->text().toStdU16String();
	WriteU16String(hPipeMonitor, toSend);
	OperationResult oper = (OperationResult)Readint8_t(hPipeMonitor);
	OperationResult previousOper = OperationResult::FAILED;
	setCancelMonitoringButton(true);
	while (true)
	{
		Writeint8_t(hPipeMonitor, Operation::GET_STATE);
		oper = (OperationResult)Readint8_t(hPipeMonitor);
		Sleep(10);
		if (oper != OperationResult::SUCCESS)
		{
			//Sleep(400);

			if (oper == OperationResult::MONITORING)
			{
				if (previousOper != oper)
				{
					previousOper = oper;
					setCancelMonitoringButton(true);
					ui->monitorStatusLabel->setText("Monitoring folder");
				}
				if (toCancelMonitoring)
				{
					Writeint8_t(hPipeMonitor, Operation::STOP);
					ui->monitorStatusLabel->setText("");
					toCancelMonitoring = false;
					setStartMonitoringButton(true);
					//outputScheduled("Schedule Canceled");
					return;
				}
				Writeint8_t(hPipeMonitor, Operation::GET_STATE);
				oper = (OperationResult)Readint8_t(hPipeMonitor);
				//outputMonitor(".");
				//Sleep(1000);
			}
			else if (oper == OperationResult::RUNNING)
			{
				if (previousOper != oper)
				{
					previousOper = oper;
					setCancelMonitoringButton(false);
					ui->monitorStatusLabel->setText("Running scan");
				}
			}
// 			writeTextMonitor("");
// 			Writeint8_t(hPipeOper, Operation::GET_STATISTICS);
// 			int numofParts = Readuint32_t(hPipeOper);
// 			for (int i = 0; i < numofParts; i++)
// 			{
// 				std::u16string stat = ReadU16String(hPipeOper);
// 				if (stat != u"")
// 					outputMonitor(QDir::fromNativeSeparators(QString::fromStdU16String(stat)));
// 			}
		}
		else if (oper == OperationResult::SUCCESS)
		{
			writeTextMonitor("");
			Writeint8_t(hPipeMonitor, Operation::GET_STATISTICS);
			int numofParts = Readuint32_t(hPipeMonitor);
			for (int i = 0; i < numofParts; i++)
			{
				std::u16string stat = ReadU16String(hPipeMonitor);
				if (stat != u"")
					outputMonitor(QDir::fromNativeSeparators(QString::fromStdU16String(stat)));
			}
		}

		Sleep(1);
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
	if (ui->pathLineEdit->text() == "")
		return;
	ui->resultTextEdit->setText("");
	QDateTime currTime(QDateTime::currentDateTime());
	ui->logTextEdit->append(currTime.time().toString() + ": Started scanning " + ui->pathLineEdit->text());

	QThread* scThread = QThread::create(&MainWindow::scan, this,0);	
	ui->stopButton->setEnabled(true);
	ui->startScanButton->setEnabled(false);
	scThread->start();
}

void MainWindow::on_setButton_clicked()
{
	if (ui->pathLineEdit_2->text() == "")
		return;
	//ui->stopButton_2->setEnabled(true);
	ui->setButton->setEnabled(false);
	ui->cancelSchedule->setEnabled(true);
	ui->resultTextEdit_2->setText("");
	QDateTime currTime(QDateTime::currentDateTime());
	ui->logTextEdit->append(currTime.time().toString() + ": Started scheduling " + ui->pathLineEdit_2->text());
	QThread* scThread = QThread::create(&MainWindow::startScheduling, this);
//	ui->pathLineEdit_2->setText(ui->pathLineEdit->text());
	scThread->start();
}

void MainWindow::on_browseButton_2_clicked()
{
	FileDialog* fd = new FileDialog(nullptr);
	fd->show();
	if (fd->exec())
	{
		QString directory = fd->selectedFiles()[0];
		if (directory != "")
		{
			directory.replace(QString("//"), QString("/"));
			ui->pathLineEdit_2->setText(directory);
		}
	}
}

void MainWindow::on_browseButton_3_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, "Choose folder to monitor", QString());
	ui->pathLineEdit_3->setText(dir);

}

void MainWindow::on_monitorButton_clicked()
{
	if (ui->pathLineEdit_3->text() == "")
		return;
	ui->monitorButton->setEnabled(false);
	ui->resultTextEdit_3->setText("");
	QDateTime currTime(QDateTime::currentDateTime());
	ui->logTextEdit->append(currTime.time().toString() +": Started monitoring " + ui->pathLineEdit_3->text());
	QThread* scThread = QThread::create(&MainWindow::startMonitoring, this);
	scThread->start();
}

void MainWindow::on_stopButton_clicked()
{
	toStopScan = true;
	QDateTime currTime(QDateTime::currentDateTime());
	ui->logTextEdit->append(currTime.time().toString() + ": Stopped scanning");
	ui->stopButton->setEnabled(false);
	ui->startScanButton->setEnabled(true);
}

void MainWindow::on_cancelSchedule_clicked()
{
	toCancelSchedule = true;
	ui->setButton->setEnabled(true);
	ui->cancelSchedule->setEnabled(false);
}

void MainWindow::on_stopButton_2_clicked()
{
	toStopScheduleScan = true;
	QDateTime currTime(QDateTime::currentDateTime());
	ui->logTextEdit->append(currTime.time().toString() + ": Stopped scheduled scanning");
	ui->stopButton_2->setEnabled(false);
	ui->setButton->setEnabled(true);
}

void MainWindow::on_cancelMonitorButton_clicked()
{
	toCancelMonitoring = true;
	ui->cancelMonitorButton->setEnabled(false);
	QDateTime currTime(QDateTime::currentDateTime());
	ui->logTextEdit->append(currTime.time().toString() + ": Canceled monitoring ");
}

void MainWindow::updateTime()
{
	QTime qtime(QTime::currentTime().hour(), QTime::currentTime().minute(), 0, 0);
	ui->dateTimeEdit->setMinimumTime(qtime);
}
