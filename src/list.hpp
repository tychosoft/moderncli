// Copyright (C) 2025 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_LIST_HPP_
#define TYCHO_LIST_HPP_

#include <list>
#include <queue>
#include <stack>
#include <deque>
#include <algorithm>
#include <stdexcept>

namespace tycho {
template<typename T>
class list : public std::list<T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::list<T>::size_type;
    using iterator = typename std::list<T>::iterator;
    using const_iterator = typename std::list<T>::const_iterator;
    using reverse_iterator = typename std::list<T>::reverse_iterator;
    using const_reverse_iterator = typename std::list<T>::const_reverse_iterator;

    using std::list<T>::list;
    explicit list(const std::list<T>& other) : std::list<T>(other) {}
    explicit list(std::list<T>&& other) : std::list<T>(std::move(other)) {}

    template <typename Iterator>
    list(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    list(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    template <typename Pred>
    void pop_back_if(Pred pred) {
        while(pred(this->back()))
            this->pop_back();
    }

    template <typename Pred>
    void pop_front_if(Pred pred) {
        while(pred(this->front()))
            this->pop_front();
    }

    template <typename Func>
    void each(Func func) {
        for(auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter(Predicate pred) const {
        list<T> result;
        std::copy_if(this->begin(), this->end(), std::back_inserter(result), pred);
        return result;
    }

    auto sublist(size_type start, size_type last) const {
        if(start >= this->size() || last > this->size() || start > last)
            throw std::out_of_range("Invalid sublist range");
        list<T>  result;
        auto it = this->begin();
        std::advance(it, start);
        for(auto i = start; i < last; ++i)
            result.list_.push_back(*it++);
        return result;
    }

    auto sublist(iterator start_it, iterator end_it) const {
        if(std::distance(this->begin(), start_it) > std::distance(start_it, end_it) || std::distance(this->begin(), end_it) > this->size())
            throw std::out_of_range("Invalid sublist iterator range");
        list<T> result;
        std::copy(start_it, end_it, std::back_inserter(result));
        return result;
    }
};

template<typename T>
class queue : public std::queue<T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::queue<T>::size_type;
    using iterator = typename std::queue<T>::iterator;
    using const_iterator = typename std::queue<T>::const_iterator;

    using std::queue<T>::queue;
    explicit queue(const std::queue<T>& other) : std::queue<T>(other) {}
    explicit queue(std::queue<T>&& other) : std::queue<T>(std::move(other)) {}

    template <typename Iterator>
    queue(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    queue(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    template <typename Pred>
    void pop_if(Pred pred) {
        while(pred(this->front()))
            this->pop();
    }

    template <typename Func>
    void each(Func func) {
        for(auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter(Predicate pred) const {
        queue<T> result;
        std::copy_if(this->begin(), this->end(), std::back_inserter(result), pred);
        return result;
    }
};

template<typename T>
class stack : public std::stack<T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::stack<T>::size_type;
    using iterator = typename std::stack<T>::iterator;
    using const_iterator = typename std::stack<T>::const_iterator;

    using std::stack<T>::stack;
    explicit stack(const std::stack<T>& other) : std::stack<T>(other) {}
    explicit stack(std::stack<T>&& other) : std::stack<T>(std::move(other)) {}

    template <typename Iterator>
    stack(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    stack(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    template <typename Pred>
    void pop_if(Pred pred) {
        while(pred(this->front()))
            this->pop();
    }

    template <typename Func>
    void each(Func func) {
        for(auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter(Predicate pred) const {
        stack<T> result;
        std::copy_if(this->begin(), this->end(), std::front_inserter(result), pred);
        return result;
    }
};

template<typename T>
class deque : public std::deque<T> {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::deque<T>::size_type;
    using iterator = typename std::deque<T>::iterator;
    using const_iterator = typename std::deque<T>::const_iterator;
    using reverse_iterator = typename std::list<T>::reverse_iterator;
    using const_reverse_iterator = typename std::list<T>::const_reverse_iterator;

    using std::deque<T>::deque;
    explicit deque(const std::deque<T>& other) : std::deque<T>(other) {}
    explicit deque(std::deque<T>&& other) : std::deque<T>(std::move(other)) {}

    template <typename Iterator>
    deque(Iterator first, Iterator last) {
        std::copy(first, last, std::back_inserter(*this));
    }

    template <typename Iterator, typename Predicate>
    deque(Iterator first, Iterator last, Predicate pred) {
        std::copy_if(first, last, std::back_inserter(*this), pred);
    }

    operator bool() const {
        return !this->empty();
    }

    auto operator!() const {
        return this->empty();
    }

    template <typename Pred>
    void pop_back_if(Pred pred) {
        while(pred(this->back()))
            this->pop_back();
    }

    template <typename Pred>
    void pop_front_if(Pred pred) {
        while(pred(this->front()))
            this->pop_front();
    }

    template <typename Func>
    void each(Func func) {
        for(auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter(Predicate pred) const {
        deque<T> result;
        std::copy_if(this->begin(), this->end(), std::back_inserter(result), pred);
        return result;
    }
};

template <typename T>
class slist {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;

    struct node_t final {
        T data;
        node_t* next;

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
        size_++;
    }

    auto push(const T& value) {
        push_front(value);
    }

    auto front() -> auto& {
        if(head_ != nullptr)
            return head_->data;
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
        return copy;
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
        head_ = nullptr;
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

    template <typename... Args>
    void emplace(Args&&... args) {
        auto node = new node_t(T(std::forward<Args>(args)...));
        node->next = head_;
        head_ = node;
        size_++;
    }

    template <typename Func>
    void each(Func func) {
        for(auto& element : *this)
            func(element);
    }

    template <typename Predicate>
    auto filter(Predicate pred) const {
        queue<T> result;
        std::copy_if(this->begin(), this->end(), std::front_inserter(result), pred);
        return result;
    }

private:
    node_t* head_{nullptr};
    size_type size_{0};
};
} // end namespace
#endif
