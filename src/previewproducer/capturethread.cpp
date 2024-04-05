/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

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
    m_looping(false),
    m_fontRatio(0.01f),
    m_fontThickness(0)
{}

static constexpr int OVERLAY_FONT = cv::FONT_HERSHEY_SIMPLEX;

struct FontParam {
    float size;
    float scale;
    int thickness;
};

static void print_text(cv::Mat &overlay, int height, int line_number, QString text, FontParam font)
{
    cv::putText(overlay, text.toStdString(), { int(1.4f * font.size), height - int(font.size * (1.4f + (1.4f * line_number))) },
                OVERLAY_FONT, font.scale, { 0, 255, 0 }, font.thickness, cv::LINE_AA);
}

class FaceHelper {

    frontal_face_detector m_faceDetector;
    dlib::shape_predictor m_posePredictor;
    std::optional<face_recognition_model_v1> m_faceEncoder;
    QList<dlib::matrix<double,0,1>> m_encodings;

public:
    FaceHelper() :
        m_faceDetector(dlib::get_frontal_face_detector()),
        m_posePredictor(dlib::shape_predictor())
    {
        auto pw = getpwuid(getuid());
        QString username;
        if (!pw || strlen(pw->pw_name) == 0) {
            qWarning("%s: unable to get user data", Q_FUNC_INFO);
        } else {
            username = QLatin1String(pw->pw_name);
        }

        QFile file;
        file.setFileName(QStringLiteral("%1/%2.dat").arg(QStringLiteral(HOWDY_MODELS_PATH)).arg(username));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray val = file.readAll();
            file.close();
            QJsonDocument d = QJsonDocument::fromJson(val);
            const QJsonArray models = d.array();
            for (auto model : models) {
                m_encodings.push_back({});
                auto &encoding = m_encodings.back();
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
        std::ifstream fin(DLIB_MODELS_PATH "/shape_predictor_5_face_landmarks.dat", std::ios::binary);
        // m_faceEncoder will be ok if m_posePredictor is ok, so we'll only check it later
        try {
            deserialize(m_posePredictor, fin);
            m_faceEncoder.emplace(DLIB_MODELS_PATH "/dlib_face_recognition_resnet_model_v1.dat");
        } catch (const serialization_error &error) {
            qWarning("%s: failed to load dlib model: %s", Q_FUNC_INFO, error.info.c_str());
        }
    }

    std::vector<dlib::rectangle> detectFaces(const dlib::array2d<uint8_t> &greyImg)
    {
        std::vector<dlib::rectangle> faceLocations;

        dlib::array2d<uint8_t> upscaledImg;
        dlib::pyramid_down<2> pyr;
        dlib::pyramid_up(greyImg, upscaledImg, pyr);

        std::vector<dlib::rect_detection> rectDetections;
        m_faceDetector(upscaledImg, rectDetections);

        for (auto &rectDetection : rectDetections) {
            rectDetection.rect = pyr.rect_down(rectDetection.rect, 1);
            faceLocations.push_back(rectDetection.rect);
        }
        return faceLocations;
    }

    struct MatchResult {
        double certainty;
        size_t index;
    };

    MatchResult matchFace(dlib::array2d<rgb_pixel> &rgbImg, const dlib::rectangle &loc)
    {
        auto faceLandmark = m_posePredictor(rgbImg, loc);
        auto faceEncoding = m_faceEncoder->compute_face_descriptor(rgbImg, faceLandmark, 1);

        std::vector<double> matches;
        matches.reserve(m_encodings.size());

        for (auto &encoding : m_encodings) {
            auto diff = encoding - faceEncoding;
            auto norm = std::sqrt(dlib::sum(dlib::squared(diff)));
            matches.push_back(norm);
        }
        size_t bestMatchIndex = std::min_element(matches.begin(), matches.end()) - matches.begin();
        return { matches[bestMatchIndex], bestMatchIndex };
    }

    bool matchNeeded()
    {
        return !m_encodings.empty() && m_faceEncoder.has_value();
    }
};

template <size_t HistSize>
static std::array<float, HistSize> calculate_hist(const cv::Mat &overlay)
{
    int channels[] = { 0 };
    int histSizeArray[] = { HistSize };
    float range0[] = { 0.f, 256.f };
    const float *ranges[] = { range0 };

    cv::Mat hist;
    cv::calcHist(&overlay, 1, channels, cv::Mat(), hist, 1, histSizeArray, ranges);

    float histTotal = cv::sum(hist)[0];

    std::array<float, HistSize> histPerc;
    for (size_t i = 0; i < HistSize; ++i) {
        float valuePerc = hist.at<float>(0, i) / histTotal * 100.f;
        histPerc[i] = valuePerc;
    }
    return histPerc;
}

template <size_t HistSize, class F>
static void draw_hist(cv::Mat &overlay, const std::array<float, HistSize> &histPerc, F &&sc)
{
    for (size_t i = 0; i < HistSize; ++i) {
        cv::Point p1{ sc(20 + 10 * i), sc(10) };
        cv::Point p2{ sc(10 + 10 * i), sc(histPerc[i] / 2 + 10) };
        cv::rectangle(overlay, p1, p2, { 0, 200, 0 }, cv::FILLED);
    }
}

template <class CleanupFunction>
struct Cleanup {
    CleanupFunction f;
    Cleanup(CleanupFunction f) : f(f) {}
    ~Cleanup() { f(); }
};

void CaptureThread::run()
{
    Q_EMIT runInitialization();
    Cleanup loopingGuard{ [this] { m_looping = false; } };

    m_cap.open(m_videoPath.toStdString(), cv::CAP_V4L);
    if (!m_cap.isOpened()) {
        Q_EMIT errorOccurred(i18n("Failed to open camera: \"%1\"", m_videoPath));
        return;
    }
    Cleanup capturerGuard{ [this] { m_cap.release(); } };

    FaceHelper faceHelper;

    int frameWidth = m_cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int frameHeight = m_cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    auto clahe = cv::createCLAHE(2.0, { 8, 8 });

    int fps = 0;
    int totalFrames = 0;
    int secFrames = 0;
    int darkFrames = 0;
    qint64 sec = 0;

    cv::Mat origFrame;
    cv::Mat greyFrame;
    cv::Mat overlay;

    int baseSize;
    auto hersheySize = cv::getTextSize("M", OVERLAY_FONT, 1.0, 0, &baseSize);

    while (m_looping) {

        FontParam font;

        font.size = m_fontRatio.load() * frameWidth;
        font.scale = font.size / hersheySize.height;
        font.thickness = m_fontThickness.load();

        // some arbitrary units, for non-text elements
        float scale = font.size / 6;
        auto sc = [scale] (auto x) {
            return int(scale * x);
        };

        qint64 time = QDateTime::currentMSecsSinceEpoch();
        ++totalFrames;
        ++secFrames;

        if (sec != time / 1000) {
            fps = secFrames;
            sec = time / 1000;
            secFrames = 1;
        }

        m_cap >> origFrame;
        if (origFrame.empty()) {
            Q_EMIT errorOccurred(i18n("Failed to capture frame from camera: \"%1\"", m_videoPath));
            return;
        }

        cvtColor(origFrame, greyFrame, cv::COLOR_BGR2GRAY);
        clahe->apply(greyFrame, greyFrame);
        cvtColor(greyFrame, overlay, cv::COLOR_GRAY2RGB);

        const int HIST_SIZE = 8;
        auto histPerc = calculate_hist<HIST_SIZE>(overlay);
        draw_hist(overlay, histPerc, sc);

        qint64 recTime = QDateTime::currentMSecsSinceEpoch();

        if (histPerc[0] > m_darkThreshold) {
            ++darkFrames;
            // sometimes dark and light frames alternate quickly, and this causes flickering
            // we will show dark frames only if there are a lot of them in a row
            if (darkFrames < 3) continue;
            cv::putText(overlay, "DARK FRAME", { frameWidth - sc(68), sc(16) }, OVERLAY_FONT, font.scale, { 255, 0, 0 }, font.thickness, cv::LINE_AA);
        } else {
            darkFrames = 0;
            cv::putText(overlay, "SCAN FRAME", { frameWidth - sc(68), sc(16) }, OVERLAY_FONT, font.scale, { 0, 255, 0 }, font.thickness, cv::LINE_AA);

            dlib::array2d<uint8_t> greyImg;
            dlib::assign_image(greyImg, dlib::cv_image<uint8_t>(greyFrame));

            cv::Mat rgbFrame;
            cvtColor(origFrame, rgbFrame, cv::COLOR_BGR2RGB);
            dlib::array2d<rgb_pixel> rgbImg;
            dlib::assign_image(rgbImg, dlib::cv_image<rgb_pixel>(rgbFrame));

            auto faceLocations = faceHelper.detectFaces(greyImg);

            for (auto &loc : faceLocations) {
                int x = (loc.right() - loc.left()) / 2 + loc.left();
                int y = (loc.bottom() - loc.top()) / 2 + loc.top();
                int r = (loc.right() - loc.left()) / 2;
                r += r * 0.2;

                cv::Scalar color{ 230, 0, 0 };

                if (faceHelper.matchNeeded()) {
                    auto match = faceHelper.matchFace(rgbImg, loc);
                    cv::Point p{ x + r / 3, y - r };
                    if (match.certainty * 10 < m_certainty) {
                        color = { 0, 230, 0 };
                        QString circleText = QStringLiteral("Model #%1 (c: %2)").arg(match.index).arg(match.certainty * 10.0, 4, 'f', 2);
                        cv::putText(overlay, circleText.toStdString(), p, OVERLAY_FONT, font.scale, { 0, 255, 0 }, font.thickness, cv::LINE_AA);
                    } else {
                        QString circle_text = QStringLiteral("no match (c: %1)").arg(match.certainty * 10.0, 4, 'f', 2);
                        cv::putText(overlay, circle_text.toStdString(), p, OVERLAY_FONT, font.scale, { 255, 0, 0 }, font.thickness, cv::LINE_AA);
                    }
                }
                cv::circle(overlay, { x, y }, r, color, 2);
            }
        }

        recTime = QDateTime::currentMSecsSinceEpoch() - recTime;

        print_text(overlay, frameHeight, 0, QStringLiteral("RESOLUTION: %1x%2").arg(frameWidth).arg(frameHeight), font);
        print_text(overlay, frameHeight, 1, QStringLiteral("FPS: %1").arg(fps), font);
        print_text(overlay, frameHeight, 2, QStringLiteral("FRAMES: %1").arg(totalFrames), font);
        if (darkFrames == 0) {
            print_text(overlay, frameHeight, 3, QStringLiteral("RECOGNITION: %1 ms").arg(recTime), font);
        }

        m_dataLock->lock();
        cvtColor(overlay, m_frame, cv::COLOR_RGB2BGRA);
        m_dataLock->unlock();

        Q_EMIT frameCaptured(&m_frame);
    }
}

void CaptureThread::setVideoPath(QString videoPath)
{
    bool wasLooping = m_looping.load();
    if (videoPath == m_videoPath && wasLooping) return;
    stopCaptureSync();
    m_videoPath = videoPath;
    if (wasLooping) startCapture();
}

void CaptureThread::stopCaptureSync()
{
    m_looping = false;
    if (!isRunning()) return;

    if (wait(10000)) return;
    qWarning("%s: Can't stop the capture thread in 10 sec!", Q_FUNC_INFO);

    if (wait(20000)) return;
    qWarning("%s: Can't stop the capture thread in 30 sec, have to terminate!", Q_FUNC_INFO);

    terminate();
    wait();
    // still try to improve the situation a little
    m_cap.release();
}

void CaptureThread::startCapture()
{
    // an empty video path means that the device is not selected
    if (m_videoPath.length() == 0) return;
    // let's turn it on in the main thread,
    // because it will also be turned off in the main thread,
    m_looping = true;
    start();
}
