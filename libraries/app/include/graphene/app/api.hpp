/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#pragma once

#include <graphene/app/database_api.hpp>

#include <graphene/chain/protocol/confidential.hpp>
#include <graphene/chain/protocol/types.hpp>

#include <graphene/net/node.hpp>

#include <graphene/accounts_list/accounts_list_plugin.hpp>
#include <graphene/affiliate_stats/affiliate_stats_api.hpp>
#include <graphene/bookie/bookie_api.hpp>
#include <graphene/debug_witness/debug_api.hpp>
#include <graphene/elasticsearch/elasticsearch_plugin.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
#include <graphene/peerplays_sidechain/sidechain_api.hpp>

#include <fc/api.hpp>
#include <fc/crypto/elliptic.hpp>
#include <fc/network/ip.hpp>
#include <fc/optional.hpp>

#include <boost/container/flat_set.hpp>

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace graphene { namespace app {
using namespace graphene::chain;
using namespace graphene::market_history;
using namespace graphene::accounts_list;
using namespace fc::ecc;
using namespace std;

class application;

struct verify_range_result {
   bool success;
   uint64_t min_val;
   uint64_t max_val;
};

struct verify_range_proof_rewind_result {
   bool success;
   uint64_t min_val;
   uint64_t max_val;
   uint64_t value_out;
   fc::ecc::blind_factor_type blind_out;
   string message_out;
};

struct account_asset_balance {
   string name;
   account_id_type account_id;
   share_type amount;
};
struct asset_holders {
   asset_id_type asset_id;
   int count;
};

/**
    * @brief The history_api class implements the RPC API for account history
    *
    * This API contains methods to access account histories
    */
class history_api {
public:
   history_api(application &app) :
         _app(app),
         database_api(std::ref(*app.chain_database())) {
   }

   /**
          * @brief Get operations relevant to the specificed account
          * @param account_id_or_name The account ID or name whose history should be queried
          * @param stop ID of the earliest operation to retrieve
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start ID of the most recent operation to retrieve
          * @return A list of operations performed by account, ordered from most recent to oldest.
          */
   vector<operation_history_object> get_account_history(const std::string account_id_or_name,
                                                        operation_history_id_type stop = operation_history_id_type(),
                                                        unsigned limit = 100,
                                                        operation_history_id_type start = operation_history_id_type()) const;

   /**
          * @brief Get only asked operations relevant to the specified account
          * @param account_id_or_name The account ID or name whose history should be queried
          * @param operation_id The ID of the operation we want to get operations in the account( 0 = transfer , 1 = limit order create, ...)
          * @param stop ID of the earliest operation to retrieve
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start ID of the most recent operation to retrieve
          * @return A list of operations performed by account, ordered from most recent to oldest.
          */
   vector<operation_history_object> get_account_history_operations(const std::string account_id_or_name,
                                                                   int operation_id,
                                                                   operation_history_id_type start = operation_history_id_type(),
                                                                   operation_history_id_type stop = operation_history_id_type(),
                                                                   unsigned limit = 100) const;

   /**
          * @breif Get operations relevant to the specified account referenced
          * by an event numbering specific to the account. The current number of operations
          * for the account can be found in the account statistics (or use 0 for start).
          * @param account_id_or_name The account ID or name whose history should be queried
          * @param stop Sequence number of earliest operation. 0 is default and will
          * query 'limit' number of operations.
          * @param limit Maximum number of operations to retrieve (must not exceed 100)
          * @param start Sequence number of the most recent operation to retrieve.
          * 0 is default, which will start querying from the most recent operation.
          * @return A list of operations performed by account, ordered from most recent to oldest.
          */
   vector<operation_history_object> get_relative_account_history(const std::string account_id_or_name,
                                                                 uint32_t stop = 0,
                                                                 unsigned limit = 100,
                                                                 uint32_t start = 0) const;

   vector<order_history_object> get_fill_order_history(std::string asset_a, std::string asset_b, uint32_t limit) const;
   vector<bucket_object> get_market_history(std::string asset_a, std::string asset_b, uint32_t bucket_seconds,
                                            fc::time_point_sec start, fc::time_point_sec end) const;
   vector<account_balance_object> list_core_accounts() const;
   flat_set<uint32_t> get_market_history_buckets() const;
   uint32_t api_limit_get_account_history_operations = 100;
   uint32_t api_limit_get_account_history = 100;
   uint32_t api_limit_get_relative_account_history = 100;

private:
   application &_app;
   graphene::app::database_api database_api;
};

/**
    * @brief Block api
    */
class block_api {
public:
   block_api(graphene::chain::database &db);
   ~block_api();

   vector<optional<signed_block>> get_blocks(uint32_t block_num_from, uint32_t block_num_to) const;

private:
   graphene::chain::database &_db;
};

/**
    * @brief The network_broadcast_api class allows broadcasting of transactions.
    */
class network_broadcast_api : public std::enable_shared_from_this<network_broadcast_api> {
public:
   network_broadcast_api(application &a);

   struct transaction_confirmation {
      transaction_id_type id;
      uint32_t block_num;
      uint32_t trx_num;
      processed_transaction trx;
   };

   typedef std::function<void(variant /*transaction_confirmation*/)> confirmation_callback;

   /**
          * @brief Broadcast a transaction to the network
          * @param trx The transaction to broadcast
          *
          * The transaction will be checked for validity in the local database prior to broadcasting. If it fails to
          * apply locally, an error will be thrown and the transaction will not be broadcast.
          */
   void broadcast_transaction(const signed_transaction &trx);

   /** this version of broadcast transaction registers a callback method that will be called when the transaction is
          * included into a block.  The callback method includes the transaction id, block number, and transaction number in the
          * block.
          */
   void broadcast_transaction_with_callback(confirmation_callback cb, const signed_transaction &trx);

   /** this version of broadcast transaction registers a callback method that will be called when the transaction is
          * included into a block.  The callback method includes the transaction id, block number, and transaction number in the
          * block.
          */
   fc::variant broadcast_transaction_synchronous(const signed_transaction &trx);

   void broadcast_block(const signed_block &block);

   /**
          * @brief Not reflected, thus not accessible to API clients.
          *
          * This function is registered to receive the applied_block
          * signal from the chain database when a block is received.
          * It then dispatches callbacks to clients who have requested
          * to be notified when a particular txid is included in a block.
          */
   void on_applied_block(const signed_block &b);

private:
   boost::signals2::scoped_connection _applied_block_connection;
   map<transaction_id_type, confirmation_callback> _callbacks;
   application &_app;
};

/**
    * @brief The network_node_api class allows maintenance of p2p connections.
    */
class network_node_api {
public:
   network_node_api(application &a);

   /**
          * @brief Return general network information, such as p2p port
          */
   fc::variant_object get_info() const;

   /**
          * @brief add_node Connect to a new peer
          * @param ep The IP/Port of the peer to connect to
          */
   void add_node(const fc::ip::endpoint &ep);

   /**
          * @brief Get status of all current connections to peers
          */
   std::vector<net::peer_status> get_connected_peers() const;

   /**
          * @brief Get advanced node parameters, such as desired and max
          *        number of connections
          */
   fc::variant_object get_advanced_node_parameters() const;

   /**
          * @brief Set advanced node parameters, such as desired and max
          *        number of connections
          * @param params a JSON object containing the name/value pairs for the parameters to set
          */
   void set_advanced_node_parameters(const fc::variant_object &params);

   /**
          * @brief Return list of potential peers
          */
   std::vector<net::potential_peer_record> get_potential_peers() const;

   /**
          * @brief Return list of pending transactions.
          */
   map<transaction_id_type, signed_transaction> list_pending_transactions() const;

   /**
          * @brief Subscribes caller for notifications about pending transactions.
          * @param callback a functional object which will be called when new transaction is created.
          */
   void subscribe_to_pending_transactions(std::function<void(const variant &)> callback);

   /**
          * @brief Unsubscribes caller from notifications about pending transactions.
          */
   void unsubscribe_from_pending_transactions();

private:
   application &_app;
   map<transaction_id_type, signed_transaction> _pending_transactions;
   boost::signals2::scoped_connection _pending_trx_connection;
   boost::signals2::scoped_connection _applied_block_connection;
   std::function<void(const variant &)> _on_pending_transaction;
};

class crypto_api {
public:
   crypto_api();

   fc::ecc::commitment_type blind(const fc::ecc::blind_factor_type &blind, uint64_t value);

   fc::ecc::blind_factor_type blind_sum(const std::vector<blind_factor_type> &blinds_in, uint32_t non_neg);

   bool verify_sum(const std::vector<commitment_type> &commits_in, const std::vector<commitment_type> &neg_commits_in, int64_t excess);

   verify_range_result verify_range(const fc::ecc::commitment_type &commit, const std::vector<char> &proof);

   std::vector<char> range_proof_sign(uint64_t min_value,
                                      const commitment_type &commit,
                                      const blind_factor_type &commit_blind,
                                      const blind_factor_type &nonce,
                                      int8_t base10_exp,
                                      uint8_t min_bits,
                                      uint64_t actual_value);

   verify_range_proof_rewind_result verify_range_proof_rewind(const blind_factor_type &nonce,
                                                              const fc::ecc::commitment_type &commit,
                                                              const std::vector<char> &proof);

   range_proof_info range_get_info(const std::vector<char> &proof);
};

/**
    * @brief
    */
class asset_api {
public:
   asset_api(graphene::app::application &app);
   ~asset_api();

   /**
          * @brief Get asset holders for a specific asset
          * @param asset The specific asset id or symbol
          * @param start The start index
          * @param limit Maximum limit must not exceed 100
          * @return A list of asset holders for the specified asset
          */
   vector<account_asset_balance> get_asset_holders(std::string asset, uint32_t start, uint32_t limit) const;

   /**
          * @brief Get asset holders count for a specific asset
          * @param asset The specific asset id or symbol
          * @return Holders count for the specified asset
          */
   int get_asset_holders_count(std::string asset) const;

   /**
          * @brief Get all asset holders
          * @return A list of all asset holders
          */
   vector<asset_holders> get_all_asset_holders() const;

   uint32_t api_limit_get_asset_holders = 100;

private:
   graphene::app::application &_app;
   graphene::chain::database &_db;
   graphene::app::database_api database_api;
};
}} // namespace graphene::app

