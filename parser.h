/*
 * parser.h
 *
 *  Created on: 14. 7. 2022
 *      Author: ondra
 */

#ifndef KISSJSON_PARSER_H_
#define KISSJSON_PARSER_H_

#include "value.h"

#include <sstream>

namespace kjson {

class ParseError: public std::exception {
public:

    enum class Error {
        unexpected_character,
        unknown_keyword,
        invalid_string_escape_sequence,
        invalid_unicode_hex_character,
        invalid_unicode_surrogate_sequence,
        invalid_number,
        invalid_array_separator,
        invalid_object_separator,
        expected_colon,
        expected_begin_of_string,
        unexpected_end_of_file,
    };

    ParseError(Error err, std::size_t offset, std::size_t line, std::size_t col)
        :_err(err), _offset(offset), _line(line), _col(col) {}

    const char *what() const noexcept override {
        if (_whatmsg.empty()) {
            std::ostringstream s;
            s << "JSON Parse error: "<< error_code_to_string(_err)
                    << " at offset " << _offset
                    << " (line: " << _line
                    << " , column: " << _col
                    << ")";
            _whatmsg = s.str();
        }
        return _whatmsg.c_str();
    }

    static const char *error_code_to_string(Error err) {
        switch (err) {
            case Error::unexpected_character: return "Unexpected character";
            case Error::unknown_keyword: return "Unknown keyword (null, true, false)";
            case Error::invalid_string_escape_sequence: return "Invalid string escape sequence";
            case Error::invalid_unicode_hex_character: return "Invalid unicode hex character";
            case Error::invalid_unicode_surrogate_sequence: return "Invalid unicode surrogate sequence";
            case Error::invalid_number: return "Invalid number";
            case Error::invalid_array_separator: return "Invalid array separator - expected comma";
            case Error::invalid_object_separator: return "Invalid object separator - expected comma";
            case Error::expected_colon: return "Expected colon";
            case Error::expected_begin_of_string: return "Expected begin of string (quotes)";
            case Error::unexpected_end_of_file: return "Unexpected end of file";
            default: return "Unknown error";
        }
    }

protected:
    Error _err;
    std::size_t _offset, _line, _col;
    mutable std::string _whatmsg;

};

class Parser {
public:

    Parser() = default;

    Parser(const Parser &) = delete;
    Parser &operator=(const Parser &) = delete;

    ///Puts char to the parser
    /**
     * @param c next character
     * @retval true need more characters
     * @retval false this was last character, do not send more characters, result is ready
     */
    bool put_char(char c);

    ///Puts eof to the parser
    /**
     * Function throw exception if end of file is not expected, otherwise you
     * can get_result()
     */
    void put_eof();

    ///Retrieve result of parsing
    /**
     * @return parsing result - if available, otherwise undefined
     */
    Value get_result() const;


    ///Parse string returns parsed result
    /**
     * @param s complete JSON in string
     * @return parsed JSON
     */
    static Value parse_string(const std::string_view &s);


    ///Parse buffer, can be incomplette
    /**
     * @param s a string buffer, can contain incomplete part
     * @return returns pair, where first(bool) contains true, when more data are
     * needed, anf false, when parsing is done. The second variable(std::string_view)
     * contains unprocessed input
     *
     * To obtain result, call get_result();
     */
    std::pair<bool, std::string_view> parse_buffer(const std::string_view &s);

protected:

    Value _result;

    enum class State {
        ready,
        detect_type,
        detect_type_or_close_array,
        check_kw,
        parse_string,
        parse_string_escape,
        parse_string_escape_hex1,
        parse_string_escape_hex2,
        parse_string_escape_hex3,
        parse_string_escape_hex4,
        parse_string_escape_hex_finish,
        parse_other_surrogate,
        parse_key,
        parse_key_or_close_object,
        parse_number,
        parse_number_digit,
        parse_number_decimal,
        parse_number_decimal_digit,
        parse_number_exponent,
        parse_number_exponent_digit,
        parse_number_exponent_digit2,
        close_object,
        close_array,
        colon
    };

    using Error = ParseError::Error;

    State _state = State::detect_type;
    bool _string_is_key = false;
    std::vector<Value> _vstack;
    std::vector<std::pair<std::size_t, bool> >_items;
    std::string _strbuff;
    std::vector<std::size_t> _stritems;

    std::size_t _chcnt = 0;
    std::size_t _line = 0;
    std::size_t _begline = 0;

    void push_string();
    std::string_view top_string();
    void pop_string();

