// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_LIST_HPP_
#define TYCHO_LIST_HPP_

#include <list>
#include <algorithm>
#include <stdexcept>

namespace tycho {
template <typename T>
class slist {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;

    struct node_t final {
        T data;
        node_t* next{nullptr};
        explicit node_t(const T& value) : data(value), next(nullptr) {}
    };

    class iterator {
    public:
        explicit iterator(node_t* node) : current_(node) {}

        auto operator*() -> auto& {
            return current_->data;
        }

        auto operator++() -> auto& {
            if(current_)
                current_ = current_->next;
            return *this;
        }

        auto operator!=(const iterator& other) const {
            return current_ != other.current_;
        }

    private:
        node_t* current_{nullptr};
    };

    class const_iterator {
    public:
        explicit const_iterator(const node_t* node) : current_(node) {}

        auto operator*() const -> auto& {
            return current_->data;
        }

        auto operator++() -> auto& {
            if(current_)
                current_ = current_->next;
            return *this;
        }

        auto operator!=(const const_iterator& other) const {
            return current_ != other.current_;
        }

    private:
        const node_t* current_{nullptr};
    };

    slist() = default;
    slist(const std::initializer_list<T>& list) {
        for(const auto& item : list)
            push_back(item);
    }

    ~slist() {
        clear();
    }

    operator bool() {
        return !empty();
    }

    auto operator!() {
        return empty();
    }

    void push_front(const T& value) {
        auto node = new node_t(value);
        node->next = head_;
        head_ = node;
        ++size_;
        if(!tail_)
            tail_ = head_;
    }

    void push_back(const T& value) {
        if(head_ == nullptr)
            push_front(value);
        else {
            auto node = new node_t(value);
            tail_->next = node;
            tail_ = node;
            ++size_;
        }
    }

    auto push(const T& value) {
        push_front(value);
    }

    auto front() -> auto& {
        if(head_ != nullptr)
            return head_->data;
        throw std::runtime_error("List is empty");
    }

    auto back() -> auto& {
        if(tail_ != nullptr)
            return tail_->data;
        throw std::runtime_error("List is empty");
    }

    auto pop() {
        if(head_ == nullptr)
            throw std::runtime_error("List is empty");

        auto copy = head_->data;
        auto next = head_->next;
        delete head_;
        head_ = next;
        --size_;
        if(!head_)
            tail_ = nullptr;
        return copy;
    }

    template <typename Pred>
    void pop_if(Pred pred) {
        if(head_ == nullptr)
            throw std::runtime_error("List is empty");
        while(head_ && pred(head_->data))
            pop();
    }

    auto empty() const {
        return head_ == nullptr;
    }

    auto size() const {
        return size_;
    }

    void clear() {
        node_t* current = head_;
        node_t* next = nullptr;

        while(current != nullptr) {
            next = current->next;
            delete current;
            current = next;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    auto begin() {
        return iterator(head_);
    }

    auto end() {
        return iterator(nullptr);
    }

    auto begin() const {
        return const_iterator(head_);
    }

    auto end() const {
        return const_iterator(nullptr);
    }

    auto contains(const T& value) const {
        return std::find(begin(), end(), value) != end();
    }

    template <typename... Args>
    void emplace_front(Args&&... args) {
        auto node = new node_t(T(std::forward<Args>(args)...));
        node->next = head_;
        head_ = node;
        if(!tail_)
            tail_ = node;
        ++size_;
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        auto node = new node_t(T(std::forward<Args>(args)...));
        if(tail_) {
            tail_->next = node;
            tail_ = node;
        }
        else
            head_ = tail_ = node;
        ++size_;
    }

    template <typename Func>
    void each(Func func) {
        for(auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter_if(Predicate pred) const {
        slist<T> result;
        std::copy_if(this->begin(), this->end(), std::front_inserter(result), pred);
        return result;
    }

private:
    node_t *head_{nullptr}, *tail_{nullptr};
    size_type size_{0};
};
} // end namespace
#endif
