#pragma once

#include <graphene/peerplays_sidechain/sidechain_net_handler.hpp>

#include <string>
#include <zmq.hpp>

#include <fc/network/http/connection.hpp>
#include <fc/signals.hpp>

namespace graphene { namespace peerplays_sidechain {

enum class network_type {
   mainnet,
   testnet,
   regtest
};

class btc_txout {
public:
   std::string txid_;
   unsigned int out_num_;
   uint64_t amount_;
};

class bitcoin_rpc_client {
public:
   bitcoin_rpc_client(std::string _ip, uint32_t _rpc, std::string _user, std::string _password, std::string _wallet, std::string _wallet_password);

   std::string addmultisigaddress(const uint32_t nrequired, const std::vector<std::string> public_keys);
   std::string combinepsbt(const vector<std::string> &psbts);
   std::string createmultisig(const uint32_t nrequired, const std::vector<std::string> public_keys);
   std::string createpsbt(const std::vector<btc_txout> &ins, const fc::flat_map<std::string, double> outs);
   std::string converttopsbt(const std::string &hex);
   std::string createrawtransaction(const std::vector<btc_txout> &ins, const fc::flat_map<std::string, double> outs);
   std::string createwallet(const std::string &wallet_name);
   std::string decodepsbt(std::string const &tx_psbt);
   std::string decoderawtransaction(std::string const &tx_hex);
   std::string encryptwallet(const std::string &passphrase);
   uint64_t estimatesmartfee(uint16_t conf_target = 128);
   std::string finalizepsbt(std::string const &tx_psbt);
   std::string getaddressinfo(const std::string &address);
   std::string getblock(const std::string &block_hash, int32_t verbosity = 2);
   std::string getblockchaininfo();
   std::string gettransaction(const std::string &txid, const bool include_watch_only = false);
   void importaddress(const std::string &address_or_script);
   std::vector<btc_txout> listunspent(const uint32_t minconf = 1, const uint32_t maxconf = 9999999);
   std::vector<btc_txout> listunspent_by_address_and_amount(const std::string &address, double transfer_amount, const uint32_t minconf = 1, const uint32_t maxconf = 9999999);
   std::string loadwallet(const std::string &filename);
   std::string sendrawtransaction(const std::string &tx_hex);
   std::string signrawtransactionwithwallet(const std::string &tx_hash);
   std::string unloadwallet(const std::string &filename);
   std::string walletlock();
   std::string walletprocesspsbt(std::string const &tx_psbt);
   bool walletpassphrase(const std::string &passphrase, uint32_t timeout = 60);

private:
   fc::http::reply send_post_request(std::string body, bool show_log = false);

   std::string ip;
   uint32_t rpc_port;
   std::string user;
   std::string password;
   std::string wallet;
   std::string wallet_password;

   fc::http::header authorization;
};

// =============================================================================

class zmq_listener {
public:
   zmq_listener(std::string _ip, uint32_t _zmq);

   fc::signal<void(const std::string &)> event_received;

private:
   void handle_zmq();
   std::vector<zmq::message_t> receive_multipart();

   std::string ip;
   uint32_t zmq_port;

   zmq::context_t ctx;
   zmq::socket_t socket;
};

// =============================================================================

class sidechain_net_handler_bitcoin : public sidechain_net_handler {
public:
   sidechain_net_handler_bitcoin(peerplays_sidechain_plugin &_plugin, const boost::program_options::variables_map &options);
   virtual ~sidechain_net_handler_bitcoin();

   bool process_proposal(const proposal_object &po);
   void process_primary_wallet();
   bool process_deposit(const son_wallet_deposit_object &swdo);
   bool process_withdrawal(const son_wallet_withdraw_object &swwo);
   std::string process_sidechain_transaction(const sidechain_transaction_object &sto);
   std::string send_sidechain_transaction(const sidechain_transaction_object &sto);

private:
   std::string ip;
   uint32_t zmq_port;
   uint32_t rpc_port;
   std::string rpc_user;
   std::string rpc_password;
   std::string wallet;
   std::string wallet_password;

   std::unique_ptr<bitcoin_rpc_client> bitcoin_client;
   std::unique_ptr<zmq_listener> listener;

   network_type network;
   uint8_t payment_address_p2kh;
   uint8_t payment_address_p2sh;
   uint8_t ec_private_wif;
   uint8_t ec_private_p2kh;
   uint16_t ec_private_version;

   fc::future<void> on_changed_objects_task;

   std::string create_primary_wallet_transaction();
   std::string create_deposit_transaction(const son_wallet_deposit_object &swdo);
   std::string create_withdrawal_transaction(const son_wallet_withdraw_object &swwo);

   std::string create_multisig_address(const std::vector<std::pair<std::string, uint16_t>> &son_pubkeys);
   std::string create_transaction(const std::vector<btc_txout> &inputs, const fc::flat_map<std::string, double> outputs);
   std::string sign_transaction(const sidechain_transaction_object &sto);
   std::string send_transaction(const sidechain_transaction_object &sto);

   std::string create_multisig_address_raw(const std::vector<std::pair<std::string, uint16_t>> &son_pubkeys);
   std::string create_multisig_address_psbt(const std::vector<std::pair<std::string, uint16_t>> &son_pubkeys);
   std::string create_multisig_address_standalone(const std::vector<std::pair<std::string, uint16_t>> &son_pubkeys);

   std::string create_transaction_raw(const std::vector<btc_txout> &inputs, const fc::flat_map<std::string, double> outputs);
   std::string create_transaction_psbt(const std::vector<btc_txout> &inputs, const fc::flat_map<std::string, double> outputs);
   std::string create_transaction_standalone(const std::vector<btc_txout> &inputs, const fc::flat_map<std::string, double> outputs);

   std::string sign_transaction_raw(const sidechain_transaction_object &sto);
   std::string sign_transaction_psbt(const sidechain_transaction_object &sto);
   std::string sign_transaction_standalone(const sidechain_transaction_object &sto);

   std::string send_transaction_raw(const sidechain_transaction_object &sto);
   std::string send_transaction_psbt(const sidechain_transaction_object &sto);
   std::string send_transaction_standalone(const sidechain_transaction_object &sto);

   void handle_event(const std::string &event_data);
   std::vector<info_for_vin> extract_info_from_block(const std::string &_block);
   void on_changed_objects(const vector<object_id_type> &ids, const flat_set<account_id_type> &accounts);
   void on_changed_objects_cb(const vector<object_id_type> &ids, const flat_set<account_id_type> &accounts);
};

}} // namespace graphene::peerplays_sidechain
