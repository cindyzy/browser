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


#include <QDebug>
//#include <QWebEnginePage>

#include "UBWebKitUtils.h"

#include "core/memcheck.h"
#include "UBTrapFlashController.h"
UBWebKitUtils::UBWebKitUtils()
{
    // NOOP
}

UBWebKitUtils::~UBWebKitUtils()
{
    // NOOP
}

void UBWebKitUtils::objectsInFrame(QWebEnginePage* frame)
{
//    QList<UBWebKitUtils::HtmlObject> htmlObjects;
//QList<UBWebKitUtils::HtmlObject> htmlObjectspara=htmlObjects;
    if (frame)
    {
         frame->runJavaScript("window.document.getElementsByTagName('embed').length",[frame](const QVariant &res)
        {
             bool ok;

             int count = res.toInt(&ok);
             qDebug()<<"count"<<count;
             if (ok)
             {
                 if(count==0)
                 {
                     UBWebKitUtils::HtmlObject test=UBWebKitUtils::HtmlObject(NULL, NULL, NULL);;
                     UBTrapFlashController::instance()->addListOfFlashes(0,test);

                 }
                 for (int i = 0; i < count; i++)
                 {
                     QString queryWidth = QString("window.document.getElementsByTagName('embed')[%1].width").arg(i);





                     frame->runJavaScript(queryWidth,[frame,i,count](const QVariant &res)
                     {
                         bool ok;
                         int width = res.toInt(&ok);
                         if (width == 0 || !ok)
                         {
                             qDebug() << "Width is not defined in pixel. 640 will be used";
                             width = 640;
                         }
                         QString queryHeigth = QString("window.document.getElementsByTagName('embed')[%1].height").arg(i);
                         frame->runJavaScript(queryHeigth,[frame,i,width,count](const QVariant &res)
                         {
                             bool ok;
                             int heigth = res.toInt(&ok);
                             if (heigth == 0 || !ok)
                             {
                                 qDebug() << "Height is not defined in pixel. 480 will be used";
                                 heigth = 480;
                             }
                              QString querySource = QString("window.document.getElementsByTagName('embed')[%1].src").arg(i);

                              frame->runJavaScript(querySource,[frame,i,width,heigth,count](const QVariant &res){
                                  QUrl baseUrl = frame->url();
                                  QUrl relativeUrl = QUrl(res.toString());

                                  QString source = baseUrl.resolved(relativeUrl).toString();

                                  if (!source.trimmed().length() == 0)
                                  {
                                      UBWebKitUtils::HtmlObject test=UBWebKitUtils::HtmlObject(source, width, heigth);
                                           UBTrapFlashController::instance()->addListOfFlashes(count,test);
                                  }
//                                      continue;
//                     QList<UBWebKitUtils::HtmlObject> htmlObjects;
//                                  htmlObjects << UBWebKitUtils::HtmlObject(source, width, heigth);

//                                  if(htmlObjects.count()==count)
//                                  {
//                                        QList<UBWebKitUtils::HtmlObject> final_htmlObjects=htmlObjects;

//                                  }


                              });

                         });

                     });








                                     }
             }

        });

           }

//    return htmlObjects;
}


