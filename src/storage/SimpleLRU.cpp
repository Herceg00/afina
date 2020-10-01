#include "SimpleLRU.h"
#include <memory>
#include <iostream>
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    std::size_t income_size = 0;
    auto iterator = _lru_index.find(key);

//    /*Detect a size*/
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

    /*Deleting tails in case of full cache.
    * Trying to push a new key first by overwriting existing key, then throw from a tail*/
    if(income_size + _temp_size > _max_size) {
        if(iterator!= _lru_index.end()){
            Delete(key);
        }
        while (income_size + _temp_size > _max_size) {
            _temp_size -= (_lru_head->value.size() + _lru_head->key.size());
            Delete(_lru_head->key);
        }
    }

    /*Move a node to the tail*/
    if(iterator!= _lru_index.end()){
        if(key != _lru_tail->key){
            lru_node &new_head = iterator->second.get();
            if(key!=_lru_head->key) {
                _lru_tail->next = std::move(new_head.prev->next);
                new_head.next->prev = new_head.prev;
                new_head.prev->next = std::move(new_head.next);
            } else {
                new_head.next->prev = nullptr;
                _lru_tail->next = std::move(_lru_head);
                _lru_head=std::move(new_head.next);
            }
            new_head.prev = _lru_tail;
            new_head.next = nullptr;
            _lru_tail = &new_head;
            _lru_tail->value = value;
        } else {
            _lru_tail->value = value;
        }
        _temp_size+=value.size() - _lru_head->value.size();
        return true;
    }

    if(iterator == _lru_index.end()){
        auto *first = new lru_node(key,value);
        if(_lru_head){
            first->prev = _lru_tail;
            _lru_tail->next.reset(first);
            _lru_tail = first;
        } else {
            first->prev = nullptr;
            _lru_tail = first;
            _lru_head.reset(first);
        }
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(_lru_tail->key),std::reference_wrapper<lru_node>(*_lru_tail)));
        _temp_size+=(key.size() + value.size());
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
    auto l = _lru_index.find(key);
    if (l == _lru_index.end()){
        return false;
    } else {
        bool answer = Put(key,value);
        return answer;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto l = _lru_index.find(key);
    if (l == _lru_index.end()) {
        return false;
    }

     lru_node &deleter = l->second.get();
     _temp_size -= (key.size() + deleter.value.size()); //Size OK

    if (_lru_head->key == key) {
        if(!deleter.next) {
            _lru_tail = nullptr;
            _lru_head.reset();
        } else {
            _lru_head = std::move(_lru_head->next);
            _lru_head->prev = nullptr;
        }
    } else {
        if (_lru_tail->key == key) {
            _lru_tail = deleter.prev;
            _lru_tail->next.reset();
        } else {
            deleter.next->prev = deleter.prev;
            deleter.prev->next = std::move(deleter.next);
        }
    }
    _lru_index.erase(l);
    return true;
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
