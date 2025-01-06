// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_DICT_HPP_
#define TYCHO_DICT_HPP_

#include <map>
#include <unordered_map>
#include <algorithm>
#include <utility>
#include <stdexcept>

namespace tycho {
template<typename K, typename T>
class sort_map : public std::map<K,T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::map<K,T>::size_type;
    using pair_type = std::pair<const K, T>;
    using iterator = typename std::map<K,T>::iterator;
    using const_iterator = typename std::map<K,T>::const_iterator;

    using std::map<K,T>::map;
    explicit sort_map(const std::map<K,T>& from) : std::map<K,T>(from) {}
    explicit sort_map(std::map<K,T>&& from) : std::map<K,T>(std::move(from)) {}

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    auto contains(const K& key) const {
        return this->find(key) != this->end();
    }

    template<typename Pred>
    void remove_if(Pred pred) {
        for(auto it = this->begin(); it != this->end(); ) {
            if(pred(*it))
                it = this->erase(it);
            else
                ++it;
        }
    }

    template <typename Pred>
    auto filter(Pred pred) const {
        sort_map<K,T> result;
        for(auto& pair : *this) {
            if(pred(pair))
                result[pair.first] = pair.second;
        }
        return result;
    }

    template <typename Func>
    void each(Func func) const {
        for(auto& pair : *this)
            func(pair);
    }
};

template<typename K, typename T>
class hash_map : public std::unordered_map<K,T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::unordered_map<K,T>::size_type;
    using pair_type = std::pair<const K,T>;
    using iterator = typename std::unordered_map<K,T>::iterator;
    using const_iterator = typename std::unordered_map<K,T>::const_iterator;

    using std::unordered_map<K,T>::unordered_map;
    explicit hash_map(const std::unordered_map<K,T>& from) : std::unordered_map<K,T>(from) {}
    explicit hash_map(std::unordered_map<K,T>&& from) : std::unordered_map<K,T>(std::move(from)) {}

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    auto contains(const K& key) const {
        return this->find(key) != this->end();
    }

    template<typename Pred>
    void remove_if(Pred pred) {
        for(auto it = this->begin(); it != this->end(); ) {
            if(pred(*it))
                it = this->erase(it);
            else
                ++it;
        }
    }

    template <typename Func>
    void each(Func func) const {
        for(auto& pair : *this)
            func(pair);
    }

    template <typename Pred>
    auto filter(Pred pred) const {
        hash_map<K,T> result;
        for(auto& pair : *this) {
            if(pred(pair))
                result[pair.first] = pair.second;
        }
        return result;
    }
};
} // end namespace
#endif
