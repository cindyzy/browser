/*
 * Copyright (C) 2015-2018 Département de l'Instruction Publique (DIP-SEM)
 *
 * Copyright (C) 2013 Open Education Foundation
 *
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour
 * l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of OpenBoard.
 *
 * OpenBoard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * OpenBoard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBoard. If not, see <http://www.gnu.org/licenses/>.
 */




#include "WBWebTrapWebView.h"

#include <QtGui>
//////#include <QWebEngineView>
//#include <QWebHitTestResult>
//#include <QWebEnginePage>

//#include "web/UBWebKitUtils.h"

#include "core/UBApplication.h"
#include "core/UBApplicationController.h"
#include "board/UBBoardController.h"

#include "core/memcheck.h"
#include <QDomElement>
WBWebTrapWebView::WBWebTrapWebView(QWidget* parent)
    : QWebEngineView(parent)
    , mCurrentContentType(Unknown)
    , mIsTrapping(false)
    , mTrapingWidget(0)
{
    connect(this, SIGNAL(loadFinished ( bool)), this, SLOT(viewLoadFinished(bool)));
}


WBWebTrapWebView::~WBWebTrapWebView()
{
    delete mTrapingWidget;
}


void WBWebTrapWebView::setIsTrapping(bool pIsTrapping)
{
    mIsTrapping = pIsTrapping;

    mDomElementRect = QRect();
    mCurrentContentType = Unknown;
    /*
#if defined(Q_OS_WIN)

    if(mIsTrapping)
    {
        if (!mTrapingWidget)
        {
            mTrapingWidget = new UBWebTrapMouseEventMask(this);
            mTrapingWidget->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
            mTrapingWidget->setAttribute(Qt::WA_TranslucentBackground, true);
        }

        QPoint topLeft = mapToGlobal (geometry().topLeft());
        QRect geom(topLeft.x(), topLeft.y(), geometry().width(), geometry().height());

        mTrapingWidget->setGeometry(geom);
        mTrapingWidget->setVisible(true);

    }
    else if (mTrapingWidget)
    {
         mTrapingWidget->setVisible(false);
    }

#endif
*/

    update();
}

void WBWebTrapWebView::hideEvent ( QHideEvent * event )
{
    setIsTrapping(false);
    QWebEngineView::hideEvent(event);
}


void WBWebTrapWebView::highliteElementAtPos ( const QPoint& pos)
{
    mCurrentContentType = Unknown;
    //确定这函数做什么的
    if(page())
    {

        page()->runJavaScript( QString("document.elementFromPoint(%1, %2).tagName").arg(pos.x()).arg(pos.y()), [this, pos](const QVariant &tagNamepara) {

        QString tagName=tagNamepara.toString().toLower();
        if(tagName == "img")
        {
            mCurrentContentType = Image;
//            qDebug() << "found pixmap at " << htr.boundingRect();
        }
        else if (tagName == "object"
            || tagName == "embed")
        {
            mCurrentContentType = ObjectOrEmbed;
        }
        else if ((tagName == "input") || (tagName == "textarea"))
        {
             potentialEmbedCodeAtPos(pos,"highliteElementAtPos");


        } else
        {
            page()->runJavaScript( QString("document.elementFromPoint(%1, %2).getAttribute('id')").arg(pos.x()).arg(pos.y()), [this,tagName, pos](const QVariant &idpara) {
            QString id =idpara.toString();
            QString idSelector = tagName + "#" + id;

            page()->runJavaScript( QString("document.elementFromPoint(%1, %2)==document.querySelector('%3')").arg(pos.x()).arg(pos.y()).arg(idSelector), [this,tagName, pos,idSelector](const QVariant &idSuccesspara) {
bool idSuccess=idSuccesspara.toBool();
if (idSuccess)
{
    mElementQuery = idSelector;
    mCurrentContentType = ElementByQuery;
}
else
{
    //bool isValid = true;

    page()->runJavaScript( QString("function myFunction(){document.elementFromPoint(%1, %2)"
                                   " var elParent = el.parentNode;"
                                   " var currentEl = el;"
                                   " var path = el.tagName;"
                                   " var pathElements=[];"
                                   "do "
                                   "{  var someSibling = elParent.firstChild; var index = 0;var foundIndex = false;"
                                   "do{"
                                   "if (someSibling.tagName == currentEl.tagName){"
                                   "if (someSibling == currentEl)"
                                   "{foundIndex = true;}"
                                   "else"
                                   "{ index++};"
                                   "}"
                                   "someSibling = someSibling.nextSibling;}"
                                   "while(!(!someSibling && typeof(someSibling)!='undefined' && someSibling!=0)&& !foundIndex);"
                                   "var part;"
                                   "if (index > 0)"
                                   "{ part=currentEl.tagName+':nth-child('+index+')';}"
                                   "else{ "
                                   "part = currentEl.tagName;pathElements.splice(0,0, part);"
                                   " currentEl = elParent;elParent = elParent.parentNode;}"
                                   "} while(!(!elParent && typeof(elParent)!='undefined' && elParent!=0));"
                                   "return pathElements.join(' > ';}").arg(pos.x()).arg(pos.y()).arg(idSelector), [this,tagName, pos,idSelector](const QVariant &treeSelectorpara) {

        QString treeSelector = treeSelectorpara.toString();

        mElementQuery = treeSelector;
        mCurrentContentType = ElementByQuery;


    });

    //QString idSelector = tagName + "#" + id;

    //bool treeSuccess = (el == el.document().findFirst(treeSelector));

    //qDebug() << "----------------------------";
    //qDebug() << idSuccess << idSelector;
    //qDebug() << treeSuccess << treeSelector;
}

            });

            });


                   }

        });

//        update(updateRect);
    }
}


