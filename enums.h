/*
 * enums.h
 *
 *  Created on: 15. 7. 2022
 *      Author: ondra
 */

#ifndef KISSJSON_ENUMS_H_
#define KISSJSON_ENUMS_H_

namespace kjson {


enum class ValueType {
    ///Undefined value - default value for uninitialized json value
    /** Undefined value is also used to 'delete' value from the container. By replacing value
     * with 'undefined' causes its deletion  */
    undefined,
    ///Null value - have extra type and only one value
    null,
    ///boolean value - can contain true or false
    boolean,
    ///number value - numbers are stored as strings and parsed when needed
    number,
    ///string value - ordinary string, but must be in UTF-8, otherwise it could be corrupted during serialization
    string,
    ///object container - contains key:value pairs, always ordered by key, indexed by key
    object,
    ///array container - contains values, indexed by zero based index
    array,
    ///key item, internal usage
    /**
     * Key items are special items, they hold keys bound to the values. They are mostly transparent,
     * but always carried with values, if they are picked from object container. You can
     * retrieve key by function get_key(), or bind different key by calling set_key()
     */
    key,
    ///allows to create user defined value node
    /**
     * Note - user defined values can't be serialized and parsed (not yet)
     */
    user_defined
};

enum class OutputType {
    ascii,
    utf8
};

enum class NumberType {
    not_number,
    real_number,
    unsigned_number,
    signed_number
};

///Defines merge behavior
enum class Merge {
    flat,     ///< merge only first level
    recursive ///< merge all levels recursively
};

///Specifies how string is stored within JSOn object
enum class StringType {
    utf8,   ///<standard string in UTF-8.
    binary,  ///<arbitrary binary data, which are serialized as BASE64
};



}



#endif /* KISSJSON_ENUMS_H_ */
