/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "devicelinks.h"

// A fake /dev: video0 and video2 nodes, and a by-path directory of links to them
class DeviceLinksTest : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dev;
    QString m_byPath;

    QString node(const QString &name) const
    {
        return m_dev.filePath(name);
    }

    void link(const QString &target, const QString &name) const
    {
        QVERIFY(QFile::link(QStringLiteral("../../") + target, m_byPath + QLatin1Char('/') + name));
    }

private Q_SLOTS:
    void initTestCase()
    {
        QVERIFY(m_dev.isValid());
        for (const char *name : {"video0", "video2"}) {
            QFile file(node(QLatin1String(name)));
            QVERIFY(file.open(QIODevice::WriteOnly));
        }
        QVERIFY(QDir(m_dev.path()).mkpath(QStringLiteral("v4l/by-path")));
        m_byPath = m_dev.filePath(QStringLiteral("v4l/by-path"));
        link(QStringLiteral("video2"), QStringLiteral("pci-0000:00:14.0-usbv2-0:6:1.2-video-index0"));
        link(QStringLiteral("video2"), QStringLiteral("pci-0000:00:14.0-usb-0:6:1.2-video-index0"));
        link(QStringLiteral("video0"), QStringLiteral("pci-0000:00:14.0-usb-0:6:1.0-video-index0"));
    }

    void findsTheFirstLinkInNameOrder()
    {
        QCOMPARE(DeviceLinks::stableLink(node(QStringLiteral("video2")), m_byPath),
                 m_byPath + QStringLiteral("/pci-0000:00:14.0-usb-0:6:1.2-video-index0"));
        QCOMPARE(DeviceLinks::stableLink(node(QStringLiteral("video0")), m_byPath),
                 m_byPath + QStringLiteral("/pci-0000:00:14.0-usb-0:6:1.0-video-index0"));
    }

    void noLinkGivesNothing()
    {
        QFile file(node(QStringLiteral("video4")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(DeviceLinks::stableLink(node(QStringLiteral("video4")), m_byPath).isEmpty());
        QVERIFY(DeviceLinks::stableLink(node(QStringLiteral("video2")), m_dev.filePath(QStringLiteral("missing"))).isEmpty());
    }

    void comparesTheNodesLinksLeadTo()
    {
        const QString usb = m_byPath + QStringLiteral("/pci-0000:00:14.0-usb-0:6:1.2-video-index0");
        const QString usbv2 = m_byPath + QStringLiteral("/pci-0000:00:14.0-usbv2-0:6:1.2-video-index0");
        QVERIFY(DeviceLinks::sameDevice(usb, node(QStringLiteral("video2"))));
        QVERIFY(DeviceLinks::sameDevice(usb, usbv2));
        QVERIFY(DeviceLinks::sameDevice(node(QStringLiteral("video2")), node(QStringLiteral("video2"))));
        QVERIFY(!DeviceLinks::sameDevice(usb, node(QStringLiteral("video0"))));
    }

    void missingPathsNeverMatch()
    {
        QVERIFY(!DeviceLinks::sameDevice(QString(), QString()));
        QVERIFY(!DeviceLinks::sameDevice(node(QStringLiteral("video9")), node(QStringLiteral("video9"))));
        QVERIFY(!DeviceLinks::sameDevice(QStringLiteral("none"), node(QStringLiteral("video2"))));
    }
};

QTEST_GUILESS_MAIN(DeviceLinksTest)

#include "devicelinks_test.moc"
