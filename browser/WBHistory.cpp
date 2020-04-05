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

#include "WBHistory.h"

#include "network/UBAutoSaver.h"
#include "WBBrowserWindow.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTextStream>

#include <QtCore/QtAlgorithms>

#include <QtGui/QClipboard>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QStyle>

#include <QWebEngineSettings>

#include <QtCore/QDebug>
#include "core/UBSettings.h"
static const unsigned int HISTORY_VERSION = 23;

WBHistoryManager::WBHistoryManager(QObject *parent)
    : QObject(parent)
    , m_saveTimer(new UBAutoSaver(this))
    , m_historyLimit(30)
    , m_historyModel(0)
    , m_historyFilterModel(0)
    , m_historyTreeModel(0)
{
    m_expiredTimer.setSingleShot(true);
    connect(&m_expiredTimer, SIGNAL(timeout()),
            this, SLOT(checkForExpired()));
    connect(this, SIGNAL(entryAdded(WBHistoryItem)),
            m_saveTimer, SLOT(changeOccurred()));
    connect(this, SIGNAL(entryRemoved(WBHistoryItem)),
            m_saveTimer, SLOT(changeOccurred()));
    load();

    m_historyModel = new WBHistoryModel(this, this);
    m_historyFilterModel = new WBHistoryFilterModel(m_historyModel, this);
    m_historyTreeModel = new WBHistoryTreeModel(m_historyFilterModel, this);
}

WBHistoryManager::~WBHistoryManager()
{
    m_saveTimer->saveIfNeccessary();
}

QList<WBHistoryItem> &WBHistoryManager::history()
{
    return m_history;
}

bool WBHistoryManager::historyContains(const QString &url) const
{
    return m_historyFilterModel->historyContains(url);
}

void WBHistoryManager::addHistoryEntry(const QString &url)
{
    QUrl cleanUrl(url);
    cleanUrl.setPassword(QString());
    cleanUrl.setHost(cleanUrl.host().toLower());
    WBHistoryItem item(cleanUrl.toString(), QDateTime::currentDateTime());
    addHistoryItem(item);
}

void WBHistoryManager::removeHistoryEntry(const QString &url)
{
    QUrl cleanUrl(url);
    cleanUrl.setPassword(QString());
    cleanUrl.setHost(cleanUrl.host().toLower());
    WBHistoryItem item(cleanUrl.toString(), QDateTime::currentDateTime());
    removeHistoryItem(item);
}

void WBHistoryManager::setHistory(const QList<WBHistoryItem> &history, bool loadedAndSorted)
{
    m_history = history;

    // verify that it is sorted by date
    if (!loadedAndSorted)
        qSort(m_history.begin(), m_history.end());

    checkForExpired(loadedAndSorted);

    if (loadedAndSorted) {
        m_lastSavedUrl = m_history.value(0).url;
    } else {
        m_lastSavedUrl = QString();
        m_saveTimer->changeOccurred();
    }
    emit historyReset();
}

WBHistoryModel *WBHistoryManager::historyModel() const
{
    return m_historyModel;
}

WBHistoryFilterModel *WBHistoryManager::historyFilterModel() const
{
    return m_historyFilterModel;
}

WBHistoryTreeModel *WBHistoryManager::historyTreeModel() const
{
    return m_historyTreeModel;
}

void WBHistoryManager::checkForExpired(bool removeEntriesDirectly)
{
    if (m_historyLimit < 0 || m_history.isEmpty())
        return;

    QDateTime now = QDateTime::currentDateTime();
    int nextTimeout = 0;

    while (!m_history.isEmpty()) {
        QDateTime checkForExpired = m_history.last().dateTime;
        checkForExpired.setDate(checkForExpired.date().addDays(m_historyLimit));
        if (now.daysTo(checkForExpired) > 7) {
            // check at most in a week to prevent int overflows on the timer
            nextTimeout = 7 * 86400;
        } else {
            nextTimeout = now.secsTo(checkForExpired);
        }
        if (nextTimeout > 0)
            break;
        WBHistoryItem item = m_history.takeLast();
        // remove from saved file also
        m_lastSavedUrl = QString();
        emit entryRemoved(item);
    }

    if (nextTimeout > 0)
        m_expiredTimer.start(nextTimeout * 1000);
}

void WBHistoryManager::addHistoryItem(const WBHistoryItem &item)
{
    if (WBBrowserWindow::instance()->privateBrowsing())
        return;
    m_history.prepend(item);
    emit entryAdded(item);
    if (m_history.count() == 1)
        checkForExpired();
}

void WBHistoryManager::removeHistoryItem(const WBHistoryItem &item)
{
    for (int i = m_history.count() - 1 ; i >= 0; --i) {
        if (item.url == m_history.at(i).url) {
            //delete all related entries with that url
            emit entryRemoved(m_history.at(i));
        }
    }
}

