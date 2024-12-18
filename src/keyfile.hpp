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

    explicit keyfile(const std::initializer_list<std::string>& paths) noexcept :
    ptr_(std::make_shared<keyfile::data>()) {
        for(const auto& path : paths)
            ptr_->load(path);
    }

    keyfile() noexcept :
    ptr_(std::make_shared<keyfile::data>()) {};

    auto operator[](const std::string& id) -> auto& {
        return ptr_->fetch(id);
    }

    auto operator[](const std::string& id) const {
        return ptr_->fetch(id);
    }

    auto at(const std::string& id = "_") const {
        return ptr_->fetch(id);
    }

    auto keyset(const std::string& id = "_") -> auto& {
        return ptr_->fetch(id);
    }

    auto exists(const std::string& id = "_") const {
        return ptr_ ? ptr_->exists(id) : false;
    }

    auto operator!() const {
        return !ptr_;
    }

    void remove(const std::string& id) {
        if(ptr_) ptr_->remove(id);
    }

    auto load(const std::string& path) {
        return ptr_ ? ptr_->load(path) : false;
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

private:
    class data final {
    public:
        data() = default;
        data(data const&) = delete;
        auto operator=(const data&) -> auto& = delete;

        auto empty() const -> bool {
            return sections.empty();
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
                auto begin = input.find_first_not_of(whitespace);
                if(begin == std::string::npos)
                    continue;

                input.remove_prefix(begin);
                auto end = input.find_last_not_of(whitespace);
                if(end != std::string::npos)
                    input.remove_suffix(input.size() - end - 1);

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
                end = key.find_last_not_of(whitespace);
                if(end != std::string::npos)
                    key.remove_suffix(key.size() - end - 1);

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
} // end namespace
#endif
