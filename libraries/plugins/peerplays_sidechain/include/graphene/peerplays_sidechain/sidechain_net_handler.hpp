#pragma once

#include <vector>

#include <boost/program_options.hpp>

#include <fc/signals.hpp>
#include <graphene/peerplays_sidechain/defs.hpp>
#include <graphene/peerplays_sidechain/peerplays_sidechain_plugin.hpp>

namespace graphene { namespace peerplays_sidechain {

class sidechain_net_handler {
public:
    sidechain_net_handler(peerplays_sidechain_plugin& _plugin, const boost::program_options::variables_map& options);
    virtual ~sidechain_net_handler();

    graphene::peerplays_sidechain::sidechain_type get_sidechain();
    std::vector<std::string> get_sidechain_addresses();

    virtual son_wallet_update_operation recreate_primary_wallet() = 0;
    virtual string recreate_primary_wallet(const vector<string>& participants) = 0;

    fc::signal<void(const sidechain_event_data&)> sidechain_event_data_received;

protected:
    peerplays_sidechain_plugin& plugin;
    graphene::chain::database& database;
    graphene::peerplays_sidechain::sidechain_type sidechain;

    virtual std::string create_multisignature_wallet( const std::vector<std::string> public_keys ) = 0;
    virtual std::string transfer( const std::string& from, const std::string& to, const uint64_t amount ) = 0;
    virtual std::string sign_transaction( const std::string& transaction ) = 0;
    virtual std::string send_transaction( const std::string& transaction ) = 0;

    virtual void handle_event( const std::string& event_data ) = 0;

private:

};

} } // graphene::peerplays_sidechain

