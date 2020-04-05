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

#include "WBUrlLineEdit.h"

#include "WBBrowserWindow.h"
#include "WBSearchLineEdit.h"
#include "WBWebView.h"

#include <QtCore/QEvent>
#include <QtCore/QMimeData>

#include <QtWidgets/QApplication>
#include <QtWidgets/QCompleter>
#include <QtGui/QFocusEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtGui/QDrag>
#include <QtGui/QPainter>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleOptionFrame>

#include <QtCore/QDebug>

WBExLineEdit::WBExLineEdit(QWidget *parent)
    : QWidget(parent)
    , m_leftWidget(0)
    , m_lineEdit(new QLineEdit(this))
    , m_clearButton(0)
{
    setFocusPolicy(m_lineEdit->focusPolicy());
    setAttribute(Qt::WA_InputMethodEnabled);
    setSizePolicy(m_lineEdit->sizePolicy());
    setBackgroundRole(m_lineEdit->backgroundRole());
    setMouseTracking(true);
    setAcceptDrops(true);
    setAttribute(Qt::WA_MacShowFocusRect, true);
    QPalette p = m_lineEdit->palette();
    setPalette(p);

    // line edit
    m_lineEdit->setFrame(false);
    m_lineEdit->setObjectName("ubWebBrowserLineEdit");
    m_lineEdit->setFocusProxy(this);
    m_lineEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
    QPalette clearPalette = m_lineEdit->palette();
    clearPalette.setBrush(QPalette::Base, QBrush(Qt::transparent));
    m_lineEdit->setPalette(clearPalette);

    // clearButton
    m_clearButton = new WBClearButton(this);
    connect(m_clearButton, SIGNAL(clicked()),
            m_lineEdit, SLOT(clear()));
    connect(m_lineEdit, SIGNAL(textChanged(QString)),
            m_clearButton, SLOT(textChanged(QString)));
     m_clearButton->hide();
}

void WBExLineEdit::setLeftWidget(QWidget *widget)
{
    delete m_leftWidget;
    m_leftWidget = widget;

    updateGeometries();

}

QWidget *WBExLineEdit::leftWidget() const
{
    return m_leftWidget;
}

void WBExLineEdit::resizeEvent(QResizeEvent *event)
{
    Q_ASSERT(m_leftWidget);
    updateGeometries();
    QWidget::resizeEvent(event);
}

void WBExLineEdit::updateGeometries()
{
    QStyleOptionFrame panel;
    initStyleOption(&panel);
    QRect rect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);

//    int height = rect.height();
    int width = rect.width();

    int m_leftWidgetHeight = m_leftWidget->height();
    m_leftWidget->setGeometry(0, -4, 32 ,32);

    int clearButtonWidth = this->height();
    m_lineEdit->setGeometry(32, 0,
                            width - this->height() - 32, this->height());

    m_clearButton->setGeometry(this->width() - this->height() - 2 , 0,
                               this->height(), this->height());
}

void WBExLineEdit::initStyleOption(QStyleOptionFrame *option) const
{
    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this);
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;
    if (m_lineEdit->isReadOnly())
        option->state |= QStyle::State_ReadOnly;
#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    option->features = QStyleOptionFrame::None;
}

QSize WBExLineEdit::sizeHint() const
{
    m_lineEdit->setFrame(true);
    QSize size = m_lineEdit->sizeHint();
    m_lineEdit->setFrame(false);
    return size;
}

void WBExLineEdit::focusInEvent(QFocusEvent *event)
{
    m_lineEdit->event(event);
    QWidget::focusInEvent(event);
}

void WBExLineEdit::focusOutEvent(QFocusEvent *event)
{
    m_lineEdit->event(event);

    if (m_lineEdit->completer()) {
        connect(m_lineEdit->completer(), SIGNAL(activated(QString)),
                         m_lineEdit, SLOT(setText(QString)));
        connect(m_lineEdit->completer(), SIGNAL(highlighted(QString)),
                         m_lineEdit, SLOT(_q_completionHighlighted(QString)));
    }
    QWidget::focusOutEvent(event);
}

void WBExLineEdit::keyPressEvent(QKeyEvent *event)
{
    m_lineEdit->event(event);
     QWidget::keyPressEvent(event);
}

bool WBExLineEdit::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride)
        return m_lineEdit->event(event);
    return QWidget::event(event);
}

void WBExLineEdit::paintEvent(QPaintEvent *)
{
//    QPainter p(this);
//    QStyleOptionFrame panel;
//    initStyleOption(&panel);
//    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
}

QVariant WBExLineEdit::inputMethodQuery(Qt::InputMethodQuery property) const
{
    return m_lineEdit->inputMethodQuery(property);
}

