// Copyright (c) 2020 The Energi Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "init.h"
#include "rpc/protocol.h"
#include "wallet/gen3migrate.h"

#include <algorithm>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>

extern "C"
{
#   include "crypto/keccak-tiny.h"
}

// A dirty have to avoid multiple definitions error due to how
// that functionality was designed.
// NOTE: could be anonymous, if that make predictable binaries
namespace gen3migrate {
    #include "support/events.h"
}

using namespace gen3migrate;

const char GEN3_NODEAPI_MAINNET[] = "nodeapi.gen3.energi.network";
const char GEN3_NODEAPI_TESTNET[] = "nodeapi.test3.energi.network";
const size_t GEN3_NODEAPI_SEARCH_MAX = 1000;

struct Gen3Migrate::Impl {
    struct HTTPReply
    {
        HTTPReply(): status(0), error(-1) {}

        int status;
        int error;
        std::string body;
    };

    static const char *http_errorstring(int code);
    static void http_request_done(struct evhttp_request *req, void *ctx);
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    static void http_error_cb(enum evhttp_request_error err, void *ctx);
#endif

    UniValue Call(const std::string& method, UniValue params, int timeout_sec=30);

    raii_event_base evbase_;
    raii_evhttp_connection evcon_;
    std::string host_;
};

bool Gen3Migrate::s_testMode = false;

void Gen3Migrate::SetTestMode(bool enable) {
    s_testMode = enable;
}

bool Gen3Migrate::IsMainnet() {
    return (Params().NetworkIDString() == CBaseChainParams::MAIN) && !s_testMode;
}


Gen3Migrate::Gen3Migrate(CWallet &wallet) :
    wallet_(wallet),
    impl_(new Impl)
{
    impl_->evbase_ = obtain_event_base();
    impl_->host_ = IsMainnet()
        ? GEN3_NODEAPI_MAINNET
        : GEN3_NODEAPI_TESTNET;

    // NOTE: bundled libevent is built without openssl
    //impl_->evcon_ = obtain_evhttp_connection_base(impl_->evbase_.get(), impl_->host_, 80);
}

// NOTE: workarounds std::unique_ptr with private Impl
Gen3Migrate::~Gen3Migrate() {}

