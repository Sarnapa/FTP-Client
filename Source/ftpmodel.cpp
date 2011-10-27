#include "ftpmodel.h"

#include <QtAlgorithms>
#include <qlocale.h>
#include <qdirmodel.h>
#include <qmimedata.h>
#include <qdebug.h>

class FtpItem {
public:
    FtpItem() : fetchedChildren(false), parent(0) {}
    inline bool isDir() const { return info.isDir(); }
    QUrlInfo info;
    bool fetchedChildren;
    QList<FtpItem> children;
    FtpItem *parent;
    inline bool operator <(const FtpItem &item) const { return item.info.name() < info.name(); }
};

/*!
    \class FtpModel FtpModel.h

    \brief The FtpModel class provides a data model for the remote filesystem
    that supports the ftp protocal.

    \ingroup model-view

    FtpModel provides some convenience functions above QAbstractItemModel
    specific to a ftp model.

    \sa {Model/View Programming}, QListView, QTreeView
*/

/*!
    \reimp
 */
FtpModel::FtpModel(QObject *parent) : QAbstractItemModel(parent)
{
    connect(&connection, SIGNAL(listInfo(const QUrlInfo &)),
            this, SLOT(gotNewListInfo(const QUrlInfo &)));
    connect(&connection, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
    connect(&connection, SIGNAL(commandFinished(int, bool)),
            this, SLOT(commandFinished(int, bool)));
    connect(&connection, SIGNAL(commandStarted(int)), this, SLOT(commandStarted(int)));
    root = new FtpItem();
    iconProvider = new QFileIconProvider();
    filters = QDir::Readable | QDir::Writable | QDir::Executable | QDir::NoDotAndDotDot;
}

/*!
    \reimp
 */
FtpModel::~FtpModel()
{
    delete root;
    delete iconProvider;
}

/*!
    \reimp
 */
int FtpModel::rowCount(const QModelIndex &parent) const
{
    if (!connected() || parent.column() > 0)
        return 0;
    return ftpItem(parent)->children.count();
}

/*!
    \reimp
 */
int FtpModel::columnCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    return 4;
}

/*!
    \reimp
 */
bool FtpModel::hasChildren(const QModelIndex &parent) const
{
    if (!connected())
        return false;
    const FtpItem *item = ftpItem(parent);
    if (!parent.isValid())
        return (item->children.count() > 0);
    return item->isDir();
}

/*!
    \reimp
 */
bool FtpModel::canFetchMore(const QModelIndex &parent) const
{
    if (!connected())
        return false;
    const FtpItem *item = ftpItem(parent);
    return (!item->fetchedChildren);
}

/*!
    \reimp
 */
void FtpModel::fetchMore(const QModelIndex &parent)
{
    if (!connected())
        return;

    FtpItem *item = parent.isValid() ? static_cast<FtpItem*>(parent.internalPointer()) : root;
    if (item->fetchedChildren)
        return;
    qDebug() << "fetch more" << (item == root) << parent.data().toString();
    item->fetchedChildren = true;
    QString fullPath = filePath(parent);
    int listCommand = connection.list(fullPath);
    listing.append(fullPath);
    listingCommands.append(listCommand);
    qDebug() << "getting list      :" << listCommand << fullPath;
}

/*!
    \reimp
 */
QVariant FtpModel::data(const QModelIndex &index, int role) const
{
    if (!connected() || !index.isValid())
        return QVariant();

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        switch (index.column()) {
        case 0: return fileName(index);
        case 1: return size(index);
        case 2: return type(index);
        case 3: return time(index);
        }
        break;
    case Qt::DecorationRole:
        if (index.column() == 0)
            return fileIcon(index);
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return Qt::AlignRight;
        break;
    }

    return QVariant();
}

/*!
    The text representation of the binary size of \a index
 */
QString FtpModel::size(const QModelIndex &index) const
{
    if (!connected())
        return QString();

    const FtpItem *item = ftpItem(index);

    if (item->isDir()) {
#ifdef Q_OS_MAC
        return "--";
#else
        return "";
#endif
    }

    quint64 bytes = item->info.size();
    if (bytes >= 1000000000)
        return QLocale().toString(bytes / 1000000000) + QString(" GB");
    if (bytes >= 1000000)
        return QLocale().toString(bytes / 1000000) + QString(" MB");
    if (bytes >= 1000)
        return QLocale().toString(bytes / 1000) + QString(" KB");
    return QLocale().toString(bytes) + QString(" bytes");
}

/*!
    \reimp
 */
QString FtpModel::type(const QModelIndex &index) const
{
    if (!connected())
        return QString();

    const FtpItem *item = ftpItem(index);

    if (item->isDir())
        return tr("Folder");
    return tr("File");
}

/*!
    \reimp
 */
QString FtpModel::time(const QModelIndex &index) const
{
    if (!connected())
        return QString();

    const FtpItem *item = ftpItem(index);

#ifndef QT_NO_DATESTRING
    return item->info.lastModified().toString(Qt::LocalDate);
#else
    return QString();
#endif
}

/*!
    \reimp
 */
