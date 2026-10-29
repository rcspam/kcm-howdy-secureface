/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef RECOGNITIONPACER_H
#define RECOGNITIONPACER_H

#include <QtGlobal>

// Spaces out the face recognitions of the preview. Each one keeps a core busy
// for its whole duration, so after one that took T ms the next waits 2T:
// about a third of a core, however fast the machine is. The frames in between
// are still shown, with the faces of the last recognition.
class RecognitionPacer
{
public:
    // times in milliseconds, from a monotonic clock
    bool due(qint64 now) const
    {
        return now >= m_next;
    }

    void done(qint64 start, qint64 end)
    {
        m_next = end + (end - start) * IDLE_FACTOR;
    }

private:
    static constexpr qint64 IDLE_FACTOR = 2;
    qint64 m_next = 0;
};

#endif // RECOGNITIONPACER_H
