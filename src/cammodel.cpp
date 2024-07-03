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
#include <systemd/sd-device.h>

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

template <class CleanupFunction>
struct Cleanup {
    CleanupFunction f;
    Cleanup(CleanupFunction f) : f(f) {}
    ~Cleanup() { f(); }
};
bool camera_has_infrared_property(QString deviceName)
{
    sd_device *device;
    const char *path = deviceName.toLocal8Bit().constData();
    int r = sd_device_new_from_path(&device, path);
    if (r < 0) {
        qDebug("%s: Failed to allocate device: %s (%d)", __func__, path, r);
        return false;
    }
    Cleanup deviceGuard{ [device] { sd_device_unref(device); } };

    const char *val;
    r = sd_device_get_property_value(device, "ID_INFRARED_CAMERA", &val);
    if (r < 0) return false;
    if (val && val[0] == '1') return true;
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
    return pixel_format_is_grey(pixelFormat) || camera_has_infrared_property(deviceName);
}

CamModel::CamModel(QObject *parent) : QAbstractListModel(parent)
{
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    for (const QCameraInfo &cameraInfo : cameras) {
        m_cams.push_back({
            cameraInfo.deviceName(),
            cameraInfo.description(),
            camera_is_infrared(cameraInfo.deviceName())
        });
    }

    // to provide data to QML by index and name
    auto roles = roleNames();
    for(auto role = roles.begin(); role != roles.end(); ++role) {
        m_invRoleNames[role.value()] = role.key();
    }

    // infrared cameras will appear at the top
    std::sort(m_cams.begin(), m_cams.end(), [](const CameraEntry &a, const CameraEntry &b) {
        if (a.infrared != b.infrared) return a.infrared > b.infrared;
        return a.description < b.description;
    });
}

int CamModel::rowCount(const QModelIndex&) const
{
    return m_cams.size();
}

QVariant CamModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    return dataFromEntry(m_cams[row], role);
}

QVariant CamModel::dataFromEntry(const CameraEntry &entry, int role) const
{
    switch (role) {

        case NameRole:
            return entry.name;

        case DescriptionRole:
            return entry.description;
        case InfraredRole:
        {
            return entry.infrared;
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

CamModelEmptyItemProxy *CamModel::getEmptyItemProxy()
{
    if (!m_emptyItemProxy) {
        m_emptyItemProxy = new CamModelEmptyItemProxy(this);
    }
    return m_emptyItemProxy;
}

CamModel::CameraEntry CamModelEmptyItemProxy::s_noCameraItem{ {}, i18n("No camera selected") };

CamModelEmptyItemProxy::CamModelEmptyItemProxy(CamModel *source)
    : QAbstractProxyModel(source)
{
    setSourceModel(source);
}

QModelIndex CamModelEmptyItemProxy::mapToSource(const QModelIndex &proxyIndex) const
{
    if (proxyIndex.row() == 0) return QModelIndex();
    return sourceModel()->index(proxyIndex.row() - 1, proxyIndex.column());
}

QModelIndex CamModelEmptyItemProxy::mapFromSource(const QModelIndex &sourceIndex) const
{
    return createIndex(sourceIndex.row() + 1, sourceIndex.column());
}

QVariant CamModelEmptyItemProxy::data(const QModelIndex &index, int role) const
{
    if (index.row() == 0) {
        return static_cast<CamModel*>(sourceModel())->dataFromEntry(s_noCameraItem, role);
    }
    return sourceModel()->data(mapToSource(index), role);
}
