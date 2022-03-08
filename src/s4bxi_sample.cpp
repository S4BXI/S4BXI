/*
 * Author: Julien EMMANUEL
 * Copyright (C) 2019-2022 Bull S.A.S
 * All rights reserved
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation,
 * which comes with this package.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#include <xbt/base.h>

#include "s4bxi/s4bxi_sample.h"
#include "s4bxi/s4bxi_bench.hpp"
#include "s4bxi/s4bxi_xbt_log.h"
#include "s4bxi/s4bxi_util.hpp"

S4BXI_LOG_NEW_DEFAULT_CATEGORY(s4bxi_sample, "Messages specific to sampling")

/* ****************************** Functions related to the S4BXI_SAMPLE_ macros ************************************/
namespace {
class SampleLocation : public std::string {
  public:
    SampleLocation(bool global, const char* file, int line)
        : std::string(std::string(file) + ":" + std::to_string(line))
    {
        if (not global)
            this->append(":" + std::to_string(simgrid::s4u::this_actor::get_pid()));
    }
};

class LocalData {
  public:
    double threshold; /* maximal stderr requested (if positive) */
    double relstderr; /* observed stderr so far */
    double mean;      /* mean of benched times, to be used if the block is disabled */
    double sum;       /* sum of benched times (to compute the mean and stderr) */
    double sum_pow2;  /* sum of the square of the benched times (to compute the stderr) */
    int iters;        /* amount of requested iterations */
    int count;        /* amount of iterations done so far */
    bool benching;    /* true: we are benchmarking; false: we have enough data, no bench anymore */

    bool need_more_benchs() const;
};

bool LocalData::need_more_benchs() const
{
    bool res = (count < iters) || (threshold > 0.0 && (count < 2 ||          // not enough data
                                                       relstderr > threshold // stderr too high yet
                                                       ));
    XBT_DEBUG("%s (count:%d iter:%d stderr:%f thres:%f mean:%fs)", (res ? "need more data" : "enough benchs"), count,
              iters, relstderr, threshold, mean);
    return res;
}

std::unordered_map<SampleLocation, LocalData, std::hash<std::string>> samples;
} // namespace

void s4bxi_sample_1(int global, const char* file, int line, int iters, double threshold)
{
    BxiMainActor* mainActor = GET_CURRENT_MAIN_ACTOR;
    SampleLocation loc(global, file, line);
    if (not mainActor->sampling()) { /* Only at first call when benchmarking, skip for next ones */
        s4bxi_bench_end();           /* Take time from previous, unrelated computation into account */
        mainActor->set_sampling(1);
    }

    auto insert = samples.emplace(loc, LocalData{
                                           threshold, // threshold
                                           0.0,       // relstderr
                                           0.0,       // mean
                                           0.0,       // sum
                                           0.0,       // sum_pow2
                                           iters,     // iters
                                           0,         // count
                                           true       // benching (if we have no data, we need at least one)
                                       });
    if (insert.second) {
        XBT_DEBUG("XXXXX First time ever on benched nest %s.", loc.c_str());
        xbt_assert(threshold > 0 || iters > 0, "You should provide either a positive amount of iterations to bench, or "
                                               "a positive maximal stderr (or both)");
    } else {
        LocalData& data = insert.first->second;
        if (data.iters != iters || data.threshold != threshold) {
            XBT_ERROR("Asked to bench block %s with different settings %d, %f is not %d, %f. "
                      "How did you manage to give two numbers at the same line??",
                      loc.c_str(), data.iters, data.threshold, iters, threshold);
            ptl_panic("sampling error");
        }

        // if we already have some data, check whether sample_2 should get one more bench or whether it should emulate
        // the computation instead
        data.benching = data.need_more_benchs();
        XBT_DEBUG("XXXX Re-entering the benched nest %s. %s", loc.c_str(),
                  (data.benching ? "more benching needed" : "we have enough data, skip computes"));
    }
}

int s4bxi_sample_2(int global, const char* file, int line, int iter_count)
{
    BxiMainActor* mainActor = GET_CURRENT_MAIN_ACTOR;
    SampleLocation loc(global, file, line);

    XBT_DEBUG("sample2 %s %d", loc.c_str(), iter_count);
    auto sample = samples.find(loc);
    if (sample == samples.end())
        xbt_die("Y U NO use S4BXI_SAMPLE_* macros? Stop messing directly with s4bxi_sample_* functions!");
    const LocalData& data = sample->second;

    if (data.benching) {
        // we need to run a new bench
        XBT_DEBUG("benchmarking: count:%d iter:%d stderr:%f thres:%f; mean:%f; total:%f", data.count, data.iters,
                  data.relstderr, data.threshold, data.mean, data.sum);
        s4bxi_bench_begin();
    } else {
        // Enough data, no more bench (either we got enough data from previous visits to this benched nest, or we just
        // ran one bench and need to bail out now that our job is done). Just sleep instead
        if (not data.need_more_benchs()) {
            XBT_DEBUG(
                "No benchmark (either no need, or just ran one): count >= iter (%d >= %d) or stderr<thres (%f<=%f). "
                "Mean is %f, will be injected %d times",
                data.count, data.iters, data.relstderr, data.threshold, data.mean, iter_count);

            // we ended benchmarking, let's inject all the time, now, and fast forward out of the loop.
            mainActor->set_sampling(0);
            s4bxi_execute(data.mean * iter_count);
            s4bxi_bench_begin();
            return 0;
        } else {
            XBT_DEBUG("Skipping - Benchmark already performed - accumulating time");
            xbt_os_threadtimer_start(mainActor->timer);
        }
    }
    return 1;
}

void s4bxi_sample_3(int global, const char* file, int line)
{
    BxiMainActor* mainActor = GET_CURRENT_MAIN_ACTOR;
    SampleLocation loc(global, file, line);

    XBT_DEBUG("sample3 %s", loc.c_str());
    auto sample = samples.find(loc);
    if (sample == samples.end())
        xbt_die("Y U NO use S4BXI_SAMPLE_* macros? Stop messing directly with s4bxi_sample_* functions!");
    LocalData& data = sample->second;

    if (not data.benching)
        THROW_IMPOSSIBLE;

    // ok, benchmarking this loop is over
    xbt_os_threadtimer_stop(mainActor->timer);

    // update the stats
    data.count++;
    double period = xbt_os_timer_elapsed(mainActor->timer);
    data.sum += period;
    data.sum_pow2 += period * period;
    double n       = data.count;
    data.mean      = data.sum / n;
    data.relstderr = sqrt((data.sum_pow2 / n - data.mean * data.mean) / n) / data.mean;

    XBT_DEBUG("Average mean after %d steps is %f, relative standard error is %f (sample was %f)", data.count, data.mean,
              data.relstderr, period);

    // That's enough for now, prevent sample_2 to run the same code over and over
    data.benching = false;
}

int s4bxi_sample_exit(int global, const char* file, int line, int iter_count)
{
    BxiMainActor* mainActor = GET_CURRENT_MAIN_ACTOR;
    if (mainActor->sampling()) {
        SampleLocation loc(global, file, line);

        XBT_DEBUG("sample exit %s", loc.c_str());
        auto sample = samples.find(loc);
        if (sample == samples.end())
            xbt_die("Y U NO use S4BXI_SAMPLE_* macros? Stop messing directly with s4bxi_sample_* functions!");

        if (mainActor->sampling()) { // end of loop, but still sampling needed
            const LocalData& data = sample->second;
            mainActor->set_sampling(0);
            s4bxi_execute(data.mean * iter_count);
            s4bxi_bench_begin();
        }
    }
    return 0;
}