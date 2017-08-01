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

#ifndef VARIANTMANAGER_H
#define VARIANTMANAGER_H

#include "qtvariantproperty.h"


class VariantManager : public QtVariantPropertyManager
{
    Q_OBJECT
public:
    VariantManager(QObject *parent = 0)
        : QtVariantPropertyManager(parent)
            { }

    virtual QVariant value(const QtProperty *property) const;
    virtual int valueType(int propertyType) const;
    virtual bool isPropertyTypeSupported(int propertyType) const;

    virtual QStringList attributes(int propertyType) const;
    virtual int attributeType(int propertyType, const QString &attribute) const;
    virtual QVariant attributeValue(const QtProperty *property, const QString &attribute) const;


    static int filePathTypeId();
public slots:
    virtual void setValue(QtProperty *property, const QVariant &val);
    virtual void setAttribute(QtProperty *property,
                const QString &attribute, const QVariant &value);
protected:
    virtual QString valueText(const QtProperty *property) const;
    virtual void initializeProperty(QtProperty *property);
    virtual void uninitializeProperty(QtProperty *property);
private:
    struct Data {
        QString value;
        QString filter;
    };
    QMap<const QtProperty *, Data> theValues;
};



#endif
