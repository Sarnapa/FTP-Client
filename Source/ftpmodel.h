#ifndef FTPMODEL_H
#define FTPMODEL_H

#include <qabstractitemmodel.h>
#include <qftp.h>
#include <qdir.h>
#include <qurl.h>
#include <qhash.h>
#include <qpair.h>



class FtpItem;
class QFileIconProvider;

class FtpModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FtpModel(QObject *parent = 0);
    ~FtpModel();

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    bool canFetchMore(const QModelIndex &parent) const;
    void fetchMore(const QModelIndex &parent);

    QVariant data(const QModelIndex &index, int role) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex& parent) const;
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());


    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;

    QModelIndex index(int, int, const QModelIndex& parent= QModelIndex()) const;
    QModelIndex index(const QString &path, int column = 0) const;
    QString fileName(const QModelIndex &index) const;
    QString filePath(const QModelIndex &index) const;
    QIcon fileIcon(const QModelIndex &index) const;
    bool isDir(const QModelIndex &index) const;

    QUrl url() const;

    QString size(const QModelIndex &index) const;
    QString type(const QModelIndex &index) const;
    QString time(const QModelIndex &index) const;

    QDir::Filters filter() const;
    void setFilter(QDir::Filters filters);

    void refresh(const QModelIndex &parent = QModelIndex());

    // For progress etc...
    QFtp connection;

    inline bool connected() const {
        return (connection.state() == QFtp::Connected || connection.state() == QFtp::LoggedIn);
    }



public slots:
    void setUrl(const QUrl &url);


private slots:
    void gotNewListInfo(const QUrlInfo &info);
    void stateChanged(int state);
    void commandStarted(int id);
    void commandFinished(int id, bool error);

private:
    QUrl ftpUrl;
    QDir::Filters filters;
    QFileIconProvider *iconProvider;
    FtpItem *root;
    void sort(FtpItem *parent, Qt::SortOrder order);

    QStringList listing;
    QList<int> listingCommands;

    QMap<int, QPair<QString, QString> > renameCommands;

    QHash<int, QFile*> copyCommands;
    QHash<int, QFile*> moveCommands;

    const FtpItem *ftpItem(const QModelIndex &index) const {
        return index.isValid() ? static_cast<const FtpItem*>(index.internalPointer()) : root;
    }
};

#endif // FTPMODEL_H
