#include <univalue.h>
#include "validation.h"
#include "crypto/egihash.h"

UniValue getepoch(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 0) {
        throw std::runtime_error("getepoch\n"
                                 "\nReturns current epoch number");
    }
    return static_cast<int>(chainActive.Height() / egihash::constants::EPOCH_LENGTH);
}

UniValue getseedhash(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1) {
        throw std::runtime_error("getseedhash\n \"epoch\" "
                                 "\nReturns the hex encoded seedhash for specified epoch n if n is not specified for current epoch"
                                 "\nArguments"
                                 "\n\"epoch\" the epoch number");
    }
    int epoch = 0;
    if (params[0].isNull()) {
        epoch = static_cast<int>(chainActive.Height() / egihash::constants::EPOCH_LENGTH);
    } else {
        try {
            epoch = std::stoi(params[0].get_str());
        } catch (const std::invalid_argument& ex) {
            throw std::runtime_error("provided argument \'" + params[0].get_str() + "\' is not an integer");
        }
    }
    auto result = egihash::dag_t::getseedhash(epoch);
    if (result.first) {
        return result.second;
    }
    throw std::runtime_error("specified epoch \'" + params[0].get_str() + "\' does not found");
}

UniValue getdagsize(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1) {
        throw std::runtime_error("getdagsize\n \"epoch\" "
                                 "\nReturns the size of the DAG in bytes for the specified epoch n or the current epoch if n is not specified"
                                 "\nArguments"
                                 "\n\"epoch\" the epoch number");
    }
    int epoch = 0;
    if (params[0].isNull()) {
        epoch = static_cast<int>(chainActive.Height() / egihash::constants::EPOCH_LENGTH);
    } else {
        try {
            epoch = std::stoi(params[0].get_str());
        } catch (const std::invalid_argument& ex) {
            throw std::runtime_error("provided argument \'" + params[0].get_str() + "\' is not an integer");
        }
    }
    auto result = egihash::dag_t::getdagsize(epoch);
    if (result.first) {
        return result.second;
    }
    throw std::runtime_error("specified epoch \'" + params[0].get_str() + "\' does not found");
}

UniValue getdagcachesize(const UniValue& params, bool fHelp)
{
    if (fHelp || params.size() > 1) {
        throw std::runtime_error("getdagcachesize\n \"epoch\" "
                                 "\nReturns the size of the DAG chache in bytes for the specified epoch n or the current epoch if n is not specified"
                                 "\nArguments"
                                 "\n\"epoch\" the epoch number");
    }
    int epoch = 0;
    if (params[0].isNull()) {
        epoch = static_cast<int>(chainActive.Height() / egihash::constants::EPOCH_LENGTH);
    } else {
        try {
            epoch = std::stoi(params[0].get_str());
        } catch (const std::invalid_argument& ex) {
            throw std::runtime_error("provided argument \'" + params[0].get_str() + "\' is not an integer");
        }
    }
    auto result = egihash::dag_t::getseedhash(epoch);
    if (result.first) {
        return result.second;
    }
    throw std::runtime_error("specified epoch \'" + params[0].get_str() + "\' does not found");
}


