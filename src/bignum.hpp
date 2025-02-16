// Copyright (C) 2023 Tycho Softworks.
// This code is licensed under MIT license.

#ifndef TYCHO_BIGNUM_HPP_
#define TYCHO_BIGNUM_HPP_

#include <string>
#include <utility>
#include <memory>
#include <cstdint>
#include <openssl/bn.h>

namespace tycho::crypto {
using key_t = std::pair<const uint8_t *, std::size_t>;

class bignum_t final {
public:
    bignum_t() noexcept :
    ctx_(BN_CTX_new()), num_(BN_new()) {}

    explicit bignum_t(BIGNUM *bn) noexcept :
    ctx_(BN_CTX_new()), num_(bn) {}

    explicit bignum_t(long value) noexcept :
    ctx_(BN_CTX_new()), num_(BN_new()) {
        if(value < 0) {
            BN_set_word(num_, -value);
            BN_set_negative(num_, 1);
        }
        else
            BN_set_word(num_, value);
    }

    bignum_t(bignum_t&& from) noexcept :
    ctx_(from.ctx_), num_(from.num_) {
        from.reset();
    }

    bignum_t(const bignum_t& copy) noexcept :
    ctx_(BN_CTX_new()), num_(BN_dup(copy.num_)) {}

    explicit bignum_t(const key_t& key) noexcept :
    ctx_(BN_CTX_new()), num_(BN_bin2bn(key.first, int(key.second), nullptr)) {}

    explicit bignum_t(const std::string& text) noexcept :
    ctx_(BN_CTX_new()) {
        BN_dec2bn(&num_, text.c_str());
        if(!num_)
            num_ = BN_new();
    }

    ~bignum_t() {
        release();
        if(ctx_)
            BN_CTX_free(ctx_);
    }

    operator BIGNUM *() const noexcept {
        return get();
    }

    operator std::string() const noexcept {
        return to_string();
    }

    operator bool() const noexcept {
        return !BN_is_zero(num_);
    }

    auto operator!() const noexcept {
        return BN_is_zero(num_);
    }

    auto operator*() const noexcept {
        return to_string();
    }

    auto operator=(bignum_t&& from) noexcept -> auto& {
        release();
        if(ctx_)
            BN_CTX_free(ctx_);
        ctx_ = from.ctx_;
        num_ = from.num_;
        from.reset();
        return *this;
    }

    auto operator=(const bignum_t& copy) noexcept -> auto& {
        if(&copy == this)
            return *this;

        release();
        if(ctx_)
            BN_CTX_free(ctx_);
        num_ = BN_dup(copy.num_);
        ctx_ = BN_CTX_new();
        return *this;
    }

    auto operator=(long value) noexcept -> auto& {
        release();
        if(value < 0) {
            BN_set_word(num_, -value);
            BN_set_negative(num_, 1);
        } else
            BN_set_word(num_, value);
        return *this;
    }

    auto operator=(const std::string& text) noexcept -> auto& {
        release();
        BN_dec2bn(&num_, text.c_str());
        if(!num_)
            num_ = BN_new();
        return *this;
    }

    auto operator=(const key_t& key) noexcept -> auto& {
        release();
        num_ = BN_bin2bn(key.first, int(key.second), nullptr);
        return *this;
    }

    auto operator++() noexcept -> auto& {
        BN_add_word(num_, 1);
        return *this;
    }

    auto operator--() noexcept -> auto& {
        BN_sub_word(num_, 1);
        return *this;
    }

    auto operator++(int) noexcept { // NOLINT
        const bignum_t current(*this);
        BN_add_word(num_, 1);
        return current;
    }

    auto operator--(int) noexcept { // NOLINT
        const bignum_t current(*this);
        BN_sub_word(num_, 1);
        return current;
    }

    auto operator-() const noexcept {
        bignum_t changed(*this);
        changed.negative(!is_negative());
        return changed;
    }

    auto operator+(const bignum_t& add) const noexcept {
        bignum_t result;
        BN_add(result.num_, num_, add.num_);
        return result;
    }

    auto operator+(std::size_t add) const noexcept {
        bignum_t result(*this);
        BN_add_word(result.num_, add);
        return result;
    }

    auto operator+=(const bignum_t& add) const noexcept -> auto& {
        const bignum_t tmp(*this);
        BN_add(num_, tmp.num_, add.num_);
        return *this;
    }

    auto operator+=(std::size_t add) const noexcept -> auto& {
        BN_add_word(num_, add);
        return *this;
    }

