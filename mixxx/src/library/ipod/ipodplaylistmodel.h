#ifndef IPODPLAYLISTMODEL_H
#define IPODPLAYLISTMODEL_H

#include <QtCore>
#include <QHash>
#include <QtGui>
#include <QtSql>

#include "library/trackmodel.h"
#include "library/trackcollection.h"
#include "library/dao/trackdao.h"

extern "C"
{
#include <gpod/itdb.h>
}

// BaseSqlTableModel is a custom-written SQL-backed table which aggressively
// caches the contents of the table and supports lightweight updates.
class IPodPlaylistModel : public QAbstractTableModel , public virtual TrackModel
{
    Q_OBJECT
  public:
    IPodPlaylistModel(QObject* pParent, TrackCollection* pTrackCollection);
    virtual ~IPodPlaylistModel();

    ////////////////////////////////////////////////////////////////////////////
    // Methods implemented from QAbstractItemModel
    ////////////////////////////////////////////////////////////////////////////

    virtual TrackPointer getTrack(const QModelIndex& index) const;
    virtual QString getTrackLocation(const QModelIndex& index) const;
    virtual int getTrackId(const QModelIndex& index) const;
    virtual const QLinkedList<int> getTrackRows(int trackId) const;
    virtual void search(const QString& searchText);
    virtual const QString currentSearch();
    virtual bool isColumnInternal(int column);
    virtual bool isColumnHiddenByDefault(int column);
    virtual void removeTrack(const QModelIndex& index);
    virtual void removeTracks(const QModelIndexList& indices);
    virtual bool addTrack(const QModelIndex& index, QString location);
    virtual void moveTrack(const QModelIndex& sourceIndex, const QModelIndex& destIndex);

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    // QMimeData* mimeData(const QModelIndexList &indexes) const;

    QItemDelegate* delegateForColumn(const int i);
    TrackModel::CapabilitiesFlags getCapabilities() const;

    virtual void sort(int column, Qt::SortOrder order);
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role=Qt::EditRole);
    virtual int columnCount(const QModelIndex& parent=QModelIndex()) const;
    virtual int rowCount(const QModelIndex& parent=QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation,
                                int role=Qt::DisplayRole) const;

    void setPlaylist(Itdb_Playlist* pPlaylist);

    ////////////////////////////////////////////////////////////////////////////
    // Other public methods
    ////////////////////////////////////////////////////////////////////////////

    virtual void search(const QString& searchText, const QString extraFilter=QString());
    virtual QString currentSearch() const;
    virtual void setSort(int column, Qt::SortOrder order);
    virtual int fieldIndex(const QString& fieldName) const;

  protected:
    /** Use this if you want a model that is read-only. */
    virtual Qt::ItemFlags readOnlyFlags(const QModelIndex &index) const;
    /** Use this if you want a model that can be changed  */
    virtual Qt::ItemFlags readWriteFlags(const QModelIndex &index) const;

    // Set the columns used for searching. Names must correspond to the column
    // names in the table provided to setTable. Must be called after setTable is
    // called.
    virtual void setSearchColumns(const QStringList& searchColumns);
    // virtual QString orderByClause() const;
    virtual void initHeaderData();
    virtual void initDefaultSearchColumns();

  private slots:
    void trackChanged(int trackId);

  private:
    inline TrackPointer lookupCachedTrack(int trackId) const;
    inline QVariant getTrackValueForColumn(TrackPointer pTrack, int column) const;
    inline QVariant getTrackValueForColumn(int trackId, int column,
                                           TrackPointer pTrack=TrackPointer()) const;
    inline void setTrackValueForColumn(TrackPointer pTrack, int column, QVariant value);
    QVariant getBaseValue(const QModelIndex& index, int role = Qt::DisplayRole) const;

    virtual int compareColumnValues(int iColumnNumber, Qt::SortOrder eSortOrder, QVariant val1, QVariant val2);
    virtual int findSortInsertionPoint(int trackId, TrackPointer pTrack,
                                       const QVector<QPair<int, QHash<int, QVariant> > >& rowInfo);

    Itdb_Track* getPTrackFromModelIndex(const QModelIndex& index) const;

    QString m_tableName;
    QStringList m_columnNames;
    QString m_columnNamesJoined;
    QHash<QString, int> m_columnIndex;
    QSet<QString> m_tableColumns;
    QString m_tableColumnsJoined;
    QSet<int> m_tableColumnIndices;

    QStringList m_searchColumns;
    QVector<int> m_searchColumnIndices;
    QString m_idColumn;

    int m_iSortColumn;
    Qt::SortOrder m_eSortOrder;

    bool m_bIndexBuilt;
    QSqlRecord m_queryRecord;
    QHash<int, QVector<QVariant> > m_recordCache;
    QVector<QPair<int, QHash<int, QVariant> > > m_rowInfo;
    QHash<int, QLinkedList<int> > m_trackIdToRows;
    QSet<int> m_trackOverrides;

    QString m_currentSearch;
    QString m_currentSearchFilter;

    QList<QPair<QString, size_t> > m_headerList;

    TrackCollection* m_pTrackCollection;
    TrackDAO& m_trackDAO;

    Itdb_Playlist* m_pPlaylist;
};

#endif /* IPODPLAYLISTMODEL_H */
