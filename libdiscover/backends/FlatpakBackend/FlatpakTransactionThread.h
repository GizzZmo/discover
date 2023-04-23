/*
 *   SPDX-FileCopyrightText: 2017 Jan Grulich <jgrulich@redhat.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "flatpak-helper.h"
#include <gio/gio.h>
#include <glib.h>

#include <QMap>
#include <QStringList>
#include <QThread>
#include <Transaction/Transaction.h>

class FlatpakResource;
class FlatpakTransactionThread : public QThread
{
    Q_OBJECT
public:
    FlatpakTransactionThread(FlatpakInstallation *installation);
    ~FlatpakTransactionThread() override;

    void add(FlatpakResource *app, Transaction::Role role);
    void cancel();
    void run() override;

    int progress() const
    {
        return m_progress;
    }
    void setProgress(int progress);
    void setSpeed(quint64 speed);

    QString errorMessage() const;
    bool result() const;
    bool cancelled() const;

    void addErrorMessage(const QString &error);
    QMap<QString, QStringList> addedRepositories() const
    {
        return m_addedRepositories;
    }

    void setCurrentRef(const QByteArray &ref)
    {
        m_currentRef = ref;
    }

Q_SIGNALS:
    void progressChanged(const QByteArray &currentRef, int progress);
    void speedChanged(const QByteArray &currentRef, quint64 speed);
    void passiveMessage(const QByteArray &currentRef, const QString &msg);
    void webflowStarted(const QByteArray &currentRef, const QUrl &url, int id);
    void webflowDone(const QByteArray &currentRef, int id);

private:
    static gboolean
    add_new_remote_cb(FlatpakTransaction * /*object*/, gint /*reason*/, gchar *from_id, gchar *suggested_remote_name, gchar *url, gpointer user_data);
    static void progress_changed_cb(FlatpakTransactionProgress *progress, gpointer user_data);
    static void
    new_operation_cb(FlatpakTransaction * /*object*/, FlatpakTransactionOperation *operation, FlatpakTransactionProgress *progress, gpointer user_data);

    static gboolean webflowStart(FlatpakTransaction *transaction, const char *remote, const char *url, GVariant *options, guint id, gpointer user_data);
    static void webflowDoneCallback(FlatpakTransaction *transaction, GVariant *options, guint id, gpointer user_data);

    QByteArray m_currentRef;
    FlatpakTransaction *m_transaction;
    bool m_result = false;
    int m_progress = 0;
    quint64 m_speed = 0;
    QString m_errorMessage;
    GCancellable *m_cancellable;

    QMap<QString, QStringList> m_addedRepositories;
    QVector<int> m_webflows;
};
