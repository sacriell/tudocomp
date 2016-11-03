#ifndef TUDOCOMP_GENERIC_INT_VECTOR_H
#define TUDOCOMP_GENERIC_INT_VECTOR_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <climits>

#include <tudocomp/ds/uint_t.hpp>
#include <tudocomp/util/IntegerBase.hpp>

#include <sdsl/bits.hpp>
#include <glog/logging.h>

namespace tdc {
    namespace int_vector {
        class IntRef;
        class ConstIntRef;
        class IntPtr;
        class ConstIntPtr;
    }
}

namespace std {
    template<>
    struct iterator_traits<tdc::int_vector::IntPtr> {
        typedef ptrdiff_t                       difference_type;
        // TODO: Fix if pointers become generic
        typedef uint64_t                        value_type;
        typedef tdc::int_vector::IntPtr         pointer;
        typedef tdc::int_vector::IntRef         reference;
        typedef std::random_access_iterator_tag iterator_category;
    };
    template<>
    struct iterator_traits<tdc::int_vector::ConstIntPtr> {
        typedef ptrdiff_t                       difference_type;
        // TODO: Fix if pointers become generic
        typedef uint64_t                        value_type;
        typedef tdc::int_vector::ConstIntPtr    pointer;
        typedef tdc::int_vector::ConstIntRef    reference;
        typedef std::random_access_iterator_tag iterator_category;
    };
}

namespace tdc {

template<>
struct IntegerBaseTraitConst<int_vector::IntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::IntRef T;

    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self);
    inline static SelfMaxBit cast_for_32_op(const T& self);
    inline static uint64_t cast_for_64_op(const T& self);
};

template<>
struct IntegerBaseTrait<int_vector::IntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::IntRef T;

    inline static void assign(T& self, uint32_t v);
    inline static void assign(T& self, uint64_t v);
};

template<>
struct IntegerBaseTraitConst<int_vector::ConstIntRef> {
    typedef sdsl::bits bits;
    typedef int_vector::ConstIntRef T;

    typedef uint64_t SelfMaxBit;

    inline static SelfMaxBit cast_for_self_op(const T& self);
    inline static SelfMaxBit cast_for_32_op(const T& self);
    inline static uint64_t cast_for_64_op(const T& self);
};

namespace int_vector {
    using sdsl::bits;

    typedef uint64_t DynamicIntValueType;

    // TODO: On rrferences, provide 32 or 64 bit ops depending on bits
    // of uint_t type

    struct Const {
        typedef const DynamicIntValueType* ptr;
        typedef uint8_t                    off;
        typedef const uint8_t              len;
        typedef DynamicIntValueType        val;
    };

    struct Mut {
        typedef DynamicIntValueType* ptr;
        typedef uint8_t              off;
        typedef const uint8_t        len;
        typedef DynamicIntValueType  val;
    };

    template<class Self, class Ptr, class Layout>
    class GenericIntRef;
    class IntRef;
    class ConstIntRef;
    class IntPtr;
    class ConstIntPtr;

    template<class Self, class Layout>
    class GenericIntPtr {
    protected:
        friend class GenericIntRef<IntRef, IntPtr, Mut>;
        friend class GenericIntRef<ConstIntRef, ConstIntPtr, Const>;
        friend class IntRef;
        friend class ConstIntRef;
        friend class IntegerBaseTrait<IntRef>;
        friend class IntegerBaseTraitConst<IntRef>;
        friend class IntegerBaseTraitConst<ConstIntRef>;

        typename Layout::ptr m_ptr;
        typename Layout::off m_bit_offset;
        typename Layout::len m_bit_size;
    public:
        GenericIntPtr(typename Layout::ptr ptr,
                      typename Layout::off bit_offset,
                      typename Layout::len bit_size):
            m_ptr(ptr),
            m_bit_offset(bit_offset),
            m_bit_size(bit_size) {}
        GenericIntPtr(): GenericIntPtr("\0\0\0\0", 0, 0) {}
        GenericIntPtr(const GenericIntPtr& other):
            GenericIntPtr(other.m_ptr, other.m_bit_offset, other.m_bit_size) {}

        Self& operator=(const Self& other) {
            DCHECK(m_bit_size == other.m_bit_size);
            m_ptr = other.m_ptr;
            m_bit_offset = other.m_bit_offset;
            return static_cast<Self&>(*this);
        }

        Self& operator++() {
            const DynamicIntValueType* tmp = m_ptr;
            bits::move_right(tmp, m_bit_offset, m_bit_size);
            m_ptr = (DynamicIntValueType*) tmp;
            return static_cast<Self&>(*this);
        }

        Self& operator--() {
            const DynamicIntValueType* tmp = m_ptr;
            bits::move_left(tmp, m_bit_offset, m_bit_size);
            m_ptr = (DynamicIntValueType*) tmp;
            return static_cast<Self&>(*this);
        }

        friend Self operator+(const Self& lhs, const ptrdiff_t& rhs) {
            auto tmp = lhs;
            if (rhs >= 0) {
                for(size_t i = 0; i < size_t(rhs); i++) {
                    ++tmp;
                }
            } else {
                for(size_t i = 0; i < size_t(-rhs); i++) {
                    --tmp;
                }
            }
            return tmp;
        }

        friend Self operator+(const ptrdiff_t& lhs, const Self& rhs) { return rhs + lhs;      }
        friend Self operator-(const Self& lhs, const ptrdiff_t& rhs) { return lhs + (-rhs);   }

