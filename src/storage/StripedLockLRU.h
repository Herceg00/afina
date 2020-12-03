#ifndef AFINA_STRIPEDLOCKLRU_H
#define AFINA_STRIPEDLOCKLRU_H

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <functional>

#include "SimpleLRU.h"
#include "ThreadSafeSimpleLRU.h"

namespace Afina {
namespace Backend {
/**
 * # StripeLockLRU thread safe version
 *
 *
 */
class StripeLockLRU : public SimpleLRU {
public:

    static StripeLockLRU* Build_Stripe(size_t max_size = 1024, size_t num_banks = 4) {
        size_t temp_capacity;
        if (num_banks != 0) {
            temp_capacity = max_size / num_banks;
        } else {
            throw std::runtime_error("Storage object creation failed");
        }
        if(temp_capacity < 256) {
            throw std::runtime_error("No point in using stripeLockLRU due to the small size");
        } else {
            return new StripeLockLRU(temp_capacity, num_banks);
        }
    }

    ~StripeLockLRU() {}

    bool Put(const std::string &key, const std::string &value) override {
        return banks_vector[hash_func(key)%bank_capacity] -> Put(key,value);
    }

    bool PutIfAbsent(const std::string &key, const std::string &value) override {
        return banks_vector[hash_func(key)%bank_capacity] -> PutIfAbsent(key,value);
    }

    bool Set(const std::string &key, const std::string &value) override {
        return banks_vector[hash_func(key)%bank_capacity] -> Set(key,value);
    }

    bool Delete(const std::string &key) override {
        return banks_vector[hash_func(key)%bank_capacity] -> Delete(key);
    }

    bool Get(const std::string &key, std::string &value) override {
        return banks_vector[hash_func(key)%bank_capacity] -> Get(key, value);
    }

private:
    StripeLockLRU(size_t capacity, size_t num_banks) { //add a private constructor
        bank_capacity = capacity;
        for (int bank = 0; bank < num_banks; bank++) {
            banks_vector.emplace_back(new ThreadSafeSimplLRU(capacity));
        }
    }

    std::hash<std::string> hash_func;
    std::size_t bank_capacity;
    std::vector<std::unique_ptr<Afina::Backend::ThreadSafeSimplLRU>> banks_vector;
};

} // namespace Backend
} // namespace Afina
#endif // AFINA_STRIPEDLOCKLRU_H
