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

#include "WBBrowserWindow.h"
//#include "browsermainwindow.h"
//#include "cookiejar.h"
#include "WBDownloadManager.h"
#include "featurepermissionbar.h"
#include "ui_passworddialog.h"
#include "ui_proxy.h"
#include "WBTabWidget.h"
#include "WBWebView.h"
#include <QWebEngineCertificateError>
#include <QtGui/QClipboard>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QMouseEvent>
#include "core/UBApplication.h"
#include "core/UBApplicationController.h"
#include "board/UBBoardController.h"
#include "network/UBNetworkAccessManager.h"
#include <QWebEngineContextMenuData>
#include "WBDownloadManager.h"
#ifndef QT_NO_UITOOLS
#include <QtUiTools/QUiLoader>
#endif  //QT_NO_UITOOLS

#include <QtCore/QDebug>
#include <QtCore/QBuffer>
#include <QtCore/QTimer>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
WBWebPage::WBWebPage(QWebEngineProfile *profile, QObject *parent)
	: UBWebPage(profile, parent)
	, m_keyboardModifiers(Qt::NoModifier)
	, m_pressedButtons(Qt::NoButton)
	, mOpenInNewTab(false)
{
#if defined(QWEBENGINEPAGE_SETNETWORKACCESSMANAGER)
	setNetworkAccessManager(WBBrowserWindow::networkAccessManager());
#endif
#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
	connect(this, SIGNAL(unsupportedContent(QNetworkReply*)),
		this, SLOT(handleUnsupportedContent(QNetworkReply*)));
#endif
	//    connect(this, SIGNAL(authenticationRequired(const QUrl &, QAuthenticator*)),
	//            SLOT(authenticationRequired(const QUrl &, QAuthenticator*)));
	//    connect(this, SIGNAL(proxyAuthenticationRequired(const QUrl &, QAuthenticator *, const QString &)),
	//            SLOT(proxyAuthenticationRequired(const QUrl &, QAuthenticator *, const QString &)));
	//    setNetworkAccessManager(UBNetworkAccessManager::defaultAccessManager());

	//    connect(this, SIGNAL(unsupportedContent(QNetworkReply *)),
    //            this, SLOT(handleUnsupportedContent(QNetworkReply *)));
}

WBBrowserWindow *WBWebPage::mainWindow()
{
    QObject *w = this->parent();
    while (w)
    {
        if (WBBrowserWindow *mw = qobject_cast<WBBrowserWindow*>(w))
            return mw;

        w = w->parent();
    }

    return 0;
}

//class PopupWindow : public QWidget {
//    Q_OBJECT
//public:
//    PopupWindow(QWebEngineProfile *profile)
//        : m_addressBar(new QLineEdit(this))
//        , m_view(new WBWebView(this))
//    {
//        m_view->setPage(new WBWebPage(profile, m_view));
//        QVBoxLayout *layout = new QVBoxLayout;
//        layout->setMargin(0);
//        setLayout(layout);
//        layout->addWidget(m_addressBar);
//        layout->addWidget(m_view);
//        m_view->setFocus();

//        connect(m_view, &WBWebView::titleChanged, this, &QWidget::setWindowTitle);
//        connect(m_view, &WBWebView::urlChanged, this, &PopupWindow::setUrl);
//        connect(page(), &WBWebPage::geometryChangeRequested, this, &PopupWindow::adjustGeometry);
//        connect(page(), &WBWebPage::windowCloseRequested, this, &QWidget::close);
//    }

//    QWebEnginePage* page() const { return m_view->page(); }

//private Q_SLOTS:
//    void setUrl(const QUrl &url)
//    {
//        m_addressBar->setText(url.toString());
//    }

//    void adjustGeometry(const QRect &newGeometry)
//    {
//        const int x1 = frameGeometry().left() - geometry().left();
//        const int y1 = frameGeometry().top() - geometry().top();
//        const int x2 = frameGeometry().right() - geometry().right();
//        const int y2 = frameGeometry().bottom() - geometry().bottom();

//        setGeometry(newGeometry.adjusted(x1, y1 - m_addressBar->height(), x2, y2));
//    }

//private:
//    QLineEdit *m_addressBar;
//    WBWebView *m_view;

//};

