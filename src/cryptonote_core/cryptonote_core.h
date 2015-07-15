// Copyright (c) 2012-2015, The CryptoNote developers, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Bytecoin is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Bytecoin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Bytecoin.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "p2p/net_node_common.h"
#include "cryptonote_protocol/cryptonote_protocol_handler_common.h"
#include "Currency.h"
#include "tx_pool.h"
#include "blockchain_storage.h"
#include "cryptonote_core/i_miner_handler.h"
#include "cryptonote_core/MinerConfig.h"
#include "crypto/hash.h"
#include "ICore.h"
#include "ICoreObserver.h"
#include "Common/ObserverManager.h"
#include <Logging/LoggerMessage.h>

namespace CryptoNote {

  struct core_stat_info;
  class miner;
  class CoreConfig;

  class core : public ICore, public i_miner_handler, public IBlockchainStorageObserver, public ITxPoolObserver {
   public:
     core(const Currency& currency, i_cryptonote_protocol* pprotocol, Logging::ILogger& logger);
     ~core();

     bool on_idle();
     virtual bool handle_incoming_tx(const blobdata& tx_blob, tx_verification_context& tvc, bool keeped_by_block);
     bool handle_incoming_block_blob(const blobdata& block_blob, block_verification_context& bvc, bool control_miner, bool relay_block);
     virtual i_cryptonote_protocol* get_protocol(){return m_pprotocol;}
     const Currency& currency() const { return m_currency; }

     //-------------------- i_miner_handler -----------------------
     virtual bool handle_block_found(Block& b);
     virtual bool get_block_template(Block& b, const AccountPublicAddress& adr, difficulty_type& diffic, uint32_t& height, const blobdata& ex_nonce);

     bool addObserver(ICoreObserver* observer);
     bool removeObserver(ICoreObserver* observer);

     miner& get_miner() { return *m_miner; }
     static void init_options(boost::program_options::options_description& desc);
     bool init(const CoreConfig& config, const MinerConfig& minerConfig, bool load_existing);
     bool set_genesis_block(const Block& b);
     bool deinit();

     // ICore
     virtual bool handle_get_objects(NOTIFY_REQUEST_GET_OBJECTS_request& arg, NOTIFY_RESPONSE_GET_OBJECTS_request& rsp) override;
     virtual bool getBackwardBlocksSizes(uint64_t fromHeight, std::vector<size_t>& sizes, size_t count) override;
     virtual bool getBlockSize(const crypto::hash& hash, size_t& size) override;
     virtual bool getAlreadyGeneratedCoins(const crypto::hash& hash, uint64_t& generatedCoins) override;
     virtual bool getBlockReward(size_t medianSize, size_t currentBlockSize, uint64_t alreadyGeneratedCoins, uint64_t fee,
                        bool penalizeFee, uint64_t& reward, int64_t& emissionChange) override;
     virtual bool scanOutputkeysForIndices(const TransactionInputToKey& txInToKey, std::list<std::pair<crypto::hash, size_t>>& outputReferences) override;
     virtual bool getBlockDifficulty(uint64_t height, difficulty_type& difficulty) override;
     virtual bool getBlockContainingTx(const crypto::hash& txId, crypto::hash& blockId, uint64_t& blockHeight) override;
     virtual bool getMultisigOutputReference(const TransactionInputMultisignature& txInMultisig, std::pair<crypto::hash, size_t>& output_reference) override;

     uint64_t get_current_blockchain_height();
     bool have_block(const crypto::hash& id);
     bool get_short_chain_history(std::list<crypto::hash>& ids);
     void on_synchronized();
     bool is_ready() override;

     virtual bool get_blockchain_top(uint64_t& heeight, crypto::hash& top_id);
     bool get_blocks(uint64_t start_offset, size_t count, std::list<Block>& blocks, std::list<Transaction>& txs);
     bool get_blocks(uint64_t start_offset, size_t count, std::list<Block>& blocks);
     template<class t_ids_container, class t_blocks_container, class t_missed_container>
     bool get_blocks(const t_ids_container& block_ids, t_blocks_container& blocks, t_missed_container& missed_bs)
     {
       return m_blockchain_storage.get_blocks(block_ids, blocks, missed_bs);
     }
     virtual bool queryBlocks(const std::list<crypto::hash>& block_ids, uint64_t timestamp,
         uint64_t& start_height, uint64_t& current_height, uint64_t& full_offset, std::list<BlockFullInfo>& entries);
     virtual crypto::hash getBlockIdByHeight(uint64_t height) override;
     void getTransactions(const std::vector<crypto::hash>& txs_ids, std::list<Transaction>& txs, std::list<crypto::hash>& missed_txs, bool checkTxPool = false) override;
     virtual bool getBlockByHash(const crypto::hash &h, Block &blk) override;
     //void get_all_known_block_ids(std::list<crypto::hash> &main, std::list<crypto::hash> &alt, std::list<crypto::hash> &invalid);

