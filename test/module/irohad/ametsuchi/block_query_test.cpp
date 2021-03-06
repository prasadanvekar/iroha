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

#include <boost/optional.hpp>
#include "ametsuchi/impl/storage_impl.hpp"
#include "crypto/hash.hpp"
#include "framework/test_subscriber.hpp"
#include "module/irohad/ametsuchi/ametsuchi_fixture.hpp"

using namespace iroha::ametsuchi;
using namespace iroha::model;
using namespace framework::test_subscriber;

class BlockQueryTest : public AmetsuchiTest {
 protected:
  void SetUp() override {
    AmetsuchiTest::SetUp();
    storage =
        StorageImpl::create(block_store_path, redishost_, redisport_, pgopt_);

    ASSERT_TRUE(storage);

    blocks = storage->getBlockQuery();
    // First transaction in block1
    Transaction txn1_1;
    txn1_1.creator_account_id = creator1;
    tx_hashes.push_back(iroha::hash(txn1_1));

    // Second transaction in block1
    Transaction txn1_2;
    txn1_2.creator_account_id = creator1;
    tx_hashes.push_back(iroha::hash(txn1_2));

    Block block1;
    block1.height = 1;
    block1.transactions.push_back(txn1_1);
    block1.transactions.push_back(txn1_2);
    auto block1hash = iroha::hash(block1);

    // First tx in block 1
    Transaction txn2_1;
    txn2_1.creator_account_id = creator1;
    tx_hashes.push_back(iroha::hash(txn2_1));

    // Second tx in block 2
    Transaction txn2_2;
    // this tx has another creator
    txn2_2.creator_account_id = creator2;
    tx_hashes.push_back(iroha::hash(txn2_2));

    Block block2;
    block2.height = 2;
    block2.prev_hash = block1hash;
    block2.transactions.push_back(txn2_1);
    block2.transactions.push_back(txn2_2);

    auto ms = storage->createMutableStorage();
    ms->apply(block1, [](const auto &, auto &, const auto &) { return true; });
    ms->apply(block2, [](const auto &, auto &, const auto &) { return true; });
    storage->commit(std::move(ms));
  }

  std::vector<iroha::hash256_t> tx_hashes;
  std::shared_ptr<StorageImpl> storage;
  std::shared_ptr<BlockQuery> blocks;
  std::string creator1 = "user1@test";
  std::string creator2 = "user2@test";
};

/**
 * @given block store with 2 blocks totally containing 3 txs created by
 * user1@test
 * AND 1 tx created by user2@test
 * @when query to get transactions created by user1@test is invoked
 * @then query over user1@test returns 3 txs
 */
TEST_F(BlockQueryTest, GetAccountTransactionsFromSeveralBlocks) {
  // Check that creator1 has created 3 transactions
  auto getCreator1TxWrapper = make_test_subscriber<CallExact>(
      blocks->getAccountTransactions(creator1), 3);
  getCreator1TxWrapper.subscribe(
      [this](auto val) { EXPECT_EQ(val.creator_account_id, creator1); });
  ASSERT_TRUE(getCreator1TxWrapper.validate());
}

/**
 * @given block store with 2 blocks totally containing 3 txs created by
 * user1@test
 * AND 1 tx created by user2@test
 * @when query to get transactions created by user2@test is invoked
 * @then query over user2@test returns 1 tx
 */
TEST_F(BlockQueryTest, GetAccountTransactionsFromSingleBlock) {
  // Check that creator1 has created 1 transaction
  auto getCreator2TxWrapper = make_test_subscriber<CallExact>(
      blocks->getAccountTransactions(creator2), 1);
  getCreator2TxWrapper.subscribe(
      [this](auto val) { EXPECT_EQ(val.creator_account_id, creator2); });
  ASSERT_TRUE(getCreator2TxWrapper.validate());
}

