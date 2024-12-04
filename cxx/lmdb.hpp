#pragma once

#include <optional>
#include <string>
#include <string_view>

typedef struct MDB_env MDB_env;
typedef unsigned int MDB_dbi;
typedef struct MDB_cursor MDB_cursor;
typedef struct MDB_txn MDB_txn;

namespace lmdb
{
typedef MDB_txn Transaction;

enum OpenFlags {
    kNone = 0,
    /// Don't flush system buffers to disk when committing a transaction
    kAsync = 1 << 0,
    /// Don't do any locking. If concurrent access is anticipated, the caller must manage all concurrency itself
    kNoLocking = 1 << 1,
    /// Don't use Thread-Local Storage
    kNoThreadLocalStorage = 1 << 2,
    /// Optimized for single threaded, max performance
    kDefault = kAsync | kNoLocking | kNoThreadLocalStorage,
};

class DB
{
public:
    DB() {}
    ~DB() {}

    void open(std::string_view path, size_t flags = OpenFlags::kDefault);
    void close();
    bool put(std::string_view key, std::string_view value, Transaction* txn = nullptr);
    std::optional<std::string_view> get(std::string_view key, Transaction* txn = nullptr);

    // transaction management
    Transaction* begin();
    void abort(Transaction* txn);
    bool commit(Transaction* txn);
    bool is_open() const { return m_is_opened; }
    std::string_view last_error() { return m_last_err; }

protected:
    /// Try putting an entry returning lmdb underlying error code
    int try_put(std::string_view key, std::string_view value, Transaction* txn = nullptr);

private:
    friend class TxnGuard;
    MDB_env* m_env = nullptr;
    MDB_dbi m_dbi = 0;
    bool m_is_opened = false;
    std::string m_last_err;
    size_t m_flags = 0;
    size_t m_map_size = (1 << 30);
};

}; // namespace lmdb