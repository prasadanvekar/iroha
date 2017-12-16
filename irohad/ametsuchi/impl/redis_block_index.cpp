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

#include "ametsuchi/impl/redis_block_index.hpp"

#include "crypto/hash.hpp"
#include "model/commands/transfer_asset.hpp"

namespace iroha {
  namespace ametsuchi {

    RedisBlockIndex::RedisBlockIndex(cpp_redis::redis_client &client)
        : client_(client) {}

    void RedisBlockIndex::index_block(uint64_t height,
                                      const model::Block &block) {
      for (size_t i = 0; i < block.transactions.size(); i++) {
        auto tx = block.transactions.at(i);
        auto account_id = tx.creator_account_id;
        auto hash = iroha::hash(tx).to_string();

        // tx hash -> block where hash is stored
        client_.set(hash, std::to_string(height));

        // to make index account_id -> list of blocks where his txs exist
        client_.sadd(account_id, {std::to_string(height)});

        // to make index account_id:height -> list of tx indexes (where
        // tx is placed in the block)
        client_.rpush(account_id + ":" + std::to_string(height),
                      {std::to_string(i)});

        // collect all assets belonging to user "account_id"
        std::set<std::string> users_assets_in_tx;
        std::for_each(tx.commands.begin(),
                      tx.commands.end(),
                      [&account_id, &users_assets_in_tx](auto command) {
                        if (instanceof <model::TransferAsset>(*command)) {
                          auto transferAsset =
                              (model::TransferAsset *)command.get();
                          if (transferAsset->dest_account_id == account_id
                              or transferAsset->src_account_id == account_id) {
                            users_assets_in_tx.insert(transferAsset->asset_id);
                          }
                        }
                      });

        // to make account_id:height:asset_id -> list of tx indexes (where tx
        // with certain asset is placed in the block )
        for (const auto &asset_id : users_assets_in_tx) {
          // create key to put user's txs with given asset_id
          std::string account_assets_key;
          account_assets_key.append(account_id);
          account_assets_key.append(":");
          account_assets_key.append(std::to_string(height));
          account_assets_key.append(":");
          account_assets_key.append(asset_id);
          client_.rpush(account_assets_key, {std::to_string(i)});
        }
      }
    }
  } // namespace ametsuchi
} // namespace iroha
