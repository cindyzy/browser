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

#include "WBDownloadManager.h"

#include "network/UBAutoSaver.h"
#include "WBBrowserWindow.h"

#include <math.h>

#include <QtCore/QMetaEnum>
#include <QtCore/QSettings>

#include <QtGui/QDesktopServices>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QFileIconProvider>

#include <QtCore/QDebug>

#include <QWebEngineSettings>
#include <QWebEngineDownloadItem>
#include "frameworks/UBFileSystemUtils.h"
#include "network/UBNetworkAccessManager.h"
#include "core/UBApplication.h"
#include "gui/UBLoginManager.h"
#include <gui/UBBookManager.h>
/*!
    DownloadWidget is a widget that is displayed in the download manager list.
    It moves the data from the QWebEngineDownloadItem into the QFile as well
    as update the information/progressbar and report errors.
 */

WBDownloadItem::WBDownloadItem(QWebEngineDownloadItem *download, QWidget *parent, QString customDownloadPath)
    : QWidget(parent)
    , m_bytesReceived(0)
    , m_download(download)
     , mCustomDownloadPath(customDownloadPath)

{
    setupUi(this);

    QPalette p = downloadInfoLabel->palette();
    p.setColor(QPalette::Text, Qt::darkGray);
    downloadInfoLabel->setPalette(p);
    progressBar->setMaximum(0);
    connect(stopButton, SIGNAL(clicked()), this, SLOT(stop()));
    connect(openButton, SIGNAL(clicked()), this, SLOT(open()));

    if (download) {
        m_file.setFile(download->path());
        m_url = download->url();
    }

    init();
}

void WBDownloadItem::init()
{
    if (m_download) {
        connect(m_download.data(), SIGNAL(downloadProgress(qint64,qint64)),
                this, SLOT(downloadProgress(qint64,qint64)));
        connect(m_download.data(), SIGNAL(finished()),
                this, SLOT(finished()));
    }

    // reset info
    downloadInfoLabel->clear();
    progressBar->setValue(0);
    getFileName();

    // start timer for the download estimation
    m_downloadTime.start();
}

bool WBDownloadItem::getFileName(bool promptForFileName)
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QString defaultLocation = !mCustomDownloadPath.isEmpty() ? mCustomDownloadPath :  QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    if (m_file.absoluteDir().exists())
        defaultLocation = m_file.absolutePath();
    QString downloadDirectory = settings.value(QLatin1String("downloadDirectory"), defaultLocation).toString();
    if (!downloadDirectory.isEmpty())
        downloadDirectory += QLatin1Char('/');

    QString defaultFileName = saveFileName(downloadDirectory);
    QString fileName = defaultFileName;
    if (promptForFileName) {
        fileName = QFileDialog::getSaveFileName(this, tr("Save File"), defaultFileName);
        if (fileName.isEmpty()) {
            if (m_download)
                m_download->cancel();
            fileNameLabel->setText(tr("Download canceled: %1").arg(QFileInfo(defaultFileName).fileName()));
            return false;
        }
    }
    m_file.setFile(fileName);

    if (m_download && m_download->state() == QWebEngineDownloadItem::DownloadRequested)
        m_download->setPath(m_file.absoluteFilePath());

    fileNameLabel->setText(m_file.fileName());
    return true;
}
QString WBDownloadItem::saveFileName(const QString &directory) const
{
    // Move this function into QNetworkReply to also get file name sent from the server
    QString path = m_url.path();
    QFileInfo info(path);
    QString baseName = info.completeBaseName();
    QString endName = info.suffix();

    if (baseName.isEmpty())
    {
        baseName = QLatin1String("unnamed_download");
        qDebug() << "DownloadManager:: downloading unknown file:" << m_url;
    }
    QString name = directory + baseName + QLatin1Char('.') + endName;
    if (QFile::exists(name))
    {
        // already exists, don't overwrite
        int i = 1;
        do {
            name = directory + baseName + QLatin1Char('-') + QString::number(i++) + QLatin1Char('.') + endName;
        } while (QFile::exists(name));
    }
    return name;
}

void WBDownloadItem::stop()
{
    setUpdatesEnabled(false);
    stopButton->setEnabled(false);
    stopButton->hide();
    setUpdatesEnabled(true);
    if (m_download)
        m_download->cancel();

    emit statusChanged();
}

void WBDownloadItem::open()
{
    QUrl url = QUrl::fromLocalFile(m_file.absoluteFilePath());
    QDesktopServices::openUrl(url);
}


