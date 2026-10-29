/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef OVERLAY_H
#define OVERLAY_H

// What the preview draws over the camera frame. The capture thread fills it,
// QML draws it at screen resolution: drawn into the small IR frame, the text
// and circles get blurry once the frame is scaled up.

#include <QList>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace Overlay {

enum class Match {
    Unchecked, // no face model to compare with
    Failed,    // the selected model does not exist
    Matched,
    NoMatch,
};

struct Face {
    // circle in frame pixels
    double x = 0;
    double y = 0;
    double r = 0;
    Match match = Match::Unchecked;
    int model = -1;
    // on Howdy's certainty scale, distance * 10
    double certainty = 0;
};

struct Info {
    int frameWidth = 0;
    int frameHeight = 0;
    bool dark = false;
    // percent of the pixels in each brightness bin, darkest first
    QList<float> hist;
    QList<Face> faces;
    int fps = 0;
    int frames = 0;
    // -1 when the frame was not scanned
    qint64 recognitionMs = -1;
};

// Circle around a detected face rectangle, 20 % larger than it.
inline Face faceCircle(long left, long top, long right, long bottom)
{
    Face face;
    face.x = (left + right) / 2.0;
    face.y = (top + bottom) / 2.0;
    face.r = (right - left) / 2.0 * 1.2;
    return face;
}

// Howdy accepts a face when distance * 10 is below its certainty setting.
// An index below 0 means the requested model was not found.
inline Match classify(int index, double distance, double certainty)
{
    if (index < 0) return Match::Failed;
    return distance * 10 < certainty ? Match::Matched : Match::NoMatch;
}

inline QString stateName(Match match)
{
    switch (match) {
    case Match::Failed: return QStringLiteral("failed");
    case Match::Matched: return QStringLiteral("matched");
    case Match::NoMatch: return QStringLiteral("nomatch");
    case Match::Unchecked: break;
    }
    return QStringLiteral("unchecked");
}

inline QVariantMap toVariant(const Info &info)
{
    QVariantList hist;
    for (float value : info.hist) hist.append(double(value));

    QVariantList faces;
    for (const Face &face : info.faces) {
        faces.append(QVariantMap{
            { QStringLiteral("x"), face.x },
            { QStringLiteral("y"), face.y },
            { QStringLiteral("r"), face.r },
            { QStringLiteral("state"), stateName(face.match) },
            { QStringLiteral("model"), face.model },
            { QStringLiteral("certainty"), face.certainty },
        });
    }

    return {
        { QStringLiteral("frameWidth"), info.frameWidth },
        { QStringLiteral("frameHeight"), info.frameHeight },
        { QStringLiteral("dark"), info.dark },
        { QStringLiteral("hist"), hist },
        { QStringLiteral("faces"), faces },
        { QStringLiteral("fps"), info.fps },
        { QStringLiteral("frames"), info.frames },
        { QStringLiteral("recognitionMs"), info.recognitionMs },
    };
}

} // namespace Overlay

#endif // OVERLAY_H
