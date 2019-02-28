#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size)
        return false;
    auto iter = _lru_index.find(key);
    if (iter == _lru_index.end())
        return PutNew(key, value);
    else
        return PutOld(key, value, iter);
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size)
        return false;
    if (_lru_index.find(key) != _lru_index.end())
        return false;
    else
        return PutNew(key, value);
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size)
        return false;
    auto iter = _lru_index.find(key);
    if (iter == _lru_index.end())
        return false;

    return PutOld(key, value, iter);
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
    if (prev != nullptr)
        prev->next = next;
    else
        _lru_head = next;

    _current_size -= delete_memory;
    return true;
  }

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value)  { // const
    if (_lru_head == nullptr)
      return false;
    auto get_element = _lru_index.find(key);
    if (get_element == _lru_index.end())
        return false;

    value = get_element->second.get().value;
    RefreshList(key); // медленно работает?

    return true;
  }

bool SimpleLRU::PutNew(const std::string &key, const std::string & value) {
  const size_t insert_memory = key.size() + value.size();
  while (_max_size - _current_size < insert_memory) {
      if (_lru_head == nullptr)
          return false;
      DeleteLast();
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
  return true;
}

bool SimpleLRU::PutOld(const std::string &key, const std::string &value,
            my_map::iterator iterator) {
  auto old_value = iterator->second.get().value;
  iterator->second.get().value = value;
  RefreshList(key);

  _current_size += value.size() - old_value.size();
  return true;
}

bool SimpleLRU::RefreshList(const std::string &key) {
  auto ptr = _lru_head;
  if (ptr == nullptr)
      return false;

  while (ptr->key != key && ptr->next != nullptr)
      ptr = ptr->next;
  if (ptr->next == nullptr)
      return true;

  _lru_tail->next = ptr;
  auto prev = ptr->prev;
  auto next = ptr->next;

  next->prev = prev;
  if (prev != nullptr)
      prev->next = next;
  else
      _lru_head = next;

  ptr->prev = _lru_tail;
  ptr->next = nullptr;
  _lru_tail = ptr;

  return true;
}

bool SimpleLRU::DeleteLast() {
  if (_lru_head == nullptr)
      return false;

  auto next = _lru_head->next;
  auto value = _lru_head->value;
  auto key = _lru_head->key;
  std::size_t delete_memory = key.size() + value.size();

  _lru_index.erase(_lru_head->key);
  if (next == nullptr) {
      _lru_head.reset();
      _lru_tail = nullptr;
      _lru_head = nullptr;
  } else {
      _lru_head = next;
      _lru_head->prev = nullptr;
  }

  _current_size -= delete_memory;
  return true;
}


} // namespace Backend
} // namespace Afina
