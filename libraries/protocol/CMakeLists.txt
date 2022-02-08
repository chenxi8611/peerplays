file(GLOB HEADERS "include/graphene/protocol/*.hpp")

list(APPEND SOURCES account.cpp
                    assert.cpp
                    asset_ops.cpp
                    block.cpp
                    confidential.cpp
                    chain_parameters.cpp
                    fee_schedule.cpp
                    memo.cpp
                    proposal.cpp
                    transfer.cpp
                    vote.cpp
                    witness.cpp
                    address.cpp
                    asset.cpp
                    authority.cpp
                    special_authority.cpp
                    committee_member.cpp
                    custom.cpp
                    market.cpp
                    operations.cpp
                    pts_address.cpp
                    small_ops.cpp
                    transaction.cpp
                    types.cpp
                    withdraw_permission.cpp
                    worker.cpp
                    betting_market.cpp
                    event.cpp
                    event_group.cpp
                    lottery_ops.cpp
                    small_ops.cpp
                    sport.cpp
                    tournament.cpp
                    small_ops.cpp
                    custom_permission.cpp
                    custom_account_authority.cpp
                    offer.cpp
                    nft.cpp
                    nft_lottery.cpp
                    account_role.cpp
                    )


if (NOT ${GRAPHENE_BUILD_DYNAMIC_LIBRARIES})
    add_library( graphene_protocol ${SOURCES} ${HEADERS} )
else()
    add_library( graphene_protocol SHARED ${SOURCES} ${HEADERS} )
endif()

target_link_libraries( graphene_protocol fc )
target_include_directories( graphene_protocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_protocol

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/protocol" )
