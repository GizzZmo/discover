/*
 *   SPDX-FileCopyrightText: 2011 Jonathan Thomas <echidnaman@kubuntu.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef UPDATEITEM_H
#define UPDATEITEM_H

// Qt includes
#include "discovercommon_export.h"
#include "resources/AbstractBackendUpdater.h"
#include <QSet>
#include <QString>

#include <QIcon>

class AbstractResource;
class DISCOVERCOMMON_EXPORT UpdateItem
{
public:
    explicit UpdateItem(AbstractResource *app);

    ~UpdateItem();

    void setProgress(qreal progress);
    qreal progress() const;

    AbstractBackendUpdater::State state() const
    {
        return m_state;
    }
    void setState(AbstractBackendUpdater::State state)
    {
        m_state = state;
    }

    QString changelog() const;
    void setChangelog(const QString &changelog);

    AbstractResource *app() const;
    QString name() const;
    QVariant icon() const;
    qint64 size() const;
    Qt::CheckState checked() const;

    void setExtended(bool extended)
    {
        m_isExtended = extended;
    }
    AbstractResource *resource() const
    {
        return m_app;
    }
    bool isVisible() const
    {
        return m_visible;
    }
    bool isExtended() const
    {
        return m_isExtended;
    }
    void setVisible(bool visible)
    {
        m_visible = visible;
    }

private:
    AbstractResource *const m_app;

    const QString m_categoryName;
    const QIcon m_categoryIcon;
    qreal m_progress = 0.;
    bool m_visible = true;
    AbstractBackendUpdater::State m_state = AbstractBackendUpdater::None;
    QString m_changelog;
    bool m_isExtended = false;
};

#endif // UPDATEITEM_H
