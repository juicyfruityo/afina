#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>
// для cout
// #include <iostream>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size),
                                        _current_size(0),
                                        _lru_tail(nullptr),
                                        _lru_head(nullptr)  {}

    ~SimpleLRU() {
        _lru_index.clear();

        auto ptr = _lru_head;
        while (ptr->next != nullptr) {
            ptr = ptr->next;
            ptr->prev.reset();
        }
        _lru_tail.reset(); // TODO: Here is stack overflow
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value)  override; //const

private:
    // LRU cache node
    using lru_node = struct lru_node {
        std::string key;
        std::string value;
        // поменял на shared_ptr
        std::shared_ptr<lru_node> prev;
        std::shared_ptr<lru_node> next;

        lru_node (const std::string &key, const std::string &value):
                  key(key), value(value), prev(nullptr), next(nullptr) {}
    };

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;
    std::size_t _current_size;

    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    // std::unique_ptr<lru_node> _lru_head;
    // lru_node *_lru_tail;
    // поменял на shared_ptr
    std::shared_ptr<lru_node> _lru_head, _lru_tail;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    // поменял на const std::string
    // потом совсем это убрал, т.к. не было перегруженной операции сравнения
    std::map<std::string, std::reference_wrapper<lru_node>> _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
