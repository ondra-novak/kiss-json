/*
 * value.h
 *
 *  Created on: 15. 7. 2022
 *      Author: ondra
 */

#ifndef KISSJSON_VALUE_H_
#define KISSJSON_VALUE_H_

#include "core.h"
#include "base64.h"

#include <iostream>
#include <numeric>

namespace kjson {

class Value;
class Object;
class Array;
class Binary;

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
    Value(const char *c):_ptr(Node::new_string(std::string_view(c), StringType::utf8)) {}
    ///Construct string value
    Value(const std::string_view &a):_ptr(Node::new_string(a, StringType::utf8)) {}
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
    ///Construct user defined value
    /**
     * @param user_type reference to user defined type descriptor
     * @param args optional arguments passed to the constructor
     */
    Value(const UserDefinedValueTypeDesc &user_type,  void *args)
        :_ptr(Node::new_user_value(user_type, args)) {}

    ///Construct object directly
    Value(const std::initializer_list<KeyValue > &obj);
    ///Construct value with bound key
    Value(const std::string_view &key, const Value &val):_ptr(val.get_handle()->set_key(key)) {}
    ///Construct value, unbind any bound key
    Value(std::nullptr_t, const Value &val):_ptr(val.get_handle()->unset_key()) {}

    ///Construct string - allows to specify string type (for example to store binary data)
    Value(const std::string_view &a, StringType type)
        :_ptr(Node::new_string(a, type)) {}

    bool operator==(const Value &other) const {return _ptr->compare(*other._ptr) == 0;}
    bool operator!=(const Value &other) const {return _ptr->compare(*other._ptr) != 0;}
    bool operator>(const Value &other) const {return _ptr->compare(*other._ptr) > 0;}
    bool operator<(const Value &other) const {return _ptr->compare(*other._ptr) < 0;}
    bool operator>=(const Value &other) const {return _ptr->compare(*other._ptr) >= 0;}
    bool operator<=(const Value &other) const {return _ptr->compare(*other._ptr) <= 0;}

    ///Retrieves value type
    auto get_type() const {return _ptr->get_type();}
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
    ///Retrieve bound key
    auto get_key() const {return _ptr->get_key();}

    ///Retrieve binary value
    /**
     * It is expected string field encoded as BASE64. Function returns Binary object with decoded
     * binary value. You can use Binary::get_string to access binary data.
     *
     * If the value is internally in binary state, no decoding is done, value is just copied
     * to Binary object. You can stull use Binary::get_string to access binary data
     *
     * @return
     */
    Binary get_binary() const;

    ///Retrieves value as an array
    /**
     * This is just a cast, which allows to reinterpret content of the value
     * as an array. However, this works only for containers, so for arrays
     * and objects (objects can act as array of key-value items)
     *
     * Some functions states that expects exactly Array as argument, so this
     * is way how to convert Value to Array
     * @return Array instance
     */
    Array get_array() const;


    ///Retrieves value as an object
    /**
     * This is just a cast, which allows to reinterpret content of the value
     * as an object. However the most functions that exception object as input
     * will fails if passed value is not object. So only objects can
     * be converted from Value to Object. Note, there is no check, no error, if
     * you try to convert something else. Non-container values looks like
     * empty container
     *
     * @return Object instance
     */

    Object get_object() const;

    ///Retrieves content of user defined value
    /**
     * @return pointer to user defined value content. Returns nullptr for all non-user-defined
     * values.
     */
    const UserDefinedValue *get_user_defined_content() const {
        return _ptr->get_user_defined_content();
    }

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
    ///Returns true if the value is string and it is binary
    /**
     * @retval true string is stored in binary form, which does mean it will be encoded to BASE64
     * @retval false string is storead as ascii/utf-8 string,
     *
     * @note Parser doesn't automatically decode binary strings, so if the given value is expected
     * as binary string, it can still emit non-binary state until it is decoded by the function
     * get_binary()
     */
    auto is_binary_string() const {return _ptr->get_string_type() != StringType::utf8;}
    ///Returns true if the value is number
    auto is_number() const {return _ptr->get_type() == ValueType::number;}
    ///Returns true if the value is boolean
    auto is_bool() const {return _ptr->get_type() == ValueType::boolean;}
    ///Returns true if the value is user defined
    auto is_user_defined() const {return _ptr->get_type() == ValueType::user_defined;}
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
    ///Retrieve size of container
    /**
     * Container is object or array
     * @return count of items in containers, returns 0 for non-container value
     */
    std::size_t size() const {return _ptr->size();}
    ///Returns true when container is empty
    bool empty() const {return _ptr->empty();}
    ///Binds a key
    /**
     * binds key to value
     * @param key key text
     *
     * @note if there is already bound key, it is rewritten
     */
    void bind_key(const std::string_view &key) {_ptr = _ptr->set_key(key);}
    ///Removes the key from the value
    /**
     * Bound key can sometimes cause an issues, for exmple if you accessing internals directly.
     * This function removes key from the value.
     */
    void unbind_key() {_ptr = _ptr->unset_key();}