void WBExLineEdit::inputMethodEvent(QInputMethodEvent *e)
{
    m_lineEdit->event(e);
}
void WBExLineEdit::setVisible(bool pVisible)
{
    QWidget::setVisible(pVisible);
    m_lineEdit->setVisible(pVisible);
    m_clearButton->setVisible(pVisible);
    m_leftWidget->setVisible(pVisible);
}


class WBUrlIconLabel : public QLabel
{

public:
    WBUrlIconLabel(QWidget *parent);

    WBWebView *m_webView;

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    QPoint m_dragStartPos;

};

WBUrlIconLabel::WBUrlIconLabel(QWidget *parent)
    : QLabel(parent)
    , m_webView(0)
{
    setMinimumWidth(16);
    setMinimumHeight(16);
}

void WBUrlIconLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        m_dragStartPos = event->pos();
    QLabel::mousePressEvent(event);
}

void WBUrlIconLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() == Qt::LeftButton
        && (event->pos() - m_dragStartPos).manhattanLength() > QApplication::startDragDistance()
         && m_webView) {
        QDrag *drag = new QDrag(this);
        QMimeData *mimeData = new QMimeData;
        mimeData->setText(QString::fromUtf8(m_webView->url().toEncoded()));
        QList<QUrl> urls;
        urls.append(m_webView->url());
        mimeData->setUrls(urls);
        drag->setMimeData(mimeData);
        drag->exec();
    }
}

WBUrlLineEdit::WBUrlLineEdit(QWidget *parent)
    : WBExLineEdit(parent)
    , m_webView(0)/*
    , m_iconLabel(0)*/
{
    // icon
//    m_iconLabel = new WBUrlIconLabel(this);
//    m_iconLabel->resize(16, 16);
    setLeftWidget(new QWidget(this));
//    m_defaultBaseColor = palette().color(QPalette::Base);
}

void WBUrlLineEdit::setWebView(WBWebView *webView)
{
//    Q_ASSERT(!m_webView);
    m_webView = webView;
//    m_iconLabel->m_webView = webView;
    connect(webView, SIGNAL(urlChanged(QUrl)),
        this, SLOT(webViewUrlChanged(QUrl)));
//    connect(webView, SIGNAL(iconChanged(QIcon)),
//        this, SLOT(webViewIconChanged(QIcon)));
    connect(webView, SIGNAL(loadProgress(int)),
        this, SLOT(update()));
}

void WBUrlLineEdit::webViewUrlChanged(const QUrl &url)
{
    m_lineEdit->setText(url.toString());
    m_lineEdit->setCursorPosition(0);
}

void WBUrlLineEdit::webViewIconChanged(const QIcon &icon)
{
    Q_ASSERT(m_webView);
    m_iconLabel->setPixmap(icon.pixmap(16, 16));
}

QLinearGradient WBUrlLineEdit::generateGradient(const QColor &color) const
{
    QLinearGradient gradient(0, 0, 0, height());
    gradient.setColorAt(0, m_defaultBaseColor);
    gradient.setColorAt(0.15, color.lighter(120));
    gradient.setColorAt(0.5, color);
    gradient.setColorAt(0.85, color.lighter(120));
    gradient.setColorAt(1, m_defaultBaseColor);
    return gradient;
}

void WBUrlLineEdit::focusOutEvent(QFocusEvent *event)
{
    if (m_lineEdit->text().isEmpty() && m_webView)
        m_lineEdit->setText(QString::fromUtf8(m_webView->url().toEncoded()));
    WBExLineEdit::focusOutEvent(event);
}

//void WBUrlLineEdit::paintEvent(QPaintEvent *event)
//{
//    QPalette p = palette();
//    if (m_webView && m_webView->url().scheme() == QLatin1String("https")) {
//        QColor lightYellow(248, 248, 210);
//        p.setBrush(QPalette::Base, generateGradient(lightYellow));
//    } else {
//        p.setBrush(QPalette::Base, m_defaultBaseColor);
//    }
//    setPalette(p);
//    WBExLineEdit::paintEvent(event);

//    QPainter painter(this);
//    QStyleOptionFrame panel;
//    initStyleOption(&panel);
//    QRect backgroundRect = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
//    if (m_webView && !hasFocus()) {
//        int progress = m_webView->progress();
//        QColor loadingColor = QColor(116, 192, 250);
//        painter.setBrush(generateGradient(loadingColor));
//        painter.setPen(Qt::transparent);
//        int mid = backgroundRect.width() / 100.0f * progress;
//        QRect progressRect(backgroundRect.x(), backgroundRect.y(), mid, backgroundRect.height());
//        painter.drawRect(progressRect);
//    }
//}
