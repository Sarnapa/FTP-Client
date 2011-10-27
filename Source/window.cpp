#include "window.h"
#include "ui_window.h"
#include "ftpmodel.h"


window::window(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::window)
{
    ui->setupUi(this);

    model = new QDirModel(this);
    ftpmodel =new FtpModel(this);
    connectStatus=false;

    ui->localView->setModel(model);
    ui->remoteView->setModel(ftpmodel);

    connect(ui->usernameLine,SIGNAL(textChanged(const QString &)),
            this,SLOT(activateConnect()));
    connect(ui->passwordLine,SIGNAL(textChanged(const QString &)),
            this,SLOT(activateConnect()));
    connect(ui->hostnameLine,SIGNAL(textChanged(const QString &)),
            this,SLOT(activateConnect()));

    connect(ui->usernameLine,SIGNAL(returnPressed()),
            this,SLOT(dis_connect()));
    connect(ui->passwordLine,SIGNAL(returnPressed()),
            this,SLOT(dis_connect()));
    connect(ui->hostnameLine,SIGNAL(returnPressed()),
            this,SLOT(dis_connect()));

    connect(ui->connectionButton,SIGNAL(clicked()),
            this,SLOT(dis_connect()));
    connect(&(this->ftpmodel->connection),SIGNAL(stateChanged(int)),
            this,SLOT(getAction(int)));

    connect(ui->toRemoteButton,SIGNAL(clicked()),
            this,SLOT(upload()));
    connect(ui->fromRemoteButton,SIGNAL(clicked()),
            this,SLOT(download()));
    connect(&(this->ftpmodel->connection),SIGNAL(commandFinished(int,bool)),
            this,SLOT(commandManage(int,bool)));
    connect(&(this->ftpmodel->connection),SIGNAL(dataTransferProgress(qint64,qint64)),
            this,SLOT(changeProgressBar(qint64,qint64)));
}

window::~window()
{
    delete ui;
}