    ///Access to item by index in a container
    /**
     * @param idx index to item, items are zero-base indexed. This applied for both
     * object and arrays. Objects are always ordered alphabetically by the key. Requesting
     * item outside of range causes that 'undefined' is returned
     *
     * @return item at given index, undefined outside of range
     */

    Value operator[](std::size_t idx) const {return Value(_ptr->get(idx));}
    ///Access to item by key name
    /**
     * @param name name of key to access
     * @return item at given key, undefined if not exists
     */
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
    void merge(const Object &obj, Merge merge = Merge::flat, const Value &unset_item = Value());


    ///Transform items in an array through a function
    /**
     * @param fn function
     *
     * @note transform is performed in place (variable is changed)
     *
     * @see map
     */
    template<typename Fn>
    void transform(Fn &&fn) {
        *this = map(std::forward<Fn>(fn));
    }

    ///Transform values of object
    /**
     * @param fn function
     *
     * @note transform is performed in place (variable is changed)
     *
     * @note returning 'undefined' from the map causes deletion if the item
     */
    template<typename Fn>
    void transform_object(Fn &&fn) {
        *this = map_to_object(std::forward<Fn>(fn));
    }

    ///Transform an array and executes flatten on transformed result
    /**
    * @param fn map function
    *
    * @note transform is performed in place (variable is changed)
    *
    * @note returning 'undefined' from the map causes deletion if the item
    *
    * @see map, flatten
    */

    template<typename Fn>
    void transform_flatten(Fn &&fn) {
        *this = map(std::forward<Fn>(fn)).flatten();
    }

    ///Push item to and array
    /**
     * @param item new items
     *
     * @note function can be slow for pushing items by one by. It is much faster to use
     * std::vector and then convert that vector to Value
     */
    void push(const Value &item);

    ///Pop last item from array
    Value pop() {
        Value x = back();
        *this = slice(0,-1);
        return x;
    }

    ///Retrieve last item from array
    Value back() const {
        return (*this)[size()-1];
    }

    ///Splice function (from javascript)
    Value splice(std::ptrdiff_t start, std::ptrdiff_t delete_count);

    ///Splice function (from javascript)
    Value splice(std::ptrdiff_t start, std::ptrdiff_t delete_count, const Array &new_items);

    ///Splice function (from javascript)
    Value splice(std::ptrdiff_t start) {
        Value ret = slice(start);
        *this = slice(0,start);
        return ret;
    }
    ///Append an array to array - in place
    void append(const Array &arr);


    ///Map array to another array
    /**
     * @param fn mapping function
     * @return mapped array, original variable doesn't change
     *
     * @note returning 'undefined' from the map causes deletion if the item
     */
    template<typename Fn>
    Value map(Fn &&fn) const;

    ///Map values of object to another object
    /**
     * @param fn mapping function
     * @return mapped object, original variable doesn't change
     *
     * @note returning 'undefined' from the map causes deletion if the item
     */
    template<typename Fn>
    Value map_object(Fn &&fn) const;

    ///Reduce function
    /**
     * @param fn reduce function (binary operator)
     * @param initial initial value
     * @return result of reduction
     */
    template<typename Fn, typename T>
    T reduce(Fn &&fn, T &&initial) const {
        return std::accumulate(begin(), end(), std::forward<T>(initial), std::forward<Fn>(fn));
    }

    ///Filters array or object
    /**
     * @param fn function which returns true to keep value, or false to delete value
     * @return filtered array or object
     */
    template<typename Fn>
    Value filter(Fn &&fn) const {
        if (is_object()) {
            return map_to_object([&](const Value &x){
                if (fn(x)) return x;
                else return Value();
            });
        } else {
            return map([&](const Value &x){
                if (fn(x)) return x;
                else return Value();
            });
        }
    }


    ///Slice of array (same as javascript)
    /**
     *
     * @param from starting index (included). If negative, it is counted from end
     * @param to ending index (excluded) . If negative, it is counted from end
     * @return slice of array
     *
     * @note slicing array creates special object 'slice' which refers original array. Even
     * if the original array is no longer reference until there is at least one slice, whole
     * array is still kept in memory
     */

    Value slice(std::ptrdiff_t from, std::ptrdiff_t to) const;

    ///Slice of array (same as javascript)
    /**
     *
     * @param from starting index (included). If negative, it is counted from end
     * @return slice of array from given index to its end
     */
    Value slice(std::ptrdiff_t from) const {
        return slice(from, size());
    }

    ///Joins this array and another array
    /**
     * @param other other array
     * @return this+other (joined)
     */
    Value concat(const Array &other) const;