void WBDownloadItem::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    m_bytesReceived = bytesReceived;
    if (bytesTotal == -1) {
        progressBar->setValue(0);
        progressBar->setMaximum(0);
    } else {
        progressBar->setValue(bytesReceived);
        progressBar->setMaximum(bytesTotal);
    }
    updateInfoLabel();
}

void WBDownloadItem::updateInfoLabel()
{
    qint64 bytesTotal = progressBar->maximum();

    // update info label
    double speed = m_bytesReceived * 1000.0 / m_downloadTime.elapsed();
    double timeRemaining = ((double)(bytesTotal - m_bytesReceived)) / speed;
    QString timeRemainingString = tr("seconds");
    if (timeRemaining > 60) {
        timeRemaining = timeRemaining / 60;
        timeRemainingString = tr("minutes");
    }
    timeRemaining = floor(timeRemaining);

    // When downloading the eta should never be 0
    if (timeRemaining == 0)
        timeRemaining = 1;

    QString info;
    if (!downloadedSuccessfully()) {
        QString remaining;
        if (bytesTotal != 0)
            remaining = tr("- %4 %5 remaining")
            .arg(timeRemaining)
            .arg(timeRemainingString);
        info = tr("%1 of %2 (%3/sec) %4")
            .arg(dataString(m_bytesReceived))
            .arg(bytesTotal == 0 ? tr("?") : dataString(bytesTotal))
            .arg(dataString((int)speed))
            .arg(remaining);
    } else {
        if (m_bytesReceived != bytesTotal) {
            info = tr("%1 of %2 - Stopped")
                .arg(dataString(m_bytesReceived))
                .arg(dataString(bytesTotal));
        } else
            info = dataString(m_bytesReceived);
    }
    downloadInfoLabel->setText(info);
}

QString WBDownloadItem::dataString(int size) const
{
    QString unit;
    if (size < 1024) {
        unit = tr("bytes");
    } else if (size < 1024*1024) {
        size /= 1024;
        unit = tr("kB");
    } else {
        size /= 1024*1024;
        unit = tr("MB");
    }
    return QString(QLatin1String("%1 %2")).arg(size).arg(unit);
}

bool WBDownloadItem::downloading() const
{
    return (progressBar->isVisible());
}

bool WBDownloadItem::downloadedSuccessfully() const
{
    bool completed = m_download
            && m_download->isFinished()
            && m_download->state() == QWebEngineDownloadItem::DownloadCompleted;
    return completed || !stopButton->isVisible();
}

void WBDownloadItem::finished()
{
    if (m_download) {
        QWebEngineDownloadItem::DownloadState state = m_download->state();
        QString message;
        bool interrupted = false;

        switch (state) {
        case QWebEngineDownloadItem::DownloadRequested: // Fall-through.
        case QWebEngineDownloadItem::DownloadInProgress:
            Q_UNREACHABLE();
            break;
        case QWebEngineDownloadItem::DownloadCompleted:
            break;
        case QWebEngineDownloadItem::DownloadCancelled:
            message = QStringLiteral("Download cancelled");
            interrupted = true;
            break;
        case QWebEngineDownloadItem::DownloadInterrupted:
            message = QStringLiteral("Download interrupted");
            interrupted = true;
            break;
        }

        if (interrupted) {
            downloadInfoLabel->setText(message);
            return;
        }
    }

    progressBar->hide();
    stopButton->setEnabled(false);
    stopButton->hide();
    updateInfoLabel();
    m_output.setFileName(m_file.fileName());
    if(m_output.fileName().toLower().endsWith(".wgt") || m_output.fileName().toLower().endsWith(".wdgt")){
        QString destPath = QFileInfo(m_output.fileName()).absolutePath() + "/Dir" + QFileInfo(m_output.fileName()).fileName();
        if (!QFileInfo(m_output.fileName()).isDir()){
            UBFileSystemUtils::expandZipToDir(m_output.fileName(), destPath);
            m_output.remove();
        }
    }

	//��ѹ�鱾�ļ����鱾Ŀ¼
	if (m_download->url().toString().contains("zy.exejk12.com", Qt::CaseInsensitive))
	{	
		if (UBApplication::mLoginManager->GetLogAccount().length()>0&& m_output.fileName().toLower().endsWith(".zip"))
		{
			QString bookPath = CUserInfomation::GetCurUserBookDir();
			QDir destPath(bookPath);

			if (!QFileInfo(destPath.path()+"/"+m_file.baseName()).isDir()) {
				UBFileSystemUtils::expandZipToDir(m_file.absoluteFilePath(), destPath);			
			}
		}
	}
    emit statusChanged();
}