//#include "WBWebView.moc"
bool WBWebPage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame)
{
    // ctrl open in new tab
    // ctrl-shift open in new tab and select
    // ctrl-alt open in new window
    if (type == QWebEnginePage::NavigationTypeLinkClicked
        && (m_keyboardModifiers & Qt::ControlModifier
            || m_pressedButtons == Qt::MidButton))
    {
        WBWebView *webView;

        bool selectNewTab = (m_keyboardModifiers & Qt::ShiftModifier);
        webView = mainWindow()->tabWidget()->newTab(selectNewTab);

        webView->load(url);
        m_keyboardModifiers = Qt::NoModifier;
        m_pressedButtons = Qt::NoButton;

        return false;
    }

    if (isMainFrame)
    {
        mLoadingUrl = url;
        emit loadingUrl(mLoadingUrl);
    }

    return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

bool WBWebPage::certificateError(const QWebEngineCertificateError &error)
{
    if (error.isOverridable()) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setText(error.errorDescription());
        msgBox.setInformativeText(tr("If you wish so, you may continue with an unverified certificate. "
            "Accepting an unverified certificate means "
            "you may not be connected with the host you tried to connect to.\n"
            "Do you wish to override the security check and continue?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        return msgBox.exec() == QMessageBox::Yes;
    }
    QMessageBox::critical(view(), tr("Certificate Error"), error.errorDescription(), QMessageBox::Ok, QMessageBox::NoButton);
    return false;
}

//#include "webview.moc"
QWebEnginePage *WBWebPage::createWindow(QWebEnginePage::WebWindowType type)
{
    Q_UNUSED(type);
    return mainWindow()->tabWidget()->newTab()->page();


}

//#if !defined(QT_NO_UITOOLS)
QObject *WBWebPage::createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues)
{
    Q_UNUSED(url);
    Q_UNUSED(paramNames);
    Q_UNUSED(paramValues);
    QUiLoader loader;
    return loader.createWidget(classId, view());
}
//#endif // !defined(QT_NO_UITOOLS)

#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
void WebPage::handleUnsupportedContent(QNetworkReply *reply)
{
    QString errorString = reply->errorString();

    if (m_loadingUrl != reply->url()) {
        // sub resource of this page
        qWarning() << "Resource" << reply->url().toEncoded() << "has unknown Content-Type, will be ignored.";
        reply->deleteLater();
        return;
    }

    if (reply->error() == QNetworkReply::NoError && !reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
        errorString = "Unknown Content-Type";
    }

    QFile file(QLatin1String(":/notfound.html"));
    bool isOpened = file.open(QIODevice::ReadOnly);
    Q_ASSERT(isOpened);
    Q_UNUSED(isOpened)

        QString title = tr("Error loading page: %1").arg(reply->url().toString());
    QString html = QString(QLatin1String(file.readAll()))
        .arg(title)
        .arg(errorString)
        .arg(reply->url().toString());

    QBuffer imageBuffer;
    imageBuffer.open(QBuffer::ReadWrite);
    QIcon icon = view()->style()->standardIcon(QStyle::SP_MessageBoxWarning, 0, view());
    QPixmap pixmap = icon.pixmap(QSize(32, 32));
    if (pixmap.save(&imageBuffer, "PNG")) {
        html.replace(QLatin1String("IMAGE_BINARY_DATA_HERE"),
            QString(QLatin1String(imageBuffer.buffer().toBase64())));
    }

    QList<QWebEngineFrame*> frames;
    frames.append(mainFrame());
    while (!frames.isEmpty()) {
        QWebEngineFrame *frame = frames.takeFirst();
        if (frame->url() == reply->url()) {
            frame->setHtml(html, reply->url());
            return;
        }
        QList<QWebEngineFrame *> children = frame->childFrames();
        foreach(QWebEngineFrame *frame, children)
            frames.append(frame);
    }
    if (m_loadingUrl == reply->url()) {
        mainFrame()->setHtml(html, reply->url());
    }
}
#endif

//void WBWebPage::authenticationRequired(const QUrl &requestUrl, QAuthenticator *auth)
//{
//    BrowserMainWindow *mainWindow = WBBrowserWindow::instance()->mainWindow();

//    QDialog dialog(mainWindow);
//    dialog.setWindowFlags(Qt::Sheet);

//    Ui::PasswordDialog passwordDialog;
//    passwordDialog.setupUi(&dialog);

//    passwordDialog.iconLabel->setText(QString());
//    passwordDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

//    QString introMessage = tr("<qt>Enter username and password for \"%1\" at %2</qt>");
//    introMessage = introMessage.arg(auth->realm()).arg(requestUrl.toString().toHtmlEscaped());
//    passwordDialog.introLabel->setText(introMessage);
//    passwordDialog.introLabel->setWordWrap(true);

//    if (dialog.exec() == QDialog::Accepted) {
//        auth->setUser(passwordDialog.userNameLineEdit->text());
//        auth->setPassword(passwordDialog.passwordLineEdit->text());
//    } else {
//        // Set authenticator null if dialog is cancelled
//        *auth = QAuthenticator();
//    }
//}

//void WBWebPage::proxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost)
//{
//    Q_UNUSED(requestUrl);
//    BrowserMainWindow *mainWindow = WBBrowserWindow::instance()->mainWindow();

//    QDialog dialog(mainWindow);
//    dialog.setWindowFlags(Qt::Sheet);

//    Ui::ProxyDialog proxyDialog;
//    proxyDialog.setupUi(&dialog);

//    proxyDialog.iconLabel->setText(QString());
//    proxyDialog.iconLabel->setPixmap(mainWindow->style()->standardIcon(QStyle::SP_MessageBoxQuestion, 0, mainWindow).pixmap(32, 32));

//    QString introMessage = tr("<qt>Connect to proxy \"%1\" using:</qt>");
//    introMessage = introMessage.arg(proxyHost.toHtmlEscaped());
//    proxyDialog.introLabel->setText(introMessage);
//    proxyDialog.introLabel->setWordWrap(true);

//    if (dialog.exec() == QDialog::Accepted) {
//        auth->setUser(proxyDialog.userNameLineEdit->text());
//        auth->setPassword(proxyDialog.passwordLineEdit->text());
//    } else {
//        // Set authenticator null if dialog is cancelled
//        *auth = QAuthenticator();
//    }
//}

WBWebView::WBWebView(QWidget* parent)
    : WBWebTrapWebView(parent)
    , m_progress(100)

{
    m_page = new WBWebPage(new QWebEngineProfile(this), this);
    setObjectName("ubBrowserWebView");
    setPage(m_page);

    //    QWebEngineView::setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    //    connect(page(), SIGNAL(statusBarMessage(const QString&)),
    //            this, SLOT(setStatusBarText(const QString&)));
    connect(this, SIGNAL(loadProgress(int)),
        this, SLOT(setProgress(int)));
    connect(this, SIGNAL(loadFinished(bool)),
        this, SLOT(loadFinished(bool)));
    connect(this, SIGNAL(loadStarted()),
        this, SLOT(loadStarted()));

    connect(page(), SIGNAL(loadingUrl(const QUrl&)),
        this, SIGNAL(urlChanged(const QUrl &)));

    //connect(page(), &WBWebPage::featurePermissionRequested, this, &WBWebView::onFeaturePermissionRequested);
    connect(this->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
        this, SLOT(downloadRequested(QWebEngineDownloadItem*)));

    connect(this, &QWebEngineView::renderProcessTerminated,
        [=](QWebEnginePage::RenderProcessTerminationStatus termStatus, int statusCode) {
        const char *status = "";
        switch (termStatus) {
        case QWebEnginePage::NormalTerminationStatus:
            status = "(normal exit)";
            break;
        case QWebEnginePage::AbnormalTerminationStatus:
            status = "(abnormal exit)";
            break;
        case QWebEnginePage::CrashedTerminationStatus:
            status = "(crashed)";
            break;
        case QWebEnginePage::KilledTerminationStatus:
            status = "(killed)";
            break;
            QMessageBox::StandardButton btn = QMessageBox::question(window(), status,
                                                       tr("Render process exited with code: %1\n"
                                                          "Do you want to reload the page ?").arg(statusCode));
            if (btn == QMessageBox::Yes)
                QTimer::singleShot(0, [this] { reload(); });
        }


//		qInfo() << "Render process exited with code" << statusCode << status;
//		QTimer::singleShot(0, [this] { reload(); });
    });
}

//void WBWebView::setPage(WBWebPage *_page)
//{
//    m_page = _page;
//    QWebEngineView::setPage(_page);
//#if defined(QWEBENGINEPAGE_STATUSBARMESSAGE)
//    connect(page(), SIGNAL(statusBarMessage(QString)),
//            SLOT(setStatusBarText(QString)));
//#endif
//    disconnect(page(), &QWebEnginePage::iconChanged, this, &WBWebView::iconChanged);
////    connect(page(), SIGNAL(iconChanged(QIcon)),
////            this, SLOT(onIconChanged(QIcon)));
//    connect(page(), &WBWebPage::featurePermissionRequested, this, &WBWebView::onFeaturePermissionRequested);
//#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
//    page()->setForwardUnsupportedContent(true);
//#endif
//}

void WBWebView::contextMenuEvent(QContextMenuEvent *event)
{

    if (page()->contextMenuData().linkUrl().isValid()) {
        QMenu menu(this);

        menu.addAction(pageAction(QWebEnginePage::OpenLinkInNewWindow));
        menu.addAction(tr("Open in New Tab"), this, SLOT(openLinkInNewTab()));
        menu.addSeparator();
        menu.addAction(pageAction(QWebEnginePage::DownloadLinkToDisk));
        // Add link to bookmarks...
        menu.addSeparator();
        menu.addAction(pageAction(QWebEnginePage::CopyLinkToClipboard));
        //        if (page()->settings()->testAttribute(QWebEngineSettings::DeveloperExtrasEnabled))
        //            menu.addAction(pageAction(QWebEnginePage::InspectElement));
        menu.exec(mapToGlobal(event->pos()));

        return;
    }


    WBWebTrapWebView::contextMenuEvent(event);
}

void WBWebView::wheelEvent(QWheelEvent *event)
{
#if defined(QWEBENGINEPAGE_SETTEXTSIZEMULTIPLIER)
    if (QApplication::keyboardModifiers() & Qt::ControlModifier) {
        int numDegrees = event->delta() / 8;
        int numSteps = numDegrees / 15;
        setTextSizeMultiplier(textSizeMultiplier() + numSteps * 0.1);
        event->accept();
        return;
    }
#endif
    QWebEngineView::wheelEvent(event);
}

void WBWebView::openLinkInNewTab()
{
    m_page->mOpenInNewTab = true;
    pageAction(QWebEnginePage::OpenLinkInNewWindow)->trigger();
}

void WBWebView::onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature feature)
{
    FeaturePermissionBar *permissionBar = new FeaturePermissionBar(this);
    connect(permissionBar, &FeaturePermissionBar::featurePermissionProvided, page(), &QWebEnginePage::setFeaturePermission);

    // Discard the bar on new loads (if we navigate away or reload).
    connect(page(), &QWebEnginePage::loadStarted, permissionBar, &QObject::deleteLater);

    permissionBar->requestPermission(securityOrigin, feature);
}