void WBWebTrapWebView::potentialEmbedCodeAtPos(const QPoint& pos,const QString& method)
{

    QString scr = QString("document.elementFromPoint(%1, %2).tagName")
            .arg(pos.x())
            .arg(pos.y());

    //    QVariant tagNameVar = page()->currentFrame()->evaluateJavaScript(scr);
    page()->runJavaScript(scr, [this, pos,method](const QVariant &tagNameVar) {


        QString tagName = tagNameVar.toString().toLower();


        if ((tagName == "input") || (tagName == "textarea"))
        {
            QString valueScript;

            if (tagName == "input")
            {
                valueScript = QString("document.elementFromPoint(%1, %2).value")
                        .arg(pos.x())
                        .arg(pos.y());
            }
            else if (tagName == "textarea")
            {
                valueScript = QString("document.elementFromPoint(%1, %2).innerHTML")
                        .arg(pos.x())
                        .arg(pos.y());
            }

            page()->runJavaScript(valueScript ,[this, pos,method](const QVariant &valueVar) {
                QString value = valueVar.toString();
                if(method=="highliteElementAtPos")
                {
                    if (value.length() > 0)
                    {
                        qDebug() << "found input data \n\n" << value;
                        mCurrentContentType = Input;
                    }
                }else if(method=="trapElementAtPos")
                {
                    if (value.length() > 0 && value.contains("width") && value.contains("height"))
                    {
                        QString embedCode=value;
                        if (embedCode.length() > 0)
                        {
                            emit embedCodeCaptured(embedCode);
                        }
                    }
                }

            });


        }


    });


    //    return "";
}


void WBWebTrapWebView::trapElementAtPos(const QPoint& pos)
{
    page()->runJavaScript( QString("document.elementFromPoint(%1, %2).tagName").arg(pos.x()).arg(pos.y()), [this, pos](const QVariant &tagNamepara) {

    QString tagName=tagNamepara.toString().toLower();
    if(tagName == "img")
    {
//        emit pixmapCaptured(htr.pixmap(), false);//需修改
//            qDebug() << "found pixmap at " << htr.boundingRect();
    }    else if (mCurrentContentType == ObjectOrEmbed)
    {
        QString embedSelector = QString("document.elementFromPoint(%1, %2)").arg(pos.x()).arg(pos.y());
        QString transembedSelector=embedSelector;

        page()->runJavaScript(embedSelector + ".tagName", [ this,pos,transembedSelector](const QVariant &tagName) {
            QString embedSelector=transembedSelector;
            if (tagName.toString().toLower() == "object")
            {
                embedSelector = QString("document.elementFromPoint(%1, %2).getElementsByTagName('object')[0]")
                        .arg(pos.x()).arg(pos.y());
            }
            QString querySource = embedSelector + ".src";
             page()->runJavaScript(querySource,[this,embedSelector](const QVariant &resSource){
                 qDebug() << "resSource" << resSource;
                 QString source = resSource.toString();
                 QString queryType = embedSelector + ".type";
                 page()->runJavaScript(queryType,[this,embedSelector,source](const QVariant &resType){
                     QString type = resType.toString();

                     if (source.trimmed().length() > 0)
                     {

                         UBApplication::boardController->downloadURL(QUrl(source));
                         UBApplication::applicationController->showBoard();
                     }
                 });


            });



        });



        page()->runJavaScript(embedSelector + ".innerHTML", [](const QVariant &innerHTML) {

//            qDebug() << "innerHTML" << innerHTML;

        });




    }
    else if (mCurrentContentType == Input)
    {
        potentialEmbedCodeAtPos(pos,"trapElementAtPos");

    }
    else if (mCurrentContentType == ElementByQuery)
    {
        webElementCaptured(page()->url(), mElementQuery);
    }
    });
//    QWebHitTestResult htr = page()->currentFrame()->hitTestContent(pos);

//    if (!htr.pixmap().isNull())
//    {
//        emit pixmapCaptured(htr.pixmap(), false);
//    }

}


