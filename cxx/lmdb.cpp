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
            m_txn = db->begin();
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
    Transaction* transaction() const { return m_txn; }

private:
    Transaction* m_txn = nullptr;
    bool m_owned = false;
};

void DB::open(std::string_view path)
{
    mkdir(path.data(), 0755);
    // Create & open environment
    int rc = mdb_env_create(&m_env);
    rc = mdb_env_open(m_env, path.data(), MDB_NOTLS | MDB_NOLOCK, 0664);
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

    TxnGuard txn_guard(this, txn);
    if (!txn_guard.has_transaction()) {
        return false;
    }

    MDB_val k, v;
    k.mv_size = key.length();
    k.mv_data = (void*)key.data();
    v.mv_size = value.length();
    v.mv_data = (void*)value.data();

    int rc = mdb_put(txn_guard.transaction(), m_dbi, &k, &v, 0);
    CHECK_RETURN_CODE_RET_FALSE(rc);

    rc = txn_guard.commit();
    CHECK_RETURN_CODE_RET_FALSE(rc);
    return true;
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
    return std::string_view(reinterpret_cast<const char*>(v.mv_data), v.mv_size);
}

Transaction* DB::begin()
{
    if (!is_open()) {
        return nullptr;
    }

    MDB_txn* txn = nullptr;
    int rc = mdb_txn_begin(m_env, nullptr, MDB_NOTLS | MDB_NOLOCK, &txn);
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
