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

#include "WBTabWidget.h"

#include "WBBrowserWindow.h"
#include "WBDownloadManager.h"
#include "fullscreennotification.h"
#include "WBHistory.h"
#include "WBSavePageDialog.h"
#include "WBUrlLineEdit.h"
#include "WBWebView.h"

#include "core/UBApplication.h" // TODO UB 4.x remove this nasty dependency
#include "core/UBApplicationController.h"// TODO UB 4.x remove this nasty dependency

#include <QWebEngineDownloadItem>
#include <QWebEngineProfile>
#include <QWebEngineFullScreenRequest>
#include <QtCore/QMimeData>
#include <QtGui/QClipboard>
#include <QtWidgets/QCompleter>
#include <QtWidgets/QListView>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMessageBox>
#include <QtGui/QDrag>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>

#include <QtCore/QDebug>

WBTabBar::WBTabBar(QWidget *parent)
    : QTabBar(parent)
{
     setObjectName("ubWebBrowserTabBar");
    setContextMenuPolicy(Qt::CustomContextMenu);
    setAcceptDrops(true);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequested(QPoint)));

    QString alt = QLatin1String("Alt+%1");
    for (int i = 1; i < 10; ++i) {
        QShortcut *shortCut = new QShortcut(alt.arg(i), this);
        m_tabShortcuts.append(shortCut);
        connect(shortCut, SIGNAL(activated()), this, SLOT(selectTabAction()));
    }
    setTabsClosable(true);
    connect(this, SIGNAL(tabCloseRequested(int)),
            this, SIGNAL(closeTab(int)));
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
     setDocumentMode(false);

#ifdef Q_OS_OSX
    QFont baseFont = font();
    baseFont.setPointSize(baseFont.pointSize() - 2);
    setFont(baseFont);
#endif
}

void WBTabBar::selectTabAction()
{
    if (QShortcut *shortCut = qobject_cast<QShortcut*>(sender())) {
        int index = m_tabShortcuts.indexOf(shortCut);
        if (index == 0)
            index = 10;
        setCurrentIndex(index);
    }
}

void WBTabBar::contextMenuRequested(const QPoint &position)
{
    QMenu menu;
    menu.addAction(tr("New &Tab"), this, SIGNAL(newTab()), QKeySequence::AddTab);
    int index = tabAt(position);
    if (-1 != index) {
        QAction *action = menu.addAction(tr("Clone Tab"),
                this, SLOT(cloneTab()));
        action->setData(index);

        menu.addSeparator();

        action = menu.addAction(tr("&Close Tab"),
                this, SLOT(closeTab()), QKeySequence::Close);
        action->setData(index);

        action = menu.addAction(tr("Close &Other Tabs"),
                this, SLOT(closeOtherTabs()));
        action->setData(index);

        menu.addSeparator();

        action = menu.addAction(tr("Reload Tab"),
                this, SLOT(reloadTab()), QKeySequence::Refresh);
        action->setData(index);

        // Audio mute / unmute.
//        action = menu.addAction(tr("Mute tab"),
//                this, SLOT(muteTab()));
//        action->setData(index);

//        action = menu.addAction(tr("Unmute tab"),
//                this, SLOT(unmuteTab()));
//        action->setData(index);
    } else {
        menu.addSeparator();
    }
    menu.addAction(tr("Reload All Tabs"), this, SIGNAL(reloadAllTabs()));
    menu.exec(QCursor::pos());
}

void WBTabBar::cloneTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit cloneTab(index);
    }
}

void WBTabBar::closeTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit closeTab(index);
    }
}

void WBTabBar::closeOtherTabs()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit closeOtherTabs(index);
    }
}

void WBTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();

    QTabBar::mousePressEvent(event);


}

void WBTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton) {
        int diffX = event->pos().x() - m_dragStartPos.x();
        int diffY = event->pos().y() - m_dragStartPos.y();
        if ((event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()
            && diffX < 3 && diffX > -3
            && diffY < -10) {
            QDrag *drag = new QDrag(this);
            QMimeData *mimeData = new QMimeData;
            QList<QUrl> urls;
            int index = tabAt(event->pos());
            QUrl url = tabData(index).toUrl();
            urls.append(url);
            mimeData->setUrls(urls);
            mimeData->setText(tabText(index));
            mimeData->setData(QLatin1String("action"), "tab-reordering");
            drag->setMimeData(mimeData);
            drag->exec();
        }
    }
    QTabBar::mouseMoveEvent(event);
}

// When index is -1 index chooses the current tab
void WBTabWidget::reloadTab(int index)
{
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;

    QWidget *widget = this->widget(index);
    if (WBWebView *tab = qobject_cast<WBWebView*>(widget))
        tab->reload();
}

void WBTabBar::reloadTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit reloadTab(index);
    }
}