void Gen3Migrate::Migrate(std::string gen3_account, bool dry_run)
try
{
    // NOTE: this check can be better...
    if (gen3_account.size() != 42 || gen3_account.substr(0, 2) != "0x") {
        throw std::runtime_error("Invalid Gen 3 address");
    }

    // Check if migration has started
    {
        UniValue params(UniValue::VARR);
        params.push_back("0x1");
        params.push_back(false);

        if (impl_->Call("eth_getBlockByNumber", params).isNull()) {
            throw std::runtime_error("Gen 3 migration has not started yet!");
        }
    }

    LOCK2(cs_main, wallet_.cs_wallet);

    LogPrintf("Gen3Migrate: starting migration to %s\n", gen3_account.c_str());

    std::map<CTxDestination, int64_t> mapKeyBirth;
    wallet_.GetKeyBirthTimes(mapKeyBirth);
    std::vector<UniValue> coin_items;

    while (!mapKeyBirth.empty()) {
        UniValue search_params(UniValue::VARR);

        {
            UniValue local_addresses(UniValue::VARR);

            for (auto i = GEN3_NODEAPI_SEARCH_MAX; i > 0 && !mapKeyBirth.empty(); --i) {
                auto citer = mapKeyBirth.begin();

                if (const CKeyID* pk = boost::get<CKeyID>(&citer->first)) { // set and test
                    auto key_id = *pk;
                    std::reverse(key_id.begin(), key_id.end());
                    auto addr = std::string("0x") + key_id.GetHex();
                    local_addresses.push_back(addr);
                    LogPrintf("Gen3Migrate: found addr %s\n", addr.c_str());
                }

                mapKeyBirth.erase(citer);
            }

            search_params.push_back(local_addresses);
            search_params.push_back(false);

            LogPrintf("Gen3Migrate: calling search with %llu addresses\n", local_addresses.size());
        }

        auto res = impl_->Call("energi_searchRawGen2Coins", search_params, 300).getValues();
        coin_items.insert(
            coin_items.end(),
            std::make_move_iterator(res.begin()),
            std::make_move_iterator(res.end())
        );

        LogPrintf("Gen3Migrate: search found %llu addresses\n", res.size());
    }

    auto packed_dst = std::string(24, '0') + gen3_account.substr(2);
    auto contract = "0x0000000000000000000000000000000000000308";
    auto ephemeral = "0x0000000000000000000000457068656d6572616c";
    auto block = "pending";
    auto gas = "0x7A120";
    auto gasPrice = "0x0";

    std::string hash;
    {
        // hashToSig()
        std::string txdata = "0x0a96cb49" + packed_dst;

        LogPrintf("Gen3Migrate: hashToSig %s\n", txdata.c_str());

        UniValue tx(UniValue::VOBJ);
        tx.push_back(Pair("to", contract));
        tx.push_back(Pair("data", txdata));

        UniValue params(UniValue::VARR);
        params.push_back(tx);
        params.push_back(block);

        hash = impl_->Call("eth_call", params).get_str().substr(2);

        // NOTE: this is required to workaround MitM with plain HTTP
        uint160 tmp_addr;
        tmp_addr.SetHex(gen3_account.substr(2));
        std::reverse(tmp_addr.begin(), tmp_addr.end());

        uint8_t msg_to_sig[20+32+32];
        uint8_t *pos = msg_to_sig;
        memcpy(pos, tmp_addr.begin(), tmp_addr.size());
        pos += tmp_addr.size();
        memcpy(pos, "||Energi Gen 2 migration claim||", 32);
        pos += 32;
        memset(pos, 0, 30);
        pos += 30;

        if (IsMainnet()) {
            *(pos++) = 0x9B;
            *(pos++) = 0x75;
        } else {
            *(pos++) = 0xC2;
            *(pos++) = 0x85;
        }
        assert(sizeof(msg_to_sig) == (pos - msg_to_sig));

        uint256 raw_hash;
        sha3_256(raw_hash.begin(), 32, msg_to_sig, (pos - msg_to_sig));
        std::reverse(raw_hash.begin(), raw_hash.end());
        auto req_hash = raw_hash.GetHex();

        if (hash != req_hash) {
            LogPrintf("Gen3Migrate: hash mismatch %s != %s\n",
                          hash.c_str(), req_hash.c_str());
            throw std::runtime_error("MitM attack on hashToSig()");
        }
    }

    for (auto &c : coin_items)
    {
        if (ShutdownRequested()) {
            return;
        }

        auto raw_owner = find_value(c, "RawOwner").get_str().substr(2);
        char dst_item[64+1] = {};
        snprintf(dst_item, sizeof(dst_item), "%064x", find_value(c, "ItemID").get_int());

        LogPrintf("Gen3Migrate: processing item %s owner %s\n", dst_item, raw_owner.c_str());

        //---
        CKeyID key_id;
        key_id.SetHex(raw_owner);
        std::reverse(key_id.begin(), key_id.end());

        CKey key;
        if (!wallet_.GetKey(key_id, key)) {
            throw std::runtime_error("Failed to load key");
        }

        std::vector<unsigned char> sig;
        auto hash_uint256 = uint256S(hash);
        std::reverse(hash_uint256.begin(), hash_uint256.end());
        if (!key.SignCompact(hash_uint256, sig)) {
            throw std::runtime_error("Failed to sign");
        }

        char sig_v[64+1] = {};
        char sig_r[64+1+4] = {};
        char sig_s[64+1+4] = {};
        snprintf(sig_v, sizeof(sig_v), "%064x", int((sig[0]-27)%4));
        for (size_t i = 0; i < 32; ++i) {
            snprintf(sig_r + (i*2), 2+4, "%02x", int(sig[1+i]));
            snprintf(sig_s + (i*2), 2+4, "%02x", int(sig[1+32+i]));
        }

        //---
        if (dry_run)
        {
            // verifyClaim(_item_id, _destination, sig_v, sig_r, sig_s)
            // 476ce0c3
            // 0000000000000000000000000000000000000000000000000000000000000001
            // 0000000000000000000000000000000000000000000000000000000000000002
            // 0000000000000000000000000000000000000000000000000000000000000003
            // 0000000000000000000000000000000000000000000000000000000000000004
            // 0000000000000000000000000000000000000000000000000000000000000005
            std::string txdata = std::string("0x476ce0c3") + dst_item + packed_dst + sig_v + sig_r + sig_s;

            LogPrintf("Gen3Migrate: verifyClaim %s\n", txdata.c_str());

            UniValue tx(UniValue::VOBJ);
            tx.push_back(Pair("to", contract));
            tx.push_back(Pair("data", txdata));

            UniValue params(UniValue::VARR);
            params.push_back(tx);
            params.push_back(block);

            auto amount = impl_->Call("eth_call", params).get_str();
            auto orig_amount = find_value(c, "Amount").get_str();

            if (amount.substr(amount.length() - orig_amount.length() + 2) != orig_amount.substr(2)) {
                LogPrintf("Gen3Migrate: amount mismatch %s != %s\n",
                          amount.c_str(), orig_amount.c_str());
            }
        } else {
            // claim(_item_id, _destination, sig_v, sig_r, sig_s)
            // f7121490
            // 0000000000000000000000000000000000000000000000000000000000000001
            // 0000000000000000000000000000000000000000000000000000000000000002
            // 0000000000000000000000000000000000000000000000000000000000000003
            // 0000000000000000000000000000000000000000000000000000000000000004
            // 0000000000000000000000000000000000000000000000000000000000000005
            std::string txdata = std::string("0xf7121490") + dst_item + packed_dst + sig_v + sig_r + sig_s;

            LogPrintf("Gen3Migrate: claim %s\n", txdata.c_str());

            UniValue tx(UniValue::VOBJ);
            tx.push_back(Pair("from", ephemeral));
            tx.push_back(Pair("to", contract));
            tx.push_back(Pair("data", txdata));
            tx.push_back(Pair("gas", gas));
            tx.push_back(Pair("gasPrice", gasPrice));

            UniValue params(UniValue::VARR);
            params.push_back(tx);

            auto tx_id = impl_->Call("eth_sendTransaction", params).get_str();
            LogPrintf("Gen3Migrate: migration TX %s\n", tx_id.c_str());
        }
    }

    LogPrintf("Gen3Migrate: ended\n");
}
catch(std::runtime_error &e)
{
    OnError(e.what());
    throw;
}

