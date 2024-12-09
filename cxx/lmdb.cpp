#include "lmdb.hpp"

#include "lmdb.h" // C API

#include <sys/stat.h>

#define __CHECK_RETURN_CODE_RETURN(rc, rv)          \
    if (rc != 0) {                                  \
        m_last_err = std::string(mdb_strerror(rc)); \
        m_is_opened = false;                        \
        return rv;                                  \
    }

#define CHECK_RETURN_CODE(rc) __CHECK_RETURN_CODE_RETURN(rc, )
#define CHECK_RETURN_CODE_RET_FALSE(rc) __CHECK_RETURN_CODE_RETURN(rc, false)
#define CHECK_RETURN_CODE_RET_NULL(rc) __CHECK_RETURN_CODE_RETURN(rc, nullptr)

namespace lmdb
{

struct TxnGuard final {
    TxnGuard(DB* db, Transaction* txn)
    {
        if (txn) {
            m_txn = txn;
        } else {
            m_rc = mdb_txn_begin(db->m_env, nullptr, db->m_flags, &m_txn);
            m_owned = true;
        }
    }

    ~TxnGuard()
    {
        if (m_owned && m_txn) {
            // Abort it
            mdb_txn_abort(m_txn);
            m_txn = nullptr;
        }
    }

    /// If the txn is owned by this guard, commit it and return the status code
    int commit()
    {
        int rc = 0;
        if (m_owned && m_txn) {
            rc = mdb_txn_commit(m_txn);
            m_txn = nullptr;
        }
        return rc;
    }

    /// Does this guard has an active txn?
    bool has_transaction() const { return m_txn != nullptr; }
    /// In case we failed to create new transaction, return the error code
    int last_error_code() const { return m_rc; }
    Transaction* transaction() const { return m_txn; }

private:
    Transaction* m_txn = nullptr;
    bool m_owned = false;
    int m_rc = 0;
};

void DB::open(std::string_view path, size_t flags)
{
    m_flags = 0;
    if (flags & kAsync) {
        m_flags |= (MDB_NOSYNC | MDB_NOMETASYNC);
    }

    if (m_flags & kNoLocking) {
        m_flags |= MDB_NOLOCK;
    }

    if (m_flags & kNoThreadLocalStorage) {
        m_flags |= MDB_NOTLS;
    }

    mkdir(path.data(), 0755);
    // Create & open environment
    int rc = mdb_env_create(&m_env);
    CHECK_RETURN_CODE(rc);

    // Set the initial size of the map to 1GB
    rc = mdb_env_set_mapsize(m_env, m_map_size);
    CHECK_RETURN_CODE(rc);

    rc = mdb_env_open(m_env, path.data(), m_flags | MDB_NORDAHEAD | MDB_NOMEMINIT, 0664);
    CHECK_RETURN_CODE(rc);

    // Create a txn for creating the db handle
    MDB_txn* txn = nullptr;
    rc = mdb_txn_begin(m_env, nullptr, MDB_NOTLS | MDB_NOLOCK, &txn);
    CHECK_RETURN_CODE(rc);

    rc = mdb_dbi_open(txn, nullptr, MDB_CREATE, &m_dbi);
    CHECK_RETURN_CODE(rc);

    rc = mdb_txn_commit(txn);
    CHECK_RETURN_CODE(rc);

    m_is_opened = true;
    m_last_err.clear();
}

void DB::close()
{
    mdb_dbi_close(m_env, m_dbi);
    mdb_env_close(m_env);

    m_env = nullptr;
    m_dbi = 0;
    m_is_opened = false;
}

bool DB::put(std::string_view key, std::string_view value, Transaction* txn)
{
    if (!is_open()) {
        m_last_err = "Database is not opened";
        return false;
    }

    int error_code = try_put(key, value, txn);
    switch (error_code) {
    case 0:
        return true;
    case MDB_MAP_FULL:
        // increase the map size and retry
        m_map_size *= 2;
        error_code = mdb_env_set_mapsize(m_env, m_map_size);
        CHECK_RETURN_CODE_RET_FALSE(error_code);
        return try_put(key, value, txn) == 0;
    default:
        m_last_err = mdb_strerror(error_code);
        return false;
    }
}

int DB::try_put(std::string_view key, std::string_view value, Transaction* txn)
{
    TxnGuard txn_guard(this, txn);
    if (!txn_guard.has_transaction()) {
        return txn_guard.last_error_code();
    }

    MDB_val k, v;
    k.mv_data = (void*)key.data();
    k.mv_size = key.length();

    v.mv_data = (void*)value.data();
    v.mv_size = value.length();

    int rc = mdb_put(txn_guard.transaction(), m_dbi, &k, &v, 0);
    if (rc != 0) {
        return rc;
    }

    rc = txn_guard.commit();
    return rc;
}

std::optional<std::string_view> DB::get(std::string_view key, Transaction* txn)
{
    if (!is_open()) {
        m_last_err = "Database is not opened";
        return {};
    }

    TxnGuard txn_guard(this, txn);
    if (!txn_guard.has_transaction()) {
        return {};
    }

    MDB_val k, v;
    k.mv_size = key.length();
    k.mv_data = (void*)key.data();

    int rc = mdb_get(txn_guard.transaction(), m_dbi, &k, &v);
    if (rc != MDB_SUCCESS) {
        m_last_err = std::string(mdb_strerror(rc));
        return {};
    }
    return std::string_view(reinterpret_cast<const char*>(v.mv_data), v.mv_size);
}

Transaction* DB::begin()
{
    if (!is_open()) {
        return nullptr;
    }

    MDB_txn* txn = nullptr;
    int rc = mdb_txn_begin(m_env, nullptr, m_flags, &txn);
    CHECK_RETURN_CODE_RET_NULL(rc);
    return txn;
}

void DB::abort(Transaction* txn) { mdb_txn_abort(txn); }
bool DB::commit(Transaction* txn)
{
    int rc = mdb_txn_commit(txn);
    CHECK_RETURN_CODE_RET_FALSE(rc);
    return true;
}

} // namespace lmdb