void WBTabBar::muteTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit muteTab(index, true);
    }
}

void WBTabBar::unmuteTab()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        int index = action->data().toInt();
        emit muteTab(index, false);
    }
}

WBTabWidget::WBTabWidget(QWidget *parent)
    : QTabWidget(parent)
    , m_recentlyClosedTabsAction(0)
    , m_newTabAction(0)
    , m_closeTabAction(0)
    , m_nextTabAction(0)
    , m_previousTabAction(0)
    , m_recentlyClosedTabsMenu(0)
    , m_lineEditCompleter(0)
    , m_lineEdits(0)
    , m_tabBar(new WBTabBar(this))
    , m_profile(QWebEngineProfile::defaultProfile())
    , m_fullScreenView(0)
    , m_fullScreenNotification(0)
	, m_camera(nullptr)
{
    setObjectName("ubWebBrowserTabWidget");
    mAddTabIcon = QPixmap(":/images/toolbar/plusBlack.png");
    setElideMode(Qt::ElideRight);

    connect(m_tabBar, SIGNAL(newTab()), this, SLOT(newTab()));
    connect(m_tabBar, SIGNAL(closeTab(int)), this, SLOT(closeTab(int)));
    connect(m_tabBar, SIGNAL(cloneTab(int)), this, SLOT(cloneTab(int)));
    connect(m_tabBar, SIGNAL(closeOtherTabs(int)), this, SLOT(closeOtherTabs(int)));
    connect(m_tabBar, SIGNAL(reloadTab(int)), this, SLOT(reloadTab(int)));
    connect(m_tabBar, SIGNAL(reloadAllTabs()), this, SLOT(reloadAllTabs()));
    connect(m_tabBar, SIGNAL(tabMoved(int,int)), this, SLOT(moveTab(int,int)));
	connect(m_tabBar, SIGNAL(tabBarClicked(int)), this, SLOT(handleTabBarDoubleClicked(int)));
    connect(m_tabBar, SIGNAL(muteTab(int,bool)), this, SLOT(setAudioMutedForTab(int,bool)));
    setTabBar(m_tabBar);
    setDocumentMode(false);


    m_recentlyClosedTabsMenu = new QMenu(this);
    connect(m_recentlyClosedTabsMenu, SIGNAL(aboutToShow()),
            this, SLOT(aboutToShowRecentTabsMenu()));
    connect(m_recentlyClosedTabsMenu, SIGNAL(triggered(QAction*)),
            this, SLOT(aboutToShowRecentTriggeredAction(QAction*)));
    m_recentlyClosedTabsAction = new QAction(tr("Recently Closed Tabs"), this);
    m_recentlyClosedTabsAction->setMenu(m_recentlyClosedTabsMenu);
    m_recentlyClosedTabsAction->setEnabled(false);



    // Actions
    m_newTabAction = new QAction(QIcon(QLatin1String(":addtab.png")), tr("New &Tab"), this);
    m_newTabAction->setShortcuts(QKeySequence::AddTab);
    m_newTabAction->setIconVisibleInMenu(false);
    connect(m_newTabAction, SIGNAL(triggered()), this, SLOT(newTab()));

    m_closeTabAction = new QAction(QIcon(QLatin1String(":closetab.png")), tr("&Close Tab"), this);
    m_closeTabAction->setShortcuts(QKeySequence::Close);
    m_closeTabAction->setIconVisibleInMenu(false);
    connect(m_closeTabAction, SIGNAL(triggered()), this, SLOT(requestCloseTab()));

    m_nextTabAction = new QAction(tr("Show Next Tab"), this);
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BraceRight));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_PageDown));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BracketRight));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Less));
    m_nextTabAction->setShortcuts(shortcuts);
    connect(m_nextTabAction, SIGNAL(triggered()), this, SLOT(nextTab()));

    m_previousTabAction = new QAction(tr("Show Previous Tab"), this);
    shortcuts.clear();
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BraceLeft));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_PageUp));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_BracketLeft));
    shortcuts.append(QKeySequence(Qt::CTRL | Qt::Key_Greater));
    m_previousTabAction->setShortcuts(shortcuts);
    connect(m_previousTabAction, SIGNAL(triggered()), this, SLOT(previousTab()));



    connect(this, SIGNAL(currentChanged(int)),
            this, SLOT(currentChanged(int)));

    m_lineEdits = new QStackedWidget(this);
    m_lineEdits->setMinimumWidth(200);
    QSizePolicy spolicy = m_lineEdits->sizePolicy();
    m_lineEdits->setSizePolicy(QSizePolicy::Maximum, spolicy.verticalPolicy());
}

WBTabWidget::~WBTabWidget()
{
    if (m_fullScreenView)
        delete m_fullScreenView;
}