void Gen3Migrate::OnError(const std::string& err)
{
    LogPrintf("Gen3Migrate: %s\n", err.c_str());
}

UniValue Gen3Migrate::Impl::Call(const std::string& method, UniValue params, int timeout_sec)
{
    // Prevent possible rate limit
    MilliSleep(150);

    // NOTE: bundled libevent is built without openssl
    // NOTE: some strange wait-until-timeout behavior is observed with connection re-use
    evcon_ = obtain_evhttp_connection_base(evbase_.get(), host_, 80);

    evhttp_connection_set_timeout(evcon_.get(), timeout_sec);

    HTTPReply response;
    raii_evhttp_request req = obtain_evhttp_request(http_request_done, (void*)&response);
    if (req == NULL)
        throw std::runtime_error("create http request failed");
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    evhttp_request_set_error_cb(req.get(), http_error_cb);
#endif


    struct evkeyvalq* output_headers = evhttp_request_get_output_headers(req.get());
    assert(output_headers);
    evhttp_add_header(output_headers, "Host", host_.c_str());
    evhttp_add_header(output_headers, "Connection", "close");
    evhttp_add_header(output_headers, "Content-Type", "application/json");

    // Attach request data
    std::string strRequest = JSONRPCRequestObj(method, params, 1).write() + "\n";
    struct evbuffer* output_buffer = evhttp_request_get_output_buffer(req.get());
    assert(output_buffer);
    evbuffer_add(output_buffer, strRequest.data(), strRequest.size());

    int r = evhttp_make_request(evcon_.get(), req.get(), EVHTTP_REQ_POST, "/");
    req.release(); // ownership moved to evcon in above call
    if (r != 0) {
        throw std::runtime_error("send http request failed");
    }

    event_base_dispatch(evbase_.get());

    if (response.status == 0)
        throw std::runtime_error(strprintf("couldn't connect to server: %s (code %d)\n(make sure server is running and you are connecting to the correct RPC port)", http_errorstring(response.error), response.error));
    else if (response.status >= 400 && response.status != HTTP_BAD_REQUEST && response.status != HTTP_NOT_FOUND && response.status != HTTP_INTERNAL_SERVER_ERROR)
        throw std::runtime_error(strprintf("server returned HTTP error %d", response.status));
    else if (response.body.empty())
        throw std::runtime_error("no response from server");

    // Parse reply
    UniValue valReply(UniValue::VSTR);
    if (!valReply.read(response.body))
        throw std::runtime_error("couldn't parse reply from server");
    const UniValue& reply = valReply.get_obj();
    if (reply.empty())
        throw std::runtime_error("expected reply to have result, error and id properties");

    if (reply.exists("error")) {
        throw std::runtime_error(find_value(find_value(reply, "error"), "message").get_str());
    }

    return find_value(reply, "result");
}


// Based on energi-cli.cpp

const char* Gen3Migrate::Impl::http_errorstring(int code)
{
    switch(code) {
#if LIBEVENT_VERSION_NUMBER >= 0x02010300
    case EVREQ_HTTP_TIMEOUT:
        return "timeout reached";
    case EVREQ_HTTP_EOF:
        return "EOF reached";
    case EVREQ_HTTP_INVALID_HEADER:
        return "error while reading header, or invalid header";
    case EVREQ_HTTP_BUFFER_ERROR:
        return "error encountered while reading or writing";
    case EVREQ_HTTP_REQUEST_CANCEL:
        return "request was canceled";
    case EVREQ_HTTP_DATA_TOO_LONG:
        return "response body is larger than allowed";
#endif
    default:
        return "unknown";
    }
}

void Gen3Migrate::Impl::http_request_done(struct evhttp_request *req, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);

    if (req == NULL) {
        /* If req is NULL, it means an error occurred while connecting: the
         * error code will have been passed to http_error_cb.
         */
        reply->status = 0;
        return;
    }

    reply->status = evhttp_request_get_response_code(req);

    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    if (buf)
    {
        size_t size = evbuffer_get_length(buf);
        const char *data = (const char*)evbuffer_pullup(buf, size);
        if (data)
            reply->body = std::string(data, size);
        evbuffer_drain(buf, size);
    }
}

#if LIBEVENT_VERSION_NUMBER >= 0x02010300
void Gen3Migrate::Impl::http_error_cb(enum evhttp_request_error err, void *ctx)
{
    HTTPReply *reply = static_cast<HTTPReply*>(ctx);
    reply->error = err;
}
#endif
