/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "Category.h"
#include <QAbstractListModel>
#include <QQmlEngine>
#include <QQmlParserStatus>

#include "discovercommon_export.h"

class QTimer;

class DISCOVERCOMMON_EXPORT CategoryModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QVariantList rootCategories READ rootCategoriesVL NOTIFY rootCategoriesChanged)
public:
    static CategoryModel *global();

    Q_SCRIPTABLE Category *findCategoryByName(const QString &name) const;
    void blacklistPlugin(const QString &name);
    QVector<Category *> rootCategories() const
    {
        return m_rootCategories;
    }
    QVariantList rootCategoriesVL() const;
    void populateCategories();

    static CategoryModel *create(QQmlEngine *, QJSEngine *)
    {
        CategoryModel *result = global();
        QQmlEngine::setObjectOwnership(result, QQmlEngine::CppOwnership);
        return result;
    }

Q_SIGNALS:
    void rootCategoriesChanged();

private:
    explicit CategoryModel(QObject *parent = nullptr);

    QTimer *m_rootCategoriesChanged;
    QVector<Category *> m_rootCategories;
};
