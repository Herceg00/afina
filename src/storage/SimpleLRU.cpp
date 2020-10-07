#include "SimpleLRU.h"
#include <memory>
#include <iostream>
namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value)
{
    if(key.size() + value.size() > _max_size){
        return false;
    }

    auto iterator = _lru_index.find(key);
    if (iterator != _lru_index.end()) {
        int delta = 0;
        delta = int(iterator->second.get().value.size()) - int(value.size());
        moveToTail(iterator->second.get());
        eraseifNeeds(delta);
        iterator->second.get().value = value;
        _used_size += delta;
        return true;
    } else {
        int delta = int(key.size()) + int(value.size());
        auto *new_node = new lru_node(key, value);
        eraseifNeeds(delta);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(new_node->key),std::reference_wrapper<lru_node>(*new_node)));
        _used_size += delta;
        return putToTail(new_node);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    auto iterator = _lru_index.find(key);
    if (iterator != _lru_index.end() || (key.size() + value.size() > _max_size)) {
        return false;
    } else {
        int delta = int(key.size()) + int(value.size());
        auto *new_node = new lru_node(key, value);
        eraseifNeeds(delta);
        _lru_index.insert(std::make_pair(std::reference_wrapper<const std::string>(new_node->key),std::reference_wrapper<lru_node>(*new_node)));
        _used_size += delta;
        return putToTail(new_node);
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    auto iterator = _lru_index.find(key);
    if (iterator == _lru_index.end() || (key.size() + value.size() > _max_size)) {
        return false;
    } else {
        int delta = int(iterator->second.get().value.size()) - int(value.size());
        moveToTail(iterator->second.get());
        eraseifNeeds(delta);
        iterator->second.get().value = value;
        _used_size += delta;
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto iterator = _lru_index.find(key);
    if (iterator == _lru_index.end()) {
        return false;
    }

     lru_node &deleter = iterator->second.get();
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
    _lru_index.erase(iterator);
    return true;
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    auto iterator = _lru_index.find(key);
    if (iterator == _lru_index.end()){
        return false;
    } else {
        value = iterator->second.get().value;
        return true;
    }
}

void SimpleLRU::moveToTail(lru_node& node) {
    if(node.key != _lru_tail->key){
        if (node.key !=_lru_head->key) {
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
    while(delta + _used_size > _max_size){
        Delete(_lru_head->key);
    }
}

bool SimpleLRU::putToTail(lru_node* new_node){
    if(_lru_head){
        new_node->prev = _lru_tail;
        _lru_tail->next.reset(new_node);
    } else {
        new_node->prev = nullptr;
        _lru_head.reset(new_node);
    }
    _lru_tail = new_node;
    return true;
}

} // namespace Backend
} // namespace Afina