        friend ptrdiff_t operator-(const Self& lhs, const Self& rhs) {
            // TODO: test
            DCHECK(lhs.m_bit_size == rhs.m_bit_size);
            auto ptr_diff = lhs.m_ptr - rhs.m_ptr;
            auto bit_count = ptr_diff * sizeof(typename Layout::val) * CHAR_BIT;
            bit_count += lhs.m_bit_offset;
            bit_count -= rhs.m_bit_offset;
            bit_count /= lhs.m_bit_size;
            return bit_count;
        }

        friend bool operator==(const Self& lhs, const Self& rhs) {
            DCHECK(lhs.m_bit_size == rhs.m_bit_size);
            return (lhs.m_ptr == rhs.m_ptr)
                && (lhs.m_bit_offset == rhs.m_bit_offset);
        }
        friend bool operator<(const Self& lhs, const Self& rhs)  {
            // TODO: test
            DCHECK(lhs.m_bit_size == rhs.m_bit_size);
            if (lhs.m_ptr < rhs.m_ptr) return true;
            if ((lhs.m_ptr == rhs.m_ptr) && (lhs.m_bit_offset < rhs.m_bit_offset)) return true;
            return false;
        }
        friend bool operator!=(const Self& lhs, const Self& rhs) { return !(lhs == rhs);               }
        friend bool operator>(const Self& lhs, const Self& rhs)  { return !(lhs < rhs);                }
        friend bool operator>=(const Self& lhs, const Self& rhs) { return (lhs > rhs) || (lhs == rhs); }
        friend bool operator<=(const Self& lhs, const Self& rhs) { return (lhs < rhs) || (lhs == rhs); }

        Self& operator+=(const ptrdiff_t& v) { auto& self = static_cast<Self&>(*this); self = (self + v); return self; }
        Self& operator-=(const ptrdiff_t& v) { auto& self = static_cast<Self&>(*this); self = (self - v); return self; }

        Self operator++(int) { auto self = static_cast<Self&>(*this); ++(*this); return self; }
        Self operator--(int) { auto self = static_cast<Self&>(*this); --(*this); return self; }

        ConstIntRef operator[](size_t i) const;
        ConstIntRef operator*() const;
    };

    class ConstIntPtr: public GenericIntPtr<ConstIntPtr, Const> {
    public:
        using GenericIntPtr<ConstIntPtr, Const>::GenericIntPtr;
        inline ConstIntPtr(const ConstIntPtr& other): GenericIntPtr(other) {}
        ConstIntPtr& operator=(const ConstIntPtr& other) {
            return ((GenericIntPtr<ConstIntPtr, Const>&) *this) = other;
        }
    };

    class IntPtr: public GenericIntPtr<IntPtr, Mut> {
    public:
        using GenericIntPtr<IntPtr, Mut>::GenericIntPtr;
        inline IntPtr(const IntPtr& other): GenericIntPtr(other) {}
        IntPtr& operator=(const IntPtr& other) {
            return ((GenericIntPtr<IntPtr, Mut>&) *this) = other;
        }

        inline IntRef operator*();
        inline IntRef operator[](size_t i);
    };

    template<class Self, class Ptr, class Layout>
    class GenericIntRef {
    protected:
        friend class IntPtr;
        friend class ConstIntPtr;
        friend class IntegerBaseTrait<IntRef>;
        friend class IntegerBaseTraitConst<IntRef>;
        friend class IntegerBaseTraitConst<ConstIntRef>;

        Ptr m_ptr;
    public:
        typedef typename Layout::val value_type;

        explicit GenericIntRef(const Ptr& ptr): m_ptr(ptr) {}

        operator value_type() const {
            return bits::read_int(m_ptr.m_ptr, m_ptr.m_bit_offset, m_ptr.m_bit_size);
        }

    };

    class IntRef: public GenericIntRef<IntRef, IntPtr, Mut>, public IntegerBase<IntRef> {
    public:
        explicit IntRef(const IntPtr& ptr): GenericIntRef(ptr) {}

        inline IntRef& operator=(value_type other) {
            bits::write_int(m_ptr.m_ptr, other, m_ptr.m_bit_offset, m_ptr.m_bit_size);
            return *this;
        };

        inline IntRef& operator=(const IntRef& other) {
            return operator=(value_type(other));
        };

        inline IntRef& operator=(const ConstIntRef& other);
    };

    class ConstIntRef: public GenericIntRef<ConstIntRef, ConstIntPtr, Const>, public ConstIntegerBase<ConstIntRef> {
    public:
        explicit ConstIntRef(const ConstIntPtr& ptr): GenericIntRef(ptr) {}
    };

    inline IntRef& IntRef::operator=(const ConstIntRef& other) {
        return operator=(value_type(other));
    };

    inline IntRef IntPtr::operator*() {
        return IntRef(*this);
    }

    template<class Self, class U>
    ConstIntRef GenericIntPtr<Self, U>::operator*() const {
        return ConstIntRef(static_cast<const Self&>(*this));
    }

    template<class Self, class U>
    ConstIntRef GenericIntPtr<Self, U>::operator[](size_t i) const {
        auto x = *this;
        x += i;
        return ConstIntRef(static_cast<const Self&>(x));
    }

    inline IntRef IntPtr::operator[](size_t i) {
        auto x = *this;
        x += i;
        return IntRef(x);
    }

