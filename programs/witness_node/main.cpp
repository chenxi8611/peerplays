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
#include <graphene/app/application.hpp>
#include <graphene/app/config_util.hpp>

#include <graphene/witness/witness.hpp>
#include <graphene/debug_witness/debug_witness.hpp>
#include <graphene/account_history/account_history_plugin.hpp>
#include <graphene/accounts_list/accounts_list_plugin.hpp>
#include <graphene/elasticsearch/elasticsearch_plugin.hpp>
#include <graphene/es_objects/es_objects.hpp>
#include <graphene/market_history/market_history_plugin.hpp>
//#include <graphene/generate_genesis/generate_genesis_plugin.hpp>
//#include <graphene/generate_uia_sharedrop_genesis/generate_uia_sharedrop_genesis.hpp>
#include <graphene/affiliate_stats/affiliate_stats_plugin.hpp>
#include <graphene/bookie/bookie_plugin.hpp>
#include <graphene/peerplays_sidechain/peerplays_sidechain_plugin.hpp>
#include <graphene/utilities/git_revision.hpp>
#include <graphene/snapshot/snapshot.hpp>

#include <fc/thread/thread.hpp>
#include <fc/interprocess/signals.hpp>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/algorithm/string.hpp>

#include <graphene/utilities/git_revision.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <iostream>

#ifdef WIN32
# include <signal.h>
#else
# include <csignal>
#endif

using namespace graphene;
namespace bpo = boost::program_options;

int main(int argc, char** argv) {
   app::application* node = new app::application();
   fc::oexception unhandled_exception;
   try {
      bpo::options_description app_options("Graphene Witness Node");
      bpo::options_description cfg_options("Graphene Witness Node");
      app_options.add_options()
            ("help,h", "Print this help message and exit.")
            ("data-dir,d", bpo::value<boost::filesystem::path>()->default_value("witness_node_data_dir"),
                    "Directory containing databases, configuration file, etc.")
            ("version,v", "Display version information");

      bpo::variables_map options;

      bpo::options_description cli, cfg;
      node->set_program_options(cli, cfg);
      cfg_options.add(cfg);

      auto witness_plug = node->register_plugin<witness_plugin::witness_plugin>();
      auto debug_witness_plug = node->register_plugin<debug_witness_plugin::debug_witness_plugin>();
      auto history_plug = node->register_plugin<account_history::account_history_plugin>();
      auto elasticsearch_plug = node->register_plugin<elasticsearch::elasticsearch_plugin>();
      auto es_objects_plug = node->register_plugin<es_objects::es_objects_plugin>();
      auto market_history_plug = node->register_plugin<market_history::market_history_plugin>();
      auto list_plug = node->register_plugin<accounts_list::accounts_list_plugin>();
      auto affiliate_stats_plug = node->register_plugin<affiliate_stats::affiliate_stats_plugin>();
      auto bookie_plug = node->register_plugin<bookie::bookie_plugin>();
      auto peerplays_sidechain = node->register_plugin<peerplays_sidechain::peerplays_sidechain_plugin>();
      auto snapshot_plug = node->register_plugin<snapshot_plugin::snapshot_plugin>();

      // add plugin options to config
      try
      {
         bpo::options_description cli, cfg;
         node->set_program_options(cli, cfg);
         app_options.add(cli);
         cfg_options.add(cfg);
         bpo::store(bpo::parse_command_line(argc, argv, app_options), options);
      }
      catch (const boost::program_options::error& e)
      {
        std::cerr << "Error parsing command line: " << e.what() << "\n";
        return 1;
      }

      if (options.count("version"))
      {
         std::string witness_version(graphene::utilities::git_revision_description);
         const size_t pos = witness_version.find('/');
         if( pos != std::string::npos && witness_version.size() > pos )
            witness_version = witness_version.substr( pos + 1 );
         std::cerr << "Version: " << witness_version << "\n";
         std::cerr << "Git Revision: " << graphene::utilities::git_revision_sha << "\n";
         std::cerr << "Built: " << __DATE__ " at " __TIME__ << "\n";
         std::cout << "SSL: " << OPENSSL_VERSION_TEXT << "\n";
         std::cout << "Boost: " << boost::replace_all_copy(std::string(BOOST_LIB_VERSION), "_", ".") << "\n";
         return 0;
      }
      if( options.count("help") )
      {
         std::cout << app_options << "\n";
         return 0;
      }

      fc::path data_dir;
      if( options.count("data-dir") )
      {
         data_dir = options["data-dir"].as<boost::filesystem::path>();
         if( data_dir.is_relative() )
            data_dir = fc::current_path() / data_dir;
      }
      app::load_configuration_options(data_dir, cfg_options, options);

      std::set<std::string> plugins;
      boost::split(plugins, options.at("plugins").as<std::string>(), [](char c){return c == ' ';});

      if(plugins.count("account_history") && plugins.count("elasticsearch")) {
         std::cerr << "Plugin conflict: Cannot load both account_history plugin and elasticsearch plugin\n";
         return 1;
      }

      std::for_each(plugins.begin(), plugins.end(), [node](const std::string& plug) mutable {
         if (!plug.empty()) {
            node->enable_plugin(plug);
         }
      });

      bpo::notify(options);

      node->initialize(data_dir, options);
      node->initialize_plugins( options );

      node->startup();
      node->startup_plugins();

      fc::promise<int>::ptr exit_promise = new fc::promise<int>("UNIX Signal Handler");

      fc::set_signal_handler([&exit_promise](int signal) {
         elog( "Caught SIGINT attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGINT);

      fc::set_signal_handler([&exit_promise](int signal) {
         elog( "Caught SIGTERM attempting to exit cleanly" );
         exit_promise->set_value(signal);
      }, SIGTERM);

      ilog("Started Peerplays node on a chain with ${h} blocks.", ("h", node->chain_database()->head_block_num()));
      ilog("Chain ID is ${id}", ("id", node->chain_database()->get_chain_id()) );

      int signal = exit_promise->wait();
      ilog("Exiting from signal ${n}", ("n", signal));
      node->shutdown_plugins();
      node->shutdown();
      delete node;
      return EXIT_SUCCESS;
   } catch( const fc::exception& e ) {
      // deleting the node can yield, so do this outside the exception handler
      unhandled_exception = e;
   }

   if (unhandled_exception)
   {
      elog("Exiting with error:\n${e}", ("e", unhandled_exception->to_detail_string()));
      node->shutdown();
      delete node;
      return EXIT_FAILURE;
   }
}