    ///Join several arrays into one
    /**
     * @param parts arrays to join
     * @return joined array
     */
    static Value concat(const std::initializer_list<Array> &parts);

    ///Converts two-dimensional array into single dimensional array
    /**
     * @return returns flatten array.
     *
     * @code
     * [1,2,3] -> [1,2,3]
     * [1,[10,20,30],2,3] -> [1,10,20,30,2,3]
     * [1,[2,[3,4],5],6] -> [1,2,[3,4],5,6]
     * @endcode
     *
     * @note map+flatten is good way to extend array during mapping.
     */
    Value flatten() const;


    ///Serialize Value to JSON
    /**
     * @param fn function which receives characters to be stored to the output
     * @param ot specifies output type
     *
     * @see OutputType
     */
    template<typename Fn>
    void serialize(Fn &&fn, OutputType ot = OutputType::ascii) const;


    ///Serializes Value to string
    /**
     * @param ot specifies output type
     * @return serialized value
     *
     * @see OutputType
     */
    std::string to_string(OutputType ot = OutputType::utf8) const;

    ///Serialize Value to an output stream
    /**
     * @param stream an output stream
     * @param ot specifies output type
     *
     * @see OutputType
     */
    void to_stream(std::ostream &stream, OutputType ot = OutputType::ascii);
    ///Parse Value from input
    /**
     * @param fn function which returns characters. For EOF, it could return -1
     * @return parsed Value
     * @exception ParseError parse error
     */
    template<typename Fn>
    static Value parse(Fn &&fn);

    ///Parse Value from string
    /**
     * @param str JSON string
     * @return parsed Value
     * @exception ParseError parse error
     */
    static Value from_string(const std::string_view &str);

    ///Parse Value from an input stream
    /**
     * @param str JSON in stream
     * @return parsed Value
     * @exception ParseError parse error
     */
    static Value from_stream(std::istream &stream);


protected:
    PNode _ptr;

};

///Iterator
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

///Helper class to construct objects
class Object: public Value {
public:
    ///construct empty object
    Object():Value(Node::shared_empty_object()) {}

