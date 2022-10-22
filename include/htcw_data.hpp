#pragma once
#include <stdlib.h>
namespace data {
    // dynamic vector of scalar (constructorless) data
    template <typename T>
    class simple_vector final {
        void* (*m_allocator)(size_t);
        void* (*m_reallocator)(void*, size_t);
        void (*m_deallocator)(void*);
        size_t m_size;  // size in number of T values
        T* m_begin;
        size_t m_capacity;  // allocated size in T values
        bool resize(size_t new_size) {
            size_t capacity = new_size;
            if (capacity > this->m_capacity) {
                size_t newcap = capacity + (this->m_capacity >> 1u);
                void* data ;
                if(this->m_begin) {
                    data = m_reallocator(this->m_begin, newcap * sizeof(T));
                } else {
                    newcap=8;
                    const size_t newsz = newcap*sizeof(T);
                    data = m_allocator(newsz);
                }
                if (data) {
                    this->m_capacity = newcap;
                    this->m_begin = (T*)data;
                } else
                    return false; //error: not enough memory
            }
            this->m_size = new_size;
            return true;
        }

    public:
        using value_type = T;
        simple_vector(void*(allocator)(size_t) = ::malloc,
                    void*(reallocator)(void*, size_t) = ::realloc,
                    void(deallocator)(void*) = ::free) : 
                        m_allocator(allocator),
                        m_reallocator(reallocator),
                        m_deallocator(deallocator) {
            m_begin = nullptr;
            m_size = 0;
            m_capacity = 0;
        }
        ~simple_vector() {
            m_size = 0;
            if (m_begin != nullptr) {
                m_deallocator(m_begin);
                m_begin = nullptr;
            }
        }
        inline size_t size() const { return m_size; }
        inline size_t capacity() const { return m_capacity; }
        inline const T* cbegin() const { return m_begin; }
        inline const T* cend() const { return m_begin + m_size; }
        inline T* begin() { return m_begin; }
        inline T* end() { return m_begin + m_size; }
        void clear() {
            if(m_begin) {
                m_size = 0;
                m_capacity = 0;
                m_deallocator(m_begin);
                m_begin = nullptr;
            }
        }
        bool push_back(const T& value) {
            if (!resize(m_size + 1)) {
                return false;
            }
            m_begin[m_size - 1] = value;
            return true;
        }
    };
    // a key value pair
    template<typename TKey, typename TValue> 
    struct simple_pair {
        TKey key;
        TValue value;
        simple_pair(TKey key,TValue value) : key(key), value(value) {

        }
        simple_pair(const simple_pair& rhs) {
            key = rhs.key;
            value = rhs.value;
        }
        simple_pair& operator=(const simple_pair& rhs) {
            key = rhs.key;
            value = rhs.value;
            return *this;
        }
        simple_pair(simple_pair&& rhs) {
            key = rhs.key;
            value = rhs.value;
        }
        simple_pair& operator=(simple_pair&& rhs) {
            key = rhs.key;
            value = rhs.value;
            return *this;
        }
    };
    // a simple hash table
    template<typename TKey,typename TValue, size_t Size> 
    class simple_fixed_map final {
        static_assert(Size>0,"Size must be a positive integer");
        using bucket_type = simple_vector<simple_pair<TKey,TValue>>;
        bucket_type m_buckets[Size];
        int(*m_hash_function)(const TKey&);
        size_t m_size;
    public:
        simple_fixed_map(int(hash_function)(const TKey&),
                        void*(allocator)(size_t) = ::malloc,
                        void*(reallocator)(void*, size_t) = ::realloc,
                        void(deallocator)(void*) = ::free) : m_hash_function(hash_function),m_size(0) {
            for(size_t i = 0;i<(int)Size;++i) {
                m_buckets[i]=bucket_type(allocator,reallocator,deallocator);
            }
        }
        using key_type = TKey;
        using mapped_type = TValue;
        using value_type = simple_pair<const TKey,TValue>;
        inline size_t size() const { return m_size; }
        void clear() {
            m_size = 0;
            for(size_t i = 0;i<Size;++i) {
                m_buckets->clear();
            }
        }
        bool insert(const value_type& value) {
            int h = m_hash_function(value.key)%Size;
            bucket_type& bucket = m_buckets[h];
            if(bucket.size()) {
                auto it = bucket.cbegin();
                while(it!=bucket.cend()) {
                    if(it->key==value.key) {
                        return false;
                    }
                    ++it;
                }
            }
            
            if(bucket.push_back({value.key,value.value})) {
                ++m_size;
                return true;
            }
            return false;
        }
        const mapped_type* find(const key_type& key) const {
            int h = m_hash_function(key)%Size;
            const bucket_type& bucket = m_buckets[h];
            if(bucket.size()) {
                auto it = bucket.cbegin();
                while(it!=bucket.cend()) {
                    if(it->key==key) {
                        return &it->value;
                    }
                    ++it;
                }
            }
            return nullptr;
        }
        mapped_type* find_mutable(const key_type& key) {
            int h = m_hash_function(key)%Size;
            bucket_type& bucket = m_buckets[h];
            if(bucket.size()) {
                auto it = bucket.begin();
                while(it!=bucket.end()) {
                    if(it->key==key) {
                        return &it->value;
                    }
                    ++it;
                }
            }
            return nullptr;
        }
    };
    // a circular buffer
    template <typename T,size_t Capacity>
    class circular_buffer {
        T m_data[Capacity];
        size_t m_head;
        size_t m_tail;
        bool m_full;
        void advance() {
            if (m_full) {
                if (++(m_tail) == capacity) {
                    m_tail = 0;
                }
            }

            if (++(m_head) == capacity) {
                m_head = 0;
            }
            m_full = (m_head == m_tail);
        }
        void retreat() {
            m_full = false;
            if (++(m_tail) == capacity) { 
                m_tail = 0;
            }
        }
    public:
        using type = circular_buffer;
        using value_type = T;
        constexpr static const size_t capacity = Capacity;

        inline circular_buffer() {
            clear();
        }
        inline bool empty() const {
            return (!m_full && (m_head == m_tail));
        }
        inline bool full() const {
            return m_full;
        }
        size_t size() const {
            size_t result = capacity;
            if(!m_full) {
                if(m_head >= m_tail) {
                    result = (m_head - m_tail);
                } else {
                    result = (capacity + m_head - m_tail);
                }
            }
            return result;
        }
        inline void clear() {
            m_head = 0;
            m_tail = 0;
            m_full = false;
        }
        void put(const value_type& value) {
            m_data[m_head] = value;
            advance();
        }
        const value_type* peek() const {
            if(!empty()) {
                return m_data+m_tail;
            }
            return nullptr;
        }
        bool get(value_type* out_value) {
            if(!empty()) {
                if(out_value!=nullptr) {
                    *out_value = m_data[m_tail];
                }
                retreat();
                return true;
            }
            return false;
        }
        
    };
    
}  // namespace data