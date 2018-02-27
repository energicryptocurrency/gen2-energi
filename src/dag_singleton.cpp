// Copyright (c) 2017 Energi Development Team
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dag_singleton.h"

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

std::unique_ptr<egihash::dag_t> const & ActiveDAG(std::unique_ptr<egihash::dag_t> next_dag, bool reset)
{
    using namespace std;

    static boost::mutex m;
    boost::lock_guard<boost::mutex> lock(m);
    static unique_ptr<egihash::dag_t> active; // only keep one DAG in memory at once

    if (reset && active)
    {
        active->unload();
        active.reset();
    }

    // if we have a next_dag swap it
    if (next_dag)
    {
        auto const previous_epoch = active ? active->epoch() : 0;
        auto const new_epoch = next_dag->epoch();

        active.swap(next_dag);
        if (new_epoch != previous_epoch) LogPrint("nrghash", "DAG swapped to new epoch (%d->%d)\n", previous_epoch, new_epoch);
        else LogPrint("nrghash", "DAG activated for epoch %d\n", new_epoch);

        if (next_dag) {
            next_dag->unload();
            next_dag.reset();
        }
        LogPrint("nrghash", "DAG for epoch %d unloaded\n", previous_epoch);
    }

    return active;
}