    // TODO: uint64_t should be a type corresponding to the element type,
    // eg uint_t<5>
    inline void swap(IntRef& lhs, IntRef& rhs) noexcept {
        uint64_t tmp = lhs;
        lhs = uint64_t(rhs);
        rhs = tmp;
    }

    // TODO: uint64_t should be a type corresponding to the element type,
    // eg uint_t<5>
    inline void swap(IntRef& lhs, uint64_t& rhs) noexcept {
        uint64_t tmp = lhs;
        lhs = uint64_t(rhs);
        rhs = tmp;
    }

    // TODO: uint64_t should be a type corresponding to the element type,
    // eg uint_t<5>
    inline void swap(uint64_t& lhs, IntRef& rhs) noexcept {
        uint64_t tmp = lhs;
        lhs = uint64_t(rhs);
        rhs = tmp;
    }

    static_assert(sizeof(GenericIntPtr<ConstIntPtr, Const>) <= (sizeof(void*) * 2), "make sure this is reasonably small");

    template<class T>
    struct even_bit_backing_data {
        typedef typename std::vector<T>::value_type             value_type;

        typedef typename std::vector<T>::reference              reference;
        typedef typename std::vector<T>::const_reference        const_reference;

        typedef typename std::vector<T>::pointer                pointer;
        typedef typename std::vector<T>::const_pointer          const_pointer;

        typedef typename std::vector<T>::iterator               iterator;
        typedef typename std::vector<T>::const_iterator         const_iterator;

        typedef typename std::vector<T>::reverse_iterator       reverse_iterator;
        typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;

        typedef typename std::vector<T>::difference_type        difference_type;
        typedef typename std::vector<T>::size_type              size_type;

        typedef T                                               internal_data_type;

        std::vector<T> m_vec;

        inline explicit even_bit_backing_data() {}
        inline explicit even_bit_backing_data(size_type n): m_vec(n) {}
        inline even_bit_backing_data(size_type n, const value_type& val): m_vec(n, val) {}
        template <class InputIterator>
        inline even_bit_backing_data (InputIterator first, InputIterator last): m_vec(first, last) {}
        inline even_bit_backing_data (const even_bit_backing_data& other): m_vec(other.m_vec) {}
        inline even_bit_backing_data (even_bit_backing_data&& other): m_vec(std::move(other.m_vec)) {}
        inline even_bit_backing_data (std::initializer_list<value_type> il): m_vec(il) {}

        inline even_bit_backing_data& operator=(const even_bit_backing_data& other) {
            m_vec = other.m_vec;
            return *this;
        }

        inline even_bit_backing_data& operator=(even_bit_backing_data&& other) {
            m_vec = std::move(other.m_vec);
            return *this;
        }

        inline even_bit_backing_data& operator=(std::initializer_list<value_type> il) {
            m_vec = il;
            return *this;
        }

        inline iterator begin() {
            return m_vec.begin();
        }

        inline iterator end() {
            return m_vec.end();
        }

        inline reverse_iterator rbegin() {
            return m_vec.rbegin();
        }

        inline reverse_iterator rend() {
            return m_vec.rend();
        }

        inline const_iterator begin() const {
            return m_vec.begin();
        }

        inline const_iterator end() const {
            return m_vec.end();
        }

        inline const_reverse_iterator rbegin() const {
            return m_vec.rbegin();
        }

        inline const_reverse_iterator rend() const {
            return m_vec.rend();
        }

        inline const_iterator cbegin() const {
            return m_vec.cbegin();
        }

        inline const_iterator cend() const {
            return m_vec.cend();
        }

        inline const_reverse_iterator crbegin() const {
            return m_vec.crbegin();
        }

        inline const_reverse_iterator crend() const {
            return m_vec.crend();
        }

        inline size_type size() const {
            return m_vec.size();
        }

        inline uint64_t bit_size() const {
            return size() * sizeof(T) * CHAR_BIT;
        }

        inline size_type max_size() const {
            return m_vec.max_size();
        }

        inline void resize(size_type n) {
            m_vec.resize(n);
        }

        inline void resize(size_type n, const value_type& val) {
            m_vec.resize(n, val);
        }

        inline size_type capacity() const {
            return m_vec.capacity();
        }

        inline bool empty() const {
            return m_vec.empty();
        }

        inline void reserve(size_type n) {
            m_vec.reserve(n);
        }

        inline reference operator[](size_type n) {
            return m_vec[n];
        }

        inline const_reference operator[](size_type n) const {
            return m_vec[n];
        }

        inline reference at(size_type n) {
            return m_vec.at(n);
        }

        inline const_reference at(size_type n) const {
            return m_vec.at(n);
        }

        inline reference front() {
            return m_vec.front();
        }

        inline const_reference front() const {
            return m_vec.front();
        }

        inline reference back() {
            return m_vec.back();
        }

        inline const_reference back() const {
            return m_vec.back();
        }

        template <class InputIterator>
        inline void assign(InputIterator first, InputIterator last) {
            m_vec.assign(first, last);
        }

        inline void assign(size_type n, const value_type& val) {
            m_vec.assign(n, val);
        }

        inline void assign(std::initializer_list<value_type> il) {
            m_vec.assign(il);
        }

        inline void push_back(const value_type& val) {
            m_vec.push_back(val);
        }

        inline void push_back(value_type&& val) {
            m_vec.push_back(std::move(val));
        }

        inline void pop_back() {
            m_vec.pop_back();
        }

        inline iterator insert(const_iterator position, const value_type& val) {
            return m_vec.insert(position, val);
        }

