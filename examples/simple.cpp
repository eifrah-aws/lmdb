#include "lmdb.h"

#include <cassert>
#include <iostream>
#include <sys/stat.h>

void check_return_code(int rc)
{
    if (rc != 0) {
        std::cerr << "error: " << mdb_strerror(rc) << std::endl;
    }
    assert(rc == 0);
}

int main(int argc, char** argv)
{
    MDB_env* env = nullptr;
    MDB_dbi dbi;
    MDB_val key, data;
    MDB_txn* txn = nullptr;

    // Directory must exist before run the test
    mkdir("testdb", 0755);

    // Create & open environment
    int rc = mdb_env_create(&env);
    rc = mdb_env_open(env, "./testdb", MDB_NOTLS | MDB_NOLOCK, 0664);
    check_return_code(rc);

    // Create a txn for creating the db handle
    rc = mdb_txn_begin(env, nullptr, MDB_NOTLS | MDB_NOLOCK, &txn);
    check_return_code(rc);

    rc = mdb_dbi_open(txn, nullptr, MDB_CREATE, &dbi);
    check_return_code(rc);

    rc = mdb_txn_commit(txn);
    check_return_code(rc);

    // dbi is still in shared memory and can be used by other txns
    rc = mdb_txn_begin(env, nullptr, MDB_NOTLS | MDB_NOLOCK, &txn);
    check_return_code(rc);

    const char* skey = "hello";
    const char* svalue = "world";
    key.mv_size = strlen(skey);
    key.mv_data = (void*)skey;
    data.mv_size = strlen(svalue);
    data.mv_data = (void*)svalue;

    rc = mdb_put(txn, dbi, &key, &data, 0);
    check_return_code(rc);

    rc = mdb_txn_commit(txn);
    check_return_code(rc);

    // Iterate over values
    MDB_cursor* cursor = nullptr;
    rc = mdb_txn_begin(env, NULL, MDB_RDONLY, &txn);
    check_return_code(rc);

    rc = mdb_cursor_open(txn, dbi, &cursor);
    check_return_code(rc);
    MDB_val read_key, read_data;
    while ((rc = mdb_cursor_get(cursor, &read_key, &read_data, MDB_NEXT)) == 0) {
        std::string_view k(reinterpret_cast<const char*>(read_key.mv_data), read_key.mv_size);
        std::string_view v(reinterpret_cast<const char*>(read_data.mv_data), read_data.mv_size);
        std::cout << "Key:" << k << ", Value:" << v << std::endl;
    }

    // Close the cursor and abort the txn that was associated with it
    mdb_cursor_close(cursor);
    mdb_txn_abort(txn);

    mdb_dbi_close(env, dbi);
    mdb_env_close(env);

    return 0;
}
