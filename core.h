/*
 * core.h
 *
 *  Created on: 15. 7. 2022
 *      Author: ondra
 */

#ifndef KISSJSON_CORE_H_
#define KISSJSON_CORE_H_

#include "number2str.h"
#include "enums.h"
#include <atomic>
#include <string_view>
#include <algorithm>
#include <charconv>
#include "user_defined_core.h"


namespace kjson {

class Node;
class Value;

class PNode {
public:
    PNode(const Node *ptr);
    PNode(const PNode &other);
    PNode(PNode &&other);
    PNode &operator=(const PNode &other);
    PNode &operator=(PNode &&other);
    ~PNode();

    const Node *operator->() const;
    const Node &operator *() const {return *_ptr;}

    bool operator==(const PNode &other) const;
    bool operator!=(const PNode &other) const;

protected:
    const Node *_ptr;
};

template<typename T>
struct Range: std::pair<T,T> {

    using std::pair<T,T>::pair;

    T begin() const {return this->first;}
    T end() const {return this->second;}
};

struct NodeAllocator {
    void *(*alloc)(std::size_t);
    void (*dealloc)(void *ptr);

};

namespace _utils {

    template<typename T>
    inline auto gen_compare(T va, T vb) {
        return (va<vb?1:0) - (va>vb?1:0);
    }
    inline auto compare_integer_string(const std::string_view &a, const std::string_view &b) {
        //empty string - could never happen, however... empty<number, number>empty, empty==empty
        if (a.empty()) return b.empty()?0:-1;
        if (b.empty()) return 1;
        //first signature if presented
        if (a[0] == '-') {
            if (b[0] == '-') {  //strip sign and swap
                return -compare_integer_string(a.substr(1), b.substr(1));
            } else {
                return -1;      //- < anything
            }
        } else if (b[0] == '-') { // anything > -
            return 1;
        }
        //more digits means bigger number
        auto res = gen_compare(a.size(), b.size());
        if (res) return res;
        //equal digits, compare strings
        return a.compare(b);
    }

    template<typename T> inline T checkNodePtr(const PNode &nd);


}

class ContBuilder { // @suppress("Miss copy constructor or assignment operator")
public:
    ContBuilder(PNode *buffer, std::size_t sz)
        :_buffer(buffer),_sz(sz),_count(0) {}

    void push_back(PNode &&nd) {
        if (_count>=_sz) throw std::runtime_error("ArrayBuilder overflow");
        new(_buffer+_count) PNode(std::move(nd));
        ++_count;
    }

    void push_back(const PNode &nd) {
        if (_count>=_sz) throw std::runtime_error("ArrayBuilder overflow");
        new(_buffer+_count) PNode(nd);
        ++_count;
    }
    std::size_t count() const {return _count;}

    PNode *begin() const {return _buffer;}
    PNode *end() const {return _buffer+_count;}

protected:
    PNode *_buffer;
    std::size_t _sz;
    std::size_t _count;
};


struct SliceInfo {
    PNode owner;
    std::size_t offset;
    std::size_t size;
};

class Node {
protected:

    class Container {
    public:
        Container():_items(0), _count(0),_owner(nullptr) {}
        Container(PNode *buff):_items(buff), _count(0),_owner(nullptr) {}
        Container(PNode *items, std::size_t count): _items(items), _count(count),_owner(nullptr) {}
        Container(PNode owner, PNode *items, std::size_t count):_items(items), _count(count), _owner(owner) {}
        Container(const Container &other) = delete;
        Container(Container &&other):_items(other._items), _count(other._count),_owner(std::move(_owner)) {
            other._items = 0;;
            other._count = 0;
        }
        Container &operator=(const Container &other) = delete;
        ~Container() {
            if (_owner == nullptr) {
                for (std::size_t i = 0; i < _count; i++) _items[i].~PNode();
            }
        }
        const PNode &operator[](std::size_t x) const {return _items[x];}
        std::size_t size() const {return _count;}
        bool empty() const {return _count == 0;}
        void push_back(PNode &&nd) {new(_items+_count) PNode(std::move(nd)); ++_count;}

        PNode *begin() const {return _items;}
        PNode *end() const {return _items+_count;}

        PNode get_owner() const {return _owner;}

    protected:
        PNode *_items;
        std::size_t _count;
        PNode _owner;       ///<if this pointer is set, container is actually slice of different array
    };

    struct KeyValue {
        std::string_view key;
        PNode value;
    };


    struct String {
        std::string_view text;
        StringType _type;
    };

    template<typename T>
    struct NodeReserveRequest { // @suppress("Miss copy constructor or assignment operator")
        std::size_t count;
        T *result = nullptr;
    };

