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


#include <torii/processor/stub_query_processor.hpp>

namespace iroha {
  namespace torii {
    using rxcpp::subscriber;
    using std::shared_ptr;
    using dao::Query;
    using dao::QueryResponse;
    using dao::Client;

    QueryProcessorStub::QueryProcessorStub() {
      handler_.insert<dao::GetBlocks>(std::bind(&QueryProcessorStub::handle_get_blocks,
                                                this,
                                                std::placeholders::_1));
    }

    void QueryProcessorStub::handle(Client client, Query &query) {
      auto handle = handler_.(query).value_or([](auto &) {
        // TODO make error handler
        return;
      });
      handle(query);
      return;
    }

    rxcpp::observable<shared_ptr<QueryResponse>> QueryProcessorStub::notifier() {
      return observer_;
    }

    void QueryProcessorStub::handle_get_blocks(dao::GetBlocks blocks) {

    }

  } //namespace torii
} //namespace iroha