void WBHistoryManager::updateHistoryItem(const QUrl &url, const QString &title)
{
    for (int i = 0; i < m_history.count(); ++i) {
        if (url == m_history.at(i).url) {
            m_history[i].title = title;
            m_saveTimer->changeOccurred();
            if (m_lastSavedUrl.isEmpty())
                m_lastSavedUrl = m_history.at(i).url;
            emit entryUpdated(i);
            break;
        }
    }
}

int WBHistoryManager::historyLimit() const
{
    return m_historyLimit;
}

void WBHistoryManager::setHistoryLimit(int limit)
{
    if (m_historyLimit == limit)
        return;
    m_historyLimit = limit;
    checkForExpired();
    m_saveTimer->changeOccurred();
}

void WBHistoryManager::clear()
{
    m_history.clear();

    m_lastSavedUrl = QString();

    m_saveTimer->changeOccurred();
    m_saveTimer->saveIfNeccessary();
     emit historyReset();
}

void WBHistoryManager::loadSettings()
{
    // load settings
    QSettings settings;
    settings.beginGroup(QLatin1String("history"));
    m_historyLimit = settings.value(QLatin1String("historyLimit"), 30).toInt();
}

void WBHistoryManager::load()
{
    loadSettings();
    QFile historyFile(UBSettings::userDataDirectory() + QLatin1String("/history"));

//    QFile historyFile(QStandardPaths::writableLocation(QStandardPaths::DataLocation)
//        + QLatin1String("/history"));
    if (!historyFile.exists())
        return;
    if (!historyFile.open(QFile::ReadOnly)) {
        qWarning() << "Unable to open history file" << historyFile.fileName();
        return;
    }

    QList<WBHistoryItem> list;
    QDataStream in(&historyFile);
    // Double check that the history file is sorted as it is read in
    bool needToSort = false;
    WBHistoryItem lastInsertedItem;
    QByteArray data;
    QDataStream stream;
    QBuffer buffer;
    stream.setDevice(&buffer);
    while (!historyFile.atEnd()) {
        in >> data;
        buffer.close();
        buffer.setBuffer(&data);
        buffer.open(QIODevice::ReadOnly);
        quint32 ver;
        stream >> ver;
        if (ver != HISTORY_VERSION)
            continue;
        WBHistoryItem item;
        stream >> item.url;
        stream >> item.dateTime;
        stream >> item.title;

        if (!item.dateTime.isValid())
            continue;

        if (item == lastInsertedItem) {
            if (lastInsertedItem.title.isEmpty() && !list.isEmpty())
                list[0].title = item.title;
            continue;
        }

        if (!needToSort && !list.isEmpty() && lastInsertedItem < item)
            needToSort = true;

        list.prepend(item);
        lastInsertedItem = item;
    }
    if (needToSort)
        qSort(list.begin(), list.end());

    setHistory(list, true);

    // If we had to sort re-write the whole history sorted
    if (needToSort) {
        m_lastSavedUrl = QString();
        m_saveTimer->changeOccurred();
    }
}

void WBHistoryManager::save()
{
    QSettings settings;
    settings.beginGroup(QLatin1String("history"));
    settings.setValue(QLatin1String("historyLimit"), m_historyLimit);

    bool saveAll = m_lastSavedUrl.isEmpty();
    int first = m_history.count() - 1;
    if (!saveAll) {
        // find the first one to save
        for (int i = 0; i < m_history.count(); ++i) {
            if (m_history.at(i).url == m_lastSavedUrl) {
                first = i - 1;
                break;
            }
        }
    }
    if (first == m_history.count() - 1)
        saveAll = true;

    QString directory = UBSettings::userDataDirectory();
    if (directory.isEmpty())
        directory = QDir::homePath() + QLatin1String("/.") + QCoreApplication::applicationName();
    if (!QFile::exists(directory)) {
        QDir dir;
        dir.mkpath(directory);
    }

    QFile historyFile(directory + QLatin1String("/history"));
    // When saving everything use a temporary file to prevent possible data loss.
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    bool open = false;
    if (saveAll) {
        open = tempFile.open();
    } else {
        open = historyFile.open(QFile::Append);
    }

    if (!open) {
        qWarning() << "Unable to open history file for saving"
                   << (saveAll ? tempFile.fileName() : historyFile.fileName());
        return;
    }

    QDataStream out(saveAll ? &tempFile : &historyFile);
    for (int i = first; i >= 0; --i) {
        QByteArray data;
        QDataStream stream(&data, QIODevice::WriteOnly);
        WBHistoryItem item = m_history.at(i);
        stream << HISTORY_VERSION << item.url << item.dateTime << item.title;
        out << data;
    }
    tempFile.close();

    if (saveAll) {
        if (historyFile.exists() && !historyFile.remove())
            qWarning() << "History: error removing old history." << historyFile.errorString();
        if (!tempFile.rename(historyFile.fileName()))
            qWarning() << "History: error moving new history over old." << tempFile.errorString() << historyFile.fileName();
    }
    m_lastSavedUrl = m_history.value(0).url;
}