        inline iterator insert(const_iterator position, size_type n, const value_type& val) {
            return m_vec.insert(position, n, val);
        }

        template <class InputIterator>
        inline iterator insert(const_iterator position, InputIterator first, InputIterator last) {
            return m_vec.insert(position, first, last);
        }

        inline iterator insert(const_iterator position, value_type&& val) {
            return m_vec.insert(position, std::move(val));
        }

        inline iterator insert(const_iterator position, std::initializer_list<value_type> il) {
            return m_vec.insert(position, il);
        }

        inline iterator erase(const_iterator position) {
            return m_vec.erase(position);
        }

        inline iterator erase(const_iterator first, const_iterator last) {
            return m_vec.erase(first, last);
        }

        inline void swap(even_bit_backing_data& other) {
            m_vec.swap(other.m_vec);
        }

        inline void clear() {
            m_vec.clear();
        }

        template <class... Args>
        inline iterator emplace(const_iterator position, Args&&... args) {
            return m_vec.emplace(position, std::forward<Args...>(args)...);
        }

        template <class... Args>
        inline void emplace_back(Args&&... args) {
            m_vec.emplace_back(std::forward<Args...>(args)...);
        }
    };

    template<class T>
    bool operator==(const even_bit_backing_data<T>& lhs, const even_bit_backing_data<T>& rhs) {
        return lhs.m_vec == rhs.m_vec;
    }

    template<class T>
    bool operator!=(const even_bit_backing_data<T>& lhs, const even_bit_backing_data<T>& rhs) {
        return lhs.m_vec != rhs.m_vec;
    }

    template<class T>
    bool operator<(const even_bit_backing_data<T>& lhs, const even_bit_backing_data<T>& rhs) {
        return lhs.m_vec < rhs.m_vec;
    }

    template<class T>
    bool operator<=(const even_bit_backing_data<T>& lhs, const even_bit_backing_data<T>& rhs) {
        return lhs.m_vec <= rhs.m_vec;
    }

    template<class T>
    bool operator>(const even_bit_backing_data<T>& lhs, const even_bit_backing_data<T>& rhs) {
        return lhs.m_vec > rhs.m_vec;
    }

    template<class T>
    bool operator>=(const even_bit_backing_data<T>& lhs, const even_bit_backing_data<T>& rhs) {
        return lhs.m_vec >= rhs.m_vec;
    }

    template<class T>
    void swap(even_bit_backing_data<T>& lhs, even_bit_backing_data<T>& rhs) {
        using std::swap;
        swap(lhs.m_vec, rhs.m_vec);
    }

    template<size_t N>
    struct odd_bit_backing_data {
        typedef uint_t<N>                                            value_type;

        typedef IntRef                                               reference;
        typedef ConstIntRef                                          const_reference;

        typedef IntPtr                                               pointer;
        typedef ConstIntPtr                                          const_pointer;

        typedef pointer                                              iterator;
        typedef const_pointer                                        const_iterator;

        typedef typename std::reverse_iterator<iterator>             reverse_iterator;
        typedef typename std::reverse_iterator<const_iterator>       const_reverse_iterator;

        typedef ptrdiff_t                                            difference_type;
        typedef size_t                                               size_type;

        typedef DynamicIntValueType                                  internal_data_type;

        std::vector<internal_data_type> m_vec;
        uint64_t m_real_size = 0;

        template<size_t M>
        friend bool operator==(const odd_bit_backing_data<M>& lhs, const odd_bit_backing_data<M>& rhs);

        inline static uint64_t backing2bits(size_t n) {
            return uint64_t(sizeof(internal_data_type) * CHAR_BIT) * uint64_t(n);
        }

        inline static uint64_t elem2bits(size_t n) {
            return uint64_t(N) * uint64_t(n);
        }

        inline static uint64_t bits2backing(uint64_t bits) {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / backing2bits(1)) + 1;
        }

        inline static uint64_t bits2elem(uint64_t bits) {
            if (bits == 0) {
                return 0;
            }
            return ((bits - 1) / elem2bits(1)) + 1;
        }

        struct PosAndOffset { size_t pos; uint8_t offset; };
        inline static PosAndOffset bitpos2backingpos(uint64_t bits) {
            return PosAndOffset {
                bits / backing2bits(1),
                uint8_t(bits % backing2bits(1))
            };
        }

        inline explicit odd_bit_backing_data() {}
        inline explicit odd_bit_backing_data(size_type n) {
            m_real_size = n;
            size_t converted_size = bits2backing(elem2bits(m_real_size));
            m_vec = std::vector<internal_data_type>(converted_size);
        }
        inline odd_bit_backing_data(size_type n, const value_type& val): odd_bit_backing_data(n) {
            auto ptr = m_vec.data();
            uint8_t offset = 0;

            for (size_t i = 0; i < n; i++) {
                bits::write_int_and_move(ptr, val, offset, N);
            }
        }

        template <class InputIterator>
        inline odd_bit_backing_data(InputIterator first, InputIterator last) {
            // TODO: specialize for random acces iterator
            for(; first != last; first++) {
                push_back(*first);
            }
        }
        inline odd_bit_backing_data (const odd_bit_backing_data& other):
            m_vec(other.m_vec), m_real_size(other.m_real_size) {}
        inline odd_bit_backing_data (odd_bit_backing_data&& other):
            m_vec(std::move(other.m_vec)), m_real_size(other.m_real_size) {}
        inline odd_bit_backing_data(std::initializer_list<value_type> il):
            odd_bit_backing_data(il.begin(), il.end()) {}

