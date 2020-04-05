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

#ifndef WBDOWNLOADMANAGER_H
#define WBDOWNLOADMANAGER_H

#include "ui_downloads.h"
#include "ui_downloaditem.h"

#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/QUrl>
#include <QNetworkAccessManager>
#include <QWebEngineDownloadItem>
#include <QFileIconProvider>
class WBDownloadManager;
class WBDownloadItem : public QWidget, public Ui_DownloadItem
{
    Q_OBJECT

signals:
    void statusChanged();

public:
    WBDownloadItem(QWebEngineDownloadItem *download, QWidget *parent = 0, QString customDownloadPath = QString());
    bool downloading() const;
    bool downloadedSuccessfully() const;

    void init();
    bool getFileName(bool promptForFileName = false);
 void setUrl(QUrl url)

 {
    m_url=url ;
 }

private slots:
    void stop();

    void open();

    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void finished();
 QString saveFileName(const QString &directory) const;
private:
    friend class WBDownloadManager;
    void updateInfoLabel();
    QString dataString(int size) const;

    QUrl m_url;
    QFileInfo m_file;
        QFile m_output;
    qint64 m_bytesReceived;
    QTime m_downloadTime;
    bool m_stopped;
QString mCustomDownloadPath;
    QScopedPointer<QWebEngineDownloadItem> m_download;
};

class UBAutoSaver;
class WBDownloadModel;


class WBDownloadManager : public QDialog, public Ui_DownloadDialog
{
    Q_OBJECT
    Q_PROPERTY(RemovePolicy removePolicy READ removePolicy WRITE setRemovePolicy)

public:
    enum RemovePolicy {
        Never,
        Exit,
        SuccessFullDownload
    };
    Q_ENUM(RemovePolicy)

    WBDownloadManager(QWidget *parent = 0);
    ~WBDownloadManager();
    int activeDownloads() const;

    RemovePolicy removePolicy() const;
    void setRemovePolicy(RemovePolicy policy);

public slots:
    void download( QWebEngineDownloadItem *downloaditem);
//    inline void download(const QWebEngineDownloadItem *downloaditem)
//        { download(  downloaditem); }
    void cleanup();
void handleUnsupportedContent(QWebEngineDownloadItem *download, QString customDownloadPath = QString());
private slots:
    void save() const;
    void updateRow();


    void processDownloadedContent(QWebEngineDownloadItem *download, QString customDownloadPath = QString());

private:
    void addItem(WBDownloadItem *item);
    void updateItemCount();
    void load();

    UBAutoSaver *m_autoSaver;
    WBDownloadModel *m_model;
     QNetworkAccessManager *mManager;
    QFileIconProvider *m_iconProvider;
    QList<WBDownloadItem*> m_downloads;
    RemovePolicy m_removePolicy;
    friend class WBDownloadModel;
    bool mIsLoading;
};

class WBDownloadModel : public QAbstractListModel
{
    friend class WBDownloadManager;
    Q_OBJECT

public:
    WBDownloadModel(WBDownloadManager *downloadManager, QObject *parent = 0);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

private:
    WBDownloadManager *m_downloadManager;

};

#endif // WBDOWNLOADMANAGER_H