/*!
    DownloadManager is a Dialog that contains a list of DownloadWidgets

    It is a basic download manager.  It only downloads the file, doesn't do BitTorrent,
    extract zipped files or anything fancy.
  */
WBDownloadManager::WBDownloadManager(QWidget *parent)
    : QDialog(parent)
    , m_autoSaver(new UBAutoSaver(this))
     , mManager(UBNetworkAccessManager::defaultAccessManager())
    , m_iconProvider(0)
    , m_removePolicy(Never)
{
    setupUi(this);
        hide();
    downloadsView->setShowGrid(false);
    downloadsView->verticalHeader()->hide();
    downloadsView->horizontalHeader()->hide();
    downloadsView->setAlternatingRowColors(true);
    downloadsView->horizontalHeader()->setStretchLastSection(true);
    m_model = new WBDownloadModel(this);
    downloadsView->setModel(m_model);
    connect(cleanupButton, SIGNAL(clicked()), this, SLOT(cleanup()));
    load();
	resize(600, 400);
}

WBDownloadManager::~WBDownloadManager()
{
    m_autoSaver->changeOccurred();
    m_autoSaver->saveIfNeccessary();
    if (m_iconProvider)
        delete m_iconProvider;
}

int WBDownloadManager::activeDownloads() const
{
    int count = 0;
    for (int i = 0; i < m_downloads.count(); ++i) {
        if (m_downloads.at(i)->stopButton->isEnabled())
            ++count;
    }
    return count;
}

void WBDownloadManager::download( QWebEngineDownloadItem *download)
{
    if (download->url().isEmpty())
        return;

    processDownloadedContent(download);

}
void WBDownloadManager::handleUnsupportedContent(QWebEngineDownloadItem *download, QString customDownloadPath)
{
    QUrl originalUrl = download->url();

    if (originalUrl.isEmpty())
        return;

    processDownloadedContent(download, customDownloadPath);

}

void WBDownloadManager::processDownloadedContent(QWebEngineDownloadItem *download, QString customDownloadPath)
{
    if (!download || download->url().isEmpty())
        return;
//    QVariant header = reply->header(QNetworkRequest::ContentLengthHeader);

//    bool ok;
//    int size = header.toInt(&ok);
//    if (ok && size == 0)
//        return;

//    //redirect ?

//    qDebug() << "redirect" << reply->header(QNetworkRequest::LocationHeader);
//    qDebug() << "ContentTypeHeader" << reply->header(QNetworkRequest::ContentTypeHeader);
//    qDebug() << "DownloadManager::handleUnsupportedContent" << reply->url() << "requestFileName" << requestFileName;

    WBDownloadItem *item = new WBDownloadItem(download,  this, customDownloadPath);
    addItem(item);
}

void WBDownloadManager::addItem(WBDownloadItem *widget)
{
    connect(widget, SIGNAL(statusChanged()), this, SLOT(updateRow()));
    int row = m_downloads.count();
    m_model->beginInsertRows(QModelIndex(), row, row);
    m_downloads.append(widget);
    m_model->endInsertRows();
    updateItemCount();
    if (!mIsLoading)
      {
        show();
//    setWindowFlags(this->windowFlags() | Qt::WindowStaysOnTopHint );
    }
    downloadsView->setIndexWidget(m_model->index(row, 0), widget);
    if (!m_iconProvider)
        m_iconProvider = new QFileIconProvider();
    QIcon icon = m_iconProvider->icon(widget->m_file.fileName());
    widget->fileIcon->setPixmap(icon.pixmap(48, 48));
    downloadsView->setRowHeight(row, widget->sizeHint().height());
}

void WBDownloadManager::updateRow()
{
    WBDownloadItem *widget = qobject_cast<WBDownloadItem*>(sender());
    int row = m_downloads.indexOf(widget);
    if (-1 == row)
        return;
    if (!m_iconProvider)
        m_iconProvider = new QFileIconProvider();
    QIcon icon = m_iconProvider->icon(widget->m_file);
    if (icon.isNull())
        icon = style()->standardIcon(QStyle::SP_FileIcon);
    widget->fileIcon->setPixmap(icon.pixmap(48, 48));
    downloadsView->setRowHeight(row, widget->minimumSizeHint().height());

    bool remove = false;
    if (!widget->downloading()
        && WBBrowserWindow::instance()->privateBrowsing())
        remove = true;

    if (widget->downloadedSuccessfully()
        && removePolicy() == WBDownloadManager::SuccessFullDownload) {
        remove = true;
    }
    if (remove)
        m_model->removeRow(row);

    cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
}

WBDownloadManager::RemovePolicy WBDownloadManager::removePolicy() const
{
    return m_removePolicy;
}