        inline odd_bit_backing_data& operator=(const odd_bit_backing_data& other) {
            m_vec = other.m_vec;
            m_real_size = other.m_real_size;
            return *this;
        }

        inline odd_bit_backing_data& operator=(odd_bit_backing_data&& other) {
            m_vec = std::move(other.m_vec);
            m_real_size = other.m_real_size;
            return *this;
        }

        inline odd_bit_backing_data& operator=(std::initializer_list<value_type> il) {
            *this = odd_bit_backing_data(il);
            return *this;
        }

        inline iterator begin() {
            auto x = bitpos2backingpos(0);
            return pointer(&m_vec[x.pos], x.offset, N);
        }

        inline iterator end() {
            auto x = bitpos2backingpos(elem2bits(m_real_size));
            return pointer(&m_vec[x.pos], x.offset, N);
        }

        inline reverse_iterator rbegin() {
            return std::reverse_iterator<iterator>(end());
        }

        inline reverse_iterator rend() {
            return std::reverse_iterator<iterator>(begin());
        }

        inline const_iterator begin() const {
            auto x = bitpos2backingpos(0);
            return const_pointer(&m_vec[x.pos], x.offset, N);
        }

        inline const_iterator end() const {
            auto x = bitpos2backingpos(elem2bits(m_real_size));
            return const_pointer(&m_vec[x.pos], x.offset, N);
        }

        inline const_reverse_iterator rbegin() const {
            return std::reverse_iterator<const_iterator>(end());
        }

        inline const_reverse_iterator rend() const {
            return std::reverse_iterator<const_iterator>(begin());
        }

        inline const_iterator cbegin() const {
            return begin();
        }

        inline const_iterator cend() const {
            return end();
        }

        inline const_reverse_iterator crbegin() const {
            return rbegin();
        }

        inline const_reverse_iterator crend() const {
            return rend();
        }

        inline size_type size() const {
            return m_real_size;
        }

        inline uint64_t bit_size() const {
            return elem2bits(m_real_size);
        }

        inline size_type max_size() const {
            // Empty vector does not allocate, so this is fine
            return std::vector<uint_t<N>>().max_size();
        }

        inline void resize(size_type n) {
            m_real_size = n;
            m_vec.resize(bits2backing(elem2bits(n)), 0);
        }

        inline void resize(size_type n, const value_type& val) {
            auto old_size = size();
            resize(n);
            if (old_size < n) {
                for (auto a = begin() + old_size, b = end(); a != b; ++a) {
                    *a = val;
                }
            }
        }

        inline size_type capacity() const {
            return bits2elem(backing2bits(m_vec.capacity()));
        }

        inline bool empty() const {
            return size() == 0;
        }

        inline void reserve(size_type n) {
            m_vec.reserve(bits2backing(elem2bits(n)));
        }

        inline reference operator[](size_type n) {
            DCHECK(n < size());
            auto x = bitpos2backingpos(elem2bits(n));
            return IntRef(IntPtr(m_vec.data() + x.pos, x.offset, N));
        }

        inline const_reference operator[](size_type n) const {
            DCHECK(n < size());
            auto x = bitpos2backingpos(elem2bits(n));
            return ConstIntRef(ConstIntPtr(m_vec.data() + x.pos, x.offset, N));
        }

        inline void range_check(size_type n) const {
            if (n >= size()) {
                std::stringstream ss;
                ss << "Out-of-range access of GenericIntVector: index is ";
                ss << n;
                ss << ", size() is ";
                ss << size();
                throw std::out_of_range(ss.str());
            }
        }

        inline reference at(size_type n) {
            range_check(n);
            return operator[](n);
        }

        inline const_reference at(size_type n) const {
            range_check(n);
            return operator[](n);
        }

        inline reference front() {
            return operator[](0);
        }

        inline const_reference front() const {
            return operator[](0);
        }

        inline reference back() {
            return operator[](size() - 1);
        }

        inline const_reference back() const {
            return operator[](size() - 1);
        }

        template <class InputIterator>
        inline void assign(InputIterator first, InputIterator last) {
            *this = odd_bit_backing_data(first, last);
        }

        inline void assign(size_type n, const value_type& val) {
            *this = odd_bit_backing_data(n, val);
        }

        inline void assign(std::initializer_list<value_type> il) {
            *this = odd_bit_backing_data(il);
        }

        inline void push_back(const value_type& val) {
            m_real_size += 1;

            while (elem2bits(m_real_size) > backing2bits(m_vec.size())) {
                m_vec.push_back(0);
            }

            back() = val;
        }

        inline void push_back(value_type&& val) {
            const auto& r = val;
            push_back(r);
        }

        inline void pop_back() {
            DCHECK(!empty());
            m_real_size -= 1;
            while (bits2backing(elem2bits(m_real_size)) < m_vec.size()) {
                m_vec.pop_back();
            }
        }

        inline iterator insert(const_iterator position, const value_type& val) {
            return insert(position, size_type(1), val);
        }