WBHistoryModel::WBHistoryModel(WBHistoryManager *history, QObject *parent)
    : QAbstractTableModel(parent)
    , m_history(history)
{
    Q_ASSERT(m_history);
    connect(m_history, SIGNAL(historyReset()),
            this, SLOT(historyReset()));
    connect(m_history, SIGNAL(entryRemoved(WBHistoryItem)),
            this, SLOT(entryRemoved(WBHistoryItem)));

    connect(m_history, SIGNAL(entryAdded(WBHistoryItem)),
            this, SLOT(entryAdded(WBHistoryItem)));
    connect(m_history, SIGNAL(entryUpdated(int)),
            this, SLOT(entryUpdated(int)));
}

void WBHistoryModel::historyReset()
{
    beginResetModel();
    endResetModel();
}

void WBHistoryModel::entryAdded(const WBHistoryItem &item)
{
    beginInsertRows(QModelIndex(), 0, 0);
    endInsertRows();
}

void WBHistoryModel::entryRemoved(const WBHistoryItem &item)
{
    int index = m_history->history().indexOf(item);
    Q_ASSERT(index > -1);
    beginRemoveRows(QModelIndex(),index, index);
    m_history->history().takeAt(index);
    endRemoveRows();
}

void WBHistoryModel::entryUpdated(int offset)
{
    QModelIndex idx = index(offset, 0);
    emit dataChanged(idx, idx);
}

QVariant WBHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal
        && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Title");
            case 1: return tr("Address");
        }
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

QVariant WBHistoryModel::data(const QModelIndex &index, int role) const
{
    QList<WBHistoryItem> lst = m_history->history();
    if (index.row() < 0 || index.row() >= lst.size())
        return QVariant();

    const WBHistoryItem &item = lst.at(index.row());
    switch (role) {
    case DateTimeRole:
        return item.dateTime;
    case DateRole:
        return item.dateTime.date();
    case UrlRole:
        return QUrl(item.url);
    case UrlStringRole:
        return item.url;
    case Qt::DisplayRole:
    case Qt::EditRole: {
        switch (index.column()) {
            case 0:
                // when there is no title try to generate one from the url
                if (item.title.isEmpty()) {
                    QString page = QFileInfo(QUrl(item.url).path()).fileName();
                    if (!page.isEmpty())
                        return page;
                    return item.url;
                }
                return item.title;
            case 1:
                return item.url;
        }
        }
    case Qt::DecorationRole:
        if (index.column() == 0) {
//            return WBBrowserWindow::instance()->icon(item.url);
        }
    }
    return QVariant();
}

int WBHistoryModel::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : 2;
}

int WBHistoryModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : m_history->history().count();
}

bool WBHistoryModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;
    int lastRow = row + count - 1;
    beginRemoveRows(parent, row, lastRow);
    QList<WBHistoryItem> &lst = m_history->history();
    for (int i = lastRow; i >= row; --i)
        lst.removeAt(i);
    disconnect(m_history, SIGNAL(historyReset()), this, SLOT(historyReset()));
    m_history->setHistory(lst);
    connect(m_history, SIGNAL(historyReset()), this, SLOT(historyReset()));
    endRemoveRows();
    return true;
}

#define MOVEDROWS 15

/*
    Maps the first bunch of items of the source model to the root
*/
WBHistoryMenuModel::WBHistoryMenuModel(WBHistoryTreeModel *sourceModel, QObject *parent)
    : QAbstractProxyModel(parent)
    , m_treeModel(sourceModel)
{
    setSourceModel(sourceModel);
}

int WBHistoryMenuModel::bumpedRows() const
{
    QModelIndex first = m_treeModel->index(0, 0);
    if (!first.isValid())
        return 0;
    return qMin(m_treeModel->rowCount(first), MOVEDROWS);
}

int WBHistoryMenuModel::columnCount(const QModelIndex &parent) const
{
    return m_treeModel->columnCount(mapToSource(parent));
}

int WBHistoryMenuModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid()) {
        int folders = sourceModel()->rowCount();
        int bumpedItems = bumpedRows();
        if (bumpedItems <= MOVEDROWS
            && bumpedItems == sourceModel()->rowCount(sourceModel()->index(0, 0)))
            --folders;
        return bumpedItems + folders;
    }

    if (parent.internalId() == -1) {
        if (parent.row() < bumpedRows())
            return 0;
    }

    QModelIndex idx = mapToSource(parent);
    int defaultCount = sourceModel()->rowCount(idx);
    if (idx == sourceModel()->index(0, 0))
        return defaultCount - bumpedRows();
    return defaultCount;
}

QModelIndex WBHistoryMenuModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    // currently not used or autotested
    Q_ASSERT(false);
    int sr = m_treeModel->mapToSource(sourceIndex).row();
    return createIndex(sourceIndex.row(), sourceIndex.column(), sr);
}

