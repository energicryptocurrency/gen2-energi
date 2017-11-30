// Copyright (c) 2017 Energi Development Team
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENERGI_DAG_SINGLETON_H
#define ENERGI_DAG_SINGLETON_H

#include "crypto/egihash.h"

#include <memory>

/** \brief Get the currently loaded DAG.
*
*	Note that this function is both a getter and a setter function.
*	If no parameters are specified, or operator bool(next_dag) == false, return the currently active DAG
*	If a valid next_dag is specified, swap the active DAG with next_dag and return the new active DAG (unloads the previous DAG)
*
*	\param next_dag (optional) swap the active DAG with next_dag
*	\returns A unique_ptr to the currently active DAG, or a null unique_ptr if no DAG is active.
*/
std::unique_ptr<egihash::dag_t> const & ActiveDAG(std::unique_ptr<egihash::dag_t> next_dag = std::unique_ptr<egihash::dag_t>());

#endif
