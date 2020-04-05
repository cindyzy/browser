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




#include "UBTrapFlashController.h"

#include <QtXml>



#include "frameworks/UBFileSystemUtils.h"

#include "core/UBApplicationController.h"
#include "core/UBApplication.h"
#include "core/UBSettings.h"

#include "network/UBNetworkAccessManager.h"

#include "domain/UBGraphicsScene.h"

#include "board/UBBoardController.h"

#include "frameworks/UBPlatformUtils.h"

#include "ui_trapFlash.h"

#include "core/memcheck.h"
UBTrapFlashController* UBTrapFlashController::s_instance = NULL;
UBTrapFlashController::UBTrapFlashController(QWidget* parent)
    : QObject(parent)
    , mTrapFlashUi(0)
    , mTrapFlashDialog(0)
    , mParentWidget(parent)
//    #if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)
    , mCurrentWebFrame(0)
//    #endif

{
    // NOOP
     s_instance = this;
}


UBTrapFlashController::~UBTrapFlashController()
{
    // NOOP
}
//创建截取网页元素的对话框

void UBTrapFlashController::showTrapFlash(
        )
{
    if (!mTrapFlashDialog)
    {
        Qt::WindowFlags flag = Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint ;
        flag &= ~ Qt::WindowContextHelpButtonHint;
        flag &= ~ Qt::WindowMinimizeButtonHint;

        mTrapFlashDialog = new QDialog(mParentWidget, flag);
        mTrapFlashUi = new Ui::trapFlashDialog();
        mTrapFlashUi->setupUi(mTrapFlashDialog);

//        mTrapFlashUi->webView->setNetworkAccessManager(UBNetworkAccessManager::defaultAccessManager());
        int viewWidth = mParentWidget->width() / 2;
        int viewHeight = mParentWidget->height() * 2. / 3.;
        mTrapFlashDialog->setGeometry(
                (mParentWidget->width() - viewWidth) / 2
                , (mParentWidget->height() - viewHeight) / 2
                , viewWidth
                , viewHeight);

        connect(mTrapFlashUi->flashCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFlash(int)));
        connect(mTrapFlashUi->widgetNameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(text_Changed(const QString &)));
        connect(mTrapFlashUi->widgetNameLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(text_Edited(const QString &)));
        connect(mTrapFlashUi->createWidgetButton, SIGNAL(clicked(bool)), this, SLOT(createWidget()));
        mTrapFlashDialog->show();
    }else
    {
        if (mTrapFlashDialog != NULL)
            {
                delete mTrapFlashDialog;
                mTrapFlashDialog = nullptr;
            }
        if (mTrapFlashUi != NULL)
            {
                delete mTrapFlashUi;
                mTrapFlashUi = nullptr;
            }
        Qt::WindowFlags flag = Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint ;
        flag &= ~ Qt::WindowContextHelpButtonHint;
        flag &= ~ Qt::WindowMinimizeButtonHint;
mTrapFlashDialog = new QDialog(mParentWidget, flag);
        mTrapFlashUi = new Ui::trapFlashDialog();
        mTrapFlashUi->setupUi(mTrapFlashDialog);

//        mTrapFlashUi->webView->setNetworkAccessManager(UBNetworkAccessManager::defaultAccessManager());
        int viewWidth = mParentWidget->width() / 2;
        int viewHeight = mParentWidget->height() * 2. / 3.;
        mTrapFlashDialog->setGeometry(
                (mParentWidget->width() - viewWidth) / 2
                , (mParentWidget->height() - viewHeight) / 2
                , viewWidth
                , viewHeight);

        connect(mTrapFlashUi->flashCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFlash(int)));
        connect(mTrapFlashUi->widgetNameLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(text_Changed(const QString &)));
        connect(mTrapFlashUi->widgetNameLineEdit, SIGNAL(textEdited(const QString &)), this, SLOT(text_Edited(const QString &)));
        connect(mTrapFlashUi->createWidgetButton, SIGNAL(clicked(bool)), this, SLOT(createWidget()));
        mTrapFlashDialog->show();
    }

//    mTrapFlashDialog->show();
}

