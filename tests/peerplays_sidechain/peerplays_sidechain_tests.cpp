#define BOOST_TEST_MODULE Peerplays SON Tests

#include <boost/test/unit_test.hpp>

#include <iostream>

BOOST_AUTO_TEST_CASE(peerplays_sidechain) {
}

struct GlobalInitializationFixture {
    GlobalInitializationFixture() {
        std::srand(time(NULL));
        std::cout << "Random number generator seeded to " << time(NULL) << std::endl;
    }
};
BOOST_TEST_GLOBAL_FIXTURE(GlobalInitializationFixture);