extern template class fc::api<graphene::app::block_api>;
extern template class fc::api<graphene::app::network_broadcast_api>;
extern template class fc::api<graphene::app::network_node_api>;
extern template class fc::api<graphene::app::history_api>;
extern template class fc::api<graphene::app::crypto_api>;
extern template class fc::api<graphene::app::asset_api>;
extern template class fc::api<graphene::debug_witness::debug_api>;

namespace graphene { namespace app {
/**
    * @brief The login_api class implements the bottom layer of the RPC API
    *
    * All other APIs must be requested from this API.
    */
class login_api {
public:
   login_api(application &a);
   ~login_api();

   /**
          * @brief Authenticate to the RPC server
          * @param user Username to login with
          * @param password Password to login with
          * @return True if logged in successfully; false otherwise
          *
          * @note This must be called prior to requesting other APIs. Other APIs may not be accessible until the client
          * has sucessfully authenticated.
          */
   bool login(const string &user, const string &password);
   /// @brief Retrieve the network block API
   fc::api<block_api> block() const;
   /// @brief Retrieve the network broadcast API
   fc::api<network_broadcast_api> network_broadcast() const;
   /// @brief Retrieve the database API
   fc::api<database_api> database() const;
   /// @brief Retrieve the history API
   fc::api<history_api> history() const;
   /// @brief Retrieve the network node API
   fc::api<network_node_api> network_node() const;
   /// @brief Retrieve the cryptography API
   fc::api<crypto_api> crypto() const;
   /// @brief Retrieve the asset API
   fc::api<asset_api> asset() const;
   /// @brief Retrieve the debug API (if available)
   fc::api<graphene::debug_witness::debug_api> debug() const;
   /// @brief Retrieve the bookie API (if available)
   fc::api<graphene::bookie::bookie_api> bookie() const;
   /// @brief Retrieve the affiliate_stats API (if available)
   fc::api<graphene::affiliate_stats::affiliate_stats_api> affiliate_stats() const;
   /// @brief Retrieve the sidechain_api API (if available)
   fc::api<graphene::peerplays_sidechain::sidechain_api> sidechain() const;