void UBTrapFlashController::text_Changed(const QString &newText)
{
    QString new_text = newText;

#ifdef Q_OS_WIN // Defined on Windows.
    QString illegalCharList("      < > : \" / \\ | ? * ");
    QRegExp regExp("[<>:\"/\\\\|?*]");
#endif

#ifdef Q_OS_OSX // Defined on Mac OS X.
    QString illegalCharList("      < > : \" / \\ | ? * ");
    QRegExp regExp("[<>:\"/\\\\|?*]");
#endif

#ifdef Q_OS_LINUX // Defined on X11.
    QString illegalCharList("      < > : \" / \\ | ? * ");
    QRegExp regExp("[<>:\"/\\\\|?*]");
#endif

    if(new_text.indexOf(regExp) > -1)
    {
        new_text.remove(regExp);
        mTrapFlashUi->widgetNameLineEdit->setText(new_text);
        QToolTip::showText(mTrapFlashUi->widgetNameLineEdit->mapToGlobal(QPoint()), "Application name can`t contain any of the following characters:\r\n"+illegalCharList);
    }
}

void UBTrapFlashController::text_Edited(const QString &newText)
{
    Q_UNUSED(newText);
}

void UBTrapFlashController::hideTrapFlash()
{
    if (mTrapFlashDialog)
    {
        mTrapFlashDialog->hide();
    }
}
void UBTrapFlashController::addListOfFlashes(int count,UBWebKitUtils::HtmlObject& flash)
{
    if(count==0)
    {
         updateListOfFlashes(mAvailableFlashes);
    }else {
        mAvailableAddFlashes.append(flash);
        qDebug()<<count;
        qDebug()<<mAvailableFlashes.count();
        if(mAvailableFlashes.count()==count)
        {
            updateListOfFlashes(mAvailableFlashes);
        }
}

}

void UBTrapFlashController::updateListOfFlashes(const QList<UBWebKitUtils::HtmlObject>& pAllFlashes)
{
    if (mTrapFlashDialog)
    {
        mAvailableFlashes = pAllFlashes;
        disconnect(mTrapFlashUi->flashCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFlash(int)));
        mTrapFlashUi->flashCombobox->clear();
        mTrapFlashUi->flashCombobox->addItem(tr("Whole page"));

        foreach(UBWebKitUtils::HtmlObject wrapper, pAllFlashes)
        {
            mTrapFlashUi->flashCombobox->addItem(widgetNameForObject(wrapper));
        }

        connect(mTrapFlashUi->flashCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectFlash(int)));
        selectFlash(mTrapFlashUi->flashCombobox->currentIndex());
    }
}


void UBTrapFlashController::selectFlash(int pFlashIndex)
{
    if (pFlashIndex == 0)
    {
        mTrapFlashUi->webView->setHtml(generateFullPageHtml("", false));
//#if defined(QWEBENGINEPAGE_CURRENTWEBFRAME)
        mCurrentWebFrame->runJavaScript("window.document.title", [this](const QVariant &res) {
           mTrapFlashUi->widgetNameLineEdit->setText(res.toString().trimmed());

       });
//    #endif


    }
    else if (pFlashIndex > 0 && pFlashIndex <= mAvailableFlashes.size())
    {
        UBWebKitUtils::HtmlObject currentObject = mAvailableFlashes.at(pFlashIndex - 1);
        generateHtml(currentObject, "", false);
        mTrapFlashUi->widgetNameLineEdit->setText(widgetNameForObject(currentObject));
    }
}