    auto operator-(const bignum_t& sub) const noexcept {
        bignum_t result;
        BN_sub(result.num_, num_, sub.num_);
        return result;
    }

    auto operator-(std::size_t sub) const noexcept {
        bignum_t result(*this);
        BN_sub_word(result.num_, sub);
        return result;
    }

    auto operator-=(const bignum_t& sub) const noexcept -> auto& {
        const bignum_t tmp(*this);
        BN_sub(num_, tmp.num_, sub.num_);
        return *this;
    }

    auto operator-=(std::size_t sub) const noexcept -> auto& {
        BN_sub_word(num_, sub);
        return *this;
    }

    auto operator*(const bignum_t& mul) const noexcept {
        bignum_t result;
        BN_mul(result.num_, num_, mul.num_, mul.ctx_);
        return result;
    }

    auto operator*(std::size_t mul) const noexcept {
        bignum_t result(*this);
        BN_mul_word(result.num_, mul);
        return result;
    }

    auto operator*=(const bignum_t& mul) const noexcept -> auto& {
        const bignum_t tmp(*this);
        BN_mul(num_, tmp.num_, mul.num_, mul.ctx_);
        return *this;
    }

    auto operator*=(std::size_t mul) const noexcept -> auto& {
        BN_mul_word(num_, mul);
        return *this;
    }

    auto operator/(const bignum_t& div) const noexcept {
        bignum_t result;
        BN_div(result.num_, nullptr, num_, div.num_, div.ctx_);
        return result;
    }

    auto operator/(std::size_t div) const noexcept {
        bignum_t result(*this);
        BN_div_word(result.num_, div);
        return result;
    }

    auto operator/=(const bignum_t& div) const noexcept -> auto& {
        const bignum_t tmp(*this);
        BN_div(num_, nullptr, tmp.num_, div.num_, div.ctx_);
        return *this;
    }

    auto operator/=(std::size_t div) const noexcept -> auto& {
        BN_div_word(num_, div);
        return *this;
    }

    auto operator%(const bignum_t& mod) const noexcept {
        bignum_t result;
        BN_div(nullptr, result.num_, num_, mod.num_, mod.ctx_);
        return result;
    }

    auto operator%(std::size_t mod) const noexcept {
        bignum_t result(*this);
        BN_mod_word(result.num_, mod);
        return result;
    }

    auto operator%=(const bignum_t& mod) const noexcept -> auto& {
        const bignum_t tmp(*this);
        BN_div(nullptr, num_, tmp.num_, mod.num_, mod.ctx_);
        return *this;
    }

    auto operator%=(std::size_t mod) const noexcept -> auto& {
        BN_mod_word(num_, mod);
        return *this;
    }

    auto operator==(const bignum_t& r) const noexcept -> auto {
        return BN_cmp(num_, r.num_) == 0;
    }

    auto operator!=(const bignum_t& r) const noexcept -> auto {
        return BN_cmp(num_, r.num_) != 0;
    }

    auto operator<(const bignum_t& r) const noexcept -> auto {
        return BN_cmp(num_, r.num_) < 0;
    }

    auto operator>(const bignum_t& r) const noexcept -> auto {
        return BN_cmp(num_, r.num_) > 0;
    }

    auto operator<=(const bignum_t& r) const noexcept -> auto {
        return BN_cmp(num_, r.num_) <= 0;
    }

    auto operator>=(const bignum_t& r) const noexcept -> auto {
        return BN_cmp(num_, r.num_) >= 0;
    }

    auto operator>>(int bits) const noexcept {
        bignum_t result(*this);
        BN_rshift(result.num_, result.num_, bits);
        return result;
    }

    auto operator>>(const bignum_t& bits) const noexcept {
        bignum_t result(*this);
        BN_rshift(result.num_, result.num_, btoi(bits));
        return result;
    }

    auto operator<<(int bits) const noexcept {
        bignum_t result(*this);
        BN_lshift(result.num_, result.num_, bits);
        return result;
    }

    auto operator<<(const bignum_t& bits) const noexcept {
        bignum_t result(*this);
        BN_lshift(result.num_, result.num_, btoi(bits));
        return result;
    }

    auto operator^(long exp) const noexcept {
        bignum_t result(*this);
        const bignum_t exponent(exp);
        BN_exp(result.num_, num_, exponent.num_, exponent.ctx_);
        return result;
    }

    auto operator^(const bignum_t& exp) const noexcept {
        bignum_t result(*this);
        BN_exp(result.num_, num_, exp.num_, exp.ctx_);
        return result;
    }

