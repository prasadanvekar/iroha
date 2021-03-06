/**
 * Copyright Soramitsu Co., Ltd. 2017 All Rights Reserved.
 * http://soramitsu.co.jp
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "crypto/hash.hpp"

extern "C" {
#include <sha3.h>
}

#include "common/types.hpp"
#include "model/converters/pb_block_factory.hpp"
#include "model/converters/pb_common.hpp"
#include "model/converters/pb_query_factory.hpp"
#include "model/converters/pb_transaction_factory.hpp"

namespace sha3 {
  void sha3_256_(const unsigned char *message, size_t message_len,
                 unsigned char *out) {
    sha3_256(message, message_len, out);
  }
  void sha3_512_(const unsigned char *message, size_t message_len,
                 unsigned char *out) {
    sha3_512(message, message_len, out);
  }
}

namespace iroha {

  void sha3_256(unsigned char *output, unsigned char *input, size_t in_size) {
    sha3::sha3_256_(input, in_size, output);
  }

  void sha3_512(unsigned char *output, unsigned char *input, size_t in_size) {
    sha3::sha3_512_(input, in_size, output);
  }

  hash256_t sha3_256(const uint8_t *input, size_t in_size) {
    hash256_t h;
    sha3::sha3_256_(input, in_size, h.data());
    return h;
  }

  hash512_t sha3_512(const uint8_t *input, size_t in_size) {
    hash512_t h;
    sha3::sha3_512_(input, in_size, h.data());
    return h;
  }

  hash256_t sha3_256(const std::string &msg) {
    hash256_t h;
    sha3::sha3_256_((uint8_t *)msg.data(), msg.size(), h.data());
    return h;
  }

  hash512_t sha3_512(const std::string &msg) {
    hash512_t h;
    sha3::sha3_512_((uint8_t *)msg.data(), msg.size(), h.data());
    return h;
  }

  // TODO: remove factories
  const static model::converters::PbTransactionFactory tx_factory;
  const static model::converters::PbBlockFactory block_factory;
  const static model::converters::PbQueryFactory query_factory;

  hash256_t hash(const model::Transaction &tx) {
    auto &&pb_dat = tx_factory.serialize(tx);
    return hash(pb_dat);
  }

  hash256_t hash(const model::Block &block) {
    auto &&pb_dat = block_factory.serialize(block);
    return hash(pb_dat);
  }

  hash256_t hash(const model::Query &query) {
    std::shared_ptr<const model::Query> qptr(&query, [](auto) {});
    auto &&pb_dat = query_factory.serialize(qptr);
    return hash(*pb_dat);
  }

}  // namespace iroha