void UBTrapFlashController::createWidget()
{
    int selectedIndex = mTrapFlashUi->flashCombobox->currentIndex();
qDebug()<<"selectedIndex"<<selectedIndex;
    if (selectedIndex == 0)
    {
        // full page widget
        QString tempDir = UBFileSystemUtils::createTempDir("TrapFlashRendering");
        QDir widgetDir(tempDir + "/" + mTrapFlashUi->widgetNameLineEdit->text() + ".wgt");

        if (widgetDir.exists() && !UBFileSystemUtils::deleteDir(widgetDir.path()))
        {
            qWarning() << "Cannot delete " << widgetDir.path();
        }

        widgetDir.mkpath(widgetDir.path());

        generateFullPageHtml(widgetDir.path(), true);

        generateIcon(widgetDir.path());
        generateConfig(800, 600, widgetDir.path());

        //generateDefaultPng(width, height, widgetDir.path());

        importWidgetInLibrary(widgetDir);

        UBFileSystemUtils::deleteDir(tempDir);
    }
    else if(selectedIndex>0)
    {
        // flash widget
        UBWebKitUtils::HtmlObject selectedObject = mAvailableFlashes.at(selectedIndex - 1);
        UBApplication::applicationController->showBoard();
        UBApplication::boardController->downloadURL(QUrl(selectedObject.source), QString(), QPoint(0, 0), QSize(selectedObject.width, selectedObject.height));
    }

    QString freezedWidgetPath = UBPlatformUtils::applicationResourcesDirectory() + "/etc/freezedWidgetWrapper.html";
    mTrapFlashUi->webView->load(QUrl::fromLocalFile(freezedWidgetPath));

    mTrapFlashDialog->hide();
}


void UBTrapFlashController::importWidgetInLibrary(QDir pSourceDir)
{
    const QString userWidgetPath = UBSettings::settings()->userInteractiveDirectory() + "/" + tr("Web");
    QDir userWidgetDir(userWidgetPath);

    if (!userWidgetDir.exists())
    {
        userWidgetDir.mkpath(userWidgetPath);
    }

    QString widgetLibraryPath = userWidgetPath + "/" + mTrapFlashUi->widgetNameLineEdit->text() + ".wgt";
    QDir widgetLibraryDir(widgetLibraryPath);

    if (widgetLibraryDir.exists())
    {
        if (!UBFileSystemUtils::deleteDir(widgetLibraryDir.path()))
        {
            qWarning() << "Cannot delete old widget " << widgetLibraryDir.path();
        }
    }

    qDebug() << "Widget imported in path " << widgetLibraryPath;
    UBFileSystemUtils::copyDir(pSourceDir.path(), widgetLibraryPath);

    // also add to current scene
    if (UBApplication::applicationController)
        UBApplication::applicationController->showBoard();

    if (UBApplication::boardController &&
        UBApplication::boardController->activeScene())
    {
        UBApplication::boardController->activeScene()->addWidget(QUrl::fromLocalFile(widgetLibraryPath));
    }
}
//#if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)

void UBTrapFlashController::updateTrapFlashFromPage(QWebEnginePage* pCurrentWebFrame)
{
    if (pCurrentWebFrame && mTrapFlashDialog && mTrapFlashDialog->isVisible())
    {
         UBWebKitUtils::objectsInFrame(pCurrentWebFrame);
//        QList<UBWebKitUtils::HtmlObject> list =
        mCurrentWebFrame = pCurrentWebFrame;
//        updateListOfFlashes(list);
    }
}
//#endif


QString UBTrapFlashController::generateIcon(const QString& pDirPath)
{
    QDesktopWidget* desktop = QApplication::desktop();
    QPoint webViewPosition = mTrapFlashUi->webView->mapToGlobal(mTrapFlashUi->webView->pos());
    QSize webViewSize = mTrapFlashUi->webView->size();
    QPixmap capture = QPixmap::grabWindow(desktop->winId(), webViewPosition.x(), webViewPosition.y()
            , webViewSize.width() - 10, webViewSize.height() -10);

    QPixmap widgetIcon(75,75);
    widgetIcon.fill(Qt::transparent);

    QPainter painter(&widgetIcon);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(QColor(180,180,180)));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, 75, 75, 10, 10);
    QPixmap icon = capture.scaled(65, 65);
    painter.drawPixmap(5,5,icon);

    QString iconFile = pDirPath + "/icon.png";
    widgetIcon.save(iconFile, "PNG");

    return iconFile;
}


