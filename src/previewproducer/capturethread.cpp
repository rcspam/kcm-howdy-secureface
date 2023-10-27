/*
    SPDX-FileCopyrightText: 2023 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "capturethread.h"

#include "config.h"

#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMutex>
#include <QTime>

#include <KLocalizedString>

#include <string>
#include <cmath>
#include <pwd.h>

#include <dlib/image_processing/shape_predictor.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_transforms/image_pyramid.h>
#include <dlib/serialize.h>

#include <opencv2/imgproc.hpp>

#include "face_recognition.h"
#include "../kcm_config.h"

CaptureThread::CaptureThread(QMutex *lock, QObject *parent) :
    QThread(parent),
    m_dataLock(lock),
    m_looping(false)
{}

static void print_text(cv::Mat &overlay, int height, int line_number, QString text, float scale = 1.f)
{
    cv::putText(overlay, text.toStdString(), { int(10 * scale), height - int(scale * (10 + (10 * line_number))) },
                cv::FONT_HERSHEY_SIMPLEX, .3 * scale, { 0, 255, 0 }, 0, cv::LINE_AA);
}

template <class CleanupFunction>
struct Cleanup {
    CleanupFunction f;
    Cleanup(CleanupFunction f) : f(f) {}
    ~Cleanup() { f(); }
};

void CaptureThread::run() {

    Q_EMIT runInitialization();
    Cleanup loopingGuard{ [this] { m_looping = false; } };

    m_cap.open(m_videoPath.toStdString(), cv::CAP_V4L);
    if (!m_cap.isOpened()) {
        Q_EMIT errorOccurred(i18n("Failed to open camera: \"%1\"", m_videoPath));
        return;
    }
    Cleanup capturerGuard{ [this] { m_cap.release(); } };

    int frameWidth = m_cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int frameHeight = m_cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    float scale = frameHeight / 340.f;
    auto sc = [scale] (auto x) {
        return int(scale * x);
    };

    auto pw = getpwuid(getuid());
    QString username;
    if (!pw || strlen(pw->pw_name) == 0) {
        qWarning("%s: unable to get user data", Q_FUNC_INFO);
    } else {
        username = QLatin1String(pw->pw_name);
    }

    QByteArray val;
    QFile file;
    QList<dlib::matrix<double,0,1>> encodings;
    file.setFileName(QStringLiteral("%1/%2.dat").arg(QStringLiteral(HOWDY_MODELS_PATH)).arg(username));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        val = file.readAll();
        file.close();
        QJsonDocument d = QJsonDocument::fromJson(val);
        QJsonArray models = d.array();

        for (int i = 0; i < models.size(); ++i) {
            QJsonValueRef model = models[i];
            encodings.push_back({});
            auto &encoding = encodings.back();
            auto data = model.toObject()[QStringLiteral("data")];
            auto coefficients = data.toArray().at(0).toArray();
            encoding.set_size(coefficients.size());
            for(int j = 0; j < coefficients.size(); ++j) {
                encoding(j) = coefficients.at(j).toDouble();
            }
        }
    } else {
        qWarning("%s: file with user face models not found: %s", Q_FUNC_INFO, qPrintable(file.fileName()));
    }

    // dlib models
    auto face_detector = dlib::get_frontal_face_detector();
    std::ifstream fin(DLIB_MODELS_PATH "/shape_predictor_5_face_landmarks.dat", std::ios::binary);
    auto posePredictor = dlib::shape_predictor();
    // face_encoder will be ok if pose_predictor is ok, so we'll only check it later
    std::optional<face_recognition_model_v1> faceEncoder;
    try {
        deserialize(posePredictor, fin);
        faceEncoder.emplace(DLIB_MODELS_PATH "/dlib_face_recognition_resnet_model_v1.dat");
    } catch (const serialization_error &error) {
        qWarning("%s: failed to load dlib model: %s", Q_FUNC_INFO, error.info.c_str());
    }

    auto clahe = cv::createCLAHE(2.0, { 8, 8 });

    int fps = 0;
    int totalFrames = 0;
    int secFrames = 0;
    int darkFrames = 0;
    qint64 sec = 0;

    while (m_looping) {

        qint64 time = QDateTime::currentMSecsSinceEpoch();
        ++totalFrames;
        ++secFrames;

        if (sec != time / 1000) {
            fps = secFrames;
            sec = time / 1000;
            secFrames = 1;
        }

        cv::Mat origFrame;

        m_cap >> origFrame;
        if (origFrame.empty()) {
            Q_EMIT errorOccurred(i18n("Failed to capture frame from camera: \"%1\"", m_videoPath));
            return;
        }

        cv::Mat greyFrame;
        cvtColor(origFrame, greyFrame, cv::COLOR_BGR2GRAY);

        clahe->apply(greyFrame, greyFrame);

        cv::Mat overlay;
        cvtColor(greyFrame, overlay, cv::COLOR_GRAY2RGB);

        const int HIST_SIZE = 8;
        int channels[] = { 0 };
        int histSizeArray[] = { HIST_SIZE };
        float range0[] = { 0.f, 256.f };
        const float *ranges[] = { range0 };

        cv::Mat hist;
        cv::calcHist(&overlay, 1, channels, cv::Mat(), hist, 1, histSizeArray, ranges);

        float histTotal = cv::sum(hist)[0];

        float histPerc[HIST_SIZE];
        for (size_t i = 0; i < HIST_SIZE; ++i) {
            float valuePerc = hist.at<float>(0, i) / histTotal * 100.f;
            histPerc[i] = valuePerc;
            cv::Point p1{ sc(20 + 10 * i), sc(10) };
            cv::Point p2{ sc(10 + 10 * i), sc(valuePerc / 2 + 10) };
            cv::rectangle(overlay, p1, p2, { 0, 200, 0 }, cv::FILLED);
        }

        qint64 recTime = QDateTime::currentMSecsSinceEpoch();

        if (histPerc[0] > m_darkThreshold) {
            ++darkFrames;
            // sometimes dark and light frames alternate quickly, and this causes flickering
            // we will show dark frames only if there are a lot of them in a row
            if (darkFrames < 3) continue;
            cv::putText(overlay, "DARK FRAME", { frameWidth - sc(68), sc(16) }, cv::FONT_HERSHEY_SIMPLEX, .3f * scale, { 255, 0, 0 }, 0, cv::LINE_AA);
        } else {
            darkFrames = 0;
            cv::putText(overlay, "SCAN FRAME", { frameWidth - sc(68), sc(16) }, cv::FONT_HERSHEY_SIMPLEX, .3f * scale, { 0, 255, 0 }, 0, cv::LINE_AA);

            dlib::array2d<uint8_t> greyImg;
            dlib::assign_image(greyImg, dlib::cv_image<uint8_t>(greyFrame));

            dlib::array2d<uint8_t> upscaledImg;
            dlib::pyramid_down<2> pyr;
            dlib::pyramid_up(greyImg, upscaledImg, pyr);

            cv::Mat rgbFrame;
            cvtColor(origFrame, rgbFrame, cv::COLOR_BGR2RGB);
            dlib::array2d<rgb_pixel> rgbImg;
            dlib::assign_image(rgbImg, dlib::cv_image<rgb_pixel>(rgbFrame));

            std::vector<dlib::rectangle> faceLocations;
            std::vector<dlib::rect_detection> rectDetections;
            face_detector(upscaledImg, rectDetections);

            for (size_t i = 0; i < rectDetections.size(); ++i) {
                rectDetections[i].rect = pyr.rect_down(rectDetections[i].rect, 1);
                faceLocations.push_back(rectDetections[i].rect);
            }

            for (auto &loc : faceLocations) {
                int x = (loc.right() - loc.left()) / 2 + loc.left();
                int y = (loc.bottom() - loc.top()) / 2 + loc.top();
                int r = (loc.right() - loc.left()) / 2;
                r += r * 0.2;

                cv::Scalar color{ 230, 0, 0 };

                if (encodings.size() > 0 && faceEncoder.has_value()) {
                    auto faceLandmark = posePredictor(rgbImg, loc);
                    auto faceEncoding = faceEncoder->compute_face_descriptor(rgbImg, faceLandmark, 1);
                    std::vector<double> matches;
                    matches.reserve(encodings.size());
                    for (auto &encoding : encodings) {
                        auto diff = encoding - faceEncoding;
                        auto norm = std::sqrt(dlib::sum(dlib::squared(diff)));
                        matches.push_back(norm);
                    }
                    size_t bestMatchIndex = std::min_element(matches.begin(), matches.end()) - matches.begin();
                    double bestMatch = matches[bestMatchIndex];
                    cv::Point p{ x + r / 3, y - r };
                    if (bestMatch * 10 < m_certainty) {
                        color = { 0, 230, 0 };
                        QString circleText = QStringLiteral("Model #%1 (c: %2)").arg(bestMatchIndex).arg(bestMatch * 10.0, 4, 'f', 2);
                        cv::putText(overlay, circleText.toStdString(), p, cv::FONT_HERSHEY_SIMPLEX, .3 * scale, { 0, 255, 0 }, 0, cv::LINE_AA);
                    } else {
                        QString circle_text = QStringLiteral("no match (c: %1)").arg(bestMatch * 10.0, 4, 'f', 2);
                        cv::putText(overlay, circle_text.toStdString(), p, cv::FONT_HERSHEY_SIMPLEX, .3 * scale, { 255, 0, 0 }, 0, cv::LINE_AA);
                    }
                }
                cv::circle(overlay, { x, y }, r, color, 2);
            }
        }

        recTime = QDateTime::currentMSecsSinceEpoch() - recTime;

        print_text(overlay, frameHeight, 0, QStringLiteral("RESOLUTION: %1x%2").arg(frameWidth).arg(frameHeight), scale);
        print_text(overlay, frameHeight, 1, QStringLiteral("FPS: %1").arg(fps), scale);
        print_text(overlay, frameHeight, 2, QStringLiteral("FRAMES: %1").arg(totalFrames), scale);
        if (darkFrames == 0) {
            print_text(overlay, frameHeight, 3, QStringLiteral("RECOGNITION: %1 ms").arg(recTime), scale);
        }

        m_dataLock->lock();
        if (m_frame.size == overlay.size) {
            overlay.copyTo(m_frame);
        } else {
            m_frame = overlay.clone();
        }
        m_dataLock->unlock();

        Q_EMIT frameCaptured(&m_frame);
    }
}

void CaptureThread::setVideoPath(QString videoPath)
{
    bool wasLooping = m_looping.load();
    stopCaptureSync();
    m_videoPath = videoPath;
    m_videoPath.detach();
    // an empty video path means that the device is not selected
    if (wasLooping && m_videoPath.length() > 0) startCapture();
}

void CaptureThread::stopCaptureSync()
{
    m_looping = false;
    if (!isRunning()) return;
    if (!wait(10000)) { //ms
        qWarning("%s: Can't stop the capture thread in 10 sec!", Q_FUNC_INFO);
    }
    if (!wait(20000)) { //ms
        qWarning("%s: Can't stop the capture thread in 30 sec, have to terminate!", Q_FUNC_INFO);
    }
    terminate();
    wait();
    // still try to improve the situation a little
    m_looping = false;
    m_cap.release();
}

void CaptureThread::startCapture()
{
    // let's turn it on in the main thread,
    // because it will also be turned off in the main thread,
    m_looping = true;
    start();
}