    enum InitNumberT{__init_number};
    enum InitTextT{__init_text};
    enum InitObjectT{__init_object};
    enum InitArrayT{__init_array};


    Node(bool b, bool static_alloc)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::boolean)
        ,_boolValue(b) {}

    Node(ValueType type, bool static_alloc)
        :_cntr(static_alloc?1:0)
        ,_type(type) {

        switch (type) {
        default:
            break;
        case ValueType::boolean: _boolValue = false;
            break;
        case ValueType::string: new(&_str) String();
            break;
        case ValueType::number: new(&_str) String{"0"};
            break;
        case ValueType::object:
        case ValueType::array: new(&_container) Container{nullptr,0};
            break;
        }
    }

    Node(InitTextT, const std::string_view &text, bool static_alloc)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::string)
        ,_str{text} {}

    Node(InitNumberT, const std::string_view &text, bool static_alloc)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::number)
        ,_str{text} {}


    void init_string(const std::string_view &text, StringType strtype, NodeReserveRequest<char> &res) {
        auto sz = std::min(text.length(), res.count);
        std::copy(text.data(), text.data()+sz, res.result);
        new(&_str) String{std::string_view(res.result, sz), strtype};
    }

    Node(InitTextT,  const std::string_view &text, StringType strtype, bool static_alloc, NodeReserveRequest<char> &res)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::string) {
            init_string(text, strtype, res);
    }
    Node(InitNumberT,  const std::string_view &text, bool static_alloc, NodeReserveRequest<char> &res)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::number) {
            init_string(text, StringType::utf8, res);
    }

    ///create slice of other array or object - owner target is not container, creates empty array
    Node(const SliceInfo &slice)
        :_cntr(0)
        ,_type(ValueType::array) {
        auto target = slice.owner;
        auto offset = std::min(target->size(),slice.offset);
        auto size = std::min(target->size() - slice.offset, slice.size);
        new (&_container) Container(target, target->_container.begin()+offset, size);
    }

    template<typename Fn, typename=decltype(std::declval<Fn>()(std::declval<ContBuilder &>()))>
    void init_container(Fn &&builder, NodeReserveRequest<PNode> &res) {
        ContBuilder bld(res.result, res.count);
        builder(bld);
        new(&_container) Container(res.result, bld.count());
    }

    template<typename Fn, typename=decltype(std::declval<Fn>()(std::declval<ContBuilder &>()))>
    Node(InitObjectT,  Fn &&builder, bool static_alloc, NodeReserveRequest<PNode> &res)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::object) {
            init_container(std::forward<Fn>(builder), res);
    }

    template<typename Fn, typename=decltype(std::declval<Fn>()(std::declval<ContBuilder &>()))>
    Node(InitArrayT,  Fn &&fn, bool static_alloc, NodeReserveRequest<PNode> &res)
        :_cntr(static_alloc?1:0)
        ,_type(ValueType::array) {
            init_container(std::forward<Fn>(fn), res);
    }

    Node(const std::string_view &key, PNode &&nd,  NodeReserveRequest<char> &res)
        :_cntr(0)
        ,_type(ValueType::key) {

        auto sz = std::min(key.length(), res.count);
        std::copy(key.data(), key.data()+sz, res.result);
        new (&_keyvalue) KeyValue{std::string_view(res.result, sz), std::move(nd)};
    }


    Node(const UserDefinedValueTypeDesc &user_type, void *args, NodeReserveRequest<char> &res)
        :_cntr(0)
        ,_type(ValueType::user_defined)
        ,_userdef{user_type, res.result, res.count} {

            if (_userdef.type_desc.init)
                _userdef.type_desc.init(_userdef, args);
        }


    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;


    template<typename T>
    void *operator new(std::size_t sz, NodeReserveRequest<T> &req) {
        auto totalsz = sz+sizeof(T)*req.count;
        void *p = getAllocator().alloc(totalsz);
        req.result = reinterpret_cast<T *>(reinterpret_cast<char *>(p)+sz);
        return p;
    }

    template<typename T>
    void operator delete(void *ptr, NodeReserveRequest<T> &req) {
        getAllocator().dealloc(ptr);
    }