void UBTrapFlashController::generateConfig(int pWidth, int pHeight, const QString& pDestinationPath)
{
    QFile configFile(pDestinationPath + "/" + "config.xml");

    if (configFile.exists())
    {
        configFile.remove(configFile.fileName());
    }

    if (!configFile.open(QIODevice::WriteOnly))
    {
        qWarning() << "Cannot open file " << configFile.fileName();
        return;
    }

    QTextStream out(&configFile);
    out.setCodec("UTF-8");
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << endl;
    out << "<widget xmlns=\"http://www.w3.org/ns/widgets\"" << endl;
    out << "        xmlns:ub=\"http://uniboard.mnemis.com/widgets\"" << endl;
    out << "        identifier=\"http://uniboard.mnemis.com/" << mTrapFlashUi->widgetNameLineEdit->text() << "\"" <<endl;

    out << "        version=\"1.0\"" << endl;
    out << "        width=\"" << pWidth << "\"" << endl;
    out << "        height=\"" << pHeight << "\"" << endl;
    out << "        ub:resizable=\"true\">" << endl;

    out << "    <name>" << mTrapFlashUi->widgetNameLineEdit->text() << "</name>" << endl;
    out << "    <content src=\"" << mTrapFlashUi->widgetNameLineEdit->text() << ".html\"/>" << endl;

    out << "</widget>" << endl;


    configFile.close();
}


QString UBTrapFlashController::generateFullPageHtml(const QString& pDirPath, bool pGenerateFile)
{

    QString htmlContentString;

    htmlContentString += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n";
    htmlContentString += "<html xmlns=\"http://www.w3.org/1999/xhtml\">\r\n";
    htmlContentString += "  <head>\r\n";
//    #if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)
    htmlContentString += "    <meta http-equiv=\"refresh\" content=\"0; " + mCurrentWebFrame->url().toString() + "\">\r\n";
//    #endif
    htmlContentString += "  </head>\r\n";
    htmlContentString += "  <body>\r\n";
    htmlContentString += "    Redirect to target...\r\n";
    htmlContentString += "  </body>\r\n";
    htmlContentString += "</html>\r\n";

    if (!pGenerateFile)
    {
        return htmlContentString;
    }
    else
    {
        QString fileName = mTrapFlashUi->widgetNameLineEdit->text();
        const QString fullHtmlFileName = pDirPath + "/" + fileName + ".html";
        QDir dir(pDirPath);
        if (!dir.exists())
        {
            dir.mkpath(dir.path());
        }
        QFile widgetHtmlFile(fullHtmlFileName);
        if (widgetHtmlFile.exists())
        {
            widgetHtmlFile.remove(widgetHtmlFile.fileName());
        }
        if (!widgetHtmlFile.open(QIODevice::WriteOnly))
        {
            qWarning() << "cannot open file " << widgetHtmlFile.fileName();
            return "";
        }
        QTextStream out(&widgetHtmlFile);
        out << htmlContentString;

        widgetHtmlFile.close();
        return widgetHtmlFile.fileName();
    }
}