void WBDownloadManager::setRemovePolicy(RemovePolicy policy)
{
    if (policy == m_removePolicy)
        return;
    m_removePolicy = policy;
    m_autoSaver->changeOccurred();
}

void WBDownloadManager::save() const
{
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    settings.setValue(QLatin1String("removeDownloadsPolicy"), QLatin1String(removePolicyEnum.valueToKey(m_removePolicy)));
    settings.setValue(QLatin1String("size"), size());
    if (m_removePolicy == Exit)
        return;

    for (int i = 0; i < m_downloads.count(); ++i) {
        QString key = QString(QLatin1String("download_%1_")).arg(i);
        settings.setValue(key + QLatin1String("url"), m_downloads[i]->m_url);
        settings.setValue(key + QLatin1String("location"), m_downloads[i]->m_file.filePath());
        settings.setValue(key + QLatin1String("done"), m_downloads[i]->downloadedSuccessfully());
    }
    int i = m_downloads.count();
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    while (settings.contains(key + QLatin1String("url"))) {
        settings.remove(key + QLatin1String("url"));
        settings.remove(key + QLatin1String("location"));
        settings.remove(key + QLatin1String("done"));
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
}

void WBDownloadManager::load()
{
     mIsLoading = true;
    QSettings settings;
    settings.beginGroup(QLatin1String("downloadmanager"));
    QSize size = settings.value(QLatin1String("size")).toSize();
    if (size.isValid())
        resize(size);
    QByteArray value = settings.value(QLatin1String("removeDownloadsPolicy"), QLatin1String("Never")).toByteArray();
    QMetaEnum removePolicyEnum = staticMetaObject.enumerator(staticMetaObject.indexOfEnumerator("RemovePolicy"));
    m_removePolicy = removePolicyEnum.keyToValue(value) == -1 ?
                        Never :
                        static_cast<RemovePolicy>(removePolicyEnum.keyToValue(value));

    int i = 0;
    QString key = QString(QLatin1String("download_%1_")).arg(i);
    while (settings.contains(key + QLatin1String("url"))) {
        QUrl url = settings.value(key + QLatin1String("url")).toUrl();
        QString fileName = settings.value(key + QLatin1String("location")).toString();
        bool done = settings.value(key + QLatin1String("done"), true).toBool();
        if ( !url.isEmpty() && !fileName.isEmpty()) {
            WBDownloadItem *widget = new WBDownloadItem(0, this);
            widget->m_file.setFile(fileName);
            widget->fileNameLabel->setText(widget->m_file.fileName());
            widget->m_url = url;
            widget->stopButton->setVisible(false);
            widget->stopButton->setEnabled(false);
            widget->progressBar->hide();
            addItem(widget);
        }
        key = QString(QLatin1String("download_%1_")).arg(++i);
    }
    cleanupButton->setEnabled(m_downloads.count() - activeDownloads() > 0);
     mIsLoading = false;
}

void WBDownloadManager::cleanup()
{
    if (m_downloads.isEmpty())
        return;
    m_model->removeRows(0, m_downloads.count());
    updateItemCount();
    if (m_downloads.isEmpty() && m_iconProvider) {
        delete m_iconProvider;
        m_iconProvider = 0;
    }
    m_autoSaver->changeOccurred();
}

void WBDownloadManager::updateItemCount()
{
    int count = m_downloads.count();
    itemCount->setText(count == 1 ? tr("1 Download") : tr("%1 Downloads").arg(count));
}

WBDownloadModel::WBDownloadModel(WBDownloadManager *downloadManager, QObject *parent)
    : QAbstractListModel(parent)
    , m_downloadManager(downloadManager)
{
}

QVariant WBDownloadModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount(index.parent()))
        return QVariant();
    if (role == Qt::ToolTipRole)
        if (!m_downloadManager->m_downloads.at(index.row())->downloadedSuccessfully())
            return m_downloadManager->m_downloads.at(index.row())->downloadInfoLabel->text();
    return QVariant();
}

int WBDownloadModel::rowCount(const QModelIndex &parent) const
{
    return (parent.isValid()) ? 0 : m_downloadManager->m_downloads.count();
}

bool WBDownloadModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    int lastRow = row + count - 1;
    for (int i = lastRow; i >= row; --i) {
        if (m_downloadManager->m_downloads.at(i)->downloadedSuccessfully()) {
            beginRemoveRows(parent, i, i);
            m_downloadManager->m_downloads.takeAt(i)->deleteLater();
            endRemoveRows();
        }
    }
    m_downloadManager->m_autoSaver->changeOccurred();
    return true;
}
