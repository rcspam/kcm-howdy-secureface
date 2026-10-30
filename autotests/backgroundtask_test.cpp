/*
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include <QTest>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include "previewproducer/backgroundtask.h"

using namespace std::chrono_literals;

class BackgroundTaskTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsIdle()
    {
        BackgroundTask<int> task;
        QVERIFY(!task.busy());
        QVERIFY(!task.takeIfDone());
    }

    void givesNothingWhileTheJobRuns()
    {
        BackgroundTask<int> task;
        std::promise<void> release;
        std::shared_future<void> released = release.get_future().share();
        task.start([released] {
            released.wait();
            return 42;
        });
        QVERIFY(task.busy());
        QVERIFY(!task.takeIfDone());
        QVERIFY(task.busy());
        release.set_value();
    }

    void handsTheResultOverOnce()
    {
        BackgroundTask<int> task;
        task.start([] { return 42; });
        std::optional<int> result;
        for (int i = 0; i < 200 && !result; ++i) {
            result = task.takeIfDone();
            if (!result) std::this_thread::sleep_for(5ms);
        }
        QCOMPARE(result, std::optional<int>(42));
        QVERIFY(!task.busy());
        QVERIFY(!task.takeIfDone());
    }

    void runsOnAnotherThread()
    {
        BackgroundTask<std::thread::id> task;
        task.start([] { return std::this_thread::get_id(); });
        std::optional<std::thread::id> worker;
        for (int i = 0; i < 200 && !worker; ++i) {
            worker = task.takeIfDone();
            if (!worker) std::this_thread::sleep_for(5ms);
        }
        QVERIFY(worker);
        QVERIFY(*worker != std::this_thread::get_id());
    }

    void waitsForTheJobWhenDestroyed()
    {
        // the job may use objects of the caller that die right after the task
        std::atomic<bool> finished = false;
        {
            BackgroundTask<int> task;
            task.start([&finished] {
                std::this_thread::sleep_for(50ms);
                finished = true;
                return 0;
            });
        }
        QVERIFY(finished);
    }
};

QTEST_GUILESS_MAIN(BackgroundTaskTest)

#include "backgroundtask_test.moc"
