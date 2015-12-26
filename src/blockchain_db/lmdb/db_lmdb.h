// Copyright (c) 2014, The Monero Project
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#pragma once

#include <atomic>

#include "blockchain_db/blockchain_db.h"
#include "cryptonote_protocol/blobdatatype.h" // for type blobdata

#include <lmdb.h>

#define ENABLE_AUTO_RESIZE

namespace cryptonote
{

struct mdb_txn_safe
{
  mdb_txn_safe();
  ~mdb_txn_safe();

  void commit(std::string message = "");

  // This should only be needed for batch transaction which must be ensured to
  // be aborted before mdb_env_close, not after. So we can't rely on
  // BlockchainLMDB destructor to call mdb_txn_safe destructor, as that's too late
  // to properly abort, since mdb_env_close would have been called earlier.
  void abort();

  operator MDB_txn*()
  {
    return m_txn;
  }

  operator MDB_txn**()
  {
    return &m_txn;
  }

  uint64_t num_active_tx() const;

  static void prevent_new_txns();
  static void wait_no_active_txns();
  static void allow_new_txns();

  MDB_txn* m_txn;
  bool m_batch_txn = false;
  static std::atomic<uint64_t> num_active_txns;

  // could use a mutex here, but this should be sufficient.
  static std::atomic_flag creation_gate;
};


// If m_batch_active is set, a batch transaction exists beyond this class, such
// as a batch import with verification enabled, or possibly (later) a batch
// network sync.
//
// For some of the lookup methods, such as get_block_timestamp(), tx_exists(),
// and get_tx(), when m_batch_active is set, the lookup uses the batch
// transaction. This isn't only because the transaction is available, but it's
// necessary so that lookups include the database updates only present in the
// current batch write.
//
// A regular network sync without batch writes is expected to open a new read
// transaction, as those lookups are part of the validation done prior to the
// write for block and tx data, so no write transaction is open at the time.
class BlockchainLMDB : public BlockchainDB
{
public:
  BlockchainLMDB(bool batch_transactions=false);
  ~BlockchainLMDB();

  virtual void open(const std::string& filename, const int mdb_flags=0);

  virtual void close();

  virtual void sync();

  virtual void reset();

  virtual std::vector<std::string> get_filenames() const;

  virtual std::string get_db_name() const;

  virtual bool lock();

  virtual void unlock();

  virtual bool block_exists(const crypto::hash& h) const;

  virtual block get_block(const crypto::hash& h) const;

  virtual uint64_t get_block_height(const crypto::hash& h) const;

  virtual block_header get_block_header(const crypto::hash& h) const;

  virtual block get_block_from_height(const uint64_t& height) const;

  virtual uint64_t get_block_timestamp(const uint64_t& height) const;

  virtual uint64_t get_top_block_timestamp() const;

  virtual size_t get_block_size(const uint64_t& height) const;

  virtual difficulty_type get_block_cumulative_difficulty(const uint64_t& height) const;

  virtual difficulty_type get_block_difficulty(const uint64_t& height) const;

  virtual uint64_t get_block_already_generated_coins(const uint64_t& height) const;

  virtual crypto::hash get_block_hash_from_height(const uint64_t& height) const;

  virtual std::vector<block> get_blocks_range(const uint64_t& h1, const uint64_t& h2) const;

  virtual std::vector<crypto::hash> get_hashes_range(const uint64_t& h1, const uint64_t& h2) const;

  virtual crypto::hash top_block_hash() const;

  virtual block get_top_block() const;

  virtual uint64_t height() const;

  virtual bool tx_exists(const crypto::hash& h) const;

  virtual uint64_t get_tx_unlock_time(const crypto::hash& h) const;

  virtual transaction get_tx(const crypto::hash& h) const;

  virtual uint64_t get_tx_count() const;

  virtual std::vector<transaction> get_tx_list(const std::vector<crypto::hash>& hlist) const;

  virtual uint64_t get_tx_block_height(const crypto::hash& h) const;

  virtual uint64_t get_num_outputs(const uint64_t& amount) const;

  virtual output_data_t get_output_data(const uint64_t& amount, const uint64_t& index);
  virtual output_data_t get_output_data(const uint64_t& global_index) const;
  virtual void get_output_data(const uint64_t &amount, const std::vector<uint64_t> &offsets, std::vector<output_data_t> &outputs);

  virtual tx_out_index get_output_tx_and_index_from_global(const uint64_t& index) const;
  virtual void get_output_tx_and_index_from_global(const std::vector<uint64_t> &global_indices,
		  std::vector<tx_out_index> &tx_out_indices) const;

  virtual tx_out_index get_output_tx_and_index(const uint64_t& amount, const uint64_t& index);
  virtual void get_output_tx_and_index(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<tx_out_index> &indices);
  virtual void get_output_global_indices(const uint64_t& amount, const std::vector<uint64_t> &offsets, std::vector<uint64_t> &indices);

  virtual std::vector<uint64_t> get_tx_output_indices(const crypto::hash& h) const;
  virtual std::vector<uint64_t> get_tx_amount_output_indices(const crypto::hash& h) const;

  virtual bool has_key_image(const crypto::key_image& img) const;