    void push_container(bool is_object);
    Value top_container();
    void pop_container();


    const char *_kw = 0;
    Value _kwval;
    int _unicode_chr = 0;
    int _trail_surrogate = 0;
    int _lead_surrogate = 0;

    void check_kw(const char *kw, const Value &x) {
        _kw = kw;
        _state = State::check_kw;
        _kwval = x;
    }

    void throw_parse_error(Error err);
    bool finish_value(Value x);
    void append_unicode(int chr);
    bool finish_container();
    bool next(char c);
    bool finish_number(char c);
    static bool is_trail_surogate(int chr);
    static bool is_lead_surrogate(int chr);
    static int calc_surrogate(int lead, int trail);

};


inline bool Parser::put_char(char c) {
    ++_chcnt;
    if (c == '\n') {
        ++_line;
        _begline = _chcnt;
    }
    return next(c);
}
inline bool Parser::next(char c) { // @suppress("No return")

    switch(_state) {
        default:
        case State::ready: return false;

        case State::detect_type:
            if (isspace(c)) return true;
            switch (c) {
            case 'n':
                check_kw("null",nullptr);
                return next(c);
            case 't':
                check_kw("true",true);
                return next(c);
            case 'f':
                check_kw("false",false);
                return next(c);
            case 'u':
                check_kw("undefined",Value());
                return next(c);
            case '[':
                push_container(false);
                _state = State::detect_type_or_close_array;
                return true;
            case '{':
                push_container(true);
                _state = State::parse_key_or_close_object;
                return true;
            case '"':
                _string_is_key = false;
                push_string();
                _state = State::parse_string;
                return true;
            case '+':
            case '-':
                push_string();
                _strbuff.push_back(c);
                _state = State::parse_number_digit;
                return true;
            default:
                if (isdigit(c)) {
                    push_string();
                    _state = State::parse_number;
                    return next(c);
                } else {
                    throw_parse_error(Error::unexpected_character);
                }
                break;
            }
            break;

       case State::detect_type_or_close_array:
           if (std::isspace(c)) {
               return true;
           } else if (c == ']') {
               return finish_container();
           } else {
               _state = State::detect_type;
               return next(c);
           }

       case State::parse_key:
           if (std::isspace(c)) {
               return true;
           }
           else if (c == '\"') {
               _string_is_key =true;
               _state = State::parse_string;
               push_string();
               return true;
           }
           else {
               throw_parse_error(Error::expected_begin_of_string);
           }
           break;

       case State::parse_key_or_close_object:
           if (std::isspace(c)) {
               return true;
           } else if (c == '}') {
               return finish_container();
           } else {
               _state = State::parse_key;
               return next(c);
           }

       case State::parse_string:
           if (c =='\\') {
               _state = State::parse_string_escape;
               return true;
           } else if (c == '"') {
               if (_string_is_key) {
                   _string_is_key = false;
                   _state = State::colon;
                   return true;
               } else {
                    Value x (top_string());
                    pop_string();
                    return finish_value(x);
               }
           } else {
               _strbuff.push_back(c);
               return true;
           }
           break;

       case State::check_kw:
           if (c == *_kw) {
               ++_kw;
               if (*_kw) return true;
               else return finish_value(_kwval);
           } else {
               throw_parse_error(Error::unknown_keyword);
               break;
           }

       case State::parse_string_escape:
           switch (c) {
            case '"':
            case '\\':
            case '/': _strbuff.push_back(c);break;
            case 'b': _strbuff.push_back('\b');break;
            case 'f': _strbuff.push_back('\f');break;
            case 'n': _strbuff.push_back('\n');break;
            case 'r': _strbuff.push_back('\r');break;
            case 't': _strbuff.push_back('\t');break;
            case 'u': _unicode_chr = 0;
                      _state = State::parse_string_escape_hex1;
                      return true;
            default: throw_parse_error(Error::invalid_string_escape_sequence);
           }
           _state = State::parse_string;
           return true;

       case State::parse_string_escape_hex1:
       case State::parse_string_escape_hex2:
       case State::parse_string_escape_hex3:
       case State::parse_string_escape_hex4:
           c = std::toupper(c);
           if (c >= '0' && c <= '9') _unicode_chr = (_unicode_chr<<4) | (c - '0');
           else if (c >= 'A' && c<= 'F') _unicode_chr = (_unicode_chr<<4) | (c - 'A' + 10);
           else throw_parse_error(Error::invalid_unicode_hex_character);
           _state = static_cast<State>(static_cast<int>(_state) + 1);
           return true;

       case State::parse_string_escape_hex_finish:
           if (is_lead_surrogate(_unicode_chr)) {
               if (_lead_surrogate) throw_parse_error(Error::invalid_unicode_surrogate_sequence);
               _lead_surrogate = _unicode_chr;
               if (_trail_surrogate) {
                   _unicode_chr = calc_surrogate(_lead_surrogate, _trail_surrogate);
                   _lead_surrogate = _trail_surrogate = 0;
               } else {
                    _state = State::parse_other_surrogate;
                    if (c != '\\') throw_parse_error(Error::invalid_unicode_surrogate_sequence);
                    return true;
               }
           } else if (is_trail_surogate(_unicode_chr)) {
               if (_trail_surrogate) throw_parse_error(Error::invalid_unicode_surrogate_sequence);
               _trail_surrogate = _unicode_chr;
               if (_lead_surrogate) {
                   _unicode_chr = calc_surrogate(_lead_surrogate, _trail_surrogate);
                   _lead_surrogate = _trail_surrogate = 0;
               } else {
                    _state = State::parse_other_surrogate;
                    if (c != '\\') throw_parse_error(Error::invalid_unicode_surrogate_sequence);
                    return true;
               }
           }
           if (_lead_surrogate || _trail_surrogate)
               throw_parse_error(Error::invalid_unicode_surrogate_sequence);
           _state = State::parse_string;
           append_unicode(_unicode_chr);
           return next(c);

       case State::parse_other_surrogate:
           if (c != 'u') throw_parse_error(Error::invalid_unicode_surrogate_sequence);
           _state = State::parse_string_escape_hex1;
           _unicode_chr = 0;
           return true;

       case State::parse_number:
           if (isdigit(c)) {
               _strbuff.push_back(c);
           }
           else if (c == '.') {
               _strbuff.push_back(c);
               _state = State::parse_number_decimal_digit;
           } else if (c == 'e' || c == 'E') {
               _strbuff.push_back(c);
               _state = State::parse_number_exponent;
           } else {
               return finish_number(c);
           }
           return true;

       case State::parse_number_digit:
           if (std::isdigit(c)) {
               _strbuff.push_back(c);
               _state = State::parse_number;
               return true;
           } else {
               throw_parse_error(Error::invalid_number);
               break;
           }

       case State::parse_number_decimal_digit:
           if (std::isdigit(c)) {
               _strbuff.push_back(c);
               _state = State::parse_number_decimal;
               return true;
           } else {
               throw_parse_error(Error::invalid_number);
               break;
           }

       case State::parse_number_exponent_digit:
           if (std::isdigit(c)) {
               _strbuff.push_back(c);
               _state = State::parse_number_exponent_digit2;
               return true;
           } else {
               throw_parse_error(Error::invalid_number);
               break;
           }


       case State::parse_number_decimal:
           if (isdigit(c)) {
               _strbuff.push_back(c);
           }
           else if (c == 'e' || c == 'E') {
               _strbuff.push_back(c);
               _state = State::parse_number_exponent;
           } else {
               return finish_number(c);
           }
           return true;

       case State::parse_number_exponent:
           if (c == '+' || c == '-') {
               _strbuff.push_back(c);
               _state = State::parse_number_exponent_digit;
               return true;
           }
           _state = State::parse_number_exponent_digit;
           return next(c);

       case State::parse_number_exponent_digit2:
           if (isdigit(c)) {
               _strbuff.push_back(c);
               return true;
           }
           else {
               return finish_number(c);
           }

       case State::close_array:
           if (std::isspace(c)) {
               return true;
           } else if (c == ',') {
               _state = State::detect_type;
               return true;
           } else if (c == ']') {
               return finish_container();
           } else {
               throw_parse_error(Error::invalid_array_separator);
               break;
           }

       case State::close_object:
           if (std::isspace(c)) {
               return true;
           } else if (c == ',') {
               _state = State::parse_key;
               return true;
           } else if (c == '}') {
               return finish_container();
           } else {
               throw_parse_error(Error::invalid_object_separator);
               break;
           }

       case State::colon: {
           if (isspace(c)) return true;
           if (c == ':') {
               _state = State::detect_type;
               return true;
           } else {
               throw_parse_error(Error::expected_colon);
           }
       }
       break;
    };
    return false;
}



inline bool Parser::finish_value(Value x) {
    if (_items.empty()) {
        _result = x;
        _state = State::ready;
        return false;
    }
    else if (_items.back().second) {
        _vstack.push_back(Value(top_string(), x));
        pop_string();
        _state = State::close_object;
        return true;
    } else {
        _vstack.push_back(x);
        _state = State::close_array;
        return true;
    }
}

inline Value Parser::get_result() const {
    return _result;
}

inline void Parser::append_unicode(int chr) {
    if (chr < 128) {
        _strbuff.push_back(static_cast<char>(chr));
    }
    else if (chr < 0x800) {
        _strbuff.push_back(static_cast<char>((chr>>6) | 0xC0));
        _strbuff.push_back(static_cast<char>((chr & 0x3F) | 0x80));
    }
    else if (chr < 0x10000) {
        _strbuff.push_back(static_cast<char>((chr>>12) | 0xE0));
        _strbuff.push_back(static_cast<char>(((chr>>6) & 0x3F) | 0x80));
        _strbuff.push_back(static_cast<char>((chr & 0x3F) | 0x80));
    }
    else {
        _strbuff.push_back(static_cast<char>((chr>>18) | 0xF0));
        _strbuff.push_back(static_cast<char>(((chr>>12) & 0x3F) | 0x80));
        _strbuff.push_back(static_cast<char>(((chr>>6) & 0x3F) | 0x80));
        _strbuff.push_back(static_cast<char>((chr & 0x3F) | 0x80));
    }
}

inline void Parser::push_string() {
    _stritems.push_back(_strbuff.size());
}

inline std::string_view Parser::top_string() {
    return std::string_view(_strbuff).substr(_stritems.back());
}

inline void Parser::pop_string() {
    _strbuff.resize(_stritems.back());
    _stritems.pop_back();
}

inline void Parser::push_container(bool is_object) {
    _items.push_back({_vstack.size(), is_object});
}

inline Value Parser::top_container() {
    Range<decltype(_vstack)::const_iterator> r(_vstack.begin()+_items.back().first, _vstack.end());
    if (_items.back().second) {
        return Value::object(r, [](const Value &v){return v;});
    } else {
        return Value::array(r);
    }
}

inline void Parser::pop_container() {
    _vstack.resize(_items.back().first);
    _items.pop_back();

}

inline void Parser::throw_parse_error(Error err) {
    throw ParseError(err, _chcnt, _line+1, _chcnt-_begline-+1);
}

inline bool Parser::finish_container() {
    Value cont = top_container();
    pop_container();
    bool res = finish_value(cont);
    return res;
}

inline bool Parser::finish_number(char c) {
    Value x(Node::new_number(top_string()));
    pop_string();
    finish_value(x);
    return next(c);
}

inline bool Parser::is_trail_surogate(int chr) {
   return chr >= 0xDC00 && chr <= 0xDFFF;
}

inline bool Parser::is_lead_surrogate(int chr) {
   return chr >= 0xD800 && chr <= 0xDBFF;
}

inline void Parser::put_eof() {
    switch(_state) {
    case State::parse_number:
    case State::parse_number_decimal:
    case State::parse_number_exponent_digit2: {
            Value x(Node::new_number(top_string()));
            pop_string();
            finish_value(x);
        }
        break;
    default:
        throw_parse_error(Error::unexpected_end_of_file);

    }
}

inline Value Parser::parse_string(const std::string_view &s)  {
    Parser p;
    for (auto c: s) {
        if (!p.put_char(c)) return p.get_result();
    }
    p.put_eof();
    return p.get_result();
}

inline std::pair<bool, std::string_view> Parser::parse_buffer(const std::string_view &s) {
    if (_state == State::ready) return {false, s};
    int ofs = 0;
    for (auto c: s) {
        ++ofs;
        if (!put_char(c)) return {false, s.substr(ofs)};
    }
    return {true, std::string_view()};
}

inline int Parser::calc_surrogate(int lead, int trail) {
   static constexpr int SURROGATE_OFFSET =  0x10000 - (0xD800 << 10) - 0xDC00;
   return (lead << 10) + trail + SURROGATE_OFFSET;
}

template<typename Fn>
Value Value::parse(Fn &&fn) {
    Parser p;
    int i = fn();
    while (i != -1) {
        if (!p.put_char(static_cast<char>(i))) {
            return p.get_result();
        }
        i = fn();
    }
    p.put_eof();
    return p.get_result();
}

Value Value::from_string(const std::string_view &str) {
    return Parser::parse_string(str);
}


}



#endif /* KISSJSON_PARSER_H_ */