        inline iterator insert(const_iterator position, size_type n, const value_type& val) {
            // Remember integer offset before iterator invalidation
            auto p = (position - cbegin());

            // Step 1: Grow backing vector by needed amount
            {
                auto new_bits_needed = elem2bits(n);
                auto existing_extra_bits = backing2bits(m_vec.size()) - elem2bits(m_real_size);

                if (new_bits_needed > existing_extra_bits) {
                    new_bits_needed -= existing_extra_bits;
                } else {
                    new_bits_needed = 0;
                }

                auto new_backing_needed = bits2backing(new_bits_needed);
                m_vec.insert(m_vec.end(), new_backing_needed, 0);
            }

            // Step 2: move elements to back, leaving a gap
            {
                auto start = std::reverse_iterator<iterator>(begin() + p);
                auto old_end = rbegin();
                m_real_size += n;
                auto new_end = rbegin();
                // TODO: std::copy_backwards
                std::copy(old_end, start, new_end);
            }

            // Step 3: insert new values at gap
            {
                for(auto a = begin() + p, b = end(); a != b; ++a) {
                    *a = val;
                }

                return begin() + p;
            }
        }

        template <class InputIterator>
        inline iterator insert(const_iterator position, InputIterator first, InputIterator last) {
            // TODO: Optimize for random access iterators (multiple insertions at once)

            // Remember integer offset before iterator invalidation
            const auto p = (position - cbegin());
            auto pi = p;

            for(; first != last; ++first) {
                insert(cbegin() + pi, value_type(*first));
                pi++;
            }

            return begin() + p;
        }

        inline iterator insert(const_iterator position, value_type&& val) {
            const auto& v = val;
            return insert(position, v);
        }

        inline iterator insert(const_iterator position, std::initializer_list<value_type> il) {
            return insert(position, il.begin(), il.end());
        }

        inline iterator erase(const_iterator position) {
            return erase(position, position + 1);
        }

        inline iterator erase(const_iterator first, const_iterator last) {
            auto from = (first - cbegin());
            auto to = (last - cbegin());
            auto n = to - from;
            std::copy(begin() + to, end(), begin() + from);

            m_real_size -= n;

            auto obsolete_backing = m_vec.size() - bits2backing(elem2bits(m_real_size));
            m_vec.erase(m_vec.cend() - obsolete_backing, m_vec.cend());
            return begin() + from;
        }

        inline void swap(odd_bit_backing_data& other) {
            m_vec.swap(other.m_vec);
            std::swap(m_real_size, other.m_real_size);
        }

        inline void clear() {
            m_vec.clear();
            m_real_size = 0;
        }

        template <class... Args>
        inline iterator emplace(const_iterator position, Args&&... args) {
            return insert(position, value_type(std::forward<Args...>(args)...));
        }