void WBTabWidget::clear()
{
    // clear the recently closed tabs
    m_recentlyClosedTabs.clear();
    // clear the line edit history
    for (int i = 0; i < m_lineEdits->count(); ++i) {
        QLineEdit *qLineEdit = lineEdit(i);
        qLineEdit->setText(qLineEdit->text());
    }
}

void WBTabWidget::moveTab(int fromIndex, int toIndex)
{
    QWidget *lineEdit = m_lineEdits->widget(fromIndex);
    m_lineEdits->removeWidget(lineEdit);
    m_lineEdits->insertWidget(toIndex, lineEdit);
}

void WBTabWidget::setAudioMutedForTab(int index, bool mute)
{
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;

    QWidget *widget = this->widget(index);
    if (WBWebView *tab = qobject_cast<WBWebView*>(widget))
        tab->page()->setAudioMuted(mute);
}

void WBTabWidget::addWebAction(QAction *action, QWebEnginePage::WebAction webAction)
{
    if (!action)
        return;
    m_actions.append(new WBWebActionMapper(action, webAction, this));
}

void WBTabWidget::currentChanged(int index)
{
    WBWebView *webView = this->webView(index);
    if (!webView)
        return;

    Q_ASSERT(m_lineEdits->count() == count());

	disconnect(QWebEngineProfile::defaultProfile());


    WBWebView *oldWebView = this->webView(m_lineEdits->currentIndex());
    if (oldWebView) {
//#if defined(QWEBENGINEVIEW_STATUSBARMESSAGE)
//        disconnect(oldWebView, SIGNAL(statusBarMessage(QString)),
//                this, SIGNAL(showStatusBarMessage(QString)));
//#endif
        disconnect(oldWebView->page(), SIGNAL(linkHovered(const QString&)),
                this, SIGNAL(linkHovered(const QString&)));
        disconnect(oldWebView, SIGNAL(loadProgress(int)),
                this, SIGNAL(loadProgress(int)));
        disconnect(oldWebView->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
                this, SLOT(downloadRequested(QWebEngineDownloadItem*)));
        disconnect(oldWebView->page(), SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
                this, SLOT(fullScreenRequested(QWebEngineFullScreenRequest)));

        disconnect(oldWebView, SIGNAL(loadFinished(bool)),
                this, SIGNAL(loadFinished(bool)));
    }

//#if defined(QWEBENGINEVIEW_STATUSBARMESSAGE)
//    connect(webView, SIGNAL(statusBarMessage(QString)),
//            this, SIGNAL(showStatusBarMessage(QString)));
//#endif

    connect(webView->page(), SIGNAL(linkHovered(const QString&)),
            this, SIGNAL(linkHovered(const QString&)));
    connect(webView, SIGNAL(loadProgress(int)),
            this, SIGNAL(loadProgress(int)));
    connect(webView->page()->profile(), SIGNAL(downloadRequested(QWebEngineDownloadItem*)),
            this, SLOT(downloadRequested(QWebEngineDownloadItem*)));
    connect(webView->page(), SIGNAL(fullScreenRequested(QWebEngineFullScreenRequest)),
            this, SLOT(fullScreenRequested(QWebEngineFullScreenRequest)));
    connect(webView, SIGNAL(loadFinished (bool)),
            this, SIGNAL(loadFinished(bool)));
    for (int i = 0; i < m_actions.count(); ++i) {
        WBWebActionMapper *mapper = m_actions[i];
        mapper->updateCurrent(webView->page());
    }
    emit setCurrentTitle(webView->title());
    m_lineEdits->setCurrentIndex(index);
    emit loadProgress(webView->progress());
    emit showStatusBarMessage(webView->lastStatusBarText());
    if (webView->url().isEmpty())
        m_lineEdits->currentWidget()->setFocus();
    else
        webView->setFocus();
}

void WBTabWidget::fullScreenRequested(QWebEngineFullScreenRequest request)
{
    WBWebPage *webPage = qobject_cast<WBWebPage*>(sender());
    if (request.toggleOn()) {
        if (!m_fullScreenView) {
           m_fullScreenView = new WBWebView();
            m_fullScreenNotification = new FullScreenNotification(m_fullScreenView);

            QAction *exitFullScreenAction = new QAction(m_fullScreenView);
            exitFullScreenAction->setShortcut(Qt::Key_Escape);
            connect(exitFullScreenAction, &QAction::triggered, [webPage] {
                webPage->triggerAction(QWebEnginePage::ExitFullScreen);
            });
            m_fullScreenView->addAction(exitFullScreenAction);
        }
        m_oldWindowGeometry = window()->geometry();
        m_fullScreenView->setGeometry(m_oldWindowGeometry);
        webPage->setView(m_fullScreenView);
        request.accept();
        m_fullScreenView->showFullScreen();
        m_fullScreenNotification->show();
//        window()->hide();
    } else {
        if (!m_fullScreenView)
            return;
        WBWebView *oldWebView = this->webView(m_lineEdits->currentIndex());
        webPage->setView(oldWebView);
        request.accept();
        delete m_fullScreenView;
        m_fullScreenView = 0;
//        window()->show();
//        window()->setGeometry(m_oldWindowGeometry);
    }
}

