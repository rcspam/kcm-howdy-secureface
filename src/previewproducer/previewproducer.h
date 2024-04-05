/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef PREVIEWPRODUCER_H
#define PREVIEWPRODUCER_H

#include <QObject>

class CaptureThread;
class QMutex;
class KcmConfig;
namespace cv { class Mat; }

class PreviewProducer : public QObject
{
    Q_OBJECT

public:

    explicit PreviewProducer(KcmConfig *config, QObject *parent = nullptr);
    ~PreviewProducer();

    Q_PROPERTY(QVariant pixmap READ getPixmap NOTIFY pixmapUpdated)
    Q_PROPERTY(bool capturing READ isCapturing WRITE setCapturing)
    Q_PROPERTY(bool waitingCamera MEMBER m_waitingCamera NOTIFY waitingCameraChanged)

    Q_INVOKABLE void setVideoPath(QString videoPath);
    Q_INVOKABLE void setFontParams(float fontRatio, int thickness);
    Q_INVOKABLE void selectModel(int modelId);
    Q_INVOKABLE void unSelectModel();
    Q_INVOKABLE void setDarkThreshold(int threshold);
    Q_INVOKABLE void setCertainty(double certainty);

public:
    QVariant getPixmap();

Q_SIGNALS:
    void pixmapUpdated();
    void captureErrorOccurred(QString errorString);
    void waitingCameraChanged();

public Q_SLOTS:
    void newFrame(cv::Mat *data);
    void cameraIsWaiting();

private:

    bool isCapturing();
    void setCapturing(bool wantCapturing);

    bool m_waitingCamera = false;
    QScopedPointer<QMutex> m_dataLock;
    CaptureThread *m_capturer;
    QScopedPointer<cv::Mat> m_currentFrame;
    QPixmap *m_pixmap;
    KcmConfig *m_config;
};

#endif // PREVIEWPRODUCER_H
