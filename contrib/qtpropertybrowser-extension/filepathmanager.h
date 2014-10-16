/****************************************************************************
**
** Copyright (C) 2006 Trolltech ASA. All rights reserved.
**
** This file is part of the documentation of Qt. It was originally
** published as part of Qt Quarterly.
**
** This file may be used under the terms of the GNU General Public License
** version 2.0 as published by the Free Software Foundation or under the
** terms of the Qt Commercial License Agreement. The respective license
** texts for these are provided with the open source and commercial
** editions of Qt.
**
** If you are unsure which license is appropriate for your use, please
** review the following information:
** http://www.trolltech.com/products/qt/licensing.html or contact the
** sales department at sales@trolltech.com.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef FILEPATHMANAGER_H
#define FILEPATHMANAGER_H

#include "qtpropertybrowser.h"
#include <QMap>

class FilePathManager : public QtAbstractPropertyManager
{
    Q_OBJECT
public:
    FilePathManager(QObject *parent = 0)
        : QtAbstractPropertyManager(parent)
            { }

    QString value(const QtProperty *property) const;
    QString filter(const QtProperty *property) const;

public slots:
    void setValue(QtProperty *property, const QString &val);
    void setFilter(QtProperty *property, const QString &fil);
signals:
    void valueChanged(QtProperty *property, const QString &val);
    void filterChanged(QtProperty *property, const QString &fil);
protected:
    virtual QString valueText(const QtProperty *property) const { return value(property); }
    virtual void initializeProperty(QtProperty *property) { theValues[property] = Data(); }
    virtual void uninitializeProperty(QtProperty *property) { theValues.remove(property); }
private:

    struct Data
    {
        QString value;
        QString filter;
    };

    QMap<const QtProperty *, Data> theValues;
};


#endif