   /// @brief Called to enable an API, not reflected.
   void enable_api(const string &api_name);

private:
   application &_app;
   optional<fc::api<block_api>> _block_api;
   optional<fc::api<database_api>> _database_api;
   optional<fc::api<network_broadcast_api>> _network_broadcast_api;
   optional<fc::api<network_node_api>> _network_node_api;
   optional<fc::api<history_api>> _history_api;
   optional<fc::api<crypto_api>> _crypto_api;
   optional<fc::api<asset_api>> _asset_api;
   optional<fc::api<graphene::debug_witness::debug_api>> _debug_api;
   optional<fc::api<graphene::bookie::bookie_api>> _bookie_api;
   optional<fc::api<graphene::affiliate_stats::affiliate_stats_api>> _affiliate_stats_api;
   optional<fc::api<graphene::peerplays_sidechain::sidechain_api>> _sidechain_api;
};

}} // namespace graphene::app

extern template class fc::api<graphene::app::login_api>;

// clang-format off

FC_REFLECT(graphene::app::network_broadcast_api::transaction_confirmation,
      (id)(block_num)(trx_num)(trx))

FC_REFLECT(graphene::app::verify_range_result,
      (success)(min_val)(max_val))

FC_REFLECT(graphene::app::verify_range_proof_rewind_result,
      (success)(min_val)(max_val)(value_out)(blind_out)(message_out))

