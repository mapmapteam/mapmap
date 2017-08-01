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

#ifndef FILEEDITFACTORY_H
#define FILEEDITFACTORY_H

#include "qtpropertybrowser.h"
#include "filepathmanager.h"

class FileEdit;

class FileEditFactory : public QtAbstractEditorFactory<FilePathManager>
{
    Q_OBJECT
public:
    FileEditFactory(QObject *parent = 0)
        : QtAbstractEditorFactory<FilePathManager>(parent)
            { }
    virtual ~FileEditFactory();
protected:
    virtual void connectPropertyManager(FilePathManager *manager);
    virtual QWidget *createEditor(FilePathManager *manager, QtProperty *property,
                QWidget *parent);
    virtual void disconnectPropertyManager(FilePathManager *manager);
private slots:
    void slotPropertyChanged(QtProperty *property, const QString &value);
    void slotFilterChanged(QtProperty *property, const QString &filter);
    void slotSetValue(const QString &value);
    void slotEditorDestroyed(QObject *object);
private:
    QMap<QtProperty *, QList<FileEdit *> > theCreatedEditors;
    QMap<FileEdit *, QtProperty *> theEditorToProperty;
};

#endif