    ///construct from initializer list definition
    Object(const std::initializer_list<KeyValue > &obj)
        :Value(Node::new_object(obj.size(), [obj](ContBuilder &bld) {
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
    Object(const Container &c, Fn &&fn):
        Value(Node::new_object(std::distance(std::begin(c), std::end(c)),
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
    Object(std::size_t count, Fn &&fn)
        :Value(Node::new_object(count, [&](ContBuilder &bld){
            for (std::size_t i = 0; i < count; i++) {
                Value v = fn(i);
                if (!v.defined()) break;
                bld.push_back(v.get_handle());
            }
            std::sort(bld.begin(),bld.end(),Node::KeyOrder());
    })) {}


    static Object from_value(const Value &v) {
        return Object(v.get_handle());
    }

protected:
    explicit Object(const PNode &ptr):Value(ptr) {}

};

///Helper class to construct arrays
class Array: public Value {
public:
    ///construct empty array
    Array():Value(Node::shared_empty_array()) {}
    ///construct array from initialized list definition
    Array(const std::initializer_list<Value> &array)
        :Value(Node::new_array(array.size(), [array](ContBuilder &bld) {
        for (const Value &v : array) {
            bld.push_back(v.get_handle());
        }
     })){}


    ///construct array from container of values
    template<typename Container,
        typename = decltype(std::begin(std::declval<const Container &>()) == std::end(std::declval<const Container &>())),
        typename = decltype(Value(*std::begin(std::declval<const Container &>())))>
    explicit Array(const Container &c)
        :Value(Node::new_array(std::distance(std::begin(c), std::end(c)),
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
    Array(const Container &c, Fn &&fn)
        :Value(Node::new_array(std::distance(std::begin(c), std::end(c)),
                             [&c, &fn](ContBuilder &b) {
           for(const auto &x : c) {
               Value v = fn(x);
               if (v.defined()) {
                   b.push_back(Value(fn(x)).get_handle());
               }
           }
    })){}

    template<typename Fn>
    Array(std::size_t count, Fn &&fn)
        :Value(Node::new_array(count, [&](ContBuilder &bld){
            for (std::size_t i = 0; i < count; i++) {
                Value v = fn(i);
                if (!v.defined()) break;
                bld.push_back(v.get_handle());
            }
    })) {}

    static Array from_value(const Value &v) {
        return Array(v.get_handle());
    }

protected:
    explicit Array(const PNode &ptr):Value(ptr) {}
};



///Helps to work with binary values.
/**
 * Binary strings are stored as BASE64 encoded string.
 *
 * To obtain this object from JSON, call Value::get_binary();
 *
 * You can also use this object to construct binary value from the string
 */
class Binary: public Value {
public:
    ///Construct binary value.
    /**
     * @param binary_data binary string (unlimited binary data)
     *
     * Because Binary inherits Value, the instance can be used anywhere the Value is expected
     */
    Binary(const std::string_view &binary_data)
        :Value(binary_data, StringType::binary) {}

    ///Constructs binary from a Value
    /**
     * @param v value object. If the value is utf-8 string, it is expected, that binary value is
     * encoded as BASE64 data. So decoding is performed.
     *
     * @return Binary object
     *
     * @note You can also use Value::get_binary();
     */
    static Binary from_value(const Value &v) {
        if (v.is_string()) {
            if (v.is_binary_string()) {
                return Binary(v);
            } else {
                std::string s;
                base64decode(v.get_string(), [&](char c){s.push_back(c);});
                return Binary(s);
            }
        } else {
            throw std::bad_cast();
        }
    }
protected:
    explicit Binary(const Value &v):Value(v) {}
};


inline Binary Value::get_binary() const {
    return Binary::from_value(*this);
}

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

inline void kjson::Value::merge(const Object &obj, Merge merge, const Value &unset_item) {
    Value src(is_object()?Value(nullptr,*this):Value(Object()));
    Value diff(obj);

    auto get_merged = [&](const Value &src, const Value &diff) {
        if (diff.is_object() && merge == Merge::recursive) {
            Value x(src);
            x.merge(Object::from_value(diff), merge, unset_item);
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
                    bld.push_back(get_merged(Object(), *iter2).get_handle());
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
                bld.push_back(get_merged(Object(), *iter2).get_handle());
            }
            ++iter2;
        }
    });
}

inline void kjson::Value::append(const Array &arr) {
    _ptr = concat(arr).get_handle();
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

inline kjson::Value kjson::Value::concat(const Array &other) const {
    std::size_t sz = size() + other.size();
    return Value( Node::new_array(sz, [&](ContBuilder &bld){
        for (const Value &x : *this) {
            bld.push_back(x.get_handle());
        }
        for (const Value &x: other) {
            bld.push_back(x.get_handle());
        }
    }));
}

inline kjson::Value kjson::Value::concat(const std::initializer_list<Array> &parts) {
    std::size_t sz = std::accumulate(parts.begin(), parts.end(), std::size_t(0),
                                     [](std::size_t a, const Value &b) {
        return b.is_container()?a+b.size():a+1;
    });
    return Value(Node::new_array(sz, [&](ContBuilder &bld) {
        for (const Value &x: parts) {
            if (x.is_container()) {
                for (const Value &y: x) {
                    bld.push_back(y.get_handle());
                }
            } else {
                bld.push_back(x.get_handle());
            }
        }
    }));
}


inline kjson::Value kjson::Value::slice(std::ptrdiff_t from, std::ptrdiff_t to) const {
    SliceInfo slc = _ptr->get_slice_info();
    auto beg = from<0?std::max<std::ptrdiff_t>(0,slc.size+from):std::min<std::ptrdiff_t >(from, slc.size);
    auto end = to<0?std::max<std::ptrdiff_t >(0,slc.size+to):std::min<std::ptrdiff_t >(to, slc.size);
    if (beg >= end) return Value(Array());
    else {
        auto sz = end - beg;
        auto ofs = beg + slc.offset;
        return Value(Node::new_slice((SliceInfo{slc.owner, static_cast<std::size_t>(ofs), static_cast<std::size_t>(sz)})));
    }
}

inline kjson::Array kjson::Value::get_array() const {
    return Array::from_value(*this);
}

inline kjson::Object kjson::Value::get_object() const {
    return Object::from_value(*this);
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

template<typename Fn>
inline kjson::Value kjson::Value::map(Fn &&fn) const {
    return Array(Array::from_value(*this), std::forward<Fn>(fn));
}

template<typename Fn>
inline kjson::Value kjson::Value::map_object(Fn &&fn) const {
    return Object(Object::from_value(*this), std::forward<Fn>(fn));

}

kjson::Value kjson::Value::splice(std::ptrdiff_t start, std::ptrdiff_t delete_count) {
    if (delete_count == 0) return *this;
    Value lead = slice(start);
    Value trail = slice(lead.size()+delete_count);
    return lead.concat(Array::from_value(trail));
}

kjson::Value kjson::Value::splice(std::ptrdiff_t start, std::ptrdiff_t delete_count, const Array &new_items) {
    if (delete_count == 0) return *this;
    Array lead = Array::from_value(slice(start));
    Array trail = Array::from_value(slice(lead.size()+delete_count));
    return Value::concat({Array::from_value(lead), new_items, Array::from_value(trail)});
}


inline kjson::Value::Value(const std::initializer_list<KeyValue > &obj)
    :Value(Object(obj)) {}

#endif /* KISSJSON_VALUE_H_ */
