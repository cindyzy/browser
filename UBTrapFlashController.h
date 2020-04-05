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




#ifndef UBTRAPFLASHCONTROLLER_H_
#define UBTRAPFLASHCONTROLLER_H_

#include <QtGui>
#include "UBWebKitUtils.h"
#include <QWebEnginePage>
namespace Ui
{
    class trapFlashDialog;
}


class UBTrapFlashController : public QObject
{
    Q_OBJECT;
    public:
        UBTrapFlashController(QWidget* parent = 0);
        virtual ~UBTrapFlashController();

        void showTrapFlash();
        void hideTrapFlash();
        static UBTrapFlashController* s_instance;

        static UBTrapFlashController* instance(){
            return s_instance;
        }
    public slots:
//        #if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)
        void updateTrapFlashFromPage(QWebEnginePage* pCurrentWebFrame);
//        #endif
        void text_Changed(const QString &);
        void text_Edited(const QString &);
  void updateListOfFlashes(const QList<UBWebKitUtils::HtmlObject>& pAllFlashes);
 void addListOfFlashes(int count,UBWebKitUtils::HtmlObject& flash);
    private slots:
        void selectFlash(int pFlashIndex);
        void createWidget();

    private:



        QString widgetNameForObject(UBWebKitUtils::HtmlObject pObject);

        QString generateFullPageHtml(const QString& pDirPath, bool pGenerateFile);
        void generateHtml(const UBWebKitUtils::HtmlObject& pObject, const QString& pDirPath, bool pGenerateFile);

        QString generateIcon(const QString& pDirPath);

        void generateConfig(int pWidth, int pHeight, const QString& pDestinationPath);

        void importWidgetInLibrary(QDir pSourceDir);

        Ui::trapFlashDialog* mTrapFlashUi;
        QDialog* mTrapFlashDialog;
        QWidget* mParentWidget;
//#if defined(QWEBENGINEPAGE_UPDATETRAPFLASHFROMPAGE)
     QWebEnginePage* mCurrentWebFrame;
//#endif

        QList<UBWebKitUtils::HtmlObject> mAvailableFlashes;

        //add by zy
        QList<UBWebKitUtils::HtmlObject> mAvailableAddFlashes;
};


#endif /* UBTRAPFLASHCONTROLLER_H_ */