void UBTrapFlashController::generateHtml(const UBWebKitUtils::HtmlObject& pObject,
        const QString& pDirPath, bool pGenerateFile)
{
    qDebug() << pObject.source;
    QUrl objectUrl(pObject.source);
    QString objectFullUrl = pObject.source;
    if (!objectUrl.isValid())
    {
        qWarning() << "invalid URL " << pObject.source;
        mTrapFlashUi->webView->setHtml("");
    }
    if (objectUrl.isRelative())
    {
//            #if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)
        int lastSlashIndex = mCurrentWebFrame->url().toString().lastIndexOf("/");
        QString objectPath = mCurrentWebFrame->url().toString().left(lastSlashIndex);
        objectFullUrl =   objectPath+ "/" + pObject.source;
//         #endif

    }

    QString htmlContentString;

    htmlContentString += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\n";
    htmlContentString += "<html>\n";
    htmlContentString += "<head>\n";
    htmlContentString += "    <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n";
    htmlContentString += "</head>\n";

    if (!pGenerateFile)
    {
        htmlContentString += "<body bgcolor=\"rgb(180,180,180)\">\n";
    }
    else
    {
        htmlContentString += "<body>\n";
    }

    htmlContentString += "        <div align='center'>\n";
    QString htmlContentStringpara=htmlContentString;
//#if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)
    if (mCurrentWebFrame->url().toString().contains("youtube"))
    {
        mCurrentWebFrame->runJavaScript("window.document.getElementById('embed_code').value", [this,htmlContentStringpara,pGenerateFile,pDirPath](const QVariant &res) {
            //force windowsless mode
    QString htmlContentString=htmlContentStringpara;
            htmlContentString += res.toString().replace("></embed>", " wmode='opaque'></embed>");
            htmlContentString += "        </div>\n";
            htmlContentString += "</body>\n";
            htmlContentString += "</html>\n";

            if (!pGenerateFile)
            {
                mTrapFlashUi->webView->setHtml(htmlContentString);
            }
            else
            {
                QString fileName = mTrapFlashUi->widgetNameLineEdit->text();
                const QString fullHtmlFileName = pDirPath + "/" + fileName + ".html";
                QDir dir(pDirPath);

                if (!dir.exists())
                {
                    dir.mkpath(dir.path());
                }

                QFile widgetHtmlFile(fullHtmlFileName);

                if (widgetHtmlFile.exists())
                {
                    widgetHtmlFile.remove(widgetHtmlFile.fileName());
                }

                if (!widgetHtmlFile.open(QIODevice::WriteOnly))
                {
                    qWarning() << "cannot open file " << widgetHtmlFile.fileName();
                    mTrapFlashUi->webView->setHtml("");
                }

                QTextStream out(&widgetHtmlFile);
                out << htmlContentString;

                widgetHtmlFile.close();
                 mTrapFlashUi->webView->setHtml(widgetHtmlFile.fileName());
            }
       });

    }
    else
    {
        htmlContentString += "            <object classid='clsid:D27CDB6E-AE6D-11cf-96B8-444553540000'\n";
        htmlContentString += "                codebase='http://download.macromedia.com/pub/shockwave/cabs/flash/swflash.cab#version=6,0,29,0'\n";
        htmlContentString += "                width='" + QString::number(pObject.width)  + "' height='" + QString::number(pObject.height) + "'>\n";
        htmlContentString += "                <param name='movie' value='" + objectFullUrl + "'>\n";
        htmlContentString += "                <param name='quality' value='high'>\n";
        htmlContentString += "                <param name='wmode' value='opaque'>\n";
        htmlContentString += "                <embed src='" + objectFullUrl + "'\n";
        htmlContentString += "                    quality='high'\n";
        htmlContentString += "                    pluginspage='http://www.macromedia.com/go/getflashplayer'\n";
        htmlContentString += "                    type='application/x-shockwave-flash'\n";
        htmlContentString += "                     width='" + QString::number(pObject.width - 20) + "' height='" + QString::number(pObject.height - 20) + "' wmode='opaque'>\n";
        htmlContentString += "                </embed>\n";
        htmlContentString += "            </object>\n";


        htmlContentString += "        </div>\n";
        htmlContentString += "</body>\n";
        htmlContentString += "</html>\n";

        if (!pGenerateFile)
        {
                mTrapFlashUi->webView->setHtml(htmlContentString);
        }
        else
        {
            QString fileName = mTrapFlashUi->widgetNameLineEdit->text();
            const QString fullHtmlFileName = pDirPath + "/" + fileName + ".html";
            QDir dir(pDirPath);

            if (!dir.exists())
            {
                dir.mkpath(dir.path());
            }

            QFile widgetHtmlFile(fullHtmlFileName);

            if (widgetHtmlFile.exists())
            {
                widgetHtmlFile.remove(widgetHtmlFile.fileName());
            }

            if (!widgetHtmlFile.open(QIODevice::WriteOnly))
            {
                qWarning() << "cannot open file " << widgetHtmlFile.fileName();
                mTrapFlashUi->webView->setHtml("");
            }

            QTextStream out(&widgetHtmlFile);
            out << htmlContentString;

            widgetHtmlFile.close();
             mTrapFlashUi->webView->setHtml( widgetHtmlFile.fileName());
        }
    }
// #endif

}

QString UBTrapFlashController::widgetNameForObject(UBWebKitUtils::HtmlObject pObject)
{
    QString url = pObject.source;
    int parametersIndex = url.indexOf("?");
    if(parametersIndex != -1)
        url = url.left(parametersIndex);
    int lastSlashIndex = url.lastIndexOf("/");

    QString result = url.right(url.length() - lastSlashIndex);
    result = UBFileSystemUtils::cleanName(result);

    return result;
}

