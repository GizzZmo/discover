/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "RpmOstreeResource.h"
#include "RpmOstreeBackend.h"

#include <appstream/AppStreamIntegration.h>

#include <KLocalizedString>
#include <KOSRelease>

#include <ostree-repo.h>
#include <ostree.h>

const QStringList RpmOstreeResource::m_objects({QStringLiteral("qrc:/qml/RemoteRefsButton.qml")});

RpmOstreeResource::RpmOstreeResource(const QVariantMap &map, RpmOstreeBackend *parent)
    : AbstractResource(parent)
    , m_state(AbstractResource::None)
{
#ifdef QT_DEBUG
    qDebug() << "rpm-ostree-backend: Creating deployments from:";
    QMapIterator<QString, QVariant> iter(map);
    while (iter.hasNext()) {
        iter.next();
        qDebug() << "rpm-ostree-backend: " << iter.key() << ": " << iter.value();
    }
    qDebug() << "";
#endif

    // All available deployments are by definition already installed.
    m_state = AbstractResource::Installed;

    // Get as much as possible from rpm-ostree
    m_osname = map.value(QStringLiteral("osname")).toString();
    m_version = map.value(QStringLiteral("base-version")).toString();
    m_timestamp = QDateTime::fromSecsSinceEpoch(map.value(QStringLiteral("base-timestamp")).toULongLong()).date();

    // Consider all deployments as pinned (and thus non-removable) until we
    // support for un-pinning and removing selected deployments.
    // TODO: Support for pinning, un-pinning and removing deployments
    // m_pined = map.value(QStringLiteral("pinned")).toBool();
    m_pinned = true;

    m_booted = map.value(QStringLiteral("booted")).toBool();

    if (m_booted) {
        // We can directly read the pretty name & variant from os-release
        // information if this is the currently booted deployment.
        auto osrelease = AppStreamIntegration::global()->osRelease();
        m_prettyname = osrelease->prettyName();
        m_name = osrelease->name();
        m_variant = osrelease->variant();
    }

    // Split remote and branch from origin
    m_origin = map.value(QStringLiteral("origin")).toString();
    auto split_ref = m_origin.split(':');
    if (split_ref.length() != 2) {
        qWarning() << "rpm-ostree-backend: Unknown origin format, ignoring:" << m_origin;
        m_remote = QStringLiteral("unkonwn");
        m_branch = QStringLiteral("unkonwn");
    } else {
        m_remote = split_ref[0];
        m_branch = split_ref[1];
    }

    // Split branch into name / version / arch / variant
    auto split_branch = m_branch.split('/');
    if (split_branch.length() < 4) {
        qWarning() << "rpm-ostree-backend: Unknown branch format, ignoring:" << m_branch;
        m_branchName = QStringLiteral("unkonwn");
        m_branchVersion = QStringLiteral("unkonwn");
        m_branchArch = QStringLiteral("unkonwn");
        m_branchVariant = QStringLiteral("unkonwn");
    } else {
        m_branchName = split_branch[0];
        m_branchVersion = split_branch[1];
        m_branchArch = split_branch[2];
        auto variant = split_branch[3];
        for (int i = 4; i < split_branch.size(); ++i) {
            variant += "/" + split_branch[i];
        }
        m_branchVariant = variant;
    }

    // Use ostree as tld to differentiate those resources and append the current branch:
    // Example: ostree.fedora-34-x86-64-kinoite
    // https://freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-id-generic
    m_appstreamid = m_branch;
    m_appstreamid = QStringLiteral("ostree.") + m_appstreamid.replace('/', '-').replace('_', '-');

    // TODO: Extract signature information
    // auto signatures = map.value(QStringLiteral("signatures")).value<QDBusArgument>();
    // signatures.beginArray();
    // while (!signatures.atEnd()) {
    //     QList<QStringList> l;
    //     signatures >> l;
    //     qInfo() << l;
    // }
    // signatures.endArray();

    // TODO: Extract the list of layered packages

    connect(this, &RpmOstreeResource::buttonPressed, parent, &RpmOstreeBackend::rebaseToNewVersion);
}

