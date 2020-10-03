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
    if(key.size() + value.size() > _max_size){
        return false;
    }

    if (iterator != _lru_index.end()) {
        int delta = int(iterator->second.get().value.size()) - int(value.size());
        moveToTail(iterator->second.get(), key);
        eraseifNeeds(delta);
        iterator->second.get().value = value;
        _used_size += delta;
        return true;
    } else {
        int income = int(key.size()) + int(value.size());
        auto *new_head = new lru_node(key, value);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(new_head->key),std::reference_wrapper<lru_node>(*new_head)));
        eraseifNeeds(income);
        iterator = _lru_index.find(key);
        if(_lru_head){
            new_head->prev = _lru_tail;
            _lru_tail->next.reset(new_head);
            _lru_tail = new_head;
        } else {
            new_head->prev = nullptr;
            _lru_tail = new_head;
            _lru_head.reset(new_head);
        }
        _used_size += income;
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto iterator = _lru_index.find(key);
    if (iterator != _lru_index.end() || (key.size() + value.size() > _max_size)) {
        return false;
    } else {
        int income = int(key.size()) + int(value.size());
        auto *new_head = new lru_node(key, value);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(new_head->key),std::reference_wrapper<lru_node>(*new_head)));
        eraseifNeeds(income);
        iterator = _lru_index.find(key);
        moveToTail(iterator->second.get(), key);
        _used_size += income;
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto iterator = _lru_index.find(key);
    if (iterator == _lru_index.end() || (key.size() + value.size() > _max_size)) {
        return false;
    } else {
        int delta = int(iterator->second.get().value.size()) - int(value.size());
        moveToTail(iterator->second.get(), key);
        eraseifNeeds(delta);
        iterator->second.get().value = value;
        _used_size += delta;
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto l = _lru_index.find(key);
    if (l == _lru_index.end()) {
        return false;
    }

     lru_node &deleter = l->second.get();
     _used_size -= (key.size() + deleter.value.size()); //Size OK

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

void SimpleLRU::moveToTail(lru_node& node, const std::string key) {
    if(key != _lru_tail->key){
        if (key!=_lru_head->key) {
            _lru_tail->next = std::move(node.prev->next);
            node.next->prev = node.prev;
            node.prev->next = std::move(node.next);
        } else {
            node.next->prev = nullptr;
            _lru_tail->next = std::move(_lru_head);
            _lru_head=std::move(node.next);
        }
        node.prev = _lru_tail;
        node.next = nullptr;
        _lru_tail = &node;
    }
}

void SimpleLRU::eraseifNeeds(int delta) {


}



} // namespace Backend
} // namespace Afina
