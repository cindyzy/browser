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

#ifndef WBTABWIDGET_H
#define WBTABWIDGET_H

#include <QtWebEngineWidgets/QWebEngineFullScreenRequest>
#include <QtWidgets/QTabBar>
#include <QtWidgets/QShortcut>
#include <QStackedWidget>
#include <QLineEdit>
#include <gui/UBCameraDlg.h>
QT_BEGIN_NAMESPACE
class QWebEngineDownloadItem;
class QWebEngineProfile;
class QWebEngineView;
QT_END_NAMESPACE

/*
    Tab bar with a few more features such as a context menu and shortcuts
 */
class WBTabBar : public QTabBar
{
    Q_OBJECT

signals:
    void newTab();
    void cloneTab(int index);
    void closeTab(int index);
    void closeOtherTabs(int index);
    void reloadTab(int index);
    void muteTab(int index, bool mute);
    void reloadAllTabs();
    void tabMoveRequested(int fromIndex, int toIndex);

public:
    WBTabBar(QWidget *parent = 0);

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);

private slots:
    void selectTabAction();
    void cloneTab();
    void closeTab();
    void closeOtherTabs();
    void reloadTab();
    void muteTab();
    void unmuteTab();
    void contextMenuRequested(const QPoint &position);

private:
    QList<QShortcut*> m_tabShortcuts;
    friend class WBTabWidget;

    QPoint m_dragStartPos;
    int m_dragCurrentIndex;
};

#include <QWebEnginePage>
#include <gui/Audiorecorder.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE
class WBWebView;


class QWebChannelCamera : public QObject
{
	Q_OBJECT
public:
	explicit QWebChannelCamera(QObject *parent = nullptr);

signals:
	void SendImagePathToWeb(QString strTemp);

public slots:
	void StartCamera(const QString& strTemp);

private:
	QCameraDialog *m_CameraDlg;
	AudioRecorder *m_AutoRecorder;

	
};



/*!
    A proxy object that connects a single browser action
    to one child webpage action at a time.

    Example usage: used to keep the main window stop action in sync with
    the current tabs webview's stop action.
 */

class WBWebActionMapper : public QObject
{
    Q_OBJECT

public:
    WBWebActionMapper(QAction *root, QWebEnginePage::WebAction webAction, QObject *parent);
    QWebEnginePage::WebAction webAction() const;
    void addChild(QAction *action);
    void updateCurrent(QWebEnginePage *currentParent);

private slots:
    void rootTriggered();
    void childChanged();
    void rootDestroyed();
    void currentDestroyed();

private:
    QWebEnginePage *m_currentParent;
    QAction *m_root;
    QWebEnginePage::WebAction m_webAction;
};

#include <QtCore/QUrl>
#include <QtWidgets/QTabWidget>

class FullScreenNotification;

QT_BEGIN_NAMESPACE
class QCompleter;
class QLineEdit;
class QMenu;
class QStackedWidget;
QT_END_NAMESPACE

/*!
    TabWidget that contains WebViews and a stack widget of associated line edits.

    Connects up the current tab's signals to this class's signal and uses WebActionMapper
    to proxy the actions.
 */
class WBTabWidget : public QTabWidget
{
    Q_OBJECT

signals:
    // tab widget signals
    void loadPage(const QString &url);
    void tabsChanged();
    void lastTabClosed();
 void loadFinished(bool pOk);
    // current tab signals
    void setCurrentTitle(const QString &url);
    void showStatusBarMessage(const QString &message);
    void linkHovered(const QString &link);
    void loadProgress(int progress);
    void geometryChangeRequested(const QRect &geometry);
    void menuBarVisibilityChangeRequested(bool visible);
    void statusBarVisibilityChangeRequested(bool visible);
    void toolBarVisibilityChangeRequested(bool visible);
#if defined(QWEBENGINEPAGE_PRINTREQUESTED)
    void printRequested(QWebEngineFrame *frame);
#endif

public:
    WBTabWidget(QWidget *parent = 0);
    ~WBTabWidget();
    void clear();
    void addWebAction(QAction *action, QWebEnginePage::WebAction webAction);

    QAction *newTabAction() const;
    QAction *closeTabAction() const;
    QAction *recentlyClosedTabsAction() const;
    QAction *nextTabAction() const;
    QAction *previousTabAction() const;

    QWidget *lineEditStack() const;
    QLineEdit *currentLineEdit() const;
    WBWebView *currentWebView() const;
    WBWebView *webView(int index) const;
    QLineEdit *lineEdit(int index) const;
    int webViewIndex(WBWebView *webView) const;

    QByteArray saveState() const;
    bool restoreState(const QByteArray &state);

    void setProfile(QWebEngineProfile *profile);
    QStackedWidget* lineEdits() { return m_lineEdits; }
     void setLineEditStackVisible(bool visible) {m_lineEdits->setVisible(visible);m_lineEdits->hide();}

protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	QRect addTabButtonRect();
	void paintEvent(QPaintEvent * event);
	//qt与HTML通信
	void InitChannel();

public slots:
    void loadUrlInCurrentTab(const QUrl &url);
    WBWebView *newTab(bool makeCurrent = true);


    void cloneTab(int index = -1);
    void requestCloseTab(int index = -1);
    void closeTab(int index);
    void closeOtherTabs(int index);
    void reloadTab(int index = -1);
    void reloadAllTabs();
    void nextTab();
    void previousTab();
    void setAudioMutedForTab(int index, bool mute);

private slots:
    void currentChanged(int index);
    void aboutToShowRecentTabsMenu();
    void aboutToShowRecentTriggeredAction(QAction *action);
    void downloadRequested(QWebEngineDownloadItem *download);
    void webViewLoadStarted();
    void webViewIconChanged(/*const QIcon &icon*/);
    void webViewTitleChanged(const QString &title);
    void webViewUrlChanged(const QUrl &url);
    void lineEditReturnPressed();
    void windowCloseRequested();
    void moveTab(int fromIndex, int toIndex);
    void fullScreenRequested(QWebEngineFullScreenRequest request);
    void handleTabBarDoubleClicked(int index);
    void webPageMutedOrAudibleChanged();

private:
    void setupPage(QWebEnginePage* page);

    QAction *m_recentlyClosedTabsAction;
    QAction *m_newTabAction;
    QAction *m_closeTabAction;
    QAction *m_nextTabAction;
    QAction *m_previousTabAction;

    QMenu *m_recentlyClosedTabsMenu;
    static const int m_recentlyClosedTabsSize = 10;
    QList<QUrl> m_recentlyClosedTabs;
    QList<WBWebActionMapper*> m_actions;

    QCompleter *m_lineEditCompleter;
    QStackedWidget *m_lineEdits;
     WBTabBar *m_tabBar;
    QWebEngineProfile *m_profile;
    QWebEngineView *m_fullScreenView;

	WBWebView *m_webEngineView;//为了通信加了此变量
	QWebChannelCamera *m_camera;

    FullScreenNotification *m_fullScreenNotification;
    QRect m_oldWindowGeometry;
    QPixmap mAddTabIcon;
};


#endif // WBTABWIDGET_H