bool FtpModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!connected() || !index.isValid())
        return false;

    if (role == Qt::EditRole && value.type() == QVariant::String) {
        // TODO finish
        /*
        FtpItem *item = ftpItem(index);
        QString oldName = fullPath(index);
        QString newName = value.toString();
        int id = connection->rename(oldName, newName);
        QPair<QString, QString> pair(oldName, newName);
        listingCommands.append(id, pair);
        // TODO move to job finished operation
        //item->info.name() = value.toString();
        //emit dataChanged(index, index);
        return true;
        */
    }
    return false;
}

/*!
    \reimp
 */
Qt::ItemFlags FtpModel::flags(const QModelIndex &index) const
{
    if (!connected())
        return 0;
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    flags |= Qt::ItemIsDragEnabled;
    if (index.column() == 0) {
        const FtpItem *item = ftpItem(index);
        if (item->info.isWritable())
            flags |= Qt::ItemIsEditable;
        if (item->isDir() && item->info.isWritable())
            flags |= Qt::ItemIsDropEnabled;
    }
    return flags;
}

/*!
    \reimp
 */
QVariant FtpModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
        case 0: return tr("Name");
        case 1: return tr("Size");
    #ifdef Q_OS_MAC
        case 2: return tr("Kind");
    #else
        case 2: return tr("Type");
    #endif
        case 3: return tr("Date Modified");
        default : return QVariant();
        }
    }
    return QAbstractItemModel::headerData(section, orientation, role);
}

/*!
    \reimp
 */
QModelIndex FtpModel::parent(const QModelIndex &index) const
{
    if (!connected() || !index.isValid())
        return QModelIndex();

    const FtpItem *item = static_cast<const FtpItem*>(index.internalPointer());
    if (!item->parent || item->parent == (root))
        return QModelIndex();

    const FtpItem *grandparent = item->parent->parent;
    if (!grandparent)
        return QModelIndex();

    for (int i = 0; i < grandparent->children.count(); ++i)
        if (&(grandparent->children.at(i)) == item->parent)
            return createIndex(i, 0, item->parent);

    return QModelIndex();
}

/*!
    \reimp
 */
QModelIndex FtpModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!connected() || !hasIndex(row, column, parent))
        return QModelIndex();

    const FtpItem *parentItem = ftpItem(parent);
    return createIndex(row, column, (void*)(&(parentItem->children[row])));
}

/*!
    \overload

    Returns the model index for a given \a path.
*/
QModelIndex FtpModel::index(const QString &path, int column) const
{
    if (!connected() || column < 0 || column >= columnCount() || path.isEmpty())
        return QModelIndex();

    QStringList builtPath = path.split("/", QString::SkipEmptyParts);
    FtpItem *item = root;

    int r = 0;
    while (!builtPath.isEmpty()) {
        bool found = false;
        for (int i = 0; i < item->children.count(); ++i) {
            if (item->children.at(i).info.name() == builtPath.first()) {
                item = &(item->children[i]);
                builtPath.takeFirst();
                r = i;
                found = true;
                break;
            }
        }
        if (!found)
            return QModelIndex();
    }
    return createIndex(r, column, item);
}

/*!
    Returns the file name for the item stored at \a index
 */
QString FtpModel::fileName(const QModelIndex &index) const
{
    if (!connected())
        return QString();
    return ftpItem(index)->info.name();
}

/*!
    Returns true if the item stored at \a index is a directory otherwise false.
 */
bool FtpModel::isDir(const QModelIndex &index) const
{
    if (!connected())
        return false;
    return ftpItem(index)->isDir();
}

/*!
    Returns the currently set directory filter
 */
QDir::Filters FtpModel::filter() const
{
    return filters;
}

/*!
    Sets the filter used on directory listing.

    \sa refresh
 */
void FtpModel::setFilter(QDir::Filters filters)
{
    this->filters = filters;
}

/*!
    Refreshes the directory located at \a parent.
 */
void FtpModel::refresh(const QModelIndex &parent)
{
    if (!connected() || !isDir(parent))
       return;
    qDebug() <<"refreshing";
    FtpItem *item = parent.isValid() ? static_cast<FtpItem*>(parent.internalPointer()) : root;
    item->children.clear();
    item->fetchedChildren = false;
    fetchMore(parent);
}

/*!
    Returns the icons for the item stored at \a index
 */
QIcon FtpModel::fileIcon(const QModelIndex &index) const
{
    if (!connected() || !index.isValid())
        return QIcon();
    const FtpItem *item = ftpItem(index);
    if (item->isDir())
        return iconProvider->icon(QFileIconProvider::Folder);

    return iconProvider->icon(QFileIconProvider::File);
}

/*!
    Returns the full path for the item stored at \a index
 */
QString FtpModel::filePath(const QModelIndex &index) const
{
    if (!connected())
        return QString();

    const FtpItem *item = ftpItem(index);
    QStringList path;
    while (item && item != root) {
        path.prepend(item->info.name());
        item = item->parent;
    }
    return path.join("/");
}

/*!
    \reimp
 */