QModelIndex WBHistoryMenuModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!proxyIndex.isValid())
        return QModelIndex();

    if (proxyIndex.internalId() == -1) {
        int bumpedItems = bumpedRows();
        if (proxyIndex.row() < bumpedItems)
            return m_treeModel->index(proxyIndex.row(), proxyIndex.column(), m_treeModel->index(0, 0));
        if (bumpedItems <= MOVEDROWS && bumpedItems == sourceModel()->rowCount(m_treeModel->index(0, 0)))
            --bumpedItems;
        return m_treeModel->index(proxyIndex.row() - bumpedItems, proxyIndex.column());
    }

    QModelIndex historyIndex = m_treeModel->sourceModel()->index(proxyIndex.internalId(), proxyIndex.column());
    QModelIndex treeIndex = m_treeModel->mapFromSource(historyIndex);
    return treeIndex;
}

QModelIndex WBHistoryMenuModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0
        || column < 0 || column >= columnCount(parent)
        || parent.column() > 0)
        return QModelIndex();
    if (!parent.isValid())
        return createIndex(row, column, quintptr(-1));

    QModelIndex treeIndexParent = mapToSource(parent);

    int bumpedItems = 0;
    if (treeIndexParent == m_treeModel->index(0, 0))
        bumpedItems = bumpedRows();
    QModelIndex treeIndex = m_treeModel->index(row + bumpedItems, column, treeIndexParent);
    QModelIndex historyIndex = m_treeModel->mapToSource(treeIndex);
    int historyRow = historyIndex.row();
    if (historyRow == -1)
        historyRow = treeIndex.row();
    return createIndex(row, column, historyRow);


}

QModelIndex WBHistoryMenuModel::parent(const QModelIndex &index) const
{
    int offset = index.internalId();
    if (offset == -1 || !index.isValid())
        return QModelIndex();

    QModelIndex historyIndex = m_treeModel->sourceModel()->index(index.internalId(), 0);
    QModelIndex treeIndex = m_treeModel->mapFromSource(historyIndex);
    QModelIndex treeIndexParent = treeIndex.parent();

    int sr = m_treeModel->mapToSource(treeIndexParent).row();
    int bumpedItems = bumpedRows();
    if (bumpedItems <= MOVEDROWS && bumpedItems == sourceModel()->rowCount(sourceModel()->index(0, 0)))
        --bumpedItems;
    return createIndex(bumpedItems + treeIndexParent.row(), treeIndexParent.column(), sr);
}


WBHistoryMenu::WBHistoryMenu(QWidget *parent)
    : WBModelMenu(parent)
    , m_history(0)
{
    connect(this, SIGNAL(activated(QModelIndex)),
            this, SLOT(activated(QModelIndex)));
    setHoverRole(WBHistoryModel::UrlStringRole);
}

void WBHistoryMenu::activated(const QModelIndex &index)
{
    emit openUrl(index.data(WBHistoryModel::UrlRole).toUrl());
}

bool WBHistoryMenu::prePopulated()
{
    if (!m_history) {
        m_history = WBBrowserWindow::historyManager();
        m_historyMenuModel = new WBHistoryMenuModel(m_history->historyTreeModel(), this);
        setModel(m_historyMenuModel);
    }
    // initial actions
    for (int i = 0; i < m_initialActions.count(); ++i)
        addAction(m_initialActions.at(i));
    if (!m_initialActions.isEmpty())
        addSeparator();
    setFirstSeparator(m_historyMenuModel->bumpedRows());

    return false;
}

void WBHistoryMenu::postPopulated()
{
//    if (m_history->history().count() > 0)
//        addSeparator();

//    QAction *showAllAction = new QAction(tr("Show All History"), this);
//    connect(showAllAction, SIGNAL(triggered()), this, SLOT(showHistoryDialog()));
//    addAction(showAllAction);

//    QAction *clearAction = new QAction(tr("Clear History"), this);
//    connect(clearAction, SIGNAL(triggered()), m_history, SLOT(clear()));
//    addAction(clearAction);
}

void WBHistoryMenu::showHistoryDialog()
{
//    WBHistoryDialog *dialog = new WBHistoryDialog(this);
//    connect(dialog, SIGNAL(openUrl(QUrl)),
//            this, SIGNAL(openUrl(QUrl)));
//    dialog->show();
}

void WBHistoryMenu::setInitialActions(QList<QAction*> actions)
{
    m_initialActions = actions;
    for (int i = 0; i < m_initialActions.count(); ++i)
        addAction(m_initialActions.at(i));
}

WBTreeProxyModel::WBTreeProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setSortRole(WBHistoryModel::DateTimeRole);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

bool WBTreeProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    if (!source_parent.isValid())
        return true;
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
}

