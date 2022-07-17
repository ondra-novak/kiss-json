/** @file Some function to fast convert numbers to strings (not using locales) */


/*
 * Why we using own conversion functions.
 *
 * State of C++ at the year 2022 - STL still doesn't support reasonable functions to
 * do this work. We need to convert numbers to strings and strings to numbers without
 * locales and reasonable fast. So turning locales off and use std::iostreams is internally
 * too complicated and not efficient
 *
 * At this time, functions from_chars and to_chars are not complete. To avoid dependency
 * hell, we won't use boost. Instead small subset of functions was written to
 * do conversion. The goal was not make the most efficient code. Just a simple and
 * straightforward.
 *
 * main functions are
 * - unsigned_to_string
 * - signed_to_string
 * - float_to_string
 * - string_to_unsigned
 * - string_to_signed
 * - string_to_float
 *
 */

#ifndef _KISSJSON_ONDRA_SHARED_TOSTRING_H_39289204239042_
#define _KISSJSON_ONDRA_SHARED_TOSTRING_H_39289204239042_

#include <cmath>
#include <cstdint>
#include <locale>
#include <string_view>

namespace kjson {

namespace _details {


template<typename Number, typename Fn>
void unsignedToString(const Number &n, Fn &&fn, int base=10, int leftZeroes=1) {

     if (n == 0 && leftZeroes<1) return;
     unsignedToString(n/base, fn, base, leftZeroes-1);
     unsigned int remainder = (unsigned int)(n % base);
     if (remainder < 10) fn(remainder+'0');
     else if (remainder < 36) fn(remainder+'A'-10);
     else fn(remainder+'a'-36);
}

template<typename Number, typename Fn>
void signedToString(const Number &n, Fn &&fn, int base=10, int leftZeroes=1) {

     if (n < 0) {
          fn('-');
          unsignedToString(-n,fn,base,leftZeroes);
     } else {
          unsignedToString(n,fn,base,leftZeroes);
     }
}



template<std::size_t bsize> struct FracMultTable;

template<> struct FracMultTable<2> {
    static constexpr int maxDecimals=4;
    static constexpr std::uint16_t bell = 65530;
    static constexpr std::uint16_t get(int index) {
        switch (index) {
        case 0: return 1U;
        case 1: return 10U;
        case 2: return 100U;
        case 3: return 1000U;
        case 4: return 10000U;
        default: return 0;
        }
    }
};

template<> struct FracMultTable<4> {
    static constexpr int maxDecimals=9;
    static constexpr std::uint32_t bell = 429496729L;
    static constexpr std::uint32_t get(int index) {
        switch (index) {
        case 0: return 1UL;
        case 1: return 10UL;
        case 2: return 100UL;
        case 3: return 1000UL;
        case 4: return 10000UL;
        case 5: return 100000UL;
        case 6: return 1000000UL;
        case 7: return 10000000UL;
        case 8: return 100000000UL;
        case 9: return 1000000000UL;
        default: return 0;
        }
    }
};
template<> struct FracMultTable<8> {
    static constexpr int maxDecimals=19;
    static constexpr std::uint64_t bell = 1844674407370955161LL;
    static constexpr std::uint64_t get(int index) {
        switch (index) {
        case  0: return 1ULL;
        case  1: return 10ULL;
        case  2: return 100ULL;
        case  3: return 1000ULL;
        case  4: return 10000ULL;
        case  5: return 100000ULL;
        case  6: return 1000000ULL;
        case  7: return 10000000ULL;
        case  8: return 100000000ULL;
        case  9: return 1000000000ULL;
        case 10: return 10000000000ULL;
        case 11: return 100000000000ULL;
        case 12: return 1000000000000ULL;
        case 13: return 10000000000000ULL;
        case 14: return 100000000000000ULL;
        case 15: return 1000000000000000ULL;
        case 16: return 10000000000000000ULL;
        case 17: return 100000000000000000ULL;
        case 18: return 1000000000000000000ULL;
        case 19: return 10000000000000000000ULL;
        default: return 0;
        }
    }
};



template<typename Number, typename Fn>
void floatToString(Number value, Fn &&fn, int maxPrecisionDigits=14) {
    using FMTable = FracMultTable<sizeof(std::size_t)>;

    const char *inf = "9e9999";

    bool sign = value < 0;
    int precisz = std::min<int>(maxPrecisionDigits, FMTable::maxDecimals);

    value = std::abs(value);
    //calculate exponent of value
    //123897 -> 5 (1.23897e5)
    //0.001248 -> 3 (1.248e-3)
    double fexp = std::floor(std::log10(value));

    if (!std::isfinite(fexp)) {
       if (fexp < 0) {
            fn('0');
       } else {
            if (sign) fn('-');
            const char *z = inf;
            while (*z) fn(*z++);
       }
       return;
    }

    //convert it to integer
    std::intptr_t iexp = (std::intptr_t)fexp;
    //if exponent is in some reasonable range, set iexp to 0
    if (iexp > -3 && iexp < 8) {
       iexp = 0;
    }
    else {
       //otherwise normalize number to be between 1 and 10
       value = value * pow(0.1, iexp);
    }
    double fint;
    //separate number to integer and fraction
    double frac = modf(value, &fint);
    //calculate multiplication of fraction
    auto fractMultiply = FMTable::get(precisz);
    //multiplicate fraction to receive best rounded number to given decimal places
    double fm = floor(frac * fractMultiply +0.5);

    //convert finteger to integer
    std::size_t intp(fint);
    //mantisa as integer number (without left zeroes)
    std::size_t m(fm);


    //if floating multiplied fraction is above or equal fractMultiply
    //something very bad happen, probably rounding error happened, so we need adjust
    if (m >= fractMultiply) {
       //increment integer part number
       intp = intp+1;
       //decrease fm
       m-=fractMultiply;
       //if integer part is equal or above 10
       if (intp >= 10 && iexp) {
            //set  integer part to 1 (because 9.99999 -> 10 -> 1e1)
            intp=1;
            //increase exponent
            iexp++;
       }
    }

    //write signum for negative number
    if (sign) fn('-');
    //write absolute integer number (remove sign)
    unsignedToString(intp,fn);
    int digits = precisz;

    if (m) {
       //put dot
       fn('.');
       //remove any rightmost zeroes
       while (m && (m % 10) == 0) {m = m / 10;--digits;}
       //write final number
       unsignedToString(m,fn,10, digits);
    }
    //if exponent is set
    if (iexp) {
       //put E
       fn('e');
       if (iexp > 0) fn('+');
       //write signed exponent
       signedToString(iexp,fn,10);
    }
    //all done
}

template<typename T>
T stringToUnsigned(std::string_view &&s, int base=10) {
    T ret = 0;
    auto sz = s.length();
    for (decltype(sz) i = 0; i < sz; ++i) {
        char c = s[i];
        int v;
        if (c < '0' || c > '9') {
            if (c >= 'A' && c <= 'Z') {
                v = c - 'A' + 10;
            } else if (c >= 'a' && c <= 'z') {
                if (base <= 36) {
                    v = c - 'a' + 10;
                } else {
                    v = c - 'a' + 36;
                }
            } else {
                s = s.substr(i);
                break;
            }
        } else {
            v = c - '0';
        }
        if (v>=base) {
            s = s.substr(i);
            break;
        }
        T nret = ret * base + v;
        if (nret < ret) ret = std::numeric_limits<T>::max();
        else ret = nret;
    }
    return ret;
}

template<typename T>
T stringToSigned(std::string_view &&s, int base=10) {
    if (s.empty()) return 0;
    else if (s[0] == '+') {
        s = s.substr(1);
        return stringToUnsigned<T>(std::move(s), base);
    }
    else if (s[0] == '-') {
        s = s.substr(1);
        return -stringToUnsigned<T>(std::move(s), base);
    }
    else {
        return stringToUnsigned<T>(std::move(s), base);
    }
}


template<typename Fn>
inline double parseDoubleNumber(Fn &&fn) {
    //parsing double is done straightforward
    //number is separated to parts [<sign>]<int>[.<decimals>][E[<sign>]<exponent>]
    //all numbers are integers

    //read signature - if none, default is false as positive number
    bool isneg = false;

    //read first character, could be sign
    int c = fn();

    if (c == '-') { //sign = -, is negative
        isneg = true;
        c = fn(); // and read next
    } else if (c == '+') { //sign = + is positive
        c = fn(); //just read next
    }

    //no digits is error
    if (!std::isdigit(c)) {
        return 0;
    }

    //d1 contains our double number, starting at zero
    double d1 = 0;
    //d2 contains decimal part, starting at zero
    double d2 = 0;
    //contains d1 exponent
    int d1_exponent = 0;
    //contains d2 exponent
    int d2_exponent = 0;

    //for digits multiply current number by ten and add number
    while (std::isdigit(c)) {
        d1 = d1 * 10 + (c - '0');
        c = fn();
    }
    //round to nearest integer
    d1 = std::round(d1);
    //we stop on dot, decimals follows
    if (c == '.') {

        //decimals are stored as d2*10^d2_exponent where d2 is integer stored in double
        c = fn();

        if (std::isdigit(c)) {
            while (std::isdigit(c)) {
                --d2_exponent;
                d2 = d2 * 10 + (c-'0');
                c = fn();
            }
            //strip any decimal numbers made because rounding errors
            d2 = std::round(d2);
        } else {
            return d1;
        }

    }
    if (c == 'E' || c == 'e') {

        bool negexp = false;
        c = fn();
        if (c == '-') {
            negexp = true;
            c = fn();
        } else if (c == '+') {
            c = fn();
        }
        if (std::isdigit(c)) {
            while (std::isdigit(c)) {
                d1_exponent = d1_exponent * 10 +(c - '0');
                c = fn();
            }

            if (negexp) d1_exponent = -d1_exponent;
        }

    }
    //compose final number d1*10^d1_exponent + d2*10^(d1_exponent+d2_exponent)
    double res = d1 * std::pow(10, d1_exponent) + d2*std::pow(10, d1_exponent+d2_exponent);

    if (isneg) res = -res;
    return res;
}

}

template<std::size_t n>
struct StrBuff {