FC_REFLECT(graphene::app::account_asset_balance,
      (name)(account_id)(amount));

FC_REFLECT(graphene::app::asset_holders,
      (asset_id)(count));

FC_API(graphene::app::history_api,
      (get_account_history)
      (get_account_history_operations)
      (get_relative_account_history)
      (get_fill_order_history)
      (get_market_history)
      (get_market_history_buckets)
      (list_core_accounts))

FC_API(graphene::app::block_api,
      (get_blocks))

FC_API(graphene::app::network_broadcast_api,
      (broadcast_transaction)
      (broadcast_transaction_with_callback)
      (broadcast_transaction_synchronous)
      (broadcast_block))

FC_API(graphene::app::network_node_api,
      (get_info)
      (add_node)
      (get_connected_peers)
      (get_potential_peers)
      (get_advanced_node_parameters)
      (set_advanced_node_parameters)
      (list_pending_transactions)
      (subscribe_to_pending_transactions)
      (unsubscribe_from_pending_transactions))

FC_API(graphene::app::crypto_api,
      (blind)
      (blind_sum)
      (verify_sum)
      (verify_range)
      (range_proof_sign)
      (verify_range_proof_rewind)
      (range_get_info))

FC_API(graphene::app::asset_api,
      (get_asset_holders)
      (get_asset_holders_count)
      (get_all_asset_holders))

FC_API(graphene::app::login_api,
      (login)
      (block)
      (network_broadcast)
      (database)
      (history)
      (network_node)
      (crypto)
      (asset)
      (debug)
      (bookie)
      (affiliate_stats)
      (sidechain))

// clang-format on