void WBTabWidget::handleTabBarDoubleClicked(int index)
{
    if (index != -1) return;
    newTab();
}

QAction *WBTabWidget::newTabAction() const
{
    return m_newTabAction;
}

QAction *WBTabWidget::closeTabAction() const
{
    return m_closeTabAction;
}

QAction *WBTabWidget::recentlyClosedTabsAction() const
{
    return m_recentlyClosedTabsAction;
}

QAction *WBTabWidget::nextTabAction() const
{
    return m_nextTabAction;
}

QAction *WBTabWidget::previousTabAction() const
{
    return m_previousTabAction;
}

QWidget *WBTabWidget::lineEditStack() const
{
    return m_lineEdits;
}

QLineEdit *WBTabWidget::currentLineEdit() const
{
    return lineEdit(m_lineEdits->currentIndex());
}

WBWebView *WBTabWidget::currentWebView() const
{
    return webView(currentIndex());
}

QLineEdit *WBTabWidget::lineEdit(int index) const
{
    WBUrlLineEdit *urlLineEdit = qobject_cast<WBUrlLineEdit*>(m_lineEdits->widget(index));
    if (urlLineEdit)
        return urlLineEdit->lineEdit();
    return 0;
}

WBWebView *WBTabWidget::webView(int index) const
{
    QWidget *widget = this->widget(index);
    if (WBWebView *webView = qobject_cast<WBWebView*>(widget)) {
        return webView;
    } else {
        // optimization to delay creating the first webview
        if (count() == 1) {
            WBTabWidget *that = const_cast<WBTabWidget*>(this);
            that->setUpdatesEnabled(false);
            that->newTab();
            that->closeTab(0);
            that->setUpdatesEnabled(true);
            return currentWebView();
        }
    }
    return 0;
}

int WBTabWidget::webViewIndex(WBWebView *webView) const
{
    int index = indexOf(webView);
    return index;
}

void WBTabWidget::setupPage(QWebEnginePage* page)
{
    connect(page, SIGNAL(windowCloseRequested()),
            this, SLOT(windowCloseRequested()));
    connect(page, SIGNAL(geometryChangeRequested(QRect)),
            this, SIGNAL(geometryChangeRequested(QRect)));
#if defined(QWEBENGINEPAGE_PRINTREQUESTED)
    connect(page, SIGNAL(printRequested(QWebEngineFrame*)),
            this, SIGNAL(printRequested(QWebEngineFrame*)));
#endif
#if defined(QWEBENGINEPAGE_MENUBARVISIBILITYCHANGEREQUESTED)
    connect(page, SIGNAL(menuBarVisibilityChangeRequested(bool)),
            this, SIGNAL(menuBarVisibilityChangeRequested(bool)));
#endif
#if defined(QWEBENGINEPAGE_STATUSBARVISIBILITYCHANGEREQUESTED)
    connect(page, SIGNAL(statusBarVisibilityChangeRequested(bool)),
            this, SIGNAL(statusBarVisibilityChangeRequested(bool)));
#endif
#if defined(QWEBENGINEPAGE_TOOLBARVISIBILITYCHANGEREQUESTED)
    connect(page, SIGNAL(toolBarVisibilityChangeRequested(bool)),
            this, SIGNAL(toolBarVisibilityChangeRequested(bool)));
#endif

    // webview actions
    for (int i = 0; i < m_actions.count(); ++i) {
        WBWebActionMapper *mapper = m_actions[i];
        mapper->addChild(page->action(mapper->webAction()));
    }
}