     bool get_alternative_blocks(std::list<Block>& blocks);
     size_t get_alternative_blocks_count();

     void set_cryptonote_protocol(i_cryptonote_protocol* pprotocol);
     void set_checkpoints(checkpoints&& chk_pts);

     std::vector<Transaction> getPoolTransactions() override;
     size_t get_pool_transactions_count();
     size_t get_blockchain_total_transactions();
     //bool get_outs(uint64_t amount, std::list<crypto::public_key>& pkeys);
     virtual bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, NOTIFY_RESPONSE_CHAIN_ENTRY_request& resp);
     virtual bool find_blockchain_supplement(const std::list<crypto::hash>& qblock_ids, std::list<std::pair<Block, std::list<Transaction> > >& blocks, uint64_t& total_height, uint64_t& start_height, size_t max_count);
     bool get_stat_info(core_stat_info& st_inf);
     
     virtual bool get_tx_outputs_gindexs(const crypto::hash& tx_id, std::vector<uint64_t>& indexs);
     crypto::hash get_tail_id();
     virtual bool get_random_outs_for_amounts(const COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_request& req, COMMAND_RPC_GET_RANDOM_OUTPUTS_FOR_AMOUNTS_response& res);
     void pause_mining();
     void update_block_template_and_resume_mining();
     blockchain_storage& get_blockchain_storage(){return m_blockchain_storage;}
     //debug functions
     void print_blockchain(uint64_t start_index, uint64_t end_index);
     void print_blockchain_index();
     std::string print_pool(bool short_format);
     void print_blockchain_outs(const std::string& file);
     virtual bool getPoolChanges(const crypto::hash& tailBlockId, const std::vector<crypto::hash>& knownTxsIds,
                                 std::vector<Transaction>& addedTxs, std::vector<crypto::hash>& deletedTxsIds) override;
     virtual void getPoolChanges(const std::vector<crypto::hash>& knownTxsIds, std::vector<Transaction>& addedTxs,
                                 std::vector<crypto::hash>& deletedTxsIds) override;

   private:
     bool add_new_tx(const Transaction& tx, const crypto::hash& tx_hash, const crypto::hash& tx_prefix_hash, size_t blob_size, tx_verification_context& tvc, bool keeped_by_block);
     bool load_state_data();
     bool parse_tx_from_blob(Transaction& tx, crypto::hash& tx_hash, crypto::hash& tx_prefix_hash, const blobdata& blob);
     bool handle_incoming_block(const Block& b, block_verification_context& bvc, bool control_miner, bool relay_block);

     bool check_tx_syntax(const Transaction& tx);
     //check correct values, amounts and all lightweight checks not related with database
     bool check_tx_semantic(const Transaction& tx, bool keeped_by_block);
     //check if tx already in memory pool or in main blockchain

     bool is_key_image_spent(const crypto::key_image& key_im);

     bool check_tx_ring_signature(const TransactionInputToKey& tx, const crypto::hash& tx_prefix_hash, const std::vector<crypto::signature>& sig);
     bool is_tx_spendtime_unlocked(uint64_t unlock_time);
     bool update_miner_block_template();
     bool handle_command_line(const boost::program_options::variables_map& vm);
     bool on_update_blocktemplate_interval();
     bool check_tx_inputs_keyimages_diff(const Transaction& tx);
     virtual void blockchainUpdated() override;
     virtual void txDeletedFromPool() override;
     void poolUpdated();

     const Currency& m_currency;
     Logging::LoggerRef logger;
     CryptoNote::RealTimeProvider m_timeProvider;
     tx_memory_pool m_mempool;
     blockchain_storage m_blockchain_storage;
     i_cryptonote_protocol* m_pprotocol;
     std::mutex m_incoming_tx_lock;
     std::unique_ptr<miner> m_miner;
     std::string m_config_folder;
     cryptonote_protocol_stub m_protocol_stub;
     friend class tx_validate_inputs;
     std::atomic<bool> m_starter_message_showed;
     tools::ObserverManager<ICoreObserver> m_observerManager;
   };
}
