#include "SimpleLRU.h"
#include <memory>
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    std::size_t income_size = 0;
    auto iterator = _lru_index.find(key);

    /*Detect a size*/
    if(iterator!= _lru_index.end()){
        income_size = value.size() - iterator->second.get().value.size() + _temp_size;
        if(income_size > _max_size){
            return false;
        }
    } else /*new key*/ {
        income_size = key.size() + value.size();
        if(income_size > _max_size){
            return false;
        }
    }

    if(iterator!= _lru_index.end()){
        if(iterator->second.get().key != _lru_head->key){ /*move to a head*/
            lru_node* deleter = iterator->second.get().next->prev;
            iterator->second.get().next->prev = deleter->prev;
            deleter->prev->next=std::move(deleter->next);
            deleter->prev = _lru_head->prev;
            deleter->next=std::move(_lru_head);
            _lru_head.reset(deleter);
            deleter->value= value;
        }
        while(income_size + _temp_size > _max_size) {
            _lru_index.erase(_lru_head->prev->key);
            _temp_size-=(_lru_head->prev->value.size() + _lru_head->prev->key.size());
            _lru_head->prev = _lru_head->prev->prev;
            _lru_head->prev->next.reset(nullptr);
        }

    } else /*new key*/ {
        while(income_size + _temp_size > _max_size) {
            _lru_index.erase(_lru_head->prev->key);
            _temp_size-=(_lru_head->prev->value.size() + _lru_head->prev->key.size());
            _lru_head->prev = _lru_head->prev->prev;
            _lru_head->prev->next.reset(nullptr);
        }
    }
    if(iterator == _lru_index.end()){
        auto *first = new lru_node(key,value);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(first->key),std::reference_wrapper<lru_node>(*first)));
        if(_lru_head){
            _temp_size += key.size() + value.size();
            first->prev = _lru_head->prev;
            _lru_head->prev = first;
            first->next = std::move(_lru_head);
            _lru_head.reset(first);
        } else {
            _lru_head.reset(first);
            first->next = nullptr;
        }
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {

    auto l = _lru_index.find(key);
    if (l == _lru_index.end()){
        bool answer = Put(key,value);
        return answer;
    } else {
        return false;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    bool answer = PutIfAbsent(key,value);
    return answer;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto l = _lru_index.find(key);
    if (l == _lru_index.end()) {
        return false;
    }
    if (_lru_head->key == key) {
        _lru_head->next->prev = _lru_head->prev;
        _lru_head = std::move(_lru_head->next);
    } else {

        _temp_size = _temp_size - (key.size() + l->second.get().value.size());
        l->second.get().next->prev = l->second.get().prev;
        l->second.get().prev->next.reset(l->second.get().next.get());
        _lru_index.erase(key);
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto l = _lru_index.find(key);
    if (l == _lru_index.end()){
        return false;
    } else {
        value = l->second.get().value;
        return true;
    }
}

} // namespace Backend
} // namespace Afina
