#include "baseeditor.h"
#include "ui_baseeditor.h"

BaseEditor::BaseEditor(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::BaseEditor)
{
    ui->setupUi(this);

}

BaseEditor::~BaseEditor()
{
    delete ui;
}


void BaseEditor::on_pushButton_clicked()
{
    ui->tableWidget->insertRow(ui->tableWidget->rowCount());
}

void BaseEditor::on_pushButton_2_clicked()
{
    ui->tableWidget->removeRow(ui->tableWidget->currentRow());
}

void BaseEditor::Save()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save DB"),"", tr("Data Base Sychev (*.dbs);;All Files(*)"));
  
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
               // Writeuint16_t(hFile,)
                WriteU16String(hFile, ui->tableWidget->item(i, 0)->text().toStdU16String());
                WriteU16String(hFile, ui->tableWidget->item(i, 1)->text().toStdU16String());
                Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 2)->text().toULongLong());
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 3)->text().toULongLong());
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 4)->text().toULongLong());
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 5)->text().toULongLong());
				Writeuint64_t(hFile, (uint64_t)ui->tableWidget->item(i, 6)->text().toULongLong());
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
			ui->tableWidget->setItem(i, 2, new QTableWidgetItem(QString::number(Readuint64_t(hFile))));
			ui->tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(Readuint64_t(hFile))));
			ui->tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(Readuint64_t(hFile))));
			ui->tableWidget->setItem(i, 5, new QTableWidgetItem(QString::number(Readuint64_t(hFile))));
			ui->tableWidget->setItem(i, 6, new QTableWidgetItem(QString::number(Readuint64_t(hFile))));
		}
		CloseHandle(hFile);
	}
}

void BaseEditor::on_pushButton_3_clicked()
{
    Save();
}

void BaseEditor::on_pushButton_4_clicked()
{
    Load();
}