public:
    ~Node() {
        switch( _type) {
            case ValueType::number:
            case ValueType::string: _str.~String();
            break;
            case ValueType::key:_keyvalue.~KeyValue();
            break;
            case ValueType::object:
            case ValueType::array:
                _container.~Container();
                break;
            case ValueType::user_defined:
                if (_userdef.type_desc.deinit) {
                    _userdef.type_desc.deinit(_userdef);
                }
                _userdef.~UserDefinedValue();
                break;
            default:
                break;
            }
    }

    void operator delete(void *ptr, std::size_t sz) {
        //destructor should handle deletion of extra data
        getAllocator().dealloc(ptr);
    }

    void *operator new(std::size_t sz) {
        return getAllocator().alloc(sz);
    }


    static PNode shared_undefined() {
        static Node nd(ValueType::undefined, true);
        return PNode(&nd);
    }

    static PNode shared_null() {
        static Node nd(ValueType::null, true);
        return PNode(&nd);
    }

    static PNode shared_boolean(bool b) {
        static Node btrue(true, true);
        static Node bfalse(false, true);
        return PNode(b?&btrue:&bfalse);
    }

    static PNode shared_empty_string() {
        static Node nd(ValueType::string, true);
        return PNode(&nd);
    }

    static PNode shared_zero() {
        static Node nd(ValueType::number, true);
        return PNode(&nd);
    }

    static PNode shared_empty_array() {
        static Node nd(ValueType::array, true);
        return PNode(&nd);
    }

    static PNode shared_empty_object() {
        static Node nd(ValueType::object, true);
        return PNode(&nd);
    }

    static NodeAllocator &getAllocator() {
        static NodeAllocator alloc = {
                [](std::size_t sz) {return ::operator new(sz);},
                [](void *ptr) {::operator delete(ptr);}
        };
        return alloc;
    }

    static PNode new_string(const std::string_view &txt, StringType strtype) {
        if (txt.empty()) return shared_empty_string();
        NodeReserveRequest<char> req{txt.length()};
        return PNode(new(req) Node(__init_text, txt, strtype, false, req));
    }

    static PNode new_number(const std::string_view &txt) {
        if (txt.empty()) return shared_zero();
        if (std::isspace(txt[0])) return new_number(txt.substr(1));\
        if (txt == "0") return shared_zero();
        NodeReserveRequest<char> req{txt.length()};
        return PNode(new(req) Node(__init_number, txt, false, req));
    }

    static PNode new_number(unsigned int v) {return v?new_number(unsigned_to_string<10>(v)):shared_zero();}
    static PNode new_number(int v) {return v?new_number(signed_to_string<10>(v)):shared_zero();}
    static PNode new_number(unsigned long v) {return v?new_number(unsigned_to_string<10>(v)):shared_zero();}
    static PNode new_number(long v) {return v?new_number(signed_to_string<10>(v)):shared_zero();}
    static PNode new_number(unsigned long long v) {return v?new_number(unsigned_to_string<10>(v)):shared_zero();}
    static PNode new_number(long long v) {return v?new_number(unsigned_to_string<10>(v)):shared_zero();}
    static PNode new_number(float v) {return v?new_number(float_to_string(v,5)):shared_zero();}
    static PNode new_number(double v) {return v?new_number(float_to_string(v,14)):shared_zero();}

    template<typename Fn, typename=decltype(std::declval<Fn>()(std::declval<ContBuilder &>()))>
    static PNode new_array(std::size_t sz, Fn &&builder) {
        if (sz == 0) return shared_empty_array();
        NodeReserveRequest<PNode> req{sz};
        return PNode(new(req) Node(__init_array,std::forward<Fn>(builder),false,req));
    }

    template<typename Fn, typename=decltype(std::declval<Fn>()(std::declval<ContBuilder &>()))>
    static PNode new_object(std::size_t sz, Fn &&builder) {
        if (sz == 0) return shared_empty_object();
        NodeReserveRequest<PNode> req{sz};
        return PNode(new(req) Node(__init_object,std::forward<Fn>(builder),false,req));
    }

    static PNode new_slice(const SliceInfo &slc) {
        return PNode(new Node(slc));
    }


    static PNode new_user_value(const UserDefinedValueTypeDesc &type,  void *v);


    PNode set_key(const std::string_view &key) const {

        static auto new_key = [] (const std::string_view &key,  PNode &&target) -> PNode {
            NodeReserveRequest<char> req{key.size()};
            return PNode(new(req) Node(key, std::move(target), req));
        };

        if (_type == ValueType::key) {
            if (_keyvalue.key == key) return this;
            return key.empty()?PNode(this):new_key(key, unset_key());
        } else {
            return key.empty()?PNode(this):new_key(key, this);
        }
    }

    PNode unset_key() const {
        return PNode (_type == ValueType::key?_keyvalue.value:this);
    }

    std::string_view get_key() const {
        if (_type == ValueType::key) return _keyvalue.key;
        else return std::string_view();
    }

    struct KeyOrder {
        bool operator()(const PNode &a, const PNode &b) const {
            return a->get_key() < b->get_key();
        }
        bool operator()(const PNode &a, const std::string_view &b) const {
            return a->get_key() < b;
        }
        bool operator()(const std::string_view &a, const PNode &b) const {
            return a < b->get_key();
        }
    };
    static bool key_ordering_equal(const PNode &a, const PNode &b) {
        return a->get_key() == b->get_key();
    }

    std::string_view get_string() const {
        switch (_type) {
        case ValueType::array: if (_container.empty()) return "[]"; else return "[...]";
        case ValueType::object: if (_container.empty()) return "{}"; else return "{...}";
        case ValueType::key: return _keyvalue.value->get_string();
        case ValueType::number:
        case ValueType::string: return _str.text;
        case ValueType::boolean: return _boolValue?"true":"false";
        case ValueType::user_defined: return _userdef.type_desc.get_string?_userdef.type_desc.get_string(_userdef):_userdef.type_desc.get_type_name();
        default:
        case ValueType::null:
        case ValueType::undefined: return "";
        }
    }

    bool get_boolean() const {
          switch (_type) {
          case ValueType::array:
          case ValueType::object:return !_container.empty();
          case ValueType::key: return _keyvalue.value->get_boolean();
          case ValueType::number: return  _str.text == "0";
          case ValueType::string: return ! _str.text.empty();
          case ValueType::boolean: return _boolValue;
          default:
          case ValueType::null:
          case ValueType::undefined: return false;
          }
    }


    unsigned int get_unsigned_int() const {return string_to_unsigned<unsigned int>(get_string());}
    unsigned long get_unsigned_long() const {return string_to_unsigned<unsigned long>(get_string());}
    unsigned long long get_unsigned_long_long() const {return string_to_unsigned<unsigned long long>(get_string());}
    int get_int() const {return string_to_signed<int>(get_string());}
    long get_long() const {return string_to_signed<long>(get_string());}
    long long get_long_long() const {return string_to_signed<long long>(get_string());}
    float get_float() const {return static_cast<float>(string_to_float(get_string()));}
    double get_double() const {return string_to_float(get_string());}
    StringType get_string_type() const {
        switch(_type) {
        case ValueType::string:
            return _str._type;
        case ValueType::key:
            return _keyvalue.value->get_string_type();
        default:
            return StringType::utf8;
        };
    }

    const UserDefinedValue *get_user_defined_content() const {
        switch(_type) {
        case ValueType::user_defined:
            return &_userdef;
        case ValueType::key:
            return _keyvalue.value->get_user_defined_content();
        default:
            return nullptr;
        };
    }

    SliceInfo get_slice_info() const {
        switch (_type) {
        case ValueType::array: {
            PNode owner = _container.get_owner();
            if (owner != nullptr) {
                return {owner, static_cast<std::size_t>(_container.begin() - owner->_container.begin()), _container.size()};
            } else {
                return {this, 0, _container.size()};
            }
        }
        case ValueType::key: {
            return _keyvalue.value->get_slice_info();
        }
        default:
            return {this, 0, 0};
        }
    }


    PNode get(const std::string_view &key) const {
        switch(_type) {
        case ValueType::array: {
                auto iter = std::find_if(_container.begin(), _container.end(), [&](const PNode &nd) {
                    return nd->get_key() == key;
                });
                return iter == _container.end()?shared_undefined():*iter;
            } break;
        case ValueType::object: {
                auto iter = std::lower_bound(_container.begin(), _container.end(), key, KeyOrder());
                return iter == _container.end() || (*iter)->get_key() != key?shared_undefined():*iter;
            } break;
        case ValueType::key:
            return _keyvalue.value->get(key);
        case ValueType::user_defined: {
            PNode nd = _userdef.type_desc.find_by_key?_userdef.type_desc.find_by_key(_userdef,key):PNode(nullptr);
            return nd == nullptr?shared_undefined():nd;
        }

        default:
            return shared_undefined();
        }
    }

    PNode get(std::size_t index) const {
        switch(_type) {
        case ValueType::array:
        case ValueType::object:
            return index>=_container.size()?shared_undefined():_container[index];
        case ValueType::key:
            return _keyvalue.value->get(index);
        case ValueType::user_defined: {
            PNode nd = _userdef.type_desc.find_by_index?_userdef.type_desc.find_by_index(_userdef, index):PNode(nullptr);
            return nd == nullptr?shared_undefined():nd;
        }
        default:
            return shared_undefined();
        }
    }

    std::size_t size() const {
        switch(_type) {
        case ValueType::array:
        case ValueType::object:
            return _container.size();
        case ValueType::key:
            return _keyvalue.value->size();
        case ValueType::user_defined:
            return _userdef.type_desc.get_conatiner_size?_userdef.type_desc.get_conatiner_size(_userdef):0;
        default:
            return 0;
        }
    }

    bool empty() const {return size() == 0;}

    ValueType get_type() const {return _type == ValueType::key?_keyvalue.value->get_type():_type;}

    NumberType get_number_type() const {
        if (get_type() != ValueType::number) return NumberType::not_number;
        auto s = get_string();
        if (s.empty()) return NumberType::not_number;
        auto n = s.find_first_of(".eE");
        if (n != s.npos) return NumberType::real_number;
        return (std::isdigit(s[0]))?NumberType::unsigned_number:NumberType::signed_number;
    }

    int compare(const Node &other) const {
        auto a = unset_key();
        auto b = other.unset_key();

        if (a == b) return 0;
        if (a->_type != b->_type) return static_cast<int>(a->_type) - static_cast<int>(b->_type);
        else switch (_type) {
            default:
            case ValueType::undefined:
            case ValueType::null: return 0;
            case ValueType::boolean: return (a->get_boolean()?1:0) - (b->get_boolean()?1:0);
            case ValueType::string: return a->get_string().compare(b->get_string());
            case ValueType::number: {
                auto nta = a->get_number_type();
                auto ntb = b->get_number_type();
                if (nta == NumberType::not_number || ntb == NumberType::not_number) {
                    return a->get_string().compare(b->get_string());
                } else if (nta == NumberType::real_number || ntb == NumberType::real_number) {
                    return _utils::gen_compare(a->get_double(), b->get_double());
                } else {
                    return _utils::compare_integer_string(a->get_string(), b->get_string());
                }
            }
            break;
            case ValueType::array: {
                std::size_t cc = std::min(a->_container.size(), b->_container.size());
                for (std::size_t i = 0; i < cc; ++i) {
                    auto res = a->_container[i]->compare(*(b->_container[i]));
                    if (res) return res;
                }
                return _utils::gen_compare(a->_container.size(), b->_container.size());
            }
            case ValueType::object: {
                std::size_t cc = std::min(a->_container.size(), b->_container.size());
                for (std::size_t i = 0; i < cc; ++i) {
                    auto res = a->_container[i]->get_key().compare(b->_container[i]->get_key());
                    if (res) return res;
                    res = a->_container[i]->compare(*(b->_container[i]));
                    if (res) return res;
                }
                return _utils::gen_compare(a->_container.size(), b->_container.size());
            }
        }
    }

    void add_ref() const {
        ++_cntr;
    }

    bool release_ref() const {
        return --_cntr == 0;
    }