bool FtpModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (!connected() || count < 1 || row < 0 || (row + count) > rowCount(parent))
        return false;

    FtpItem *item = parent.isValid() ? static_cast<FtpItem*>(parent.internalPointer()) : root;
    beginRemoveRows(parent, row, row + count - 1);

    // TODO ftp remove calls
    for (int r = 0; r < count; ++r)
        item->children.removeAt(row);

    endRemoveRows();
    return true;
}

/*!
    \reimp
 */
Qt::DropActions FtpModel::supportedDropActions() const
{
    if (!connected())
        return 0;
    return Qt::CopyAction | Qt::MoveAction;
}

/*!
    \reimp
 */
QStringList FtpModel::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

/*!
    \reimp
 */
QMimeData *FtpModel::mimeData(const QModelIndexList &indexes) const
{
    if (!connected())
        return 0;
    QList<QUrl> urls;
    for (int i = 0; i <indexes.count(); ++i) {
        if (indexes.at(i).column() != 0)
            continue;
        QUrl url = ftpUrl;
        url.setPath(filePath(indexes.at(i)));
    }
    QMimeData *data = new QMimeData();
    data->setUrls(urls);
    return data;
}

/*!
    \reimp
 */
bool FtpModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                             int row, int column, const QModelIndex &parent)
{
    if (!connected() || (action != Qt::CopyAction && action != Qt::MoveAction))
        return false;

    QString to = filePath(index(row, column, parent)) + QDir::separator();
    QList<QUrl> urls = data->urls();
    for (int i = 0; i < urls.count(); ++i) {
        QFile *file = new QFile(urls.at(i).path());
        file->open(QIODevice::ReadOnly);
        int cmd = connection.put(file, to + file->fileName());
        if (action == Qt::CopyAction)
            copyCommands[cmd] = file;
        else
            moveCommands[cmd] = file;
    }

    return true;
}

/*!
    Helper function to recursivly sort parent
  */
void FtpModel::sort(FtpItem *parent, Qt::SortOrder order) {
    qSort(parent->children);
    int childCount = parent->children.count();
    if (Qt::AscendingOrder != order) {
        for (int i = 0; i < childCount / 2; ++i)
            parent->children.swap(i, childCount - i - 1);
    }
    for (int i = 0; i < childCount; ++i)
        sort(&parent->children[i], order);
}

/*!
    \reimp
 */
void FtpModel::sort(int column, Qt::SortOrder order)
{
    Q_UNUSED(column);
    if (!connected())
        return;
    sort(root, order);
    emit layoutChanged();
}

void FtpModel::setUrl(const QUrl &url)
{
    ftpUrl = url;
    qDebug() << "connectToHost" ;//<< connection.connectToHost(url.host(), url.port(21));
}

QUrl FtpModel::url() const
{
    return ftpUrl;
}

void FtpModel::gotNewListInfo(const QUrlInfo &info)
{
    if (!connected())
        return;
    // These if's are not "optimal", but it is very readable.
    if (info.isExecutable() && !(filters & QDir::Executable))
        return;
    if (info.isReadable() && !(filters & QDir::Readable))
        return;
    if (info.isWritable() && !(filters & QDir::Writable))
        return;
    if (info.isSymLink() && filters & QDir::NoSymLinks)
        return;
    if (info.isDir() && filters & QDir::Files)
        return;
    if ((info.name() == "." || info.name() == "..") && filters & QDir::NoDotAndDotDot)
        return;
    if (info.name().at(0) == '.' && !(filters & QDir::Hidden))
        return;

    qDebug() << "got new Item";
    QModelIndex idx = index(listing.first());
    beginInsertRows(idx, rowCount(idx), rowCount(idx));

    FtpItem *parentItem = idx.isValid() ? static_cast<FtpItem*>(idx.internalPointer()) : root;

    FtpItem item;
    item.parent = parentItem;
    item.info = info;
    item.fetchedChildren = !info.isDir();
    parentItem->children.append(item);
    endInsertRows();
}

void FtpModel::stateChanged(int state)
{
    if (!connected() && root->fetchedChildren) {
        delete root;
        root = new FtpItem();
        reset();
    }
    switch
 (state) {
    case 0: qDebug() << "QFtp::Unconnected"; break;
    case 1: qDebug() << "QFtp::HostLookup"; break;
    case 2: qDebug() << "QFtp::Connecting"; break;
    case 3: {
        qDebug() << "QFtp::Connected";
        qDebug() << "login" << connection.login(ftpUrl.userName(), ftpUrl.password());
        fetchMore(QModelIndex());
        break;
    }
    case 4: qDebug() << "QFtp::LoggedIn"; break;
    case 5: qDebug() << "QFtp::Closing"; break;
    default:
        qDebug() << "new state" << state;
    }
}

void FtpModel::commandStarted(int id)
{
    qDebug() << "started operation :" << id;
}

void FtpModel::commandFinished(int id, bool error)
{
    if (error)
        qWarning() << "FtpModel" << connection.errorString();

    qDebug() << "finished operation:" << id << (error ? "Error" : "");
    if (!listingCommands.isEmpty() && listingCommands.first() == id) {
        listing.pop_front();
        listingCommands.pop_front();
    }
}