//WBHistoryDialog::WBHistoryDialog(QWidget *parent, WBHistoryManager *setHistory) : QDialog(parent)
//{
//    WBHistoryManager *history = setHistory;
//    if (!history)
//        history = WBBrowserWindow::historyManager();
//    setupUi(this);
//    tree->setUniformRowHeights(true);
//    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
//    tree->setTextElideMode(Qt::ElideMiddle);
//    QAbstractItemModel *model = history->historyFilterModel();
//    WBTreeProxyModel *proxyModel = new WBTreeProxyModel(this);
//    connect(search, SIGNAL(textChanged(QString)),
//            proxyModel, SLOT(setFilterFixedString(QString)));
//    connect(removeButton, SIGNAL(clicked()), tree, SLOT(removeOne()));
//    connect(removeAllButton, SIGNAL(clicked()), tree, SLOT(removeAll()));
//    proxyModel->setSourceModel(model);
//    proxyModel->setFilterKeyColumn(1);
//    tree->setModel(proxyModel);
//    tree->setSortingEnabled(true);
//    tree->setExpanded(proxyModel->index(0, 0), true);
//    tree->setAlternatingRowColors(true);
//    QFontMetrics fm(font());
//    int header = fm.width(QLatin1Char('m')) * 40;
//    tree->header()->resizeSection(0, header);
//    tree->header()->setStretchLastSection(true);
//    connect(tree, SIGNAL(activated(QModelIndex)),
//            this, SLOT(open()));
//    tree->setContextMenuPolicy(Qt::CustomContextMenu);
//    connect(tree, SIGNAL(customContextMenuRequested(QPoint)),
//            this, SLOT(customContextMenuRequested(QPoint)));
//}

//void WBHistoryDialog::customContextMenuRequested(const QPoint &pos)
//{
//    QMenu menu;
//    QModelIndex index = tree->indexAt(pos);
//    index = index.sibling(index.row(), 0);
//    if (index.isValid() && !tree->model()->hasChildren(index)) {
//        menu.addAction(tr("Open"), this, SLOT(open()));
//        menu.addSeparator();
//        menu.addAction(tr("Copy"), this, SLOT(copy()));
//    }
//    menu.addAction(tr("Delete"), tree, SLOT(removeOne()));
//    menu.exec(QCursor::pos());
//}

//void WBHistoryDialog::open()
//{
//    QModelIndex index = tree->currentIndex();
//    if (!index.parent().isValid())
//        return;
//    emit openUrl(index.data(WBHistoryModel::UrlRole).toUrl());
//}

//void WBHistoryDialog::copy()
//{
//    QModelIndex index = tree->currentIndex();
//    if (!index.parent().isValid())
//        return;
//    QString url = index.data(WBHistoryModel::UrlStringRole).toString();

//    QClipboard *clipboard = QApplication::clipboard();
//    clipboard->setText(url);
//}

WBHistoryFilterModel::WBHistoryFilterModel(QAbstractItemModel *sourceModel, QObject *parent)
    : QAbstractProxyModel(parent),
    m_loaded(false)
{
    setSourceModel(sourceModel);
}
int WBHistoryFilterModel::historyLocation(const QString &url) const
{
    load();
    if (!m_historyHash.contains(url))
        return 0;
    return sourceModel()->rowCount() - m_historyHash.value(url);
}
QVariant WBHistoryFilterModel::data(const QModelIndex &index, int role) const
{
    return QAbstractProxyModel::data(index, role);
}
void WBHistoryFilterModel::setSourceModel(QAbstractItemModel *newSourceModel)
{
//    beginResetModel();
    if (sourceModel()) {
        disconnect(sourceModel(), SIGNAL(modelReset()), this, SLOT(sourceReset()));
        disconnect(sourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
        disconnect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (sourceModel()) {
         m_loaded = false;
        connect(sourceModel(), SIGNAL(modelReset()), this, SLOT(sourceReset()));
        connect(sourceModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)),
                   this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
        connect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
    }
    load();
//    endResetModel();
}

void WBHistoryFilterModel::sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
{
    emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight));
}

QVariant WBHistoryFilterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}

void WBHistoryFilterModel::sourceReset()
{
    m_loaded = false;
    beginResetModel();
    endResetModel();
}

int WBHistoryFilterModel::rowCount(const QModelIndex &parent) const
{
    load();
    if (parent.isValid())
        return 0;
    return m_historyHash.count();
}

int WBHistoryFilterModel::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : 2;
}

QModelIndex WBHistoryFilterModel::mapToSource(const QModelIndex &proxyIndex) const
{
    load();
    int sourceRow = sourceModel()->rowCount() - proxyIndex.internalId();
    return sourceModel()->index(sourceRow, proxyIndex.column());
}

