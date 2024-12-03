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

class DB
{
public:
    DB() {}
    ~DB() {}

    void open(std::string_view path);
    void close();
    bool put(std::string_view key, std::string_view value, Transaction* txn = nullptr);
    std::optional<std::string_view> get(std::string_view key, Transaction* txn = nullptr);

    // transaction management
    Transaction* begin();
    void abort(Transaction* txn);
    bool commit(Transaction* txn);
    bool is_open() const { return m_is_opened; }

private:
    MDB_env* m_env = nullptr;
    MDB_dbi m_dbi = 0;
    bool m_is_opened = false;
    std::string m_last_err;
};

}; // namespace lmdb