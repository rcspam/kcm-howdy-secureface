/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef PREVIEWPRODUCER_H
#define PREVIEWPRODUCER_H

#include <QObject>
#include <QVariantMap>

class CaptureThread;
class QMutex;
class KcmConfig;
namespace cv { class Mat; }
namespace Overlay { struct Info; }

class PreviewProducer : public QObject
{
    Q_OBJECT

public:

    explicit PreviewProducer(KcmConfig *config, QObject *parent = nullptr);
    ~PreviewProducer();

    Q_PROPERTY(QVariant pixmap READ getPixmap NOTIFY pixmapUpdated)
    // faces, frame state and statistics of the current pixmap, see overlay.h
    Q_PROPERTY(QVariantMap overlay READ overlay NOTIFY pixmapUpdated)
    Q_PROPERTY(bool capturing READ isCapturing WRITE setCapturing)
    Q_PROPERTY(bool waitingCamera MEMBER m_waitingCamera NOTIFY waitingCameraChanged)

    Q_INVOKABLE void setVideoPath(QString videoPath);
    Q_INVOKABLE void selectModel(int modelId);
    Q_INVOKABLE void unSelectModel();
    Q_INVOKABLE void setDarkThreshold(int threshold);
    Q_INVOKABLE void setCertainty(double certainty);
    Q_INVOKABLE void setUserName(QString userName);

public:
    QVariant getPixmap();
    QVariantMap overlay() const { return m_overlay; }

Q_SIGNALS:
    void pixmapUpdated();
    void captureErrorOccurred(QString errorString);
    void waitingCameraChanged();

public Q_SLOTS:
    void newFrame(cv::Mat *data, Overlay::Info *overlay);
    void cameraIsWaiting();

private:

    bool isCapturing();
    void setCapturing(bool wantCapturing);

    bool m_waitingCamera = false;
    QScopedPointer<QMutex> m_dataLock;
    CaptureThread *m_capturer;
    QScopedPointer<cv::Mat> m_currentFrame;
    QPixmap *m_pixmap;
    QVariantMap m_overlay;
    KcmConfig *m_config;
};

#endif // PREVIEWPRODUCER_H
