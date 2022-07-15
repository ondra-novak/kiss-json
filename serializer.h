/*
 * serializer.h
 *
 *  Created on: 15. 7. 2022
 *      Author: ondra
 */

#ifndef KISSJSON_SERIALIZER_H_
#define KISSJSON_SERIALIZER_H_

#include <array>
#include "value.h"

namespace kjson {


class Serializer {
public:
    Serializer(const Value &v, OutputType ot = OutputType::utf8)
        :_v(v),_ot(ot) {}

    Serializer(const Serializer &v) = delete;
    Serializer &operator=(const Serializer &v) = delete;

    int get_next();

    template<std::size_t n>
    std::size_t read(std::array<char, n> &buffer);


protected:

    enum class State {
        finish,
        analyze,
        direct_string,
        string,
        string_escape,
        string_hex_1,
        string_hex_2,
        string_hex_3,
        string_hex_4,
        string_hex_5,
        string_hex_6,
        begin_key,
    };


    Value _v;
    OutputType _ot;
    State _state = State::analyze;
    std::vector<Value::iterator> _path;
    std::string_view _text;
    std::string_view::iterator _text_iter = nullptr;
    int _h = 0, _sgt = 0;
    bool _key_finished = false;

    int read_unicode(int c);
    static int hex_char(int c);
    int safe_next_char();
};

inline int Serializer::get_next() {
    switch (_state) {
        case State::analyze: switch (_v.get_type()) {
            default:
            case ValueType::undefined:
                _text = "\"undefined\"";
                _text_iter = _text.begin();
                _state = State::direct_string;
                return get_next();
            case ValueType::boolean:
                _text = _v.get_bool()?"true":"false";
                _text_iter = _text.begin();
                _state = State::direct_string;
                return get_next();
            case ValueType::null:
                _text = "null";
                _text_iter = _text.begin();
                _state = State::direct_string;
                return get_next();
            case ValueType::string:
                _text = _v.get_string();
                _text_iter = _text.begin();
                _state = State::string;
                return '"';
            case ValueType::number:
                _text = _v.get_string();
                _text_iter = _text.begin();
                _state = State::direct_string;
                return get_next();
            case ValueType::object:
                if (_v.empty()) {
                    _text = "}";
                    _text_iter = _text.begin();
                    _state = State::direct_string;
                } else {
                    _path.push_back(_v.begin());
                    _v = *_path.back();
                    _state = State::begin_key;
                }
                return '{';

            case ValueType::array:
                if (_v.empty()) {
                    _text = "]";
                    _text_iter = _text.begin();
                    _state = State::direct_string;
                } else {
                    _path.push_back(_v.begin());
                    _v = *_path.back();
                    _state = State::analyze;
                }
                return '[';
        }
        return -1;

        case State::direct_string:
            if (_text_iter == _text.end()) {
                _state = State::finish;
                return get_next();
            } else {
                int c = static_cast<unsigned char>(*_text_iter);
                ++_text_iter;
                return c;
            }

        case State::string:
            if (_text_iter == _text.end()) {
                _state = State::finish;
                return '"';
            } else {
                int c = static_cast<unsigned char>(*_text_iter);
                if (c < 32 || (c > 127 && _ot == OutputType::ascii) || c == '"' || c == '\\' || c == '/') {
                    _state = State::string_escape;
                    return '\\';
                } else {
                    ++_text_iter;
                    return c;
                }
            }

        case State::string_escape: {
                int c = static_cast<unsigned char>(*_text_iter);
                ++_text_iter;
                _state = State::string;
                switch (c) {
                    case '\\':
                    case '/':
                    case '\"': return c;
                    case '\b': return 'b';
                    case '\f': return 'f';
                    case '\n': return 'n';
                    case '\r': return 'r';
                    case '\t': return 't';
                    default:
                         _h = read_unicode(c);
                         if (_h > 0xFFFF) {
                             constexpr int LEAD_OFFSET = 0xD800 - (0x10000 >> 10);
                             _sgt = 0xDC00 + (_h & 0x3FF);
                             _h = LEAD_OFFSET + (_h >> 10);
                         } else {
                             _sgt = 0;
                         }
                         _state = State::string_hex_1;
                         return 'u';
                }
        }

        case State::string_hex_1:
            _state = State::string_hex_2;
            return hex_char(_h >> 12);
        case State::string_hex_2:
            _state = State::string_hex_3;
            return hex_char(_h >> 8);
        case State::string_hex_3:
            _state = State::string_hex_4;
            return hex_char(_h >> 4);
        case State::string_hex_4:
            if (_sgt) {
                _state = State::string_hex_5;
            } else {
                _state = State::string;
            }
            return hex_char(_h);
        case State::string_hex_5:
            _state = State::string_hex_6;
            return '\\';
        case State::string_hex_6:
            _h = _sgt;
            _sgt = 0;
            _state = State::string_hex_1;
            return 'u';

        case State::begin_key:
            _key_finished = true;
            _text = _v.get_key();
            _text_iter = _text.begin();
            _state = State::string;
            return '"';

        default:
        case State::finish: {
            if (_key_finished) {
                _key_finished = false;
                _state = State::analyze;
                return ':';
            }
            if (_path.empty()) return -1;
            auto &iter = _path.back();
            Value cont = iter.container();
            ++iter;
            if (iter == cont.end()) {
                _path.pop_back();
                if (cont.is_object()) {
                    return '}';
                } else {
                    return ']';
                }
            } else {
                _v = *iter;
                if (cont.is_object()) {
                    _state = State::begin_key;
                } else {
                    _state = State::analyze;
                }
                return ',';
            }
        }

    }
}

inline int Serializer::safe_next_char() {
    if (_text_iter == _text.end()) return 0;
    else return static_cast<unsigned char>(*_text_iter++);
}

inline int Serializer::read_unicode(int c) {
    int out = 0xFFFD;
    if ((c & 0xE0) == 0xC0)  {
        out = c & 0x1F;
        out = (out << 6) | (safe_next_char() & 0x3F);
    } else if ((c & 0xF0) == 0xE0)  {
        out = c & 0xF;
        out = (out << 6) | (safe_next_char() & 0x3F);
        out = (out << 6) | (safe_next_char() & 0x3F);
    } else if ((c & 0xF8) == 0xF0)  {
        out = c & 0x7;
        out = (out << 6) | (safe_next_char() & 0x3F);
        out = (out << 6) | (safe_next_char() & 0x3F);
        out = (out << 6) | (safe_next_char() & 0x3F);
    }
    return out;
}

int Serializer::hex_char(int c) {
    constexpr const char *h = "0123456789ABCDEF";
    return h[c & 0xF];
}

template<typename Fn>
inline void Value::serialize(Fn &&fn, OutputType ot) const {
    Serializer sr(*this, ot);
    int i = sr.get_next();
    while (i!=-1) {
        fn(static_cast<char>(i));
        i = sr.get_next();
    }
}

template<std::size_t n>
inline std::size_t Serializer::read(std::array<char, n> &buffer) {
    std::size_t idx = 0;
    int i = get_next();
    while (i!=-1) {
        buffer[idx] = static_cast<char>(i);
        ++idx;
        if (idx == n) break;
        i = get_next();
    }
    return idx;
}


};


#endif /* SERIALIZER_H_ */
