/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifndef BACKGROUNDTASK_H
#define BACKGROUNDTASK_H

#include <chrono>
#include <future>
#include <optional>
#include <utility>

// Runs one job at a time on its own thread and hands the result back to the
// loop that started it, which never waits: the preview keeps reading frames
// while a face recognition runs. Destroying the task waits for the job, which
// may use objects of the loop.
template<typename Result>
class BackgroundTask
{
public:
    bool busy() const
    {
        return m_future.valid();
    }

    // only when not busy
    template<typename Job>
    void start(Job &&job)
    {
        m_future = std::async(std::launch::async, std::forward<Job>(job));
    }

    // the result of the job once it is done, a single time
    std::optional<Result> takeIfDone()
    {
        if (!busy() || m_future.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
            return std::nullopt;
        }
        return m_future.get();
    }

private:
    std::future<Result> m_future;
};

#endif // BACKGROUNDTASK_H
