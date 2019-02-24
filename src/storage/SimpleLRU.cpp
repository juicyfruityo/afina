#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        // std::cout << "False";
        return false;
      }
    if (_lru_index.find(key) != _lru_index.end()){
        // std::cout << "Need to delete" << std::endl;
        Delete(key);
      }
    // std::cout << "Make Put()" << std::endl;
    return PutIfAbsent(key, value);
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size)
        return false;
    if (_lru_index.find(key) != _lru_index.end())
        return false;

    const size_t insert_memory = key.size() + value.size();
    while (_max_size - _current_size < insert_memory) {
        if (_lru_head == nullptr)
            return false;
        Delete(_lru_head->key);
    }

    auto new_element = std::make_shared<lru_node>(key, value);
    if (_lru_tail != nullptr) {
        new_element->prev = _lru_tail;
        _lru_tail->next = new_element;
        _lru_tail = new_element;
    } else {
        _lru_tail = new_element;
        _lru_head = new_element;
    }

    _lru_index.insert(std::make_pair(key,
                                     std::reference_wrapper<lru_node>(*_lru_tail)));

    _current_size += insert_memory;
    // std::cout << "Make PutIfAbsent()" << std::endl;
    return true;
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size)
        return false;
    if (_lru_index.find(key) == _lru_index.end())
        return false;

    return Put(key, value);
    // return false;
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    if (_lru_head == nullptr)
        return false;

    auto delete_element = _lru_index.find(key);
    if (delete_element == _lru_index.end())
        return false;

    auto value = delete_element->second.get().value;
    std::size_t delete_memory = key.size() + value.size();

    auto next = delete_element->second.get().next;
    auto prev = delete_element->second.get().prev;
    _lru_index.erase(key);

    if (next != nullptr)
        next->prev = prev;
    else
        _lru_tail = prev;
        // std::cout << "I'm here  " << key << std::endl;
        // _lru_tail->next = nullptr; // ошибка здесь
    if (prev != nullptr)
        prev->next = next;
    else
        _lru_head = next;
        // _lru_head->prev = nullptr;

    _current_size -= delete_memory;
    // std::cout << "Make Delete()" << std::endl;
    return true;
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) const { // const
    if (_lru_head == nullptr)
      return false;
    auto get_element = _lru_index.find(key);
    if (get_element == _lru_index.end())
        return false;

    value = get_element->second.get().value;
    // new:
    // auto reindex_element = get_element->second.get();
    // if (reindex_element.next != nullptr) {
    //     auto next = reindex_element.next;
    //     auto prev = reindex_element.prev;
    //
    //     next->prev = prev;
    //     if (prev != nullptr)
    //         prev->next = next;
    //     else
    //         _lru_head = next;
    //
    //     auto new_element = std::make_shared<lru_node>(key, value);
    //     if (_lru_tail != nullptr) {
    //         new_element->prev = _lru_tail;
    //         _lru_tail->next = new_element;
    //         _lru_tail = new_element;
    //     } else {
    //         _lru_tail = new_element;
    //         _lru_head = new_element;
    //     }
    // }
// не хочет работать Delete Put

    return true;
  }

} // namespace Backend
} // namespace Afina