void WBWebTrapWebView::mouseMoveEvent ( QMouseEvent * event )
{
    if (mIsTrapping)
    {
        highliteElementAtPos(event->pos());
    }
    else
    {
        QWebEngineView::mouseMoveEvent(event);
    }
}

void WBWebTrapWebView::mousePressEvent(QMouseEvent* event)
{
    if (mIsTrapping)
    {
        // NOOP - avoid event routing to web kit (accept is not enough)
    }
    else
    {
        QWebEngineView::mousePressEvent(event);
    }
}


void WBWebTrapWebView::mouseReleaseEvent ( QMouseEvent * event )
{
    //bool eventConsumed = false;

    //qDebug() << "mouse release" << event->pos();

    if (mIsTrapping)
    {
        trapElementAtPos(event->pos());
    }
    else
    {
        QWebEngineView::mouseReleaseEvent(event);
    }

    //if (!eventConsumed)


    /*
        QString script;
        script += QString("var element = document.elementFromPoint(%1, %2);").arg(event->pos().x()).arg(event->pos().y());

        script += "var path = '';";
        script += "var currentNode = element;";
        script += "while(currentNode){";

        script += "    if (currentNode.nodeType == Node.ELEMENT_NODE){";
        script += "        var tn = currentNode.tagName;";

        script += "        var elp = currentNode.parentNode;";
        script += "        var index = -1;";
        script += "        if(elp){";

        script += "            for(i = 0; i < elp.childNodes.length; i++){";
        script += "                var node = elp.childNodes[i];";
        script += "                if(node.nodeType == Node.ELEMENT_NODE && node.tagName == tn){";
        script += "                    index++;";
        script += "                }";
        script += "                if(node == currentNode){";
        script += "                    break;";
        script += "                }";

        script += "            }";
        script += "        }";

        script += "        path = tn + '[' + index + '] > ' + path;";
        script += "    }";

        script += "    currentNode = currentNode.parentNode;";
        script += "}";

        script += "'id: ' + element.id + ', TagName: ' + element.tagName + ', at: ' + path;";

        QVariant result = page()->currentFrame()->evaluateJavaScript(script);

        qDebug() << result.isValid() << result.toString();
        */


    //if (result.isValid())
    //{
    //emit trapped(url(), result.toString());
    //}
}


void WBWebTrapWebView::paintEvent ( QPaintEvent * event )
{
    QWebEngineView::paintEvent(event);

    if (mIsTrapping && mDomElementRect.isValid()
            && page() )
    {
        QPainter p(this);

        QPen pen;

        if (mCurrentContentType != Unknown)
            pen = QPen(QColor(0, 255, 0, 127));
        //else if (mCurrentContentType == ObjectOrEmbed)
        //    pen = QPen(QColor(0, 0, 255, 127));
        //else if (mCurrentContentType == Input)
        //    pen = QPen(QColor(0, 0, 255, 127));
        else
            pen = QPen(QColor(255, 0, 0, 127));

        pen.setJoinStyle(Qt::RoundJoin);
        pen.setWidth(8);
        p.setPen(pen);

        QPointF scrollPosition = page()->scrollPosition();
        QRect webViewPosition = mDomElementRect;
        webViewPosition.translate(-scrollPosition.toPoint());

        mWebViewElementRect = webViewPosition.adjusted(-4, -4, 4, 4);

        p.drawRect(mWebViewElementRect);

        //pen.setColor(Qt::black);
        //p.setPen(pen);
        //p.drawText(mDomElementRect.adjusted(8, 8, -8, -8), mElementID);
    }

}


void WBWebTrapWebView::viewLoadFinished ( bool ok )
{
    Q_UNUSED(ok);

}



UBWebTrapMouseEventMask::UBWebTrapMouseEventMask(WBWebTrapWebView* pWebView)
    : mTrappedWebView(pWebView)
{
    setMouseTracking(true);
}

UBWebTrapMouseEventMask::~UBWebTrapMouseEventMask()
{
    // NOOP
}

void UBWebTrapMouseEventMask::mousePressEvent(QMouseEvent* event)
{
    if (mTrappedWebView && mTrappedWebView->isTrapping())
    {
        // NOOP
    }
    else
    {
        QWidget::mousePressEvent(event);
    }
}


void UBWebTrapMouseEventMask::mouseMoveEvent ( QMouseEvent * event )
{
    if (mTrappedWebView && mTrappedWebView->isTrapping())
    {
        mTrappedWebView->highliteElementAtPos(event->pos());
    }
    else
    {
        QWidget::mouseMoveEvent(event);
    }
}


void UBWebTrapMouseEventMask::mouseReleaseEvent ( QMouseEvent * event )
{
    if (mTrappedWebView && mTrappedWebView->isTrapping())
    {
        mTrappedWebView->trapElementAtPos(event->pos());
    }
    else
    {
        QWidget::mouseReleaseEvent(event);
    }
}


void UBWebTrapMouseEventMask::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    //painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(127, 127, 127, 15));

    painter.drawRect(0, 0, width(), height());
}

