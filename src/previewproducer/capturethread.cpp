/*
    SPDX-FileCopyrightText: 2023-2024 Anton Golubev <golubevan@altlinux.org>

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "capturethread.h"

#include "config.h"

#include <QElapsedTimer>
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
#include <dlfcn.h>
#include <pwd.h>

#include <dlib/image_processing/shape_predictor.h>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/opencv/cv_image.h>
#include <dlib/image_transforms/image_pyramid.h>
#include <dlib/serialize.h>

#include <opencv2/imgproc.hpp>

#include "face_recognition.h"
#include "backgroundtask.h"
#include "recognitionpacer.h"
#include "../kcm_config.h"

CaptureThread::CaptureThread(QMutex *lock, QObject *parent) :
    QThread(parent),
    m_dataLock(lock),
    m_looping(false),
    // Howdy's defaults until the config is sent: a default-constructed atomic
    // holds no defined value before C++20, and 0 marks every IR frame as dark
    m_darkThreshold(60),
    m_certainty(3.5),
    m_selectModel(-1)
{}

class FaceHelper {

    struct Face {
        int id;
        dlib::matrix<double,0,1> data;
    };

    frontal_face_detector m_faceDetector;
    dlib::shape_predictor m_posePredictor;
    std::optional<face_recognition_model_v1> m_faceEncoder;
    QList<Face> m_faces;

public:
    FaceHelper(QString userName) :
        m_faceDetector(dlib::get_frontal_face_detector()),
        m_posePredictor(dlib::shape_predictor())
    {
        QFile file;
        file.setFileName(QStringLiteral("%1/%2.dat").arg(QStringLiteral(HOWDY_MODELS_PATH)).arg(userName));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QByteArray val = file.readAll();
            file.close();
            QJsonDocument d = QJsonDocument::fromJson(val);
            const QJsonArray models = d.array();
            for (auto model : models) {

                auto obj = model.toObject();

                const int COULD_NOT_READ = -100;
                int id = obj[QStringLiteral("id")].toInt(COULD_NOT_READ);
                if (id == COULD_NOT_READ) {
                    qWarning("%s: unable to read model id, skip", Q_FUNC_INFO);
                    continue;
                }

                auto data = obj[QStringLiteral("data")];
                if (data.isNull() || !data.isArray() || !data.toArray().at(0).isArray()) {
                    qWarning("%s: unable to get model data, skip", Q_FUNC_INFO);
                    continue;
                }

                auto coefficients = data.toArray().at(0).toArray();
                if (coefficients.size() == 0) {
                    qWarning("%s: there are no coefficients in the model data, skip", Q_FUNC_INFO);
                    continue;
                }

                m_faces.push_back({});
                auto &face = m_faces.back();

                face.id = id;
                face.data.set_size(coefficients.size());
                for(int j = 0; j < coefficients.size(); ++j) {
                    face.data(j) = coefficients.at(j).toDouble();
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
        static constexpr int INVALID = -1;
        int index;
        double certainty;
    };

    MatchResult matchFace(dlib::array2d<rgb_pixel> &rgbImg, const dlib::rectangle &loc, int modelId = -1)
    {
        auto faceLandmark = m_posePredictor(rgbImg, loc);
        auto faceEncoding = m_faceEncoder->compute_face_descriptor(rgbImg, faceLandmark, 1);

        if (modelId >= 0) {

            auto face = std::find_if(m_faces.cbegin(), m_faces.cend(), [modelId](const Face &f) {
                return modelId == f.id;
            });
            if (face == m_faces.cend()) return { MatchResult::INVALID };

            auto diff = face->data - faceEncoding;
            auto norm = std::sqrt(dlib::sum(dlib::squared(diff)));
            return { modelId, norm };
        }

        std::vector<double> matches;
        matches.reserve(m_faces.size());

        for (int i = 0; i < m_faces.size(); ++i) {

            auto diff = m_faces[i].data - faceEncoding;
            auto norm = std::sqrt(dlib::sum(dlib::squared(diff)));
            matches.push_back(norm);
        }

        size_t bestMatchIndex =  std::min_element(matches.begin(), matches.end()) - matches.begin();
        return { m_faces[bestMatchIndex].id, matches[bestMatchIndex]};
    }

    bool matchNeeded()
    {
        return !m_faces.empty() && m_faceEncoder.has_value();
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

// OpenBLAS starts one worker per core, and the workers spin between calls:
// on 20 cores the face encoder of the preview kept 6 of them busy. One thread
// is also faster here (86 ms per recognition against 112), the matrices of a
// single face are small. Does nothing with another BLAS.
static void useOneBlasThread()
{
    void *openblas = dlopen("libopenblas.so.0", RTLD_NOW | RTLD_NOLOAD);
    if (!openblas) return;
    using SetNumThreads = void (*)(int);
    if (auto setNumThreads = reinterpret_cast<SetNumThreads>(dlsym(openblas, "openblas_set_num_threads"))) {
        setNumThreads(1);
    }
    dlclose(openblas);
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

    useOneBlasThread();

    m_cap.open(m_videoPath.toStdString(), cv::CAP_V4L);
    if (!m_cap.isOpened()) {
        Q_EMIT errorOccurred(i18n("Failed to open camera: \"%1\"", m_videoPath));
        return;
    }
    Cleanup capturerGuard{ [this] { m_cap.release(); } };

    if (m_userName.isEmpty()) {
        Q_EMIT errorOccurred(i18n("User unavailable"));
        return;
    }
    FaceHelper faceHelper(m_userName);

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
    cv::Mat shownFrame;

    // a recognition keeps a core busy, the frames in between show its faces
    RecognitionPacer pacer;
    QElapsedTimer clock;
    clock.start();
    QList<Overlay::Face> lastFaces;
    qint64 lastRecognitionMs = -1;
    // Recognitions run beside the capture: run inline, each one froze the
    // preview for its whole duration, then the queued frames rushed by.
    // Declared after faceHelper, which the running job uses: it is destroyed
    // first and waits for that job.
    struct Recognition {
        QList<Overlay::Face> faces;
        qint64 start;
        qint64 end;
    };
    BackgroundTask<Recognition> recognition;
    // the running recognition saw a frame from before a dark spell
    bool recognitionStale = false;

    while (m_looping) {

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
        cvtColor(greyFrame, shownFrame, cv::COLOR_GRAY2RGB);

        const int HIST_SIZE = 8;
        auto histPerc = calculate_hist<HIST_SIZE>(shownFrame);

        // drawn by the QML preview, at screen resolution
        Overlay::Info overlay;
        overlay.frameWidth = frameWidth;
        overlay.frameHeight = frameHeight;
        overlay.hist = QList<float>(histPerc.cbegin(), histPerc.cend());

        // a finished recognition hands its faces over
        if (auto done = recognition.takeIfDone()) {
            pacer.done(done->start, done->end);
            lastRecognitionMs = done->end - done->start;
            if (!recognitionStale) {
                lastFaces = done->faces;
            }
        }

        if (histPerc[0] > m_darkThreshold) {
            ++darkFrames;
            // sometimes dark and light frames alternate quickly, and this causes flickering
            // we will show dark frames only if there are a lot of them in a row
            if (darkFrames < 3) continue;
            overlay.dark = true;
            // nothing is seen in the dark, do not bring old faces back after it
            lastFaces.clear();
            recognitionStale = recognition.busy();
        } else {
            darkFrames = 0;

            if (!recognition.busy() && pacer.due(clock.elapsed())) {
                recognitionStale = false;
                // copies: the loop reads the next frames into these while the job runs
                recognition.start([this, &faceHelper, &clock, grey = greyFrame.clone(), orig = origFrame.clone()] {
                    Recognition result;
                    result.start = clock.elapsed();

                    dlib::array2d<uint8_t> greyImg;
                    dlib::assign_image(greyImg, dlib::cv_image<uint8_t>(grey));

                    cv::Mat rgbFrame;
                    cvtColor(orig, rgbFrame, cv::COLOR_BGR2RGB);
                    dlib::array2d<rgb_pixel> rgbImg;
                    dlib::assign_image(rgbImg, dlib::cv_image<rgb_pixel>(rgbFrame));

                    auto faceLocations = faceHelper.detectFaces(greyImg);

                    for (auto &loc : faceLocations) {
                        auto face = Overlay::faceCircle(loc.left(), loc.top(), loc.right(), loc.bottom());

                        if (faceHelper.matchNeeded()) {
                            auto match = faceHelper.matchFace(rgbImg, loc, m_selectModel.load());
                            face.match = Overlay::classify(match.index, match.certainty, m_certainty);
                            face.model = match.index;
                            face.certainty = match.certainty * 10.0;
                        }
                        result.faces.append(face);
                    }

                    result.end = clock.elapsed();
                    return result;
                });
            }
            overlay.faces = lastFaces;
        }

        overlay.fps = fps;
        overlay.frames = totalFrames;
        if (darkFrames == 0) {
            overlay.recognitionMs = lastRecognitionMs;
        }

        m_dataLock->lock();
        cvtColor(shownFrame, m_frame, cv::COLOR_RGB2BGRA);
        m_overlay = overlay;
        m_dataLock->unlock();

        Q_EMIT frameCaptured(&m_frame, &m_overlay);
    }
}

void CaptureThread::setVideoPath(QString videoPath)
{
    if (videoPath == m_videoPath) return;
    bool wasLooping = m_looping.load();
    stopCaptureSync();
    m_videoPath = videoPath;
    if (wasLooping) startCapture();
}

void CaptureThread::setUserName(QString userName)
{
    if (userName == m_userName) return;
    bool wasLooping = m_looping.load();
    stopCaptureSync();
    m_userName = userName;
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