        template <class... Args>
        inline void emplace_back(Args&&... args) {
            push_back(value_type(std::forward<Args...>(args)...));
        }
    };

    template<size_t N>
    bool operator==(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        if (lhs.size() == rhs.size()) {
            auto extra_bits = odd_bit_backing_data<N>::backing2bits(lhs.m_vec.size())
                              - odd_bit_backing_data<N>::elem2bits(lhs.m_real_size);

            if (extra_bits == 0) {
                return lhs.m_vec == rhs.m_vec;
            } else {
                if (!std::equal(lhs.m_vec.cbegin(), lhs.m_vec.cend() - 1, rhs.m_vec.cbegin())) {
                    return false;
                }
                auto occupied_bits = odd_bit_backing_data<N>::backing2bits(1) - extra_bits;
                // NB: Underflow can not happen here because there are never
                // completely unoccupied backing integers
                auto elements = (occupied_bits - 1) / odd_bit_backing_data<N>::elem2bits(1) + 1;

                return std::equal(lhs.cend() - elements, lhs.cend(), rhs.cend() - elements);
            }
        }
        return false;
    }

    template<size_t N>
    bool operator!=(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return !(lhs == rhs);
    }

    template<size_t N>
    bool operator<(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
    }

    template<size_t N>
    bool operator<=(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return !(lhs > rhs);
    }

    template<size_t N>
    bool operator>(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return std::lexicographical_compare(rhs.cbegin(), rhs.cend(), lhs.cbegin(), lhs.cend());
    }

    template<size_t N>
    bool operator>=(const odd_bit_backing_data<N>& lhs, const odd_bit_backing_data<N>& rhs) {
        return !(lhs < rhs);
    }

    template<size_t N>
    void swap(odd_bit_backing_data<N>& x, odd_bit_backing_data<N>& y) {
        x.swap(y);
    }

    template<class T, class X = void>
    struct GenericIntVectorTrait {
        typedef typename even_bit_backing_data<T>::value_type             value_type;

        typedef typename even_bit_backing_data<T>::reference              reference;
        typedef typename even_bit_backing_data<T>::const_reference        const_reference;

        typedef typename even_bit_backing_data<T>::pointer                pointer;
        typedef typename even_bit_backing_data<T>::const_pointer          const_pointer;

        typedef typename even_bit_backing_data<T>::iterator               iterator;
        typedef typename even_bit_backing_data<T>::const_iterator         const_iterator;

        typedef typename even_bit_backing_data<T>::reverse_iterator       reverse_iterator;
        typedef typename even_bit_backing_data<T>::const_reverse_iterator const_reverse_iterator;

        typedef typename even_bit_backing_data<T>::difference_type        difference_type;
        typedef typename even_bit_backing_data<T>::size_type              size_type;

        typedef          even_bit_backing_data<T>                         backing_data;
        typedef typename even_bit_backing_data<T>::internal_data_type     internal_data_type;
    };

    template<size_t N>
    struct GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) == 0>::type> {
        typedef typename even_bit_backing_data<uint_t<N>>::value_type             value_type;

        typedef typename even_bit_backing_data<uint_t<N>>::reference              reference;
        typedef typename even_bit_backing_data<uint_t<N>>::const_reference        const_reference;

        typedef typename even_bit_backing_data<uint_t<N>>::pointer                pointer;
        typedef typename even_bit_backing_data<uint_t<N>>::const_pointer          const_pointer;

        typedef typename even_bit_backing_data<uint_t<N>>::iterator               iterator;
        typedef typename even_bit_backing_data<uint_t<N>>::const_iterator         const_iterator;

        typedef typename even_bit_backing_data<uint_t<N>>::reverse_iterator       reverse_iterator;
        typedef typename even_bit_backing_data<uint_t<N>>::const_reverse_iterator const_reverse_iterator;

        typedef typename even_bit_backing_data<uint_t<N>>::difference_type        difference_type;
        typedef typename even_bit_backing_data<uint_t<N>>::size_type              size_type;

        typedef          even_bit_backing_data<uint_t<N>>                         backing_data;
        typedef typename even_bit_backing_data<uint_t<N>>::internal_data_type     internal_data_type;
    };

    template<size_t N>
    struct GenericIntVectorTrait<uint_t<N>, typename std::enable_if<(N % 8) != 0>::type> {
        typedef typename odd_bit_backing_data<N>::value_type             value_type;

        typedef typename odd_bit_backing_data<N>::reference              reference;
        typedef typename odd_bit_backing_data<N>::const_reference        const_reference;

        typedef typename odd_bit_backing_data<N>::pointer                pointer;
        typedef typename odd_bit_backing_data<N>::const_pointer          const_pointer;

        typedef typename odd_bit_backing_data<N>::iterator               iterator;
        typedef typename odd_bit_backing_data<N>::const_iterator         const_iterator;

        typedef typename odd_bit_backing_data<N>::reverse_iterator       reverse_iterator;
        typedef typename odd_bit_backing_data<N>::const_reverse_iterator const_reverse_iterator;

        typedef typename odd_bit_backing_data<N>::difference_type        difference_type;
        typedef typename odd_bit_backing_data<N>::size_type              size_type;

        typedef          odd_bit_backing_data<N>                         backing_data;
        typedef typename odd_bit_backing_data<N>::internal_data_type     internal_data_type;
    };

    template<class T>
    class GenericIntVector {
        // TODO: Add custom allocator support
    public:
        typedef typename GenericIntVectorTrait<T>::value_type             value_type;
        typedef typename GenericIntVectorTrait<T>::reference              reference;
        typedef typename GenericIntVectorTrait<T>::const_reference        const_reference;
        typedef typename GenericIntVectorTrait<T>::pointer                pointer;
        typedef typename GenericIntVectorTrait<T>::const_pointer          const_pointer;
        typedef typename GenericIntVectorTrait<T>::iterator               iterator;
        typedef typename GenericIntVectorTrait<T>::const_iterator         const_iterator;
        typedef typename GenericIntVectorTrait<T>::reverse_iterator       reverse_iterator;
        typedef typename GenericIntVectorTrait<T>::const_reverse_iterator const_reverse_iterator;
        typedef typename GenericIntVectorTrait<T>::difference_type        difference_type;
        typedef typename GenericIntVectorTrait<T>::size_type              size_type;

        /// The element type of the internal data buffer accessed with data()
        typedef typename GenericIntVectorTrait<T>::internal_data_type     internal_data_type;
    private:
        typename GenericIntVectorTrait<T>::backing_data m_data;
    public:
        // default
        inline explicit GenericIntVector() {}

        // fill
        explicit GenericIntVector(size_type n): m_data(n) {}
        inline GenericIntVector(size_type n, const value_type& val): m_data(n, val) {}

        // range
        template <class InputIterator>
        inline GenericIntVector(InputIterator first, InputIterator last): m_data(first, last) {}

        // copy
        inline GenericIntVector(const GenericIntVector& other): m_data(other.m_data) {}

        // move
        inline GenericIntVector(GenericIntVector&& other): m_data(std::move(other.m_data)) {}

        // initializer list
        inline GenericIntVector(std::initializer_list<value_type> il): m_data(il) {}

        inline GenericIntVector& operator=(const GenericIntVector& other) {
            m_data = other.m_data;
            return *this;
        }

        inline GenericIntVector& operator=(GenericIntVector&& other) {
            m_data = std::move(other.m_data);
            return *this;
        }

        inline GenericIntVector& operator=(std::initializer_list<value_type> il) {
            m_data = il;
            return *this;
        }

        inline iterator begin() {
            return m_data.begin();
        }

        inline iterator end() {
            return m_data.end();
        }

        inline reverse_iterator rbegin() {
            return m_data.rbegin();
        }

        inline reverse_iterator rend() {
            return m_data.rend();
        }

        inline const_iterator begin() const {
            return m_data.begin();
        }

        inline const_iterator end() const {
            return m_data.end();
        }

        inline const_reverse_iterator rbegin() const {
            return m_data.rbegin();
        }

        inline const_reverse_iterator rend() const {
            return m_data.rend();
        }

        inline const_iterator cbegin() const {
            return m_data.cbegin();
        }

        inline const_iterator cend() const {
            return m_data.cend();
        }

        inline const_reverse_iterator crbegin() const {
            return m_data.crbegin();
        }

        inline const_reverse_iterator crend() const {
            return m_data.crend();
        }

        inline size_type size() const {
            return m_data.size();
        }

        inline uint64_t bit_size() const {
            return m_data.bit_size();
        }

        inline size_type max_size() const {
            return m_data.max_size();
        }

        inline void resize(size_type n) {
            m_data.resize(n);
        }

        inline void resize(size_type n, const value_type& val) {
            m_data.resize(n, val);
        }

        inline size_type capacity() const {
            return m_data.capacity();
        }

        inline bool empty() const {
            return m_data.empty();
        }

        inline void reserve(size_type n) {
            m_data.reserve(n);
        }

        inline void shrink_to_fit() {
            m_data.m_vec.shrink_to_fit();
        }

        inline reference operator[](size_type n) {
            return m_data[n];
        }

        inline const_reference operator[](size_type n) const {
            return m_data[n];
        }

        inline reference at(size_type n) {
            return m_data.at(n);
        }

        inline const_reference at(size_type n) const {
            return m_data.at(n);
        }

        inline reference front() {
            return m_data.front();
        }

        inline const_reference front() const {
            return m_data.front();
        }

        inline reference back() {
            return m_data.back();
        }

        inline const_reference back() const {
            return m_data.back();
        }

        inline internal_data_type* data() noexcept {
            return m_data.m_vec.data();
        }

        inline const internal_data_type* data() const noexcept {
            return m_data.m_vec.data();
        }

        template <class InputIterator>
        inline void assign(InputIterator first, InputIterator last) {
            m_data.assign(first, last);
        }

        inline void assign(size_type n, const value_type& val) {
            m_data.assign(n, val);
        }

        inline void assign(std::initializer_list<value_type> il) {
            m_data.assign(il);
        }

        inline void push_back(const value_type& val) {
            m_data.push_back(val);
        }

        inline void push_back(value_type&& val) {
            m_data.push_back(std::move(val));
        }

        inline void pop_back() {
            m_data.pop_back();
        }

        inline iterator insert(const_iterator position, const value_type& val) {
            return m_data.insert(position, val);
        }

        inline iterator insert(const_iterator position, size_type n, const value_type& val) {
            return m_data.insert(position, n, val);
        }

        template <class InputIterator>
        inline iterator insert(const_iterator position, InputIterator first, InputIterator last) {
            return m_data.insert(position, first, last);
        }

        inline iterator insert(const_iterator position, value_type&& val) {
            return m_data.insert(position, std::move(val));
        }

        inline iterator insert(const_iterator position, std::initializer_list<value_type> il) {
            return m_data.insert(position, il);
        }

        inline iterator erase(const_iterator position) {
            return m_data.erase(position);
        }

        inline iterator erase(const_iterator first, const_iterator last) {
            return m_data.erase(first, last);
        }

        inline void swap(GenericIntVector& other) {
            m_data.swap(other.m_data);
        }

        inline void clear() {
            m_data.clear();
        }

        template <class... Args>
        inline iterator emplace(const_iterator position, Args&&... args) {
            return m_data.emplace(position, std::forward<Args...>(args)...);
        }

        template <class... Args>
        inline void emplace_back(Args&&... args) {
            m_data.emplace_back(std::forward<Args...>(args)...);
        }

        template<class U>
        friend bool operator==(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator!=(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator<(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator<=(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator>(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend bool operator>=(const GenericIntVector<U>& lhs, const GenericIntVector<U>& rhs);
        template<class U>
        friend void swap(GenericIntVector<U>& lhs, GenericIntVector<U>& rhs);
    };

    template<class T>
    bool operator==(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data == rhs.m_data;
    }

    template<class T>
    bool operator!=(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data != rhs.m_data;
    }

    template<class T>
    bool operator<(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data < rhs.m_data;
    }

    template<class T>
    bool operator<=(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data <= rhs.m_data;
    }

    template<class T>
    bool operator>(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data > rhs.m_data;
    }

    template<class T>
    bool operator>=(const GenericIntVector<T>& lhs, const GenericIntVector<T>& rhs) {
        return lhs.m_data >= rhs.m_data;
    }

    template<class T>
    void swap(GenericIntVector<T>& lhs, GenericIntVector<T>& rhs) {
        using std::swap;
        swap(lhs.m_data, rhs.m_data);
    }

}

inline IntegerBaseTraitConst<int_vector::IntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::IntRef>::cast_for_self_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline IntegerBaseTraitConst<int_vector::IntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::IntRef>::cast_for_32_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline uint64_t IntegerBaseTraitConst<int_vector::IntRef>::cast_for_64_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline void IntegerBaseTrait<int_vector::IntRef>::assign(T& self, uint32_t v) {
    bits::write_int(self.m_ptr.m_ptr,
                    v,
                    self.m_ptr.m_bit_offset,
                    self.m_ptr.m_bit_size);
}
inline void IntegerBaseTrait<int_vector::IntRef>::assign(T& self, uint64_t v) {
    bits::write_int(self.m_ptr.m_ptr,
                    v,
                    self.m_ptr.m_bit_offset,
                    self.m_ptr.m_bit_size);
}

inline IntegerBaseTraitConst<int_vector::ConstIntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::ConstIntRef>::cast_for_self_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline IntegerBaseTraitConst<int_vector::ConstIntRef>::SelfMaxBit IntegerBaseTraitConst<int_vector::ConstIntRef>::cast_for_32_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}
inline uint64_t IntegerBaseTraitConst<int_vector::ConstIntRef>::cast_for_64_op(const T& self) {
    return bits::read_int(self.m_ptr.m_ptr,
                            self.m_ptr.m_bit_offset,
                            self.m_ptr.m_bit_size);
}

}

#endif
