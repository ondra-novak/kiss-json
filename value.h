/*
 * value.h
 *
 *  Created on: 15. 7. 2022
 *      Author: ondra
 */

#ifndef KISSJSON_VALUE_H_
#define KISSJSON_VALUE_H_

#include "core.h"

#include <iostream>
#include <numeric>

namespace kjson {

class Value;

using KeyValue = std::pair<std::string_view, Value>;


///Generic JSON value - can store any JSON type
class Value {
public:
    ///Construct undefined value
    Value():_ptr(Node::shared_undefined()) {}
    ///Construct null value
    Value(std::nullptr_t):_ptr(Node::shared_null()) {}
    ///Construct value object from json node (internal object)
    explicit Value(PNode nd): _ptr(nd) {}

    ///Value can be copied
    Value(const Value &) = default;
    ///Value can be moved
    Value(Value &&) = default;

    ///Value can be assigned
    Value &operator=(const Value &) = default;
    ///Value can be assigned while moved
    Value &operator=(Value &&) = default;

    ///Retrieves pointer to value storage (internal object)
    const PNode &get_handle() const {return _ptr;}

    ///Construct boolean value
    Value(bool b):_ptr(Node::shared_boolean(b)) {}
    ///Construct string value (must exists, otherwise compiler chooses bool)
    Value(const char *c):_ptr(Node::new_string(std::string_view(c))) {}
    ///Construct string value
    Value(const std::string_view &a):_ptr(Node::new_string(a)) {}
    ///Construct a number
    Value(int a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(unsigned int a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(long a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(unsigned long a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(long long a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(unsigned long long a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(float a):_ptr(Node::new_number(a)) {}
    ///Construct a number
    Value(double a):_ptr(Node::new_number(a)) {}

    ///Helper class to construct objects
    class object {
    public:
        ///construct empty object
        object():_ptr(Node::shared_empty_object()) {}

        ///construct from initializer list definition
        object(const std::initializer_list<KeyValue > &obj)
            :_ptr(Node::new_object(obj.size(), [obj](ContBuilder &bld) {
            for (const auto &itm: obj) {
                bld.push_back(itm.second.get_handle()->set_key(itm.first));
            }
            std::sort(bld.begin(),bld.end(),Node::KeyOrder());
        })){}
        ///construct object from container of values with ability to filter and transfer items
        /**
         * @param c source container
         * @param fn function which is responsible to convert item to Value. All
         *           undefined results are not stored. The function must also bind
         *           keys to values. To do this, use Value(key, value) constructor
         */
        template<typename Container, typename Fn,
            typename = decltype(std::begin(std::declval<const Container &>()) == std::end(std::declval<const Container &>())),
            typename = decltype(Value(std::declval<Fn>()(*std::begin(std::declval<const Container &>()))))>
        object(const Container &c, Fn &&fn):
            _ptr(Node::new_object(std::distance(std::begin(c), std::end(c)),
                                 [&c, &fn](ContBuilder &b) {
               for(const auto &x : c) {
                   Value v = fn(x);
                   if (v.defined()) {
                       b.push_back(Value(fn(x)).get_handle());
                   }
               }
               std::sort(b.begin(),b.end(),Node::KeyOrder());
        })){}

        template<typename Fn>
        object(std::size_t count, Fn &&fn)
            :_ptr(Node::new_object(count, [&](ContBuilder &bld){
                for (std::size_t i = 0; i < count; i++) {
                    Value v = fn(i);
                    if (!v.defined()) break;
                    bld.push_back(v.get_handle());
                }
                std::sort(bld.begin(),bld.end(),Node::KeyOrder());
        })) {}


        static object from_value(const Value &v) {
            return object(v.get_handle());
        }

    protected:
        explicit object(const PNode &ptr):_ptr(ptr) {}

        friend class Value;
        PNode _ptr;
    };

    ///Helper class to construct arrays
    class array {
    public:
        ///construct empty array
        array():_ptr(Node::shared_empty_array()) {}
        ///construct array from initialized list definition
        array(const std::initializer_list<Value> &array)
            :_ptr(Node::new_array(array.size(), [array](ContBuilder &bld) {
            for (const Value &v : array) {
                bld.push_back(v.get_handle());
            }
         })){}

        explicit array(const Value &v):_ptr(v.get_handle()) {}

        ///construct array from container of values
        template<typename Container,
            typename = decltype(std::begin(std::declval<const Container &>()) == std::end(std::declval<const Container &>())),
            typename = decltype(Value(*std::begin(std::declval<const Container &>())))>
        explicit array(const Container &c):
            _ptr(Node::new_array(std::distance(std::begin(c), std::end(c)),
                                 [&c](ContBuilder &b) {
               for(const auto &x : c) {
                   b.push_back(Value(x).get_handle());
               }
        })){}
        ///construct array from container of values with ability to filter and transfer items
        /**
         * @param c source container
         * @param fn function which is responsible to convert item to Value. All
         *           undefined results are not stored
         */
        template<typename Container, typename Fn,
            typename = decltype(std::begin(std::declval<const Container &>()) == std::end(std::declval<const Container &>())),
            typename = decltype(Value(std::declval<Fn>()(*std::begin(std::declval<const Container &>()))))>
        array(const Container &c, Fn &&fn):
            _ptr(Node::new_array(std::distance(std::begin(c), std::end(c)),
                                 [&c, &fn](ContBuilder &b) {
               for(const auto &x : c) {
                   Value v = fn(x);
                   if (v.defined()) {
                       b.push_back(Value(fn(x)).get_handle());
                   }
               }
        })){}

        template<typename Fn>
        array(std::size_t count, Fn &&fn)
            :_ptr(Node::new_array(count, [&](ContBuilder &bld){
                for (std::size_t i = 0; i < count; i++) {
                    Value v = fn(i);
                    if (!v.defined()) break;
                    bld.push_back(v.get_handle());
                }
        })) {}

        static array from_value(const Value &v) {
            return array(v.get_handle());
        }

    protected:
        explicit array(const PNode &ptr):_ptr(ptr) {}
        friend class Value;
        PNode _ptr;
    };


    ///Construct value using helper object class
    Value(const object &obj):_ptr(obj._ptr) {}
    ///Construct value using helper array class
    Value(const array &arr):_ptr(arr._ptr) {}
    ///Construct object directly
    Value(const std::initializer_list<KeyValue > &obj):Value(object(obj)) {}
    ///Construct value with bound key
    Value(const std::string_view &key, const Value &val):_ptr(val.get_handle()->set_key(key)) {}
    ///Construct value, unbind any bound key
    Value(std::nullptr_t, const Value &val):_ptr(val.get_handle()->unset_key()) {}

    bool operator==(const Value &other) const {return _ptr->compare(*other._ptr) == 0;}
    bool operator!=(const Value &other) const {return _ptr->compare(*other._ptr) != 0;}
    bool operator>(const Value &other) const {return _ptr->compare(*other._ptr) > 0;}
    bool operator<(const Value &other) const {return _ptr->compare(*other._ptr) < 0;}
    bool operator>=(const Value &other) const {return _ptr->compare(*other._ptr) >= 0;}
    bool operator<=(const Value &other) const {return _ptr->compare(*other._ptr) <= 0;}

    ///Get string value
    auto get_string() const {return _ptr->get_string();}
    ///Get numeric value
    auto get_int() const {return _ptr->get_int();}
    ///Get numeric value
    auto get_unsigned_int() const {return _ptr->get_unsigned_int();}
    ///Get numeric value
    auto get_long() const {return _ptr->get_long();}
    ///Get numeric value
    auto get_unsigned_long() const {return _ptr->get_unsigned_long();}
    ///Get numeric value
    auto get_long_long() const {return _ptr->get_long_long();}
    ///Get numeric value
    auto get_unsigned_long_long() const {return _ptr->get_unsigned_long_long();}
    ///Get numeric value
    auto get_float() const {return _ptr->get_float();}
    ///Get numeric value
    auto get_double() const {return _ptr->get_double();}
    ///Get boolean value
    auto get_bool() const {return _ptr->get_boolean();}
    ///Returns true, if the value is defined, false if undefined
    /**
     * @retval true value is defined
     * @retval false value is undefined
     */
    auto defined() const {return _ptr->get_type() != ValueType::undefined;}
    ///Returns true, if the value is defined and contains different value than null
    /**
     * @retval true value is defined and it is not null
     * @retval false value is undefined or null
     */
    auto has_value() const {
        auto t = _ptr->get_type();
        return t != ValueType::undefined && t != ValueType::null;
    }

    auto get_key() const {return _ptr->get_key();}
    ///Retrieves value type
    auto get_type() const {return _ptr->get_type();}
    ///Returns true, if the value is null
    /**
     * @retval true value is null
     * @retval false value is not null, so it has value or it is undefined
     */
    auto is_null() const {return _ptr->get_type() == ValueType::null;}
    ///Returns true, if the value is a container - object or array
    /**
     * @retval true value is container - object or array
     * @retval false value is not container - can be single value, null or undefined
     */
    auto is_container() const  {
        auto t = _ptr->get_type();
        return t == ValueType::object && t != ValueType::array;
    }
    ///Returns true if the value is an object (key valued container)
    auto is_object() const {return _ptr->get_type() == ValueType::object;}
    ///Returns true if the value is an array
    auto is_array() const {return _ptr->get_type() == ValueType::array;}
    ///Returns true if the value is string
    auto is_string() const {return _ptr->get_type() == ValueType::string;}
    ///Returns true if the value is number
    auto is_number() const {return _ptr->get_type() == ValueType::number;}
    ///Returns true if the value is boolean
    auto is_bool() const {return _ptr->get_type() == ValueType::boolean;}
    ///Returns true if the value is copy of other value
    /**
     * This is faster comparison for a value which was created as copy of other
     * value, so they are equal. Note that returned false doesn't mean, that
     * values are not equal, you will need to use standard comparison to
     * determine this
     *
     * @param other other value to check. Keys are ignored
     * @retval true this value is copy of other value
     * @retval false this value is not copy of other value
     *
     * @note FYI - standard comparison operator also use this function to faster
     * determine, whether values are equal, so you don't need to call this function
     * in this case.
     *
     * @code
     * Value a("hello");
     * Value b("hello");
     * Value c(a);
     *
     * bool r1 = c.is_copy_of(a) ; //< true
     * bool r2 = c.is_copy_of(b) ; //< false
     * bool r3 = a.is_copy_of(b) ; //< false
     * @endcode
     *
     * @note for 'undefined', 'null', boolean values, and some special values, this
     * function can return true even if the values was not copied. That is because
     * these values are actually copy of statically allocated values kept to
     * reduce allocations in these cases
     *
     * @code
     * Value a(true);
     * Value b(true);
     *
     * bool r = a.is_copy_of(b); //< true - because Value(true) is preallocated
     * @endcode
     *
     */
    auto is_copy_of(const Value &other) const {return _ptr->unset_key() == other._ptr->unset_key();}

    void bind_key(const std::string_view &key) {_ptr = _ptr->set_key(key);}
    void unbind_key() {_ptr = _ptr->unset_key();}

    std::size_t size() const {return _ptr->size();}

    bool empty() const {return _ptr->empty();}

    Value operator[](std::size_t idx) const {return Value(_ptr->get(idx));}
    Value operator[](std::string_view name) const {return Value(_ptr->get(name));}

    class iterator;
    using reverse_iterator = std::reverse_iterator<iterator>;

    iterator begin() const;
    iterator end() const;
    reverse_iterator rbegin() const;
    reverse_iterator rend() const;

    ///Merges two object into one. Result replaces current value
    /**
     * This is the most useful way to modify objects
     *
     * @param obj object to be 'applied'. Keys from obj are applied to current object. To
     * delete particular key, set its value to undefined (this can be changed by third argument).
     * @param merge specifies, how to merge sub-objects. If the parameter contains Merge::flat
     *          , only first level is merged, but subobjects are replaces.
     *          If the parameter contains Merge::recursive, then merge is applied
     *          to all sub-objects. Arrays are not merged
     *
     * @param unset_item allows to define item used as 'delete key' flag. Default
     * value is 'undefined'. You can specify different value, but note that
     * this value is compared using function is_copy_of(). The main reason
     * for this option is to allow specify empty object to able merge JSON file
     * which doesn't support 'undefined' value
     */
    void merge(const object &obj, Merge merge = Merge::flat, const Value &unset_item = Value());

    template<typename Fn>
    Value map(Fn &&fn) const {
        return Value(array::from_value(*this), std::forward<Fn>(fn));
    }

    template<typename Fn>
    Value map_to_object(Fn &&fn) const {
        return Value(object::from_value(*this), std::forward<Fn>(fn));
    }

    template<typename Fn, typename T>
    T reduce(Fn &&fn, T &&initial) const {
        return std::accumulate(begin(), end(), std::forward<T>(initial), std::forward<Fn>(fn));
    }

    template<typename Fn>
    Value filter(Fn &&fn) const {
        return map([&](const Value &x){
            if (fn(x)) return x;
            else return Value();
        });
    }

    template<typename Fn>
    Value filter_object(Fn &&fn) const {
        return map_to_object([&](const Value &x){
            if (fn(x)) return x;
            else return Value();
        });
    }

    template<typename Fn>
    void transform(Fn &&fn) {
        *this = map(std::forward<Fn>(fn));
    }

    template<typename Fn>
    void transform_object(Fn &&fn) {
        *this = map_to_object(std::forward<Fn>(fn));
    }

    template<typename Fn>
    void transform_flatten(Fn &&fn) {
        *this = map(std::forward<Fn>(fn)).flatten();
    }

    void push(const Value &item);

    void append(const array &arr);

    Value slice(std::ptrdiff_t from, std::ptrdiff_t to) const;


    Value flatten() const;



    template<typename Fn>
    void serialize(Fn &&fn, OutputType ot = OutputType::ascii) const;

    std::string to_string(OutputType ot = OutputType::utf8) const {
        std::string out;
        serialize([&](char c){
            out.push_back(c);
        }, ot);
        return out;
    }

    void to_stream(std::ostream &stream, OutputType ot = OutputType::ascii) {
        serialize([&](char c){stream.put(c);}, ot);
    }

    template<typename Fn>
    static Value parse(Fn &&fn);

    static Value from_string(const std::string_view &str);

    static Value from_stream(std::istream &stream) {
        return parse([&]{return stream.get();});
    }


protected:
    PNode _ptr;
};

class Value::iterator {
public:
    using value_type = Value;
    using reference = const Value &;
    using pointer = const Value *;
    using difference_type = std::ptrdiff_t;

    iterator(const PNode &ptr, std::size_t idx):_ptr(ptr), _idx(idx) {}
    iterator &operator++() {++_idx;return *this;}
    iterator &operator--() {--_idx;return *this;}
    iterator operator++(int) {++_idx; return iterator(_ptr, _idx-1);}
    iterator operator--(int) {--_idx; return iterator(_ptr, _idx+1);}
    reference operator *() const { _tmp = Value(_ptr->get(_idx)); return _tmp;}
    pointer operator ->() const { _tmp = Value(_ptr->get(_idx)); return &_tmp;}

    bool operator == (const iterator &other) const {return _ptr == other._ptr && _idx == other._idx;}
    bool operator != (const iterator &other) const {return !operator==(other);}
    bool operator < (const iterator &other) const {return _idx < other._idx;}
    bool operator > (const iterator &other) const {return _idx > other._idx;}
    bool operator <= (const iterator &other) const {return _idx <= other._idx;}
    bool operator >= (const iterator &other) const {return _idx >= other._idx;}

    iterator& operator+=(std::size_t sz) {_idx += sz; return *this;}
    friend iterator operator+(const iterator&me, std::size_t sz) {return iterator(me._ptr,me._idx+sz);}
    friend iterator operator+(std::size_t sz, const iterator&me) {return iterator(me._ptr,me._idx+sz);}
    iterator& operator-=(std::size_t sz) {_idx -= sz; return *this;}
    friend iterator operator-(const iterator &me, std::size_t sz) {return iterator(me._ptr,me._idx-sz);}
    friend difference_type operator-(const iterator &me1, const iterator &me2) {
        return static_cast<difference_type>(me1._idx) - static_cast<difference_type>(me2._idx);
    }

    Value container() const {return Value(_ptr);}

    reference operator[](std::size_t sz) const {
        _tmp = Value(_ptr->get(_idx+sz)); return _tmp;
    }

protected:
    PNode _ptr;
    std::size_t _idx;
    mutable Value _tmp;
};

}

template<>
struct std::iterator_traits<kjson::Value::iterator> {
    typedef std::ptrdiff_t difference_type; //almost always ptrdiff_t
    typedef kjson::Value value_type; //almost always T
    typedef const kjson::Value &reference; //almost always T& or const T&
    typedef const kjson::Value *pointer; //almost always T* or const T*
    typedef std::random_access_iterator_tag iterator_category;  //usually std::forward_iterator_tag or similar
};

inline kjson::Value::iterator kjson::Value::begin() const {
    return iterator(_ptr, 0);
}
inline kjson::Value::iterator kjson::Value::end() const {
    return iterator(_ptr, _ptr->size());
}
inline kjson::Value::reverse_iterator kjson::Value::rbegin() const {
    return reverse_iterator(iterator(_ptr, _ptr->size()-1));
}
inline kjson::Value::reverse_iterator kjson::Value::rend() const {
    return reverse_iterator(iterator(_ptr, -1));
}

inline void kjson::Value::merge(const object &obj, Merge merge, const Value &unset_item) {
    Value src(is_object()?Value(nullptr,*this):Value(Value::object()));
    Value diff(obj);

    auto get_merged = [&](const Value &src, const Value &diff) {
        if (diff.is_object() && merge == Merge::recursive) {
            Value x(src);
            x.merge(object::from_value(diff), merge, unset_item);
            return Value(src.get_key(),x);
        } else {
            return diff;
        }
    };

    _ptr = Node::new_object(src.size()+diff.size(), [&](ContBuilder &bld){
        auto iter1 = src.begin(), end1= src.end();
        auto iter2 = diff.begin(), end2 = diff.end();
        while (iter1 != end1 && iter2 != end2) {
            int kc = iter1->get_key().compare(iter2->get_key());
            if (kc<0) {
                bld.push_back(iter1->get_handle());
                ++iter1;
            } else if (kc>0) {
                if (!iter2->is_copy_of(unset_item)) {
                    bld.push_back(get_merged(object(), *iter2).get_handle());
                }
                ++iter2;
            } else if (iter2->is_copy_of(unset_item)) {
                ++iter1;
                ++iter2;
            } else {
                bld.push_back(get_merged(*iter1, *iter2).get_handle());
                ++iter1;
                ++iter2;
            }
        }
        while (iter1 != end1) {
            bld.push_back(iter1->get_handle());
            ++iter1;
        }
        while (iter2 != end2) {
            if (!iter2->is_copy_of(unset_item)) {
                bld.push_back(get_merged(object(), *iter2).get_handle());
            }
            ++iter2;
        }
    });
}

inline void kjson::Value::append(const array &arr) {
    std::size_t sz = size() + Value(arr).size();
    _ptr = Node::new_array(sz, [&](ContBuilder &bld){
        for (const Value &x : *this) {
            bld.push_back(x.get_handle());
        }
        for (const Value &x: Value(arr)) {
            bld.push_back(x.get_handle());
        }
    });
}

inline void kjson::Value::push(const Value &x) {
    std::size_t sz = size() + 1;
    _ptr = Node::new_array(sz, [&](ContBuilder &bld){
        for (const Value &x : *this) {
            bld.push_back(x.get_handle());
        }
        bld.push_back(x.get_handle());
    });
}

kjson::Value kjson::Value::flatten() const {
    std::size_t sz = reduce([](std::size_t n, const Value &v){
        if (v.is_container()) return n+v.size();
        else return n+1;
    }, std::size_t(0));

    return Value(Node::new_array(sz, [&](ContBuilder &bld){
        for (const Value &v: *this) {
            if (v.is_container()) {
                for (const Value &z: v) {
                    bld.push_back(z.get_handle());
                }
            } else {
                bld.push_back(v.get_handle());
            }
        }
    }));
}






#endif /* KISSJSON_VALUE_H_ */