WBWebView *WBTabWidget::newTab(bool makeCurrent) 
{
    // line edit
    WBUrlLineEdit *urlLineEdit = new WBUrlLineEdit;

    QSizePolicy urlPolicy = urlLineEdit->sizePolicy();
    urlLineEdit->setSizePolicy(QSizePolicy::MinimumExpanding, urlPolicy.verticalPolicy());

    QLineEdit *lineEdit = urlLineEdit->lineEdit();
    QSizePolicy policy = lineEdit->sizePolicy();
    lineEdit->setSizePolicy(QSizePolicy::MinimumExpanding, policy.verticalPolicy());

    if (!m_lineEditCompleter && count() > 0) {
        WBHistoryCompletionModel *completionModel = new WBHistoryCompletionModel(this);
        completionModel->setSourceModel(WBBrowserWindow::historyManager()->historyFilterModel());
        m_lineEditCompleter = new QCompleter(completionModel, this);
        // Should this be in Qt by default?
        QAbstractItemView *popup = m_lineEditCompleter->popup();
        QListView *listView = qobject_cast<QListView*>(popup);
        if (listView)
            listView->setUniformItemSizes(true);
    }
    lineEdit->setCompleter(m_lineEditCompleter);
    connect(lineEdit, SIGNAL(returnPressed()), this, SLOT(lineEditReturnPressed()));
    m_lineEdits->addWidget(urlLineEdit);
    m_lineEdits->setSizePolicy(lineEdit->sizePolicy());

    // optimization to delay creating the more expensive WebView, history, etc
    if (count() == 0) {
        QWidget *emptyWidget = new QWidget;
        QPalette p = emptyWidget->palette();
        p.setColor(QPalette::Window, palette().color(QPalette::Base));
        emptyWidget->setPalette(p);
        emptyWidget->setAutoFillBackground(true);
        disconnect(this, SIGNAL(currentChanged(int)),
            this, SLOT(currentChanged(int)));
        addTab(emptyWidget, tr("(Untitled)"));
        connect(this, SIGNAL(currentChanged(int)),
            this, SLOT(currentChanged(int)));
        return 0;
    }

    // webview
    m_webEngineView = new WBWebView;

	WBWebView* webView = m_webEngineView;
	webView->setPage(new WBWebPage(m_profile, webView));
    urlLineEdit->setWebView(webView);

	//加入通信功能qt与html通信,在作业页面找开相机功能
	
	InitChannel();

    connect(webView, SIGNAL(loadStarted()),
            this, SLOT(webViewLoadStarted()));
    connect(webView, SIGNAL(loadFinished(bool)), this, SLOT(webViewIconChanged()));
    connect(webView, SIGNAL(iconChanged(QIcon)),
            this, SLOT(webViewIconChanged()));
    connect(webView, SIGNAL(titleChanged(QString)),
            this, SLOT(webViewTitleChanged(QString)));

    connect(webView->page(), SIGNAL(audioMutedChanged(bool)),
            this, SLOT(webPageMutedOrAudibleChanged()));
    connect(webView->page(), SIGNAL(recentlyAudibleChanged(bool)),
            this, SLOT(webPageMutedOrAudibleChanged()));
    connect(webView, SIGNAL(urlChanged(QUrl)),
            this, SLOT(webViewUrlChanged(QUrl)));
    connect(webView, SIGNAL(pixmapCaptured(const QPixmap&, bool)), UBApplication::applicationController, SLOT(addCapturedPixmap(const QPixmap &, bool)));

    connect(webView, SIGNAL(embedCodeCaptured(const QString&)), UBApplication::applicationController, SLOT(addCapturedEmbedCode(const QString&)));



    connect(webView->page(), SIGNAL(windowCloseRequested()), this, SLOT(windowCloseRequested()));
//    connect(webView->page(), SIGNAL(geometryChangeRequested(const QRect &)), this, SIGNAL(geometryChangeRequested(const QRect &)));
//    connect(webView->page(), SIGNAL(printRequested()), this, SIGNAL(printRequested()));
//    connect(webView->page(), SIGNAL(menuBarVisibilityChangeRequested(bool)), this, SIGNAL(menuBarVisibilityChangeRequested(bool)));
//    connect(webView->page(), SIGNAL(statusBarVisibilityChangeRequested(bool)), this, SIGNAL(statusBarVisibilityChangeRequested(bool)));
//    connect(webView->page(), SIGNAL(toolBarVisibilityChangeRequested(bool)), this, SIGNAL(toolBarVisibilityChangeRequested(bool)));


    addTab(webView, tr("(Untitled)"));


    if (makeCurrent)
        setCurrentWidget(webView);

    // webview actions
    for (int i = 0; i < m_actions.count(); ++i)
    {
        WBWebActionMapper *mapper = m_actions[i];
        mapper->addChild(webView->page()->action(mapper->webAction()));
    }

    if (count() == 1)
        currentChanged(currentIndex());
emit tabsChanged();
    return webView;
//    if (makeCurrent)
//        setCurrentWidget(webView);

//    setupPage(webView->page());

//    if (count() == 1)
//        currentChanged(currentIndex());
//    emit tabsChanged();
//    return webView;
}

void WBTabWidget::InitChannel()
{
	m_camera = new QWebChannelCamera();
	QWebChannel *pChannel = new QWebChannel(this);
	pChannel->registerObject(QStringLiteral("channelCamera"), m_camera);
	m_webEngineView->page()->setWebChannel(pChannel);
}

void WBTabWidget::reloadAllTabs()
{
    for (int i = 0; i < count(); ++i) {
        QWidget *tabWidget = widget(i);
        if (WBWebView *tab = qobject_cast<WBWebView*>(tabWidget)) {
            tab->reload();
        }
    }
}