  virtual bool for_all_key_images(std::function<bool(const crypto::key_image&)>) const;
  virtual bool for_all_blocks(std::function<bool(uint64_t, const crypto::hash&, const cryptonote::block&)>) const;
  virtual bool for_all_transactions(std::function<bool(const crypto::hash&, const cryptonote::transaction&)>) const;
  virtual bool for_all_outputs(std::function<bool(uint64_t amount, const crypto::hash &tx_hash, size_t tx_idx)> f) const;

  virtual uint64_t add_block( const block& blk
                            , const size_t& block_size
                            , const difficulty_type& cumulative_difficulty
                            , const uint64_t& coins_generated
                            , const std::vector<transaction>& txs
                            );

  virtual void set_batch_transactions(bool batch_transactions);
  virtual void batch_start(uint64_t batch_num_blocks=0);
  virtual void batch_commit();
  virtual void batch_stop();
  virtual void batch_abort();

  virtual void pop_block(block& blk, std::vector<transaction>& txs);

  virtual bool can_thread_bulk_indices() const { return true; }
private:
  void do_resize(uint64_t size_increase=0);

  bool need_resize(uint64_t threshold_size=0) const;
  void check_and_resize_for_batch(uint64_t batch_num_blocks);
  uint64_t get_estimated_batch_size(uint64_t batch_num_blocks) const;

  virtual void add_block( const block& blk
                , const size_t& block_size
                , const difficulty_type& cumulative_difficulty
                , const uint64_t& coins_generated
                , const crypto::hash& block_hash
                );

  virtual void remove_block();

  virtual void add_transaction_data(const crypto::hash& blk_hash, const transaction& tx, const crypto::hash& tx_hash);

  virtual void remove_transaction_data(const crypto::hash& tx_hash, const transaction& tx);

  virtual void add_output(const crypto::hash& tx_hash, const tx_out& tx_output, const uint64_t& local_index, const uint64_t unlock_time);

  virtual void remove_output(const tx_out& tx_output);

  void remove_tx_outputs(const crypto::hash& tx_hash, const transaction& tx);

  void remove_output(const uint64_t& out_index, const uint64_t amount);
  void remove_amount_output_index(const uint64_t amount, const uint64_t global_output_index);

  virtual void add_spent_key(const crypto::key_image& k_image);

  virtual void remove_spent_key(const crypto::key_image& k_image);

  // Hard fork
  virtual void set_hard_fork_starting_height(uint8_t version, uint64_t height);
  virtual uint64_t get_hard_fork_starting_height(uint8_t version) const;
  virtual void set_hard_fork_version(uint64_t height, uint8_t version);
  virtual uint8_t get_hard_fork_version(uint64_t height) const;

  /**
   * @brief convert a tx output to a blob for storage
   *
   * @param output the output to convert
   *
   * @return the resultant blob
   */
  blobdata output_to_blob(const tx_out& output) const;

  /**
   * @brief convert a tx output blob to a tx output
   *
   * @param blob the blob to convert
   *
   * @return the resultant tx output
   */
  tx_out output_from_blob(const blobdata& blob) const;

  /**
   * @brief get the global index of the index-th output of the given amount
   *
   * @param amount the output amount
   * @param index the index into the set of outputs of that amount
   *
   * @return the global index of the desired output
   */
  uint64_t get_output_global_index(const uint64_t& amount, const uint64_t& index);

  void check_open() const;

  virtual bool is_read_only() const;

  // fix up anything that may be wrong due to past bugs
  virtual void fixup();

  MDB_env* m_env;

  MDB_dbi m_blocks;
  MDB_dbi m_block_heights;
  MDB_dbi m_block_hashes;
  MDB_dbi m_block_timestamps;
  MDB_dbi m_block_sizes;
  MDB_dbi m_block_diffs;
  MDB_dbi m_block_coins;

  MDB_dbi m_txs;
  MDB_dbi m_tx_unlocks;
  MDB_dbi m_tx_heights;
  MDB_dbi m_tx_outputs;

  MDB_dbi m_output_txs;
  MDB_dbi m_output_indices;
  MDB_dbi m_output_amounts;
  MDB_dbi m_output_data;

  MDB_dbi m_spent_keys;

  MDB_dbi m_hf_starting_heights;
  MDB_dbi m_hf_versions;

  MDB_dbi m_properties;

  uint64_t m_height;
  uint64_t m_num_outputs;
  std::string m_folder;
  mdb_txn_safe* m_write_txn; // may point to either a short-lived txn or a batch txn
  mdb_txn_safe* m_write_batch_txn; // persist batch txn outside of BlockchainLMDB

  bool m_batch_transactions; // support for batch transactions
  bool m_batch_active; // whether batch transaction is in progress

#if defined(__arm__)
  // force a value so it can compile with 32-bit ARM
  constexpr static uint64_t DEFAULT_MAPSIZE = 1LL << 31;
#else
#if defined(ENABLE_AUTO_RESIZE)
  constexpr static uint64_t DEFAULT_MAPSIZE = 1LL << 30;
#else
  constexpr static uint64_t DEFAULT_MAPSIZE = 1LL << 33;
#endif
#endif

  constexpr static float RESIZE_PERCENT = 0.8f;
};

}  // namespace cryptonote
