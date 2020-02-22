// Copyright (c) 2020 The Energi Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_GEN3MIGRATE_H
#define BITCOIN_WALLET_GEN3MIGRATE_H

#include "wallet/wallet.h"

#include <string>
#include <univalue.h>

class Gen3Migrate {
public:
    Gen3Migrate(CWallet &wallet);
    virtual ~Gen3Migrate();
    void Migrate(std::string gen3_account, bool dry_run);

protected:
    virtual void OnError(const std::string&);

private:
    CWallet &wallet_;

    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif // BITCOIN_WALLET_GEN3MIGRATE_H
