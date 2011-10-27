#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QtGui>
#include <QFtp>
#include <QFileSystemModel>
#include <QModelIndex>
#include "ftpmodel.h"
#include <QModelIndexList>
#include <QItemSelectionModel>

namespace Ui {
    class window;
}

class window : public QWidget
{
    Q_OBJECT

public:
    explicit window(QWidget *parent = 0);
    ~window();


protected:
    void changeEvent(QEvent *event);
protected slots:
    void closeEvent(QCloseEvent *event);

private slots:
    void dis_connect();
    void getAction(int);
    void activateConnect();
    void upload();
    void download();
    void commandManage(int,bool);
    void changeProgressBar(qint64,qint64);
private:
    Ui::window *ui;

    QDirModel *model;
    FtpModel *ftpmodel;
    QFileSystemModel remoteModel;
    QUrl url;

    bool connectStatus;


};

#endif // WINDOW_H