void WBWebView::setProgress(int progress)
{
    m_progress = progress;
}
void WBWebView::loadStarted()
{
    mLoadStartTime.start();
	m_progress = 0;
}

void WBWebView::loadFinished(bool success)
{
   /* if (success && 100 != m_progress) {
        qWarning() << "Received finished signal while progress is still:" << progress()
            << "Url:" << url();
    }
    QString error;

    if (!success)
        error = "with error";

    qDebug() << "page loaded in" << mLoadStartTime.elapsed() << "ms" << url().toString() << error;

    m_progress = 0;*/
	m_progress = success ? 100 : -1;
}

void WBWebView::load(const QUrl &url)
{

    qDebug() << "loading " << url.toString();

    m_initialUrl = url;

    WBWebTrapWebView::load(url);
}

QString WBWebView::lastStatusBarText() const
{
    return m_statusBarText;
}

QUrl WBWebView::url() const
{

    QUrl url;
    try {
        url = QWebEngineView::url();
    }
    catch (...)
    {
    }

    if (!url.isEmpty())
        return url;

    return m_initialUrl;
}

//void WBWebView::onIconChanged(const QIcon &icon)
//{
//    if (icon.isNull())
//        emit iconChanged(WBBrowserWindow::instance()->defaultIcon());
//    else
//        emit iconChanged(icon);
//}

void WBWebView::mousePressEvent(QMouseEvent *event)
{
    m_page->m_pressedButtons = event->buttons();
    m_page->m_keyboardModifiers = event->modifiers();
    WBWebTrapWebView::mousePressEvent(event);
}

void WBWebView::mouseReleaseEvent(QMouseEvent *event)
{
    WBWebTrapWebView::mouseReleaseEvent(event);
    if (!event->isAccepted() && (m_page->m_pressedButtons & Qt::MidButton)) {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            setUrl(url);
        }
    }
}

void WBWebView::setStatusBarText(const QString &string)
{
    m_statusBarText = string;
}
void WBWebView::downloadRequested(QWebEngineDownloadItem* downloaditem)
{
    WBBrowserWindow::downloadManager()->download(downloaditem);
}

void WBWebView::load(const QWebEngineHttpRequest & request)
{
    WBWebTrapWebView::load(request);
}