    auto operator^=(const bignum_t& exp) const noexcept -> auto& {
        BN_exp(num_, num_, exp.num_, exp.ctx_);
        return *this;
    }

    auto operator^=(long exp) const noexcept -> auto& {
        const bignum_t exponent(exp);
        BN_exp(num_, num_, exponent.num_, exponent.ctx_);
        return *this;
    }

    auto get() const noexcept -> BIGNUM * {
        return num_;
    }

    auto size() const noexcept {
        return std::size_t(BN_num_bytes(num_));
    }

    auto bits() const noexcept {
        return std::size_t(BN_num_bits(num_));
    }

    auto to_string() const noexcept -> std::string {
        char *tmp = BN_bn2dec(num_);
        if(!tmp) return {};
        std::string out = tmp;
        OPENSSL_free(tmp);
        return out;
    }

    auto put(uint8_t *out, std::size_t max) const noexcept {
        auto used = size();
        if(max < used) return std::size_t(0);
        BN_bn2bin(num_, out);
        std::size_t pos = used;
        while(pos < max)
            out[pos++] = 0;
        return used;
    }

    void clear() noexcept {
        BN_clear(num_);
    }

    static auto make_rand(int bits, int top = BN_RAND_TOP_ANY, int bottom = BN_RAND_BOTTOM_ANY) noexcept {
        bignum_t result;
        BN_rand(result.num_, bits, top, bottom);
        return result;
    }

    static auto make_priv(int bits, int strength, int top = BN_RAND_TOP_ANY, int bottom = BN_RAND_BOTTOM_ANY) noexcept {
        bignum_t result;
        BN_priv_rand_ex(result.num_, bits, top, bottom, strength, result.ctx_);
        return result;
    }

private:
    friend auto btoi(const bignum_t& bn) noexcept -> int;
    friend auto btol(const bignum_t& bn) noexcept -> long;
    friend auto abs(const bignum_t& bn) noexcept -> bignum_t;
    friend auto pow(const bignum_t& base, const bignum_t& exp) noexcept -> bignum_t;
    friend auto sqr(const bignum_t& bn) noexcept -> bignum_t;
    friend auto gcd(const bignum_t& a, const bignum_t& b) noexcept -> bignum_t;

    void reset() {
        ctx_ = BN_CTX_new();
        num_ = BN_new();
    }

    void negative(bool flag) noexcept {
        BN_set_negative(num_, flag ? 1 : 0);
    }

    auto is_negative() const noexcept -> bool {
        return BN_is_negative(num_);
    }

    void release() noexcept {
        if(num_) {
            BN_clear_free(num_);
            num_ = nullptr;
        }
    }

    BN_CTX *ctx_{nullptr};
    BIGNUM *num_{nullptr};
};

using bitnum = bignum_t;

inline auto btoi(const bignum_t& bn) noexcept -> int {
    if(bn.is_negative()) return static_cast<int>(-BN_get_word(bn.num_));
    return static_cast<int>(BN_get_word(bn.num_));
}

inline auto btol(const bignum_t& bn) noexcept -> long {
    if(bn.is_negative()) return static_cast<long>(-BN_get_word(bn.num_));
    return static_cast<long>(BN_get_word(bn.num_));
}

inline auto abs(const bignum_t& bn) noexcept -> bignum_t {
    auto result(bn);
    if(result.is_negative())
        result.negative(false);
    return result;
}

inline auto pow(const bignum_t& base, const bignum_t& exp) noexcept -> bignum_t {
    bignum_t result;
    BN_exp(result.num_, base.num_, exp.num_, base.ctx_);
    return result;
}

inline auto sqr(const bignum_t& bn) noexcept -> bignum_t {
    bignum_t result;
    BN_sqr(result.num_, bn.num_, bn.ctx_);
    return result;
}

inline auto gcd(const bignum_t& a, const bignum_t& b) noexcept -> bignum_t {
    bignum_t result;
    BN_gcd(result.num_, a.num_, b.num_, a.ctx_);
    return result;
}
} // end namespace

namespace std {
template<>
struct hash<tycho::crypto::bignum_t> {
    auto operator()(const tycho::crypto::bignum_t& bn) const {
        auto size = bn.size();
        auto data = std::make_unique<uint8_t[]>(size);
        bn.put(data.get(), size);
        std::size_t result{0U};
        for(std::size_t pos = 0; pos < size; ++pos) {
            result = (result * 131) + data[pos];
        }
        return result;
    }
};
} // end namespace


inline auto operator<<(std::ostream& out, const tycho::crypto::bignum_t& bn) -> std::ostream& {
    out << bn.to_string();
    return out;
}

#endif