/**
 * @given block store
 * @when query to get transactions created by user with id not registered in the
 * system is invoked
 * @then query returns empty result
 */
TEST_F(BlockQueryTest, GetAccountTransactionsNonExistingUser) {
  // Check that "nonexisting" user has no transaction
  auto getNonexistingTxWrapper = make_test_subscriber<CallExact>(
      blocks->getAccountTransactions("nonexisting user"), 0);
  getNonexistingTxWrapper.subscribe();
  ASSERT_TRUE(getNonexistingTxWrapper.validate());
}

/**
 * @given block store with 2 blocks totally containing 3 txs created by
 * user1@test
 * AND 1 tx created by user2@test
 * @when query to get transactions with existing transaction hashes
 * @then queried transactions
 */
TEST_F(BlockQueryTest, GetTransactionsExistingTxHashes) {
  auto wrapper = make_test_subscriber<CallExact>(
      blocks->getTransactions({tx_hashes[1], tx_hashes[3]}), 2);
  wrapper.subscribe([this](auto tx) {
    static auto subs_cnt = 0;
    subs_cnt++;
    if (subs_cnt == 1) {
      EXPECT_TRUE(tx);
      EXPECT_EQ(this->tx_hashes[1], iroha::hash(*tx));
    } else {
      EXPECT_TRUE(tx);
      EXPECT_EQ(this->tx_hashes[3], iroha::hash(*tx));
    }
  });
  ASSERT_TRUE(wrapper.validate());
}

/**
 * @given block store with 2 blocks totally containing 3 txs created by
 * user1@test
 * AND 1 tx created by user2@test
 * @when query to get transactions with non-existing transaction hashes
 * @then nullopt values are retrieved
 */
TEST_F(BlockQueryTest, GetTransactionsIncludesNonExistingTxHashes) {
  iroha::hash256_t invalid_tx_hash_1, invalid_tx_hash_2;
  invalid_tx_hash_1[0] = 1;
  invalid_tx_hash_2[0] = 2;
  auto wrapper = make_test_subscriber<CallExact>(
      blocks->getTransactions({invalid_tx_hash_1, invalid_tx_hash_2}), 2);
  wrapper.subscribe(
      [](auto transaction) { EXPECT_EQ(boost::none, transaction); });
  ASSERT_TRUE(wrapper.validate());
}

/**
 * @given block store with 2 blocks totally containing 3 txs created by
 * user1@test
 * AND 1 tx created by user2@test
 * @when query to get transactions with empty vector
 * @then no transactions are retrieved
 */
TEST_F(BlockQueryTest, GetTransactionsWithEmpty) {
  // transactions' hashes are empty.
  auto wrapper =
      make_test_subscriber<CallExact>(blocks->getTransactions({}), 0);
  wrapper.subscribe();
  ASSERT_TRUE(wrapper.validate());
}

/**
 * @given block store with 2 blocks totally containing 3 txs created by
 * user1@test
 * AND 1 tx created by user2@test
 * @when query to get transactions with non-existing txhash and existing txhash
 * @then queried transactions and empty transaction
 */
TEST_F(BlockQueryTest, GetTransactionsWithInvalidTxAndValidTx) {
  // TODO 15/11/17 motxx - Use EqualList VerificationStrategy
  iroha::hash256_t invalid_tx_hash_1;
  invalid_tx_hash_1[0] = 1;
  auto wrapper = make_test_subscriber<CallExact>(
      blocks->getTransactions({invalid_tx_hash_1, tx_hashes[0]}), 2);
  wrapper.subscribe([this](auto tx) {
    static auto subs_cnt = 0;
    subs_cnt++;
    if (subs_cnt == 1) {
      EXPECT_EQ(boost::none, tx);
    } else {
      EXPECT_TRUE(tx);
      EXPECT_EQ(this->tx_hashes[0], iroha::hash(*tx));
    }
  });
  ASSERT_TRUE(wrapper.validate());
}