    void push_back(char c) {
        if (pos < n) buff[pos++] = c;
    }
    operator std::string_view() const {
        return std::string_view(buff, pos);
    }
protected:
    char buff[n];
    std::size_t pos = 0;
};



template<int base, typename Number>
auto unsigned_to_string(const Number &n) {
    StrBuff<static_cast<std::size_t>(std::numeric_limits<Number>::digits*std::log(2)/std::log(base)+4)> out;
    _details::unsignedToString(n, [&](char c){
            out.push_back(c);
        });
    return out;
}

template<int base, typename Number>
auto signed_to_string(const Number &n) {
    StrBuff<static_cast<std::size_t>(std::numeric_limits<Number>::digits*std::log(2)/std::log(base)+4)> out;
    _details::unsignedToString(n, [&](char c){
            out.push_back(c);
        });
    return out;
}

template<typename Number>
auto float_to_string(const Number &n, int maxPrecisionDigits=14) {
    StrBuff<30> out;
    _details::floatToString(n, [&](char c){
                out.push_back(c);
        }, maxPrecisionDigits);
    return out;
}

template<typename T, int base = 10>
auto string_to_unsigned(std::string_view &&s) {
    return _details::stringToUnsigned<T>(std::move(s), base);
}

template<typename T, int base = 10>
auto string_to_signed(std::string_view &&s) {
    return _details::stringToSigned<T>(std::move(s), base);
}


inline double string_to_float(std::string_view &&s) {
    bool consumed = false;
    auto iter = s.begin();
    auto e = s.end();
    double z = _details::parseDoubleNumber([&]()->int {
      if (iter == e) {
          consumed = true;
          return -1;
      }
      else return *iter++;
    });
    if (!consumed) s = s.substr(std::distance(s.begin(), iter)-1);
    else s = std::string_view();
    return z;

}


}


#endif
