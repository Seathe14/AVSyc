#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ReadWrite.h"
#include <thread>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //hPipe = CreateFile(lpszPipeName,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
	//To consider: Admin rights -> service, service -> no admin rights(doesn't work if AVSyc requires admin rights). Should AVSyc be always with admin rights?
	std::thread t1(&MainWindow::connectPipe,this);
	SC_HANDLE sc = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
	int a = GetLastError();
	SC_HANDLE OS = OpenService(sc, L"AVSyc", SERVICE_START);
	a = GetLastError();
	StartService(OS, 0, NULL);
	CloseHandle(sc);
	CloseHandle(OS);
	t1.detach();
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


void MainWindow::on_lineEdit_returnPressed()
{
    Writeint8_t(hPipe,codes::codes::INT8);
    int8_t toSend = ui->lineEdit->text().toInt();
    Writeint8_t(hPipe,toSend);
    int8_t Received = Readint8_t(hPipe);
    if (Received == 5)
    {
        this->close();
        return;
    }
	QString h = QChar::fromLatin1(Received);
    ui->textEdit->setText(h);
}

void MainWindow::on_lineEdit_2_returnPressed()
{
	Writeint8_t(hPipe, codes::codes::INT16);
	int16_t toSend = ui->lineEdit_2->text().toInt();
	Writeint16_t(hPipe, toSend);
	int16_t Received = Readint16_t(hPipe);

	ui->textEdit_2->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_3_returnPressed()
{
    Writeint8_t(hPipe, codes::codes::INT32);
    int32_t toSend = ui->lineEdit_3->text().toLongLong();
    Writeint32_t(hPipe,toSend);
    int32_t Received = Readint32_t(hPipe);

    ui->textEdit_3->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_4_returnPressed()
{
	Writeint8_t(hPipe, codes::codes::INT64);
	int64_t toSend = ui->lineEdit_4->text().toLongLong();
	Writeint64_t(hPipe, toSend);
	int64_t Received = Readint64_t(hPipe);

	ui->textEdit_4->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_5_returnPressed()
{
	Writeint8_t(hPipe, codes::codes::UINT8);
	uint8_t toSend = ui->lineEdit_5->text().toLongLong();
	Writeuint8_t(hPipe, toSend);
	uint8_t Received = Readuint8_t(hPipe);

	ui->textEdit_5->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_6_returnPressed()
{
	Writeint8_t(hPipe, codes::codes::UINT16);
	uint16_t toSend = ui->lineEdit_6->text().toLongLong();
	Writeuint16_t(hPipe, toSend);
	uint16_t Received = Readuint16_t(hPipe);

	ui->textEdit_6->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_7_returnPressed()
{
	Writeint8_t(hPipe, codes::codes::UINT32);
	uint32_t toSend = ui->lineEdit_7->text().toLongLong();
	Writeuint32_t(hPipe, toSend);
	uint32_t Received = Readuint32_t(hPipe);

	ui->textEdit_7->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_8_returnPressed()
{
	Writeint8_t(hPipe, codes::codes::UINT64);
	uint64_t toSend = ui->lineEdit_8->text().toULongLong();
	Writeuint64_t(hPipe, toSend);
	uint64_t Received = Readuint64_t(hPipe);

	ui->textEdit_8->setText(QString::number(Received));
}

void MainWindow::on_lineEdit_9_returnPressed()
{
	
}

void MainWindow::on_lineEdit_9_textChanged(const QString& text)
{
	Writeint8_t(hPipe, codes::codes::PATH);
	TCHAR toSend[MAX_PATH] = { 0 };
	//TCHAR toReceive[MAX_PATH] = { 0 };

	ui->lineEdit_9->text().toWCharArray(toSend);
	WritePath(hPipe, toSend);

	std::u16string stat = ReadU16String(hPipe);
	ui->textEdit_9->append(QString::fromStdU16String(stat));
}


void MainWindow::on_pushButton_clicked()
{
    QString directory = QFileDialog::getOpenFileName(this, tr("Browse file to scan"),"C:\\", tr("All files(*)"));
	ui->lineEdit_9->setText(directory);
}

