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

#include "fileeditfactory.h"
#include "fileedit.h"

FileEditFactory::~FileEditFactory()
{
    QList<FileEdit *> editors = theEditorToProperty.keys();
    QListIterator<FileEdit *> it(editors);
    while (it.hasNext())
        delete it.next();
}

void FileEditFactory::connectPropertyManager(FilePathManager *manager)
{
    connect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    connect(manager, SIGNAL(filterChanged(QtProperty *, const QString &)),
                this, SLOT(slotFilterChanged(QtProperty *, const QString &)));
}

QWidget *FileEditFactory::createEditor(FilePathManager *manager,
        QtProperty *property, QWidget *parent)
{
    FileEdit *editor = new FileEdit(parent);
    editor->setFilePath(manager->value(property));
    editor->setFilter(manager->filter(property));
    theCreatedEditors[property].append(editor);
    theEditorToProperty[editor] = property;

    connect(editor, SIGNAL(filePathChanged(const QString &)),
                this, SLOT(slotSetValue(const QString &)));
    connect(editor, SIGNAL(destroyed(QObject *)),
                this, SLOT(slotEditorDestroyed(QObject *)));
    return editor;
}

void FileEditFactory::disconnectPropertyManager(FilePathManager *manager)
{
    disconnect(manager, SIGNAL(valueChanged(QtProperty *, const QString &)),
                this, SLOT(slotPropertyChanged(QtProperty *, const QString &)));
    disconnect(manager, SIGNAL(filterChanged(QtProperty *, const QString &)),
                this, SLOT(slotFilterChanged(QtProperty *, const QString &)));
}

void FileEditFactory::slotPropertyChanged(QtProperty *property,
                const QString &value)
{
    if (!theCreatedEditors.contains(property))
        return;

    QList<FileEdit *> editors = theCreatedEditors[property];
    QListIterator<FileEdit *> itEditor(editors);
    while (itEditor.hasNext())
        itEditor.next()->setFilePath(value);
}

void FileEditFactory::slotFilterChanged(QtProperty *property,
            const QString &filter)
{
    if (!theCreatedEditors.contains(property))
        return;

    QList<FileEdit *> editors = theCreatedEditors[property];
    QListIterator<FileEdit *> itEditor(editors);
    while (itEditor.hasNext())
        itEditor.next()->setFilter(filter);
}

void FileEditFactory::slotSetValue(const QString &value)
{
    QObject *object = sender();
    QMap<FileEdit *, QtProperty *>::ConstIterator itEditor =
                theEditorToProperty.constBegin();
    while (itEditor != theEditorToProperty.constEnd()) {
        if (itEditor.key() == object) {
            QtProperty *property = itEditor.value();
            FilePathManager *manager = propertyManager(property);
            if (!manager)
                return;
            manager->setValue(property, value);
            return;
        }
        itEditor++;
    }
}

void FileEditFactory::slotEditorDestroyed(QObject *object)
{
    QMap<FileEdit *, QtProperty *>::ConstIterator itEditor =
                theEditorToProperty.constBegin();
    while (itEditor != theEditorToProperty.constEnd()) {
        if (itEditor.key() == object) {
            FileEdit *editor = itEditor.key();
            QtProperty *property = itEditor.value();
            theEditorToProperty.remove(editor);
            theCreatedEditors[property].removeAll(editor);
            if (theCreatedEditors[property].isEmpty())
                theCreatedEditors.remove(property);
            return;
        }
        itEditor++;
    }
}

