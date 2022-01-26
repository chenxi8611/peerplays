#include <graphene/peerplays_sidechain/hive/types.hpp>

#include <fc/crypto/base58.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/raw.hpp>

namespace graphene { namespace peerplays_sidechain { namespace hive {

std::string public_key_type::prefix = KEY_PREFIX_STM;

public_key_type::public_key_type() :
      key_data(){};

public_key_type::public_key_type(const fc::ecc::public_key_data &data) :
      key_data(data){};

public_key_type::public_key_type(const fc::ecc::public_key &pubkey) :
      key_data(pubkey){};

public_key_type::public_key_type(const std::string &base58str) {
   const size_t prefix_len = prefix.size();
   FC_ASSERT(base58str.size() > prefix_len);
   FC_ASSERT(base58str.substr(0, prefix_len) == prefix, "", ("base58str", base58str));
   auto bin = fc::from_base58(base58str.substr(prefix_len));
   auto bin_key = fc::raw::unpack<binary_key>(bin);
   key_data = bin_key.data;
   FC_ASSERT(fc::ripemd160::hash((const char*)key_data.data(), key_data.size())._hash[0].value() == bin_key.check);
};

public_key_type::operator fc::ecc::public_key_data() const {
   return key_data;
};

public_key_type::operator fc::ecc::public_key() const {
   return fc::ecc::public_key(key_data);
};

public_key_type::operator std::string() const {
   binary_key k;
   k.data = key_data;
   k.check = fc::ripemd160::hash((const char*)k.data.data(), k.data.size())._hash[0].value();
   auto data = fc::raw::pack(k);
   return prefix + fc::to_base58(data.data(), data.size());
}

bool operator==(const public_key_type &p1, const fc::ecc::public_key &p2) {
   return p1.key_data == p2.serialize();
}

bool operator==(const public_key_type &p1, const public_key_type &p2) {
   return p1.key_data == p2.key_data;
}

bool operator!=(const public_key_type &p1, const public_key_type &p2) {
   return p1.key_data != p2.key_data;
}

}}} // namespace graphene::peerplays_sidechain::hive

namespace fc {

using namespace std;

void to_variant(const graphene::peerplays_sidechain::hive::public_key_type &var, fc::variant &vo, uint32_t max_depth) {
   vo = std::string(var);
}

void from_variant(const fc::variant &var, graphene::peerplays_sidechain::hive::public_key_type &vo, uint32_t max_depth) {
   vo = graphene::peerplays_sidechain::hive::public_key_type(var.as_string());
}

} // namespace fc
