/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QTest>

#include <cstring>

#include "v4l2devices.h"

class V4l2DevicesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void readsTheNodeCapabilitiesWhenThereAreAny()
    {
        // the whole device can capture, this node only carries metadata
        const quint32 device = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_META_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_DEVICE_CAPS;
        QVERIFY(!V4l2Devices::isVideoCapture(device, V4L2_CAP_META_CAPTURE | V4L2_CAP_STREAMING));
        QVERIFY(V4l2Devices::isVideoCapture(device, V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING));
    }

    void fallsBackOnTheDeviceCapabilities()
    {
        // old drivers do not fill device_caps
        QVERIFY(V4l2Devices::isVideoCapture(V4L2_CAP_VIDEO_CAPTURE, 0));
        QVERIFY(!V4l2Devices::isVideoCapture(V4L2_CAP_VIDEO_OUTPUT, 0));
    }

    void skipsOutputOnlyNodes()
    {
        // v4l2loopback with exclusive_caps=1, before anything writes to it
        QVERIFY(!V4l2Devices::isVideoCapture(V4L2_CAP_VIDEO_OUTPUT | V4L2_CAP_DEVICE_CAPS, V4L2_CAP_VIDEO_OUTPUT));
        QVERIFY(!V4l2Devices::isVideoCapture(V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_DEVICE_CAPS, V4L2_CAP_VIDEO_CAPTURE_MPLANE));
    }

    void readsTheCardName()
    {
        __u8 card[32] = {};
        std::memcpy(card, "FHD Webcam: IR Camera", 21);
        QCOMPARE(V4l2Devices::cardName(card), QStringLiteral("FHD Webcam: IR Camera"));
    }

    void readsACardNameWithoutTerminator()
    {
        __u8 card[32];
        std::memset(card, 'a', sizeof card);
        QCOMPARE(V4l2Devices::cardName(card), QString(32, QLatin1Char('a')));
    }
};

QTEST_GUILESS_MAIN(V4l2DevicesTest)

#include "v4l2devices_test.moc"