void WBTabWidget::lineEditReturnPressed()
{
    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(sender())) {
        emit loadPage(lineEdit->text());
        if (m_lineEdits->currentWidget() == lineEdit)
            currentWebView()->setFocus();
    }
}

void WBTabWidget::windowCloseRequested()
{
    WBWebPage *webPage = qobject_cast<WBWebPage*>(sender());
    WBWebView *webView = qobject_cast<WBWebView*>(webPage->view());
    int index = webViewIndex(webView);
    if (index >= 0)
    {
        if (count() == 1)
            webView->webPage()->mainWindow()->close();
        else
            closeTab(index);
    }
}

void WBTabWidget::closeOtherTabs(int index)
{
    if (-1 == index)
        return;
    for (int i = count() - 1; i > index; --i)
        closeTab(i);
    for (int i = index - 1; i >= 0; --i)
        closeTab(i);
}

// When index is -1 index chooses the current tab
void WBTabWidget::cloneTab(int index)
{
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;
    WBWebView *tab = newTab(false);
    tab->setUrl(webView(index)->url());
}

// When index is -1 index chooses the current tab
void WBTabWidget::requestCloseTab(int index)
{
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;
    WBWebView *tab = webView(index);
    if (!tab)
        return;
    tab->page()->triggerAction(QWebEnginePage::RequestClose);
}

void WBTabWidget::closeTab(int index)
{
    if (index < 0)
        index = currentIndex();
    if (index < 0 || index >= count())
        return;


    bool hasFocus = false;
    if (WBWebView *tab = webView(index)) {
        hasFocus = tab->hasFocus();

//        if (m_profile == QWebEngineProfile::defaultProfile()) {
            m_recentlyClosedTabsAction->setEnabled(true);
            m_recentlyClosedTabs.prepend(tab->url());
            if (m_recentlyClosedTabs.size() >= WBTabWidget::m_recentlyClosedTabsSize)
                m_recentlyClosedTabs.removeLast();
//        }
    }
    QWidget *lineEdit = m_lineEdits->widget(index);
    m_lineEdits->removeWidget(lineEdit);
    lineEdit->deleteLater();
    QWidget *webView = widget(index);
    removeTab(index);
    webView->deleteLater();
    emit tabsChanged();
    if (hasFocus && count() > 0)
        currentWebView()->setFocus();
//    if (count() == 0)
//        emit lastTabClosed();
    if (count() == 0){
        newTab();
        emit currentChanged(0);
    }
}

void WBTabWidget::setProfile(QWebEngineProfile *profile)
{
    m_profile = profile;
    for (int i = 0; i < count(); ++i) {
        QWidget *tabWidget = widget(i);
        if (WBWebView *tab = qobject_cast<WBWebView*>(tabWidget)) {
            WBWebPage* webPage = new WBWebPage(m_profile, tab);
            setupPage(webPage);
            webPage->load(tab->page()->url());
            tab->setPage(webPage);
        }
    }
}

void WBTabWidget::webViewLoadStarted()
{
    WBWebView *webView = qobject_cast<WBWebView*>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        QIcon icon(QLatin1String(":loading.gif"));
        setTabIcon(index, icon);

    }
}

void WBTabWidget::webViewIconChanged(/*const QIcon &icon*/)
{
    WBWebView *webView = qobject_cast<WBWebView*>(sender());
    int index = webViewIndex(webView);
    if (-1 != index)
        {
//        setTabIcon(index, icon);
        QIcon icon = webView->icon();

        WBUrlLineEdit *urlLineEdit = qobject_cast<WBUrlLineEdit*>(m_lineEdits->widget(index));

        if (urlLineEdit)
        {
            QLabel *label = new QLabel(urlLineEdit);
            label->setGeometry(0, 0, 32, 32);
            label->setPixmap(icon.pixmap(16, 16));
            label->setAlignment(Qt::AlignCenter);
            label->show();
            urlLineEdit->setLeftWidget(label);
        }
        }
}

void WBTabWidget::webViewTitleChanged(const QString &title)
{
    WBWebView *webView = qobject_cast<WBWebView*>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        setTabText(index, title);
    }
    if (currentIndex() == index)
        emit setCurrentTitle(title);
    WBBrowserWindow::historyManager()->updateHistoryItem(webView->url(), title);
}

void WBTabWidget::webPageMutedOrAudibleChanged() {
    QWebEnginePage* webPage = qobject_cast<QWebEnginePage*>(sender());
    WBWebView *webView = qobject_cast<WBWebView*>(webPage->view());

    int index = webViewIndex(webView);
    if (-1 != index) {
        QString title = webView->title();

        bool muted = webPage->isAudioMuted();
        bool audible = webPage->recentlyAudible();
        if (muted) title += tr(" (muted)");
        else if (audible) title += tr(" (audible)");

        setTabText(index, title);
    }
}

