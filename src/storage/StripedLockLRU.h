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

    StripeLockLRU(size_t max_size = 1024, size_t num_banks = 4){
        for (int bank = 0; bank < num_banks; bank++){
            bank_capacity = max_size/num_banks;
            banks_vector.emplace_back(new ThreadSafeSimplLRU(bank_capacity));
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
    std::hash<std::string> hash_func;
    std::size_t bank_capacity;
    std::vector<std::unique_ptr<Afina::Backend::ThreadSafeSimplLRU>> banks_vector;
};

} // namespace Backend
} // namespace Afina
#endif // AFINA_STRIPEDLOCKLRU_H
