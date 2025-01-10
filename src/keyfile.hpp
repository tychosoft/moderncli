// Copyright (C) 2020 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_KEYFILE_HPP_
#define TYCHO_KEYFILE_HPP_

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <string>

namespace tycho {
class keyfile {
public:
    using keys = std::unordered_map<std::string, std::string>;
    using iterator = std::unordered_map<std::string, keys>::const_iterator;

    keyfile() noexcept : ptr_(std::make_shared<keyfile::data>()) {}
    explicit keyfile(const std::initializer_list<std::string>& paths) noexcept :    ptr_(std::make_shared<keyfile::data>()) {
        for(const auto& path : paths)
            ptr_->load(path);
    }

    auto operator[](const std::string& id) -> auto& {
        return ptr_->fetch(id);
    }

    auto operator[](const std::string& id) const -> const auto& {
        return ptr_->fetch(id);
    }

    auto at(const std::string& id = "_") const {
        return ptr_->fetch(id);
    }

    auto get_or(const std::string& id, const std::string& or_else = "_") const {
        if(!ptr_->exists(id))
            return ptr_->fetch(or_else);
        return ptr_->fetch(id);
    }

    auto keyset(const std::string& id = "_") -> auto& {
        return ptr_->fetch(id);
    }

    auto exists(const std::string& id = "_") const {
        return ptr_ ? ptr_->exists(id) : false;
    }

    auto contains(const std::string& id) const {
        return ptr_ ? ptr_->exists(id) : false;
    }

    auto operator!() const {
        return !ptr_;
    }

    void remove(const std::string& id) {
        if(ptr_) ptr_->remove(id);
    }

    auto load(const std::string& path) -> auto& {
        if(ptr_) ptr_->load(path);
        return *this;
    }

    auto load(const std::string& id, const std::initializer_list<std::pair<std::string,std::string>>& list) -> auto& {
        auto& group = ptr_->fetch(id);
        for(const auto& [key, value] : list)
            group[key] = value;
        return *this;
    }

    void clear() {
        if(ptr_)
            ptr_.reset();
    }

    auto flatten(const std::string& id = "_") const {
        std::string result;
        const auto& list = ptr_->fetch(id);
        for(const auto& [key, value] : list)
            result += key + "='" + value + "' ";
        return result;
    }

    auto write(const std::string& path) const {
        return ptr_ ? ptr_->save(path) : false;
    }

    auto empty() const {
        return !ptr_ || ptr_->empty();
    }

    auto begin() const -> iterator {
        return ptr_->cbegin();
    }

    auto end() const -> iterator {
        return ptr_->cend();
    }

    static auto create(const std::initializer_list<std::string>& list) {
        keyfile keys;
        for(const auto& group : list)
            keys.ptr_->fetch(group);
        return keys;
    }

    static auto create() {
        return keyfile();
    }

private:
    class data final {
    public:
        data() = default;
        data(data const&) = delete;
        auto operator=(const data&) -> auto& = delete;

        auto empty() const -> bool {
            return sections.empty();
        }

        auto cbegin() const -> iterator {
            return sections.cbegin();
        }

        auto cend() const -> iterator {
            return sections.cend();
        }

        auto exists(const std::string& id) const -> bool {
            return sections.count(id) > 0;
        }

        void remove(const std::string& id) {
            sections.erase(id);
        }

        auto fetch(const std::string& id) -> keys& {
            return sections[id];
        }

        auto load(const std::string& path) -> bool {
            const std::string_view whitespace(" \t\n\r");
            std::ifstream file(path);
            if(!file.is_open())
                return false;

            std::string buffer;
            std::string section = "_";

            while(std::getline(file, buffer)) {
                auto input = std::string_view(buffer);
                auto first = input.find_first_not_of(whitespace);
                if(first == std::string::npos)
                    continue;

                input.remove_prefix(first);
                auto last = input.find_last_not_of(whitespace);
                if(last != std::string::npos)
                    input.remove_suffix(input.size() - last - 1);

                if(!input.empty() && input[0] == '[' && input.back() == ']') {
                    section = input.substr(1, input.size() - 2);
                    continue;
                }

                if(input.empty() || !isalnum(input[0]))
                    continue;

                auto pos = input.find_first_of('=');
                if(pos < 1 || pos == std::string::npos)
                    continue;

                auto key = input.substr(0, pos);
                auto value = input.substr(++pos);
                last = key.find_last_not_of(whitespace);
                if(last != std::string::npos)
                    key.remove_suffix(key.size() - last - 1);

                pos = value.find_first_not_of(whitespace);
                value.remove_prefix(pos);
                sections[section][std::string(key)] = std::string(value);
            }
            return true;
        }

        auto save(const std::string& path) -> bool {
            std::ofstream out(path, std::ios::binary);
            if(!out.is_open())
                return false;
            for(auto const& [key, value] : sections["_"]) {
                if(!value.empty())
                    out << key << " = " << value << std::endl;
            }
            if(!sections["_"].empty())
                out << std::endl;
            for(auto const& [id, keys] : sections) {
                if(id == "_")
                    continue;
                if(keys.empty())
                    continue;
                out << "[" << id << "]" << std::endl;
                for(auto const& [key, value] : keys) {
                    if(!value.empty())
                        out << key << " = " << value << std::endl;
                }
                out << std::endl;
            }
            return true;
        }

    private:
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> sections;
    };
    std::shared_ptr<data> ptr_;
};

inline auto key_or(const keyfile::keys& keys, const std::string& id, const std::string& or_else = "") {
    try {
        return keys.at(id);
    }
    catch(...) {
        return or_else;
    }
}
} // end namespace
#endif
