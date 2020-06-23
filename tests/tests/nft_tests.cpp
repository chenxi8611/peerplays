#include <boost/test/unit_test.hpp>

#include "../common/database_fixture.hpp"

#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/nft_object.hpp>

using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_FIXTURE_TEST_SUITE( nft_tests, database_fixture )

BOOST_AUTO_TEST_CASE( nft_create_test ) {

   BOOST_TEST_MESSAGE("nft_create_test");

   generate_block();
   set_expiration(db, trx);

   ACTORS((alice));
   ACTORS((bob));
   ACTORS((operator1));
   ACTORS((operator2));

   generate_block();
   set_expiration(db, trx);

   {
      BOOST_TEST_MESSAGE("Send nft_create_operation");

      nft_create_operation op;
      op.owner = alice_id;
      op.approved = alice_id;
      op.approved_operators.push_back(operator1_id);
      op.approved_operators.push_back(operator2_id);
      op.metadata = "metadata";

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   BOOST_TEST_MESSAGE("Check nft_create_operation results");

   const auto& idx = db.get_index_type<nft_index>().indices().get<by_id>();
   BOOST_REQUIRE( idx.size() == 1 );
   auto obj = idx.begin();
   BOOST_REQUIRE( obj != idx.end() );
   BOOST_CHECK( obj->owner == alice_id );
   BOOST_CHECK( obj->approved_operators.size() == 2 );
   BOOST_CHECK( obj->approved_operators.at(0) == operator1_id );
   BOOST_CHECK( obj->approved_operators.at(1) == operator2_id );
   BOOST_CHECK( obj->metadata == "metadata" );
}


BOOST_AUTO_TEST_CASE( nft_safe_transfer_from_test ) {

   BOOST_TEST_MESSAGE("nft_safe_transfer_from_test");

   INVOKE(nft_create_test);

   GET_ACTOR(alice);
   GET_ACTOR(bob);

   {
      BOOST_TEST_MESSAGE("Check nft_safe_transfer_operation preconditions");

      const auto& idx = db.get_index_type<nft_index>().indices().get<by_id>();
      BOOST_REQUIRE( idx.size() == 1 );
      auto obj = idx.begin();
      BOOST_REQUIRE( obj->owner == alice_id );
   }

   {
      BOOST_TEST_MESSAGE("Send nft_safe_transfer_operation");

      nft_safe_transfer_from_operation op;
      op.from = alice_id;
      op.to = bob_id;
      op.token_id = nft_id_type(0);
      op.data = "data";

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Check nft_safe_transfer_operation results");

      const auto& idx = db.get_index_type<nft_index>().indices().get<by_id>();
      BOOST_REQUIRE( idx.size() == 1 );
      auto obj = idx.begin();
      BOOST_REQUIRE( obj->owner == bob_id );
   }
}

BOOST_AUTO_TEST_CASE( nft_approve_operation_test ) {

   BOOST_TEST_MESSAGE("nft_approve_operation_test");

   INVOKE(nft_create_test);

   GET_ACTOR(alice);
   GET_ACTOR(operator1);
   GET_ACTOR(operator2);

   ACTORS((operator3));

   {
      BOOST_TEST_MESSAGE("Send nft_approve_operation");

      nft_approve_operation op;
      op.operator_ = alice_id;
      op.approved = operator3_id;
      op.token_id = nft_id_type(0);

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Check nft_approve_operation results");

      const auto& idx = db.get_index_type<nft_index>().indices().get<by_id>();
      BOOST_REQUIRE( idx.size() == 1 );
      auto obj = idx.begin();
      BOOST_REQUIRE( obj != idx.end() );
      BOOST_CHECK( obj->approved == operator3_id );
      BOOST_CHECK( obj->approved_operators.size() == 2 );
      BOOST_CHECK( obj->approved_operators.at(0) == operator1_id );
      BOOST_CHECK( obj->approved_operators.at(1) == operator2_id );
   }
}

BOOST_AUTO_TEST_CASE( nft_set_approval_for_all_test ) {

   BOOST_TEST_MESSAGE("nft_set_approval_for_all_test");

   generate_block();
   set_expiration(db, trx);

   ACTORS((alice));
   ACTORS((bob));

   generate_block();
   set_expiration(db, trx);

   BOOST_TEST_MESSAGE("Create NFT assets");

   {
      BOOST_TEST_MESSAGE("Send nft_create_operation 1");

      nft_create_operation op;
      op.owner = alice_id;
      op.metadata = "metadata 1";

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Send nft_create_operation 2");

      nft_create_operation op;
      op.owner = bob_id;
      op.metadata = "metadata 2";

      trx.operations.push_back(op);
      sign(trx, bob_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Send nft_create_operation 3");

      nft_create_operation op;
      op.owner = alice_id;
      op.metadata = "metadata 3";

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Send nft_create_operation 4");

      nft_create_operation op;
      op.owner = bob_id;
      op.metadata = "metadata 4";

      trx.operations.push_back(op);
      sign(trx, bob_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Send nft_create_operation 5");

      nft_create_operation op;
      op.owner = alice_id;
      op.metadata = "metadata 5";

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();


   {
      BOOST_TEST_MESSAGE("Send nft_approve_operation");

      nft_set_approval_for_all_operation op;
      op.owner = alice_id;
      op.operator_ = bob_id;
      op.approved = true;

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Send nft_approve_operation");

      nft_set_approval_for_all_operation op;
      op.owner = alice_id;
      op.operator_ = bob_id;
      op.approved = true;

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Check nft_approve_operation results");

      const auto& idx = db.get_index_type<nft_index>().indices().get<by_owner>();
      const auto &idx_range = idx.equal_range(alice_id);
      std::for_each(idx_range.first, idx_range.second, [&](const nft_object &obj) {
         BOOST_CHECK( obj.approved_operators.size() == 1 );
         BOOST_CHECK( obj.approved_operators.at(0) == bob_id );
      });
   }

   {
      BOOST_TEST_MESSAGE("Send nft_approve_operation");

      nft_set_approval_for_all_operation op;
      op.owner = alice_id;
      op.operator_ = bob_id;
      op.approved = false;

      trx.operations.push_back(op);
      sign(trx, alice_private_key);
      PUSH_TX(db, trx, ~0);
   }
   generate_block();

   {
      BOOST_TEST_MESSAGE("Check nft_approve_operation results");

      const auto& idx = db.get_index_type<nft_index>().indices().get<by_owner>();
      const auto &idx_range = idx.equal_range(alice_id);
      std::for_each(idx_range.first, idx_range.second, [&](const nft_object &obj) {
         BOOST_CHECK( obj.approved_operators.size() == 0 );
      });
   }
}

BOOST_AUTO_TEST_SUITE_END()