QModelIndex WBHistoryFilterModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    load();
    QString url = sourceIndex.data(WBHistoryModel::UrlStringRole).toString();
    if (!m_historyHash.contains(url))
        return QModelIndex();

    // This can be done in a binary search, but we can't use qBinary find
    // because it can't take: qBinaryFind(m_sourceRow.end(), m_sourceRow.begin(), v);
    // so if this is a performance bottlneck then convert to binary search, until then
    // the cleaner/easier to read code wins the day.
    int realRow = -1;
    int sourceModelRow = sourceModel()->rowCount() - sourceIndex.row();

    for (int i = 0; i < m_sourceRow.count(); ++i) {
        if (m_sourceRow.at(i) == sourceModelRow) {
            realRow = i;
            break;
        }
    }
    if (realRow == -1)
        return QModelIndex();

    return createIndex(realRow, sourceIndex.column(), sourceModel()->rowCount() - sourceIndex.row());
}

QModelIndex WBHistoryFilterModel::index(int row, int column, const QModelIndex &parent) const
{
     load();
    if (row < 0 || row >= rowCount(parent)
        || column < 0 || column >= columnCount(parent))
        return QModelIndex();

    return createIndex(row, column, m_sourceRow[row]);
}

QModelIndex WBHistoryFilterModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

void WBHistoryFilterModel::load() const
{
    if (m_loaded)
        return;
    m_sourceRow.clear();
    m_historyHash.clear();
    m_historyHash.reserve(sourceModel()->rowCount());
    for (int i = 0; i < sourceModel()->rowCount(); ++i) {
        QModelIndex idx = sourceModel()->index(i, 0);
        QString url = idx.data(WBHistoryModel::UrlStringRole).toString();
        if (!m_historyHash.contains(url)) {
            m_sourceRow.append(sourceModel()->rowCount() - i);
            m_historyHash[url] = sourceModel()->rowCount() - i;
        }
    }
     m_loaded = true;
}

void WBHistoryFilterModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_ASSERT(start == end && start == 0);
    Q_UNUSED(end);
    QModelIndex idx = sourceModel()->index(start, 0, parent);
    QString url = idx.data(WBHistoryModel::UrlStringRole).toString();
    if (m_historyHash.contains(url)) {
        int sourceRow = sourceModel()->rowCount() - m_historyHash[url];
        int realRow = mapFromSource(sourceModel()->index(sourceRow, 0)).row();
        beginRemoveRows(QModelIndex(), realRow, realRow);
        m_sourceRow.removeAt(realRow);
        m_historyHash.remove(url);
        endRemoveRows();
    }
    beginInsertRows(QModelIndex(), 0, 0);
    m_historyHash.insert(url, sourceModel()->rowCount() - start);
    m_sourceRow.insert(0, sourceModel()->rowCount());
    endInsertRows();
}

void WBHistoryFilterModel::sourceRowsRemoved(const QModelIndex &, int start, int end)
{
    Q_UNUSED(start);
    Q_UNUSED(end);
    sourceReset();
}

/*
    Removing a continuous block of rows will remove filtered rows too as this is
    the users intention.
*/
bool WBHistoryFilterModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count <= 0 || row + count > rowCount(parent) || parent.isValid())
        return false;
    int lastRow = row + count - 1;
    disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
    beginRemoveRows(parent, row, lastRow);
    int oldCount = rowCount();
    int start = sourceModel()->rowCount() - m_sourceRow.value(row);
    int end = sourceModel()->rowCount() - m_sourceRow.value(lastRow);
    sourceModel()->removeRows(start, end - start + 1);
    endRemoveRows();
    connect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
    m_loaded = false;
    if (oldCount - count != rowCount()) {
        beginResetModel();
        endResetModel();
    }
    return true;
}

WBHistoryCompletionModel::WBHistoryCompletionModel(QObject *parent)
    : QAbstractProxyModel(parent)
{
}

QVariant WBHistoryCompletionModel::data(const QModelIndex &index, int role) const
{
    if (sourceModel()
        && (role == Qt::EditRole || role == Qt::DisplayRole)
        && index.isValid()) {
        QModelIndex idx = mapToSource(index);
        idx = idx.sibling(idx.row(), 1);
        QString urlString = idx.data(WBHistoryModel::UrlStringRole).toString();
        if (index.row() % 2) {
            QUrl url = urlString;
            QString s = url.toString(QUrl::RemoveScheme
                                     | QUrl::RemoveUserInfo
                                     | QUrl::StripTrailingSlash);
            return s.mid(2);  // strip // from the front
        }
        return urlString;
    }
    return QAbstractProxyModel::data(index, role);
}

int WBHistoryCompletionModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid() || !sourceModel()) ? 0 : sourceModel()->rowCount(parent) * 2;
}

int WBHistoryCompletionModel::columnCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : 1;
}

QModelIndex WBHistoryCompletionModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    int row = sourceIndex.row() * 2;
    return index(row, sourceIndex.column());
}

QModelIndex WBHistoryCompletionModel::mapToSource(const QModelIndex &proxyIndex) const
{
    if (!sourceModel())
        return QModelIndex();
    int row = proxyIndex.row() / 2;
    return sourceModel()->index(row, proxyIndex.column());
}

QModelIndex WBHistoryCompletionModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0 || row >= rowCount(parent)
        || column < 0 || column >= columnCount(parent))
        return QModelIndex();
    return createIndex(row, column);
}