void WBTabWidget::webViewUrlChanged(const QUrl &url)
{
    WBWebView *webView = qobject_cast<WBWebView*>(sender());
    int index = webViewIndex(webView);
    if (-1 != index) {
        m_tabBar->setTabData(index, url);
//        WBHistoryManager *manager = WBBrowserWindow::historyManager();
//        if (url.isValid())
//            manager->addHistoryEntry(url.toString());
    }
        webView->show();
//    emit tabsChanged();
}

void WBTabWidget::aboutToShowRecentTabsMenu()
{
    m_recentlyClosedTabsMenu->clear();
    for (int i = 0; i < m_recentlyClosedTabs.count(); ++i) {
        QAction *action = new QAction(m_recentlyClosedTabsMenu);
        action->setData(m_recentlyClosedTabs.at(i));
//        QIcon icon = WBBrowserWindow::instance()->icon(m_recentlyClosedTabs.at(i));
//        action->setIcon(icon);
        action->setText(m_recentlyClosedTabs.at(i).toString());
        m_recentlyClosedTabsMenu->addAction(action);
    }
}

void WBTabWidget::aboutToShowRecentTriggeredAction(QAction *action)
{
    QUrl url = action->data().toUrl();
    loadUrlInCurrentTab(url);
}

void WBTabWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!childAt(event->pos())
            // Remove the line below when QTabWidget does not have a one pixel frame
            && event->pos().y() < (tabBar()->y() + tabBar()->height())) {
        newTab();
        return;
    }
    QTabWidget::mouseDoubleClickEvent(event);
}

void WBTabWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (!childAt(event->pos())) {
        m_tabBar->contextMenuRequested(event->pos());
        return;
    }
    QTabWidget::contextMenuEvent(event);
}

void WBTabWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QRect addTabRect = addTabButtonRect();

    if (addTabRect.contains(event->pos()))
    {
        newTab();
    }
    if (event->button() == Qt::MidButton && !childAt(event->pos())
            // Remove the line below when QTabWidget does not have a one pixel frame
            && event->pos().y() < (tabBar()->y() + tabBar()->height())) {
        QUrl url(QApplication::clipboard()->text(QClipboard::Selection));
        if (!url.isEmpty() && url.isValid() && !url.scheme().isEmpty()) {
            WBWebView *webView = newTab();
            webView->setUrl(url);
        }
    }
}

void WBTabWidget::loadUrlInCurrentTab(const QUrl &url)
{
    WBWebView *webView = currentWebView();
    if (webView) {
        webView->load(url);
        webView->setFocus();
    }
}

void WBTabWidget::nextTab()
{
    int next = currentIndex() + 1;
    if (next == count())
        next = 0;
    setCurrentIndex(next);
}

void WBTabWidget::previousTab()
{
    int next = currentIndex() - 1;
    if (next < 0)
        next = count() - 1;
    setCurrentIndex(next);
}

static const qint32 TabWidgetMagic = 0xaa;

QByteArray WBTabWidget::saveState() const
{
    int version = 1;
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << qint32(TabWidgetMagic);
    stream << qint32(version);

    QStringList tabs;
    for (int i = 0; i < count(); ++i) {
        if (WBWebView *tab = qobject_cast<WBWebView*>(widget(i))) {
            tabs.append(tab->url().toString());
        } else {
            tabs.append(QString());
        }
    }
    stream << tabs;
    stream << currentIndex();
    return data;
}

bool WBTabWidget::restoreState(const QByteArray &state)
{
    int version = 1;
    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);
    if (stream.atEnd())
        return false;

    qint32 marker;
    qint32 v;
    stream >> marker;
    stream >> v;
    if (marker != TabWidgetMagic || v != version)
        return false;

    QStringList openTabs;
    stream >> openTabs;

    for (int i = 0; i < openTabs.count(); ++i) {
        if (i != 0)
            newTab();
        loadPage(openTabs.at(i));
    }

    int currentTab;
    stream >> currentTab;
    setCurrentIndex(currentTab);

    return true;
}

void WBTabWidget::downloadRequested(QWebEngineDownloadItem *download)
{
    if (download->savePageFormat() != QWebEngineDownloadItem::UnknownSaveFormat) {
        WBSavePageDialog dlg(this, download->savePageFormat(), download->path());
        if (dlg.exec() != WBSavePageDialog::Accepted)
            return;
        download->setSavePageFormat(dlg.pageFormat());
        download->setPath(dlg.filePath());
    }

    WBBrowserWindow::downloadManager()->download(download);
    download->accept();
}

WBWebActionMapper::WBWebActionMapper(QAction *root, QWebEnginePage::WebAction webAction, QObject *parent)
    : QObject(parent)
    , m_currentParent(0)
    , m_root(root)
    , m_webAction(webAction)
{
    if (!m_root)
        return;
    connect(m_root, SIGNAL(triggered()), this, SLOT(rootTriggered()));
    connect(root, SIGNAL(destroyed(QObject*)), this, SLOT(rootDestroyed()));
    root->setEnabled(false);
}

