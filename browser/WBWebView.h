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

#ifndef WBWEBVIEW_H
#define WBWEBVIEW_H

#include <QIcon>
#include <QWebEngineView>
#include "WBWebTrapWebView.h"
#include "web/UBWebPage.h"
QT_BEGIN_NAMESPACE
class QAuthenticator;
class QMouseEvent;
class QNetworkProxy;
class QNetworkReply;
class QSslError;
QT_END_NAMESPACE

class WBBrowserWindow;
class WBWebPage : public UBWebPage {
	Q_OBJECT
signals:
	void loadingUrl(const QUrl &url);
public:
	WBWebPage(QWebEngineProfile *profile, QObject *parent = 0);
	WBBrowserWindow *mainWindow();

protected:
	QWebEnginePage *createWindow(QWebEnginePage::WebWindowType type) Q_DECL_OVERRIDE;
#if !defined(QT_NO_UITOOLS)
	QObject *createPlugin(const QString &classId, const QUrl &url, const QStringList &paramNames, const QStringList &paramValues);
#endif
	virtual bool certificateError(const QWebEngineCertificateError &error) Q_DECL_OVERRIDE;
	bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame);

private slots:
#if defined(QWEBENGINEPAGE_UNSUPPORTEDCONTENT)
	void handleUnsupportedContent(QNetworkReply *reply);
#endif
	//    void authenticationRequired(const QUrl &requestUrl, QAuthenticator *auth);
	//    void proxyAuthenticationRequired(const QUrl &requestUrl, QAuthenticator *auth, const QString &proxyHost);

private:
	friend class WBWebView;

	// set the webview mousepressedevent
	Qt::KeyboardModifiers m_keyboardModifiers;
	Qt::MouseButtons m_pressedButtons;
	bool mOpenInNewTab;
	QUrl mLoadingUrl;
};

class WBWebView : public WBWebTrapWebView {
	Q_OBJECT

public:
	WBWebView(QWidget *parent = 0);
	WBWebPage *webPage() const { return m_page; }
	//    void setPage(WBWebPage *page);

	void load(const QUrl &url);
	QUrl url() const;
	void load(const QWebEngineHttpRequest & request);
	QString lastStatusBarText() const;
	inline int progress() const { return m_progress; }

protected:
	void mousePressEvent(QMouseEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);
	void contextMenuEvent(QContextMenuEvent *event);
	void wheelEvent(QWheelEvent *event);

private slots:
	void setProgress(int progress);
	void loadFinished(bool success);
	void loadStarted();
	void setStatusBarText(const QString &string);
	void onFeaturePermissionRequested(const QUrl &securityOrigin, QWebEnginePage::Feature);
	//    void onIconChanged(const QIcon &icon);
	void downloadRequested(QWebEngineDownloadItem* downloaditem);
	void openLinkInNewTab();
private:
	QString m_statusBarText;
	QUrl m_initialUrl;
	int m_progress;
	WBWebPage *m_page;
	QTime mLoadStartTime;
};

#endif
