// Copyright (c) 2017 Energi Development Team
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dag_singleton.h"

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

std::unique_ptr<egihash::dag_t> const & ActiveDAG(std::unique_ptr<egihash::dag_t> next_dag)
{
    using namespace std;

    static boost::mutex m;
    boost::lock_guard<boost::mutex> lock(m);
    static unique_ptr<egihash::dag_t> active; // only keep one DAG in memory at once

    // if we have a next_dag swap it
    if (next_dag)
    {
        auto const previous_epoch = active ? active->epoch() : 0;
        auto const new_epoch = next_dag->epoch();
        // unload the previous dag
        if (active) {
            active->unload();
        }
        // load the dag first if generated in low memory mode
        next_dag->load();

        // swap
        active.swap(next_dag);
        next_dag.reset();
        if (new_epoch != previous_epoch) LogPrint("nrghash", "DAG swapped to new epoch (%d->%d)\n", previous_epoch, new_epoch);
        else LogPrint("nrghash", "DAG activated for epoch %d\n", new_epoch);
    }

    return active;
}
