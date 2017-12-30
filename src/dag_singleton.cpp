// Copyright (c) 2017 Energi Development Team
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "dag_singleton.h"
#include "crypto/egihash.h"
#include "uint256.h"

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include <univalue.h>

std::unique_ptr<egihash::dag_t> const & ActiveDAG(std::unique_ptr<egihash::dag_t> next_dag)
{
    using namespace std;

    static boost::mutex m;
    boost::lock_guard<boost::mutex> lock(m);
    static unique_ptr<egihash::dag_t> active; // only keep one DAG in memory at once

    // if we have a next_dag swap it
    if (next_dag)
    {
        active.swap(next_dag);
    }

    // unload the previous dag
    if (next_dag)
    {
        next_dag->unload();
        next_dag.reset();
    }

    return active;
}

UniValue getactivedag(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1) {
        throw std::runtime_error("getactivedag\n"
                                 "\nReturns a JSON list specifying loaded DAG");
    }
    const auto& dag = ActiveDAG();
    if (dag == nullptr) {
        throw std::runtime_error("there is no active dag");
    }
    using namespace egihash;
    UniValue result(UniValue::VOBJ);
    result.push_back(Pair("size", dag->size()));
    result.push_back(Pair("epoch", dag->epoch()));
    std::string seedhash = get_seedhash(dag->epoch()* constants::EPOCH_LENGTH);
    result.push_back(Pair("seedhash", seedhash.empty() ? "0" : uint256S(seedhash).GetHex()));
    return std::move(result);
}