QModelIndex WBHistoryCompletionModel::parent(const QModelIndex &) const
{
    return QModelIndex();
}

void WBHistoryCompletionModel::setSourceModel(QAbstractItemModel *newSourceModel)
{
    if (sourceModel()) {
        disconnect(sourceModel(), SIGNAL(modelReset()), this, SLOT(sourceReset()));
        disconnect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceReset()));
        disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceReset()));
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (newSourceModel) {
        connect(newSourceModel, SIGNAL(modelReset()), this, SLOT(sourceReset()));
        connect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceReset()));
        connect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceReset()));
    }

    beginResetModel();
    endResetModel();
}

void WBHistoryCompletionModel::sourceReset()
{
    beginResetModel();
    endResetModel();
}

WBHistoryTreeModel::WBHistoryTreeModel(QAbstractItemModel *sourceModel, QObject *parent)
    : QAbstractProxyModel(parent)
{
    setSourceModel(sourceModel);
}

QVariant WBHistoryTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return sourceModel()->headerData(section, orientation, role);
}

QVariant WBHistoryTreeModel::data(const QModelIndex &index, int role) const
{
    if ((role == Qt::EditRole || role == Qt::DisplayRole)) {
        int start = index.internalId();
        if (start == 0) {
            int offset = sourceDateRow(index.row());
            if (index.column() == 0) {
                QModelIndex idx = sourceModel()->index(offset, 0);
                QDate date = idx.data(WBHistoryModel::DateRole).toDate();
                if (date == QDate::currentDate())
                    return tr("Earlier Today");
                return date.toString(QLatin1String("dddd, MMMM d, yyyy"));
            }
            if (index.column() == 1) {
                return tr("%1 items").arg(rowCount(index.sibling(index.row(), 0)));
            }
        }
    }
    if (role == Qt::DecorationRole && index.column() == 0 && !index.parent().isValid())
        return QIcon(QLatin1String(":history.png"));
    if (role == WBHistoryModel::DateRole && index.column() == 0 && index.internalId() == 0) {
        int offset = sourceDateRow(index.row());
        QModelIndex idx = sourceModel()->index(offset, 0);
        return idx.data(WBHistoryModel::DateRole);
    }

    return QAbstractProxyModel::data(index, role);
}

int WBHistoryTreeModel::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount(mapToSource(parent));
}

int WBHistoryTreeModel::rowCount(const QModelIndex &parent) const
{
    if ( parent.internalId() != 0
        || parent.column() > 0
        || !sourceModel())
        return 0;

    // row count OF dates
    if (!parent.isValid()) {
        if (!m_sourceRowCache.isEmpty())
            return m_sourceRowCache.count();
        QDate currentDate;
        int rows = 0;
        int totalRows = sourceModel()->rowCount();

        for (int i = 0; i < totalRows; ++i) {
            QDate rowDate = sourceModel()->index(i, 0).data(WBHistoryModel::DateRole).toDate();
            if (rowDate != currentDate) {
                m_sourceRowCache.append(i);
                currentDate = rowDate;
                ++rows;
            }
        }
        Q_ASSERT(m_sourceRowCache.count() == rows);
        return rows;
    }

    // row count FOR a date
    int start = sourceDateRow(parent.row());
    int end = sourceDateRow(parent.row() + 1);
    return (end - start);
}

// Translate the top level date row into the offset where that date starts
int WBHistoryTreeModel::sourceDateRow(int row) const
{
    if (row <= 0)
        return 0;

    if (m_sourceRowCache.isEmpty())
        rowCount(QModelIndex());

    if (row >= m_sourceRowCache.count()) {
        if (!sourceModel())
            return 0;
        return sourceModel()->rowCount();
    }
    return m_sourceRowCache.at(row);
}

QModelIndex WBHistoryTreeModel::mapToSource(const QModelIndex &proxyIndex) const
{
    int offset = proxyIndex.internalId();
    if (offset == 0)
        return QModelIndex();
    int startDateRow = sourceDateRow(offset - 1);
    return sourceModel()->index(startDateRow + proxyIndex.row(), proxyIndex.column());
}

QModelIndex WBHistoryTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (row < 0
        || column < 0 || column >= columnCount(parent)
        || parent.column() > 0)
        return QModelIndex();

    if (!parent.isValid())
        return createIndex(row, column);
    return createIndex(row, column, parent.row() + 1);
}

QModelIndex WBHistoryTreeModel::parent(const QModelIndex &index) const
{
    int offset = index.internalId();
    if (offset == 0 || !index.isValid())
        return QModelIndex();
    return createIndex(offset - 1, 0);
}

bool WBHistoryTreeModel::hasChildren(const QModelIndex &parent) const
{
    QModelIndex grandparent = parent.parent();
    if (!grandparent.isValid())
        return true;
    return false;
}

Qt::ItemFlags WBHistoryTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
}

