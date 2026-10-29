/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef CAPTURETHREAD_H
#define CAPTURETHREAD_H

#include <QString>
#include <QThread>
#include <opencv2/opencv.hpp>

#include "overlay.h"

class QMutex;

class CaptureThread : public QThread
{
    Q_OBJECT
public:
    explicit CaptureThread(QMutex *lock, QObject *parent = nullptr);
    void setVideoPath(QString videoPath);
    void setLooping(bool looping) { m_looping.store(looping); }
    bool isLooping() { return m_looping.load(); }
    void setDarkThreshold(int threshold) { m_darkThreshold.store(threshold); }
    void setCertainty(double certainty) { m_certainty.store(certainty); }
    void setUserName(QString userName);
    void selectModel(int modelId) { m_selectModel.store(modelId); }
    void stopCaptureSync();
    void startCapture();

protected:
    void run() override;

Q_SIGNALS:
    void runInitialization();
    // both are guarded by the data lock
    void frameCaptured(cv::Mat *data, Overlay::Info *overlay);
    void errorOccurred(QString errorString);

private:
    QString m_videoPath;
    QString m_userName;
    QMutex *m_dataLock;
    cv::Mat m_frame;
    Overlay::Info m_overlay;
    cv::VideoCapture m_cap;
    std::atomic<bool> m_looping;
    std::atomic<int> m_darkThreshold;
    std::atomic<double> m_certainty;
    std::atomic<int> m_selectModel;
};

#endif // CAPTURETHREAD_H
