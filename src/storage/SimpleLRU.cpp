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
//    if(iterator!= _lru_index.end()){
//        income_size = value.size() - iterator->second.get().value.size() + _temp_size;
//        if(income_size > _max_size){
//            return false;
//        }
//    } else /*new key*/ {
//        income_size = key.size() + value.size();
//        if(income_size > _max_size){
//            return false;
//        }
//    }

    /*Deleting tails in case of full cache.
    * Trying to push a new key first by overwriting existing key, then throw from a tail*/
//    if(income_size + _temp_size > _max_size) {
//        if(iterator!= _lru_index.end()){
//            Delete(key);
//        }
//        while (income_size + _temp_size > _max_size) {
//            _lru_index.erase(_lru_tail->value);
//            _temp_size -= (_lru_tail->value.size() + _lru_tail->key.size());
//            _lru_tail = _lru_tail->prev;
//            if(_lru_tail) {
//                _lru_tail->next.reset(nullptr);
//            }
//        }
//    }

    /*Move a node to the head*/
    if(iterator!= _lru_index.end()){
        if(key != _lru_head->key){
            lru_node &new_head = iterator->second.get();
            if(new_head.next) {
                new_head.next->prev = new_head.prev;
                new_head.prev->next=std::move(new_head.next);
                new_head.next=std::move(_lru_head);
                _lru_head.reset(&new_head);
            } else {
                _lru_tail = new_head.prev;
                new_head.next=std::move(_lru_head);
                _lru_head = std::move(new_head.prev->next);
            }
            new_head.prev = nullptr;
        } else {
            _lru_head->value = value;
        }
        _temp_size+=value.size() - _lru_head->value.size();
        return true;
    }

    if(iterator == _lru_index.end()){
        auto *first = new lru_node(key,value);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(first->key),std::reference_wrapper<lru_node>(*first)));
        if(_lru_head){
            _temp_size += key.size() + value.size();
            _lru_head->prev = first;
            first->next = std::move(_lru_head);
            _lru_head.reset(first);
        } else {
            _lru_head.reset(first);
            _lru_tail = first;
        }
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
     _temp_size = _temp_size - (key.size() + deleter.value.size());

    if (_lru_head->key == key) {
        if (_lru_head->next) {
            _lru_head->next->prev = nullptr;
            _lru_head.reset(_lru_head->next.get());
        } else {
            //_lru_head.reset(nullptr);
            _lru_tail = nullptr;
        }
    } else {
        if(deleter.next) {
            deleter.next->prev = deleter.prev;
            deleter.prev->next.reset(deleter.next.get());
        } else {
            _lru_tail = deleter.prev;
            //_lru_tail->next.reset(nullptr);
        }
    }
    _lru_index.erase(key);
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