void WBWebActionMapper::rootDestroyed()
{
    m_root = 0;
}

void WBWebActionMapper::currentDestroyed()
{
    updateCurrent(0);
}

void WBWebActionMapper::addChild(QAction *action)
{
    if (!action)
        return;
    connect(action, SIGNAL(changed()), this, SLOT(childChanged()));
}

QWebEnginePage::WebAction WBWebActionMapper::webAction() const
{
    return m_webAction;
}

void WBWebActionMapper::rootTriggered()
{
    if (m_currentParent) {
        QAction *gotoAction = m_currentParent->action(m_webAction);
        gotoAction->trigger();
    }
}

void WBWebActionMapper::childChanged()
{
    if (QAction *source = qobject_cast<QAction*>(sender())) {
        if (m_root
            && m_currentParent
            && source->parent() == m_currentParent) {
            m_root->setChecked(source->isChecked());
            m_root->setEnabled(source->isEnabled());
        }
    }
}

void WBWebActionMapper::updateCurrent(QWebEnginePage *currentParent)
{
    if (m_currentParent)
        disconnect(m_currentParent, SIGNAL(destroyed(QObject*)),
                   this, SLOT(currentDestroyed()));

    m_currentParent = currentParent;
    if (!m_root)
        return;
    if (!m_currentParent) {
        m_root->setEnabled(false);
        m_root->setChecked(false);
        return;
    }
    QAction *source = m_currentParent->action(m_webAction);
    m_root->setChecked(source->isChecked());
    m_root->setEnabled(source->isEnabled());
    connect(m_currentParent, SIGNAL(destroyed(QObject*)),
            this, SLOT(currentDestroyed()));
}
QRect WBTabWidget::addTabButtonRect()
{
    QRect lastTabRect = tabBar()->tabRect(tabBar()->count() -1);
    int x = lastTabRect.topRight().x();
    int y = 0;

    if (tabPosition() == QTabWidget::North)
        y = lastTabRect.topRight().y();
    else if (tabPosition() == QTabWidget::South)
        y = geometry().height() - lastTabRect.height();
    else
        qDebug() << "WBTabWidget::addTabButtonRect() - unsupported tab posion";

    // all this is in synch with CSS QTabBar ...
    QRect r(x + 3, y + 6 , 25, lastTabRect.height() - 8);

    return r;
}
void WBTabWidget::paintEvent ( QPaintEvent * event )
{
    QPainter painter(this);

    // all this is in synch with CSS QTabBar ...
    QLinearGradient linearGrad(QPointF(0, 0), QPointF(0, 1));
    linearGrad.setColorAt(0, QColor("#d3d3d3"));
    linearGrad.setColorAt(1, QColor("#dddddd"));
    painter.setBrush(linearGrad);

    QRect r = addTabButtonRect();

    painter.setPen(QColor("#888888"));
    painter.drawRoundedRect(r, 3, 3);

    painter.setPen(Qt::NoPen);
    painter.drawRect(QRect(r.x(), r.y() + r.height() / 2, r.width() + 1, r.height() / 2 + 1));

    painter.setPen(QColor("#888888"));
    painter.drawLine(r.x(), r.y() + r.height() / 2, r.x(), r.bottom());
    painter.drawLine(r.right() + 1, r.y() + r.height() / 2, r.right() + 1, r.bottom());

    if (tabPosition() == QTabWidget::South)
    {
        QPen pen = painter.pen();
        pen.setColor(QColor("#b3b3b3"));
        pen.setWidth(2);
        painter.setPen(pen);
        painter.drawLine(0, r.bottom() + 2, width(), r.bottom() + 2);
    }

    QPointF topLeft = r.center() - QPointF(mAddTabIcon.width() / 2, mAddTabIcon.height() / 2);
    painter.drawPixmap(topLeft, mAddTabIcon);

    painter.end();

    QTabWidget::paintEvent(event);
}

QWebChannelCamera::QWebChannelCamera(QObject *parent /*= nullptr*/):m_CameraDlg(nullptr)
{
	m_CameraDlg = new QCameraDialog();
	connect(m_CameraDlg, SIGNAL(sigImagePath(QString)), this, SIGNAL(SendImagePathToWeb(QString)));

	m_AutoRecorder = new AudioRecorder();
	connect(m_AutoRecorder, SIGNAL(sigFilePath(QString)), this, SIGNAL(SendImagePathToWeb(QString)));

}

void QWebChannelCamera::StartCamera(const QString& strTemp)
{
	if (strTemp== START_MAIC)
	{
		m_AutoRecorder->show();
	}
	else
	{
		m_CameraDlg->CameraStart();
		m_CameraDlg->show();
	}
}
