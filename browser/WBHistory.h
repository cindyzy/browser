/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef WBHISTORY_H
#define WBHISTORY_H

#include "WBModelMenu.h"

#include <QtCore/QDateTime>
#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QUrl>

#include <QtCore/QSortFilterProxyModel>

class WBHistoryItem
{
public:
    WBHistoryItem() {}
    WBHistoryItem(const QString &u,
                const QDateTime &d = QDateTime(), const QString &t = QString())
        : title(t), url(u), dateTime(d) {}

    inline bool operator==(const WBHistoryItem &other) const
        { return other.title == title
          && other.url == url && other.dateTime == dateTime; }

    // history is sorted in reverse
    inline bool operator <(const WBHistoryItem &other) const
        { return dateTime > other.dateTime; }

    QString title;
    QString url;
    QDateTime dateTime;
};

class UBAutoSaver;
class WBHistoryModel;
class WBHistoryFilterModel;
class WBHistoryTreeModel;

class WBHistoryManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int historyLimit READ historyLimit WRITE setHistoryLimit)

signals:
    void historyReset();
    void entryAdded(const WBHistoryItem &item);
    void entryRemoved(const WBHistoryItem &item);
    void entryUpdated(int offset);

public:
    WBHistoryManager(QObject *parent = 0);
    ~WBHistoryManager();

    bool historyContains(const QString &url) const;

    void addHistoryEntry(const QString &url);
    void removeHistoryEntry(const QString &url);

    void updateHistoryItem(const QUrl &url, const QString &title);

    int historyLimit() const;
    void setHistoryLimit(int limit);

    QList<WBHistoryItem>& history();
    void setHistory(const QList<WBHistoryItem> &history, bool loadedAndSorted = false);

    // History manager keeps around these models for use by the completer and other classes
    WBHistoryModel *historyModel() const;
    WBHistoryFilterModel *historyFilterModel() const;
    WBHistoryTreeModel *historyTreeModel() const;

public slots:
    void clear();
    void loadSettings();

private slots:
    void save();
    void checkForExpired(bool removeExpiredEntriesDirectly = false);

protected:
    void addHistoryItem(const WBHistoryItem &item);
    void removeHistoryItem(const WBHistoryItem &item);

private:
    void load();

    UBAutoSaver *m_saveTimer;
    int m_historyLimit;
    QTimer m_expiredTimer;
    QList<WBHistoryItem> m_history;
    QString m_lastSavedUrl;

    WBHistoryModel *m_historyModel;
    WBHistoryFilterModel *m_historyFilterModel;
    WBHistoryTreeModel *m_historyTreeModel;
};

class WBHistoryModel : public QAbstractTableModel
{
    Q_OBJECT

public slots:
    void historyReset();
    void entryAdded(const WBHistoryItem &item);
    void entryRemoved(const WBHistoryItem &item);
    void entryUpdated(int offset);

public:
    enum Roles {
        DateRole = Qt::UserRole + 1,
        DateTimeRole = Qt::UserRole + 2,
        UrlRole = Qt::UserRole + 3,
        UrlStringRole = Qt::UserRole + 4
    };

    WBHistoryModel(WBHistoryManager *history, QObject *parent = 0);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    WBHistoryManager *m_history;
};

/*!
    Proxy model that will remove any duplicate entries.
    Both m_sourceRow and m_historyHash store their offsets not from
    the front of the list, but as offsets from the back.
  */
class WBHistoryFilterModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    WBHistoryFilterModel(QAbstractItemModel *sourceModel, QObject *parent = 0);

    inline bool historyContains(const QString &url) const
        { load(); return m_historyHash.contains(url); }
 int historyLocation(const QString &url) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    void setSourceModel(QAbstractItemModel *sourceModel);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index(int, int, const QModelIndex& = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index= QModelIndex()) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

private slots:
    void sourceReset();
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight);
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex &, int, int);

private:
    void load() const;

    mutable QList<int> m_sourceRow;
    mutable QHash<QString, int> m_historyHash;
    mutable bool m_loaded;
};

/*
    The history menu
    - Removes the first twenty entries and puts them as children of the top level.
    - If there are less then twenty entries then the first folder is also removed.

    The mapping is done by knowing that HistoryTreeModel is over a table
    We store that row offset in our index's private data.
*/
class WBHistoryMenuModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    WBHistoryMenuModel(WBHistoryTreeModel *sourceModel, QObject *parent = 0);
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex mapFromSource(const QModelIndex & sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex & proxyIndex) const;
    QModelIndex index(int, int, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index = QModelIndex()) const;

    int bumpedRows() const;

private:
    WBHistoryTreeModel *m_treeModel;
};

// Menu that is dynamically populated from the history
class WBHistoryMenu : public WBModelMenu
{
    Q_OBJECT

signals:
    void openUrl(const QUrl &url);

public:
     WBHistoryMenu(QWidget *parent = 0);
     void setInitialActions(QList<QAction*> actions);

protected:
    bool prePopulated();
    void postPopulated();

private slots:
    void activated(const QModelIndex &index);
    void showHistoryDialog();

private:
    WBHistoryManager *m_history;
    WBHistoryMenuModel *m_historyMenuModel;
    QList<QAction*> m_initialActions;
};

// proxy model for the history model that
// exposes each url http://www.foo.com and it url starting at the host www.foo.com
class WBHistoryCompletionModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    WBHistoryCompletionModel(QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex index(int, int, const QModelIndex& = QModelIndex()) const;
    QModelIndex parent(const QModelIndex& index= QModelIndex()) const;
    void setSourceModel(QAbstractItemModel *sourceModel);

private slots:
    void sourceReset();

};

// proxy model for the history model that converts the list
// into a tree, one top level node per day.
// Used in the HistoryDialog.
class WBHistoryTreeModel : public QAbstractProxyModel
{
    Q_OBJECT

public:
    WBHistoryTreeModel(QAbstractItemModel *sourceModel, QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const;
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index= QModelIndex()) const;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    void setSourceModel(QAbstractItemModel *sourceModel);

private slots:
    void sourceReset();
    void sourceRowsInserted(const QModelIndex &parent, int start, int end);
    void sourceRowsRemoved(const QModelIndex &parent, int start, int end);
    void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                           const QVector<int> roles);
private:
    int sourceDateRow(int row) const;
    mutable QList<int> m_sourceRowCache;

};

// A modified QSortFilterProxyModel that always accepts the root nodes in the tree
// so filtering is only done on the children.
// Used in the HistoryDialog
class WBTreeProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    WBTreeProxyModel(QObject *parent = 0);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};

//#include "ui_history.h"

//class WBHistoryDialog : public QDialog, public Ui_HistoryDialog
//{
//    Q_OBJECT

//signals:
//    void openUrl(const QUrl &url);

//public:
//    WBHistoryDialog(QWidget *parent = 0, WBHistoryManager *history = 0);

//private slots:
//    void customContextMenuRequested(const QPoint &pos);
//    void open();
//    void copy();

//};

#endif // WBHISTORY_H
