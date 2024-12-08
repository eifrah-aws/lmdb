#include "lmdb.hpp"

#include <iostream>

int main(int argc, char** argv)
{
    lmdb::DB db;
    db.open("lmdb_cxx.db");

    {
        db.put("hello", "world");
        auto value = db.get("hello");
        std::cout << "hello=" << value.value_or("Not found!") << std::endl;
    }

    {
        // Use external txn
        auto txn = db.begin();
        db.put("key_1", "value_1", txn); // value is not committed yet
        db.put("key_2", "value_2", txn); // value is not committed yet
        db.commit(txn);

        // we can now read the values
        auto value1 = db.get("key_1");
        std::cout << "key_1=" << value1.value_or("Not found!") << std::endl;

        auto value2 = db.get("key_2");
        std::cout << "key_2=" << value2.value_or("Not found!") << std::endl;

        auto value3 = db.get("key_3");
        std::cout << "key_3=" << value3.value_or("Not found!") << std::endl;
    }
    return 0;
}
