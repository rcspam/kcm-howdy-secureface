/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "previewproducer.h"

#include <QMutex>
#include <QPixmap>

#include <KLocalizedString>

#include "capturethread.h"
#include "../kcm_config.h"

PreviewProducer::PreviewProducer(KcmConfig *config, QObject *parent) :
    QObject(parent),
    m_dataLock(new QMutex),
    m_capturer(new CaptureThread(m_dataLock.get(), this)),
    m_currentFrame(new cv::Mat),
    m_pixmap(new QPixmap),
    m_config(config)
{
    connect(m_capturer, &CaptureThread::frameCaptured, this, &PreviewProducer::newFrame);
    connect(m_capturer, &CaptureThread::errorOccurred, this, &PreviewProducer::captureErrorOccurred);
    connect(m_capturer, &CaptureThread::runInitialization, this, &PreviewProducer::cameraIsWaiting);
}

PreviewProducer::~PreviewProducer()
{
    m_capturer->stopCaptureSync();
}

void PreviewProducer::newFrame(cv::Mat *data)
{
    try {
        QMutexLocker locker(m_dataLock.get());
        if (m_currentFrame->size == data->size) {
            data->copyTo(*m_currentFrame);
        } else {
            *m_currentFrame = data->clone();
        }
    } catch (...) {
        Q_EMIT captureErrorOccurred(i18n("Frame copy error."));
        setCapturing(false);
        return;
    }

    if (m_waitingCamera) {
        m_waitingCamera = false;
        Q_EMIT waitingCameraChanged();
    }

    QImage frame(
        m_currentFrame->data,
        m_currentFrame->cols,
        m_currentFrame->rows,
        m_currentFrame->step,
        QImage::Format_RGB888);
    m_pixmap->convertFromImage(frame);
    Q_EMIT pixmapUpdated();
}

bool PreviewProducer::isCapturing()
{
    return m_capturer->isRunning();
}

void PreviewProducer::setCapturing(bool wantCapturing)
{
    if (wantCapturing && !isCapturing()) {
        auto useCnn = m_config->getConfigValue(QStringLiteral("core/use_cnn"));
        if (useCnn.toBool()) {
            Q_EMIT captureErrorOccurred(i18n("Preview is not supported if CNN model is active (use_cnn option)"));
            return;
        }
        m_capturer->startCapture();
        return;
    }

    if (!wantCapturing && isCapturing()) {
        m_capturer->setLooping(false);
        return;
    }
}

QVariant PreviewProducer::getPixmap()
{
    return *m_pixmap;
}

void PreviewProducer::setVideoPath(QString videoPath)
{
    m_capturer->setVideoPath(videoPath);
}

void PreviewProducer::setDarkThreshold(int threshold)
{
    m_capturer->setDarkThreshold(threshold);
}

void PreviewProducer::setCertainty(double certainty)
{
    m_capturer->setCertainty(certainty);
}

void PreviewProducer::cameraIsWaiting()
{
    m_waitingCamera = true;
    Q_EMIT waitingCameraChanged();
}

#include "moc_previewproducer.cpp"