void RpmOstreeResource::fetchRemoteRefs()
{
    g_autoptr(GFile) path = g_file_new_for_path("/ostree/repo");
    g_autoptr(OstreeRepo) repo = ostree_repo_new(path);
    if (repo == NULL) {
        qWarning() << "rpm-ostree-backend: Could not find ostree repo:" << path;
        return;
    }

    g_autoptr(GError) err = NULL;
    gboolean res = ostree_repo_open(repo, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not open ostree repo:" << path;
        return;
    }

    g_autoptr(GHashTable) refs;
    QByteArray rem = m_remote.toLocal8Bit();
    res = ostree_repo_remote_list_refs(repo, rem.data(), &refs, NULL, &err);
    if (!res) {
        qWarning() << "rpm-ostree-backend: Could not get the list of refs for ostree repo:" << path;
        return;
    }

    // Clear out existing refs
    m_remoteRefs.clear();

    int currentVersion = m_branchVersion.toInt();

    // Iterate over the remote refs to keep only the branches matching our
    // variant and to find if there is a newer version available.
    // TODO: Implement new release detection as currently this will offer to
    // rebase to a newer version as soon as it is branched in the Fedora
    // release process which is well before the official release.
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, refs);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        auto ref = QString((char *)key);
        // Split branch into name / version / arch / variant
        auto split_branch = ref.split('/');
        if (split_branch.length() < 4) {
            qWarning() << "rpm-ostree-backend: Unknown branch format, ignoring:" << ref;
            continue;
        } else {
            auto refVariant = split_branch[3];
            for (int i = 4; i < split_branch.size(); ++i) {
                refVariant += "/" + split_branch[i];
            }
            if (split_branch[0] == m_branchName && split_branch[2] == m_branchArch && refVariant == m_branchVariant) {
                // Add to the list of available refs
                m_remoteRefs.push_back(ref);
                // Look for the branch with the next version
                // This will fail to parse "rawhide" and return 0 and will thus skip it
                int version = split_branch[1].toInt();
                if (version == currentVersion + 1) {
                    m_nextMajorVersion = split_branch[1];
                }
            }
        }
    }

    if (m_remoteRefs.size() == 0) {
        qWarning() << "rpm-ostree-backend: Could not find any corresponding remote ref in ostree repo:" << path;
    }
}

QString RpmOstreeResource::getNextMajorVersion()
{
    return m_nextMajorVersion;
}

bool RpmOstreeResource::isNextMajorVersionAvailable()
{
    return m_nextMajorVersion != "";
}

QString RpmOstreeResource::availableVersion() const
{
    return m_newVersion;
}

void RpmOstreeResource::setNewVersion(QString newVersion)
{
    m_newVersion = newVersion;
}

QString RpmOstreeResource::appstreamId() const
{
    return m_appstreamid;
}

bool RpmOstreeResource::canExecute() const
{
    return false;
}

QVariant RpmOstreeResource::icon() const
{
    return QStringLiteral("application-x-rpm");
}

QString RpmOstreeResource::installedVersion() const
{
    return m_version;
}

QUrl RpmOstreeResource::url() const
{
    return QUrl();
}

QUrl RpmOstreeResource::donationURL()
{
    return QUrl();
}

QUrl RpmOstreeResource::homepage()
{
    return QUrl("https://spins.fedoraproject.org/en/kde/");
}

QUrl RpmOstreeResource::helpURL()
{
    return QUrl("https://docs.fedoraproject.org/en-US/fedora-kinoite/");
}

QUrl RpmOstreeResource::bugURL()
{
    return QUrl("https://pagure.io/fedora-kde/SIG/issues");
}

QJsonArray RpmOstreeResource::licenses()
{
    return {QJsonObject{{QStringLiteral("name"), i18n("GPL and other licenses")},
                        {QStringLiteral("url"), QStringLiteral("https://fedoraproject.org/wiki/Legal:Licenses")}}};
}

QString RpmOstreeResource::longDescription()
{
    return i18n("Remote: ") + m_origin;
}

QString RpmOstreeResource::name() const
{
    // If we are the currently booted deployment then we have a pretty name
    if (m_prettyname != "") {
        return m_prettyname;
    }
    // Otherwise construct one from what we have
    // TODO: Remove hardcoded values
    QString name;
    QTextStream(&name) << "Fedora Linux " << m_version << " (Kinoite)";
    return name;
}

QString RpmOstreeResource::origin() const
{
    return QStringLiteral("rpm-ostree");
}

QString RpmOstreeResource::packageName() const
{
    // TODO: Remove hardcoded values
    return QStringLiteral("Fedora Kinoite");
}

QString RpmOstreeResource::section()
{
    return {};
}

AbstractResource::State RpmOstreeResource::state()
{
    return m_state;
}

QString RpmOstreeResource::author() const
{
    return {};
}

QString RpmOstreeResource::comment()
{
    if (m_booted) {
        return i18n("The currently running version of %1.", packageName());
    }
    return i18n("Installed but not currently running version of %1.", packageName());
}

quint64 RpmOstreeResource::size()
{
    return 0;
}

QDate RpmOstreeResource::releaseDate() const
{
    return m_timestamp;
}

QString RpmOstreeResource::executeLabel() const
{
    return {};
}

void RpmOstreeResource::setState(AbstractResource::State state)
{
    m_state = state;
    Q_EMIT stateChanged();
}

void RpmOstreeResource::rebaseToNewVersion()
{
    QString ref;
    QTextStream(&ref) << m_branchName << '/' << m_nextMajorVersion << '/' << m_branchArch << '/' << m_variant;
    Q_EMIT buttonPressed(ref);
}

QString RpmOstreeResource::sourceIcon() const
{
    return QStringLiteral("application-x-rpm");
}

QStringList RpmOstreeResource::extends() const
{
    return {};
}
AbstractResource::Type RpmOstreeResource::type() const
{
    return Technical;
}

bool RpmOstreeResource::isRemovable() const
{
    return !m_booted && !m_pinned;
}

QList<PackageState> RpmOstreeResource::addonsInformation()
{
    return QList<PackageState>();
}

QStringList RpmOstreeResource::categories()
{
    return {};
}

bool RpmOstreeResource::isBooted()
{
    return m_booted;
}
