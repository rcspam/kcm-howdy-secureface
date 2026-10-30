/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include <QString>

#include <linux/videodev2.h>

// Reading of VIDIOC_QUERYCAP answers, to list the cameras without
// QMediaDevices: it builds its list once and only updates it when a /dev node
// appears or goes away, so a node that becomes a camera later, such as a
// v4l2loopback device fed by a phone, would never be listed.
namespace V4l2Devices
{

// The node is a single-planar video capture device, the kind Qt lists and
// OpenCV opens. device_caps describes the node itself, when the driver fills it.
inline bool isVideoCapture(quint32 capabilities, quint32 deviceCaps)
{
    const quint32 caps = (capabilities & V4L2_CAP_DEVICE_CAPS) ? deviceCaps : capabilities;
    return caps & V4L2_CAP_VIDEO_CAPTURE;
}

// The card field of v4l2_capability, which is not terminated when it is full
template<size_t N>
inline QString cardName(const __u8 (&card)[N])
{
    const char *text = reinterpret_cast<const char *>(card);
    return QString::fromUtf8(text, qstrnlen(text, N));
}

}