bool WBHistoryTreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || count <= 0 || row + count > rowCount(parent))
        return false;

    if (parent.isValid()) {
        // removing pages
        int offset = sourceDateRow(parent.row());
        return sourceModel()->removeRows(offset + row, count);
    } else {
        // removing whole dates
        for (int i = row + count - 1; i >= row; --i) {
            QModelIndex dateParent = index(i, 0);
            int offset = sourceDateRow(dateParent.row());
            if (!sourceModel()->removeRows(offset, rowCount(dateParent)))
                return false;
        }
    }
    return true;
}

void WBHistoryTreeModel::setSourceModel(QAbstractItemModel *newSourceModel)
{

    if (sourceModel()) {
        disconnect(sourceModel(), SIGNAL(modelReset()), this, SLOT(sourceReset()));
        disconnect(sourceModel(), SIGNAL(layoutChanged()), this, SLOT(sourceReset()));
        disconnect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
        disconnect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
        disconnect(sourceModel(), &QAbstractItemModel::dataChanged, this,
                   &WBHistoryTreeModel::sourceDataChanged);
    }

    QAbstractProxyModel::setSourceModel(newSourceModel);

    if (newSourceModel) {
        connect(sourceModel(), SIGNAL(modelReset()), this, SLOT(sourceReset()));
        connect(sourceModel(), SIGNAL(layoutChanged()), this, SLOT(sourceReset()));
        connect(sourceModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(sourceRowsInserted(QModelIndex,int,int)));
        connect(sourceModel(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
                this, SLOT(sourceRowsRemoved(QModelIndex,int,int)));
        connect(sourceModel(), &QAbstractItemModel::dataChanged, this,
                &WBHistoryTreeModel::sourceDataChanged);
    }
       beginResetModel();
    endResetModel();
}

void WBHistoryTreeModel::sourceReset()
{
     m_sourceRowCache.clear();
    beginResetModel();

    endResetModel();
}

void WBHistoryTreeModel::sourceRowsInserted(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent); // Avoid warnings when compiling release
    Q_ASSERT(!parent.isValid());
    if (start != 0 || start != end) {
          m_sourceRowCache.clear();
        beginResetModel();

        endResetModel();
        return;
    }

    m_sourceRowCache.clear();
    QModelIndex treeIndex = mapFromSource(sourceModel()->index(start, 0));
    QModelIndex treeParent = treeIndex.parent();
    if (rowCount(treeParent) == 1) {
        beginInsertRows(QModelIndex(), 0, 0);
        endInsertRows();
    } else {
        beginInsertRows(treeParent, treeIndex.row(), treeIndex.row());
        endInsertRows();
    }
}

QModelIndex WBHistoryTreeModel::mapFromSource(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid())
        return QModelIndex();

    if (m_sourceRowCache.isEmpty())
        rowCount(QModelIndex());

    QList<int>::iterator it;
    it = qLowerBound(m_sourceRowCache.begin(), m_sourceRowCache.end(), sourceIndex.row());
    if (*it != sourceIndex.row())
        --it;
    int dateRow = qMax(0, it - m_sourceRowCache.begin());
    int row = sourceIndex.row() - m_sourceRowCache.at(dateRow);
    return createIndex(row, sourceIndex.column(), dateRow + 1);
}

void WBHistoryTreeModel::sourceRowsRemoved(const QModelIndex &parent, int start, int end)
{
    Q_UNUSED(parent); // Avoid warnings when compiling release
    Q_ASSERT(!parent.isValid());
    if (m_sourceRowCache.isEmpty())
        return;
    for (int i = end; i >= start;) {
        QList<int>::iterator it;
        it = qLowerBound(m_sourceRowCache.begin(), m_sourceRowCache.end(), i);
        // playing it safe
        if (it == m_sourceRowCache.end()) {
             m_sourceRowCache.clear();
            beginResetModel();

            endResetModel();
            return;
        }

        if (*it != i)
            --it;
        int row = qMax(0, it - m_sourceRowCache.begin());
        int offset = m_sourceRowCache[row];
        QModelIndex dateParent = index(row, 0);
        // If we can remove all the rows in the date do that and skip over them
        int rc = rowCount(dateParent);
        if (i - rc + 1 == offset && start <= i - rc + 1) {
            beginRemoveRows(QModelIndex(), row, row);
            m_sourceRowCache.removeAt(row);
            i -= rc + 1;
        } else {
            beginRemoveRows(dateParent, i - offset, i - offset);
            ++row;
            --i;
        }
        for (int j = row; j < m_sourceRowCache.count(); ++j)
            --m_sourceRowCache[j];
        endRemoveRows();
    }
}

void WBHistoryTreeModel::sourceDataChanged(const QModelIndex &topLeft,
                                         const QModelIndex &bottomRight,
                                         const QVector<int> roles)
{
     emit dataChanged(mapFromSource(topLeft), mapFromSource(bottomRight), roles);
}
