/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "cammodel.h"

#include <QCameraInfo>
#include <QFile>

#include <KLocalizedString>

#include <fcntl.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>

uint32_t camera_pixel_format(int fd)
{
    v4l2_capability vcap{};
    if (ioctl(fd, VIDIOC_QUERYCAP, &vcap)) {
        qDebug("%s: failed to request v4l2_capability", __func__);
        return 0;
    }

    unsigned capabilities = vcap.capabilities;
    if (capabilities & V4L2_CAP_DEVICE_CAPS) {
        capabilities = vcap.device_caps;
    }

    unsigned priv_magic = (capabilities & V4L2_CAP_EXT_PIX_FORMAT)
        ? V4L2_PIX_FMT_PRIV_MAGIC
        : 0;

    bool is_multiplanar = capabilities & (V4L2_CAP_VIDEO_CAPTURE_MPLANE |
                                          V4L2_CAP_VIDEO_M2M_MPLANE |
                                          V4L2_CAP_VIDEO_OUTPUT_MPLANE);

    uint32_t vidcap_buftype = is_multiplanar
        ? V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE
        : V4L2_BUF_TYPE_VIDEO_CAPTURE;

    v4l2_format vfmt{};
    vfmt.fmt.pix.priv = priv_magic;
    vfmt.type = vidcap_buftype;

    if (ioctl(fd, VIDIOC_G_FMT, &vfmt)) {
        qDebug("%s: failed to request v4l2_format", __func__);
        return 0;
    }
    return vfmt.fmt.pix.pixelformat;
}

bool pixel_format_is_grey(uint32_t pixelFormat)
{
    uint32_t greyFormats[] = {
        V4L2_PIX_FMT_GREY,
        V4L2_PIX_FMT_Y4,
        V4L2_PIX_FMT_Y6,
        V4L2_PIX_FMT_Y10,
        V4L2_PIX_FMT_Y12,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
        V4L2_PIX_FMT_Y012,
#endif
        V4L2_PIX_FMT_Y14,
        V4L2_PIX_FMT_Y16,
        V4L2_PIX_FMT_Y16_BE
    };

    for (uint32_t format : greyFormats) {
        if (pixelFormat == format) return true;
    }

    return false;
}

bool camera_is_infrared(QString deviceName)
{
    QFile devFile(deviceName);
    if (!devFile.open(QIODevice::ReadWrite)) {
        qDebug("%s: can't open device file: %s", __func__, qPrintable(deviceName));
        return false;
    }
    uint32_t pixelFormat = camera_pixel_format(devFile.handle());
    if (!pixelFormat) {
        qDebug("%s: can't get pixel format for device file: %s", __func__, qPrintable(deviceName));
        return false;
    }
    return pixel_format_is_grey(pixelFormat);
}

CamModel::CamModel(QObject *parent) : QAbstractListModel(parent)
{
    m_cams = {{{}, i18n("No camera selected") }};

    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        m_cams.push_back({
            cameraInfo.deviceName(),
            cameraInfo.description(),
            camera_is_infrared(cameraInfo.deviceName())
        });
    }
}

int CamModel::rowCount(const QModelIndex&) const
{
    return m_cams.size();
}

QVariant CamModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();

    switch (role) {

        case NameRole:
            return m_cams[row].name;

        case DescriptionRole:
            return m_cams[row].description;
        case InfraredRole:
        {
            return m_cams[row].infrared;
        }
    }
    return {};
}

QHash<int, QByteArray> CamModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[DescriptionRole] = "description";
    roles[InfraredRole] = "infrared";
    return roles;
}