void window::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type())
    {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void window::closeEvent(QCloseEvent *event)
{
    switch(QMessageBox::question(
            this,
            tr("Are you sure want to quit?"),
            tr("Do you want to quit?"
                "\nAll existing connections will be lost!")
                ,
            tr("&Yes"), tr("&No"),
            QString::null, 0, 1 ) )
    {
        case 0:
            event->accept();
            break;
        case 1:
            event->ignore();
            break;
    }

}

void window::activateConnect()
{
    if(ui->usernameLine->text().isEmpty() ||
       ui->passwordLine->text().isEmpty() ||
       ui->hostnameLine->text().isEmpty())

        ui->connectionButton->setEnabled(false);
    else ui->connectionButton->setEnabled(true);
}
void window::dis_connect()
{
    if(!connectStatus && ui->connectionButton->isEnabled())
    {
        url = QString("ftp://%1:%2@%3").arg(ui->usernameLine->text()).arg(ui->passwordLine->text()).arg(ui->hostnameLine->text());
        this->ftpmodel->setUrl(url);
        this->ftpmodel->connection.connectToHost(url.host(), url.port(21));
        ui->remoteView->setModel(ftpmodel);

    }
    else if(connectStatus)
    {
        this->ftpmodel->connection.close();

    }
}

void window::getAction(int state)
{
    switch(state)
    {
    case 0:
        ui->connectionButton->setText("&Connect ");
        ui->watermarkLabel->setText("Disconnected - Eylül Tasyürek & Seçkin Savasçi @ 2010");
        connectStatus=false;
        break;
    case 1:
        ui->watermarkLabel->setText("Looking for host... - Eylül Tasyürek & Seçkin Savasçi @ 2010");
        ui->connectionButton->setText("&Disconnect");

        break;
    case 2:
        ui->watermarkLabel->setText("Connecting... - Eylül Tasyürek & Seçkin Savasçi @ 2010");
        break;
    case 3:
        ui->watermarkLabel->setText("Connected - Eylül Tasyürek & Seçkin Savasçi @ 2010");
        connectStatus=true;
        break;
    case 4:
        ui->watermarkLabel->setText("Connected & Logged in - Eylül Tasyürek & Seçkin Savasçi @ 2010");
        break;
    case 5:
        ui->watermarkLabel->setText("Disconnecting - Eylül Tasyürek & Seçkin Savasçi @ 2010");
        break;
    }
}
void window::commandManage(int id,bool error)
{
    qDebug() <<"commandmanage" << id << error;
    qDebug() << ftpmodel->connection.currentCommand();
    if(error) ui->watermarkLabel->setText("Error Occured, Please Retry! - Eylül Tasyürek & Seçkin Savasçi @ 2010");

    else if(ftpmodel->connection.currentCommand() == QFtp::Put) ui->watermarkLabel->setText("Uploaded - Eylül Tasyürek & Seçkin Savasçi @ 2010");
    else if(ftpmodel->connection.currentCommand() == QFtp::Get) ui->watermarkLabel->setText("Downloaded - Eylül Tasyürek & Seçkin Savasçi @ 2010");
}

void window::changeProgressBar(qint64 value,qint64 max)
{
    if(this->ftpmodel->connection.currentCommand() == QFtp::Get )
    ui->progressBar->setInvertedAppearance(true);
    if(this->ftpmodel->connection.currentCommand() == QFtp::Put )
        ui->progressBar->setInvertedAppearance(false);
    ui->progressBar->setValue(value);
    if(max != ui->progressBar->maximum())ui->progressBar->setMaximum(max);
}

void window::upload()
{   QItemSelectionModel *selectionModel = ui->localView->selectionModel();
    QModelIndexList selectedOnes = selectionModel->selectedRows();
    QItemSelectionModel *destinationSelectionModel = ui->remoteView->selectionModel();
    QModelIndexList destination = destinationSelectionModel->selectedRows();

    for(int i=0;i<selectedOnes.size();i++)
    {
        if(destination.size())
            for(int j=0;j<destination.size();j++)
            {
                qDebug() << model->filePath(selectedOnes[i]);
                qDebug() << ftpmodel->filePath(destination[j]);
                qDebug() << ftpmodel->filePath(ftpmodel->parent(destination[j]));

                QFile *file= new QFile(model->filePath(selectedOnes[i]));
                if(ftpmodel->isDir(destination[j]))
                   this->ftpmodel->connection.cd(ftpmodel->filePath(destination[j]));
                else
                   this->ftpmodel->connection.cd(ftpmodel->filePath(ftpmodel->parent(destination[j])));

                this->ftpmodel->connection.put(file,model->fileName(selectedOnes[i]));
                ui->watermarkLabel->setText(QString("Uploading %1 of %2 file(s) to destination %3 - Eylül Tasyürek & Seçkin Savasçi @ 2010").arg(i+1).arg(selectedOnes.size()).arg(j+1));
            }
        else
            {
                qDebug() << model->filePath(selectedOnes[i]);
                QFile *file= new QFile(model->filePath(selectedOnes[i]));
                this->ftpmodel->connection.put(file,model->fileName(selectedOnes[i]));
                ui->watermarkLabel->setText(QString("Uploading %1 of %2 file(s) - Eylül Tasyürek & Seçkin Savasçi @ 2010").arg(i+1).arg(selectedOnes.size()));
            }
    }

    this->ftpmodel->connection.close();
    connectStatus=false;
    this->dis_connect();
}

void window::download()
{
    QItemSelectionModel *selectionModel = ui->remoteView->selectionModel();
    QModelIndexList selectedOnes = selectionModel->selectedRows();
    QItemSelectionModel *destinationSelectionModel = ui->localView->selectionModel();
    QModelIndexList destination = destinationSelectionModel->selectedRows();

    for(int i=0;i<selectedOnes.size();i++)
    {
        if(destination.size())
            for(int j=0;j<destination.size();j++)
            {
            qDebug() << "file: " <<ftpmodel->filePath(selectedOnes[i]);
            qDebug() << "destination: " <<model->filePath(destination[j]);
            qDebug() << "afterTransfer: " << QString("%1%2%3").arg(model->filePath(destination[j])).arg("/").arg(ftpmodel->fileName(selectedOnes[i]));
            qDebug() << "parent: " << model->filePath(model->parent(destination[j]));

        //QFile *downloaded = new QFile(ftpmodel->filePath(selectedOnes[i]));
        QFile *downloaded = new QFile(QString("%1%2%3").arg(model->filePath(destination[j])).arg("/").arg(ftpmodel->fileName(selectedOnes[i])));
        if (QFile::exists(QString("%1%2%3").arg(model->filePath(destination[j])).arg("/").arg(ftpmodel->fileName(selectedOnes[i]))))
            {
                 switch(QMessageBox::question(this,
                                              tr("File exits!"),
                                              tr("There already exists a file called %1 in "
                                             "the current directory.Do you want to overwrite?")
                                          .arg(ftpmodel->fileName(selectedOnes[i])),
                                            tr("&Yes"),tr("&No"),
                                            QString::null, 0, 1 ))
                 {
            case 0:
                 break;
            case 1:
                 qDebug() << "File upload ignored";
                 return;
                 }
             }
        if (!downloaded->open(QIODevice::WriteOnly)) {
                 QMessageBox::information(this, tr("FTP"),
                                          tr("Unable to save the file %1.")
                                          .arg(downloaded->errorString()));
                 delete downloaded;
                 return;
             }

        this->ftpmodel->connection.get(ftpmodel->filePath(selectedOnes[i]),downloaded);

    }
        else
        {   ui->watermarkLabel->setText(QString("Downloading to default directory : %1 of %2 file(s) - Eylül Tasyürek & Seçkin Savasçi @ 2010").arg(i+1).arg(selectedOnes.size()));
            QFile *downloaded = new QFile(QString("%1%2%3").arg(QDir::currentPath()).arg("/").arg(ftpmodel->fileName(selectedOnes[i])));
            if (QFile::exists(QString("%1%2%3").arg(QDir::currentPath()).arg("/").arg(ftpmodel->fileName(selectedOnes[i]))))
                {
                     switch(QMessageBox::question(this,
                                                  tr("File exits!"),
                                                  tr("There already exists a file called %1 in "
                                                 "the current directory.Do you want to overwrite?")
                                              .arg(ftpmodel->fileName(selectedOnes[i])),
                                                tr("&Yes"),tr("&No"),
                                                QString::null, 0, 1 ))
                     {
                case 0:
                     break;
                case 1:
                     qDebug() << "File upload ignored";
                     return;
                     }
                 }
            if (!downloaded->open(QIODevice::WriteOnly)) {

                     QMessageBox::information(this, tr("FTP"),
                                              tr("Unable to save the file %1.")
                                              .arg(downloaded->errorString()));
                     delete downloaded;
                     return;
                 }

            this->ftpmodel->connection.get(ftpmodel->filePath(selectedOnes[i]),downloaded);
        }


    }
}