protected:
    mutable std::atomic<unsigned long> _cntr;
    ValueType _type;
    union {
        bool _boolValue;
        String _str;
        Container _container;
        KeyValue _keyvalue;
        UserDefinedValue _userdef;
    };
};


inline PNode::PNode(const Node *ptr):_ptr(ptr) {
    if (_ptr) _ptr->add_ref();
}

inline PNode::PNode(const PNode &other):_ptr(other._ptr) {
    if (_ptr) _ptr->add_ref();
}

inline PNode::PNode(PNode &&other):_ptr(other._ptr) {
    other._ptr = nullptr;
}

inline PNode& PNode::operator =(const PNode &other) {
    if (this != &other) {
        if (other._ptr) other._ptr->add_ref();
        if (_ptr && _ptr->release_ref()) delete _ptr;
        _ptr = other._ptr;
    }
    return *this;
}

inline PNode& PNode::operator =(PNode &&other) {
    if (this != &other) {
        if (_ptr && _ptr->release_ref()) delete _ptr;
        _ptr = other._ptr;
        other._ptr = nullptr;
    }
    return *this;
}

inline PNode::~PNode() {
    if (_ptr && _ptr->release_ref()) delete _ptr;
}

inline const Node* PNode::operator ->() const {
    return _ptr;
}

inline bool PNode::operator ==(const PNode &other) const {
    return _ptr == other._ptr;
}

inline bool PNode::operator !=(const PNode &other) const {
    return _ptr != other._ptr;
}

inline PNode Node::new_user_value(const UserDefinedValueTypeDesc &type, void *args) {
    NodeReserveRequest<char> req;
    req.count = type.get_required_size?type.get_required_size(args):0;
    return PNode(new(req) Node(type, args, req));
}


}



#endif /* KISSJSON_CORE_H_ */
