/***************************************************************************
 *                                                                         *
 *   copyright : (C) 2011 Joshua Netterfield                               *
 *                   joshua.netterfield@gmail.com                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LABELSCRIPTINTERFACE_H
#define LABELSCRIPTINTERFACE_H

#include <QByteArray>
#include <QString>
#include <QList>
#include <QObject>
#include "viewitemscriptinterface.h"
typedef QList<QByteArray> QByteArrayList;

namespace Kst {

class LabelTabSI;
class LabelItem;

class LabelSI : public ScriptInterface
{
    Q_OBJECT
public:
    LayoutTabSI* layout;
    DimensionTabSI* dim;
    LabelTabSI*lab;
    LabelSI(LabelItem* it);
    QByteArrayList commands();
    QString doCommand(QString);
    bool isValid();
    QByteArray getHandle();
};


}

#endif // LABELSCRIPTINTERFACE_H
