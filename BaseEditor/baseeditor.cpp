#include "baseeditor.h"
#include "ui_baseeditor.h"
std::vector<uint8_t> HexToBytes(const std::string& hex) {
	std::vector<uint8_t> bytes;

	for (unsigned int i = 0; i < hex.length(); i += 2) {
		std::string byteString = hex.substr(i, 2);
		uint8_t byte = (uint8_t)strtol(byteString.c_str(), NULL, 16);
		bytes.push_back(byte);
	}

	return bytes;
}
BaseEditor::BaseEditor(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BaseEditor)
{


    ui->setupUi(this);
	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
}

BaseEditor::~BaseEditor()
{
    delete ui;
}

void BaseEditor::on_pushButton_clicked()
{
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
    QTableWidgetItem* lenItem = new QTableWidgetItem();
	QTableWidgetItem* shaItem = new QTableWidgetItem();

    lenItem->setFlags(lenItem->flags() ^ Qt::ItemIsEditable);
    shaItem->setFlags(shaItem->flags() ^ Qt::ItemIsEditable);

    ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 3, lenItem);
	ui->tableWidget->setItem(ui->tableWidget->rowCount()-1, 4, shaItem);

}

void BaseEditor::on_pushButton_2_clicked()
{
    ui->tableWidget->removeRow(ui->tableWidget->currentRow());
}

void BaseEditor::Save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save DB"),"", tr("Data Base Sychev (*.dbs);;All Files(*)"));
    bool ok;
    if (fileName.isEmpty())
        return;
    else
    {
        hFile = CreateFile((LPCWSTR)fileName.toStdU16String().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (GetLastError() == ERROR_FILE_EXISTS)
        {
			hFile = CreateFile((LPCWSTR)fileName.toStdU16String().c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        }
        std::u16string header = u"Sychev";
		WriteU16String(hFile, header);
        Writeuint16_t(hFile, ui->tableWidget->rowCount());
        for (int i = 0; i < ui->tableWidget->rowCount(); i++)
        {
                WriteU16String(hFile, ui->tableWidget->item(i, 0)->text().toStdU16String());
                WriteU16String(hFile, ui->tableWidget->item(i, 1)->text().toStdU16String());
                Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 2)->text().toULongLong(&ok,16));
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 3)->text().toULongLong());
                WriteU16String(hFile, ui->tableWidget->item(i, 4)->text().toStdU16String());
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 5)->text().toULongLong(&ok,16));
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 6)->text().toULongLong(&ok,16));
        }
        CloseHandle(hFile);
    }
}

void BaseEditor::Load()
{
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open DB"),"", tr("Data Base Sychev (*.dbs);;All Files(*)"));
	if (fileName.isEmpty())
		return;
	else
	{
		hFile = CreateFile((LPCWSTR)fileName.toStdU16String().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        std::u16string header = ReadU16String(hFile);
        uint16_t rowsCount = Readuint16_t(hFile);
        ui->tableWidget->setRowCount(rowsCount);
		for (int i = 0; i < rowsCount; i++)
		{
            ui->tableWidget->setItem(i, 0, new QTableWidgetItem(QString::fromStdU16String(ReadU16String(hFile))));

			ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::fromStdU16String(ReadU16String(hFile))));
            QString hexSig = QString::number(Readuint64_t(hFile), 16);
            hexSig = hexSig.toUpper();

			ui->tableWidget->setItem(i, 2, new QTableWidgetItem(hexSig));

			ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(Readuint64_t(hFile))));
            ui->tableWidget->item(i, 3)->setFlags(ui->tableWidget->item(i, 3)->flags() ^ Qt::ItemIsEditable);

            ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::fromStdU16String(ReadU16String(hFile))));
            ui->tableWidget->item(i, 4)->setFlags(ui->tableWidget->item(i, 4)->flags() ^ Qt::ItemIsEditable);

            hexSig = QString::number(Readuint64_t(hFile),16);
            hexSig = hexSig.toUpper();
			ui->tableWidget->setItem(i, 5, new QTableWidgetItem(hexSig));

			hexSig = QString::number(Readuint64_t(hFile),16);
			hexSig = hexSig.toUpper();
			ui->tableWidget->setItem(i, 6, new QTableWidgetItem(hexSig));
		}
		CloseHandle(hFile);
	}
}


void BaseEditor::setHash()
{
    QString sValue;
    QString abv;
    bool bStatus = false;
    uint sigLength;
    for (int i = 0; i < ui->tableWidget->rowCount(); i++)
    {
        if (ui->tableWidget->item(i, 2))
        {
            abv = ui->tableWidget->item(i, 2)->text();
            abv.replace(" ", "");
            ui->tableWidget->item(i, 2)->setText(abv);
            sigLength = ui->tableWidget->item(i, 2)->text().length();
            if (ui->tableWidget->item(i, 2)->text().length() > 16)
            {
                QString s = ui->tableWidget->item(i, 2)->text().mid(16);
                std::vector<uint8_t> bytes = HexToBytes(s.toStdString());
                //ui->tableWidget->item(i, 3)->setFlags(ui->tableWidget->item(i, 3)->flags() ^ Qt::ItemIsEditable);
                SHA256 sha;
                sha.update(bytes.data(),bytes.size());
                uint8_t* digest = sha.digest();
                ui->tableWidget->item(i, 4)->setText(QString::fromStdString(SHA256::toString(digest)));
                ui->tableWidget->item(i, 3)->setText(QString::number((sigLength - 16) / 2));
                ui->tableWidget->item(i, 2)->setText(ui->tableWidget->item(i, 2)->text().left(16));
                //ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::fromStdString(SHA256::toString(digest))));
                //ui->tableWidget->setItem(i, 2, new QTableWidgetItem(ui->tableWidget->item(i, 2)->text().left(16)));
                delete[]digest;
            }
            else
            {
                sValue = ui->tableWidget->item(i, 2)->text();

            }
        }
        else return;
    }
}

void BaseEditor::on_pushButton_3_clicked()
{
    Save();
    setHash();

}

void BaseEditor::on_pushButton_4_clicked()
{
    Load();
}

void BaseEditor::on_tableWidget_itemChanged(QTableWidgetItem *item)
{

    if(item == ui->tableWidget->item(ui->tableWidget->currentRow(),2))
    {
		QRegularExpression re("[0-9a-fA-F ]+");
		QRegularExpressionValidator v(re, 0);
		QString string = item->text();
		int pos = 0;
		auto state = v.validate(string, pos);
        if (state == QValidator::State::Acceptable)
        {
            if(string.length()>8)
                setHash();
        }
        else
        {
            item->setText(QString(""));
        }
    }
}

void BaseEditor::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
    if (item == ui->tableWidget->item(ui->tableWidget->currentRow(), 4))
    {
        item->setToolTip(item->text());
    }
}

void BaseEditor::on_tableWidget_itemDoubleClicked(QTableWidgetItem *item)
{
	if (item == ui->tableWidget->item(ui->tableWidget->currentRow(), 4))
	{
        timer->start(100);
		m_tooltipTimerStart = QDateTime::currentMSecsSinceEpoch();
		QClipboard* clipBoard = QApplication::clipboard();
		clipBoard->setText(item->text());
	}
}


void BaseEditor::update()
{
    auto howLongShown = QDateTime::currentMSecsSinceEpoch() - m_tooltipTimerStart;
    if(howLongShown < 600)
         QToolTip::showText(QCursor::pos(), "Copied to clipboard.", this);
    else
    {
        QToolTip::hideText();
        timer->stop();
    }
}
