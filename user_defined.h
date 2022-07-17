
#ifndef KISSJSON_USER_DEFINED_H_
#define KISSJSON_USER_DEFINED_H_

#include "core.h"

namespace kjson {


///Creates user defined value and stores the value to it
/**
 * @param v value
 * @return Value
 */
template<typename T>
Value create_value(T &&v) {
    static UserDefinedValueTypeDesc desc = {
        [] /*get_type_name*/ {return std::string_view(typeid(T).name());},
        [] /*get_required_size*/ (void *){return sizeof(T);},
        [] /*init*/ (UserDefinedValue &value, void *ptr){
            new(value._data) T(std::move(*reinterpret_cast<T *>(ptr)));
        },
        [] /*deinit*/ (UserDefinedValue &value){
            T *x = reinterpret_cast<T *>(value._data);
            x->~T();
        },
        /*get_string*/ nullptr,
        /*get_conatiner_size*/ nullptr,
        /*find_by_index*/ nullptr,
        /*find_by_key*/ nullptr,
    };

    return Value(desc, reinterpret_cast<void *>(&v));
}

///Casts existing Value to a type
/**
 * @param v Value object with user defined type specified as template parameter
 * @return pointer to stored value, nullptr if bad cast
 */
template<typename T>
const T *cast_value(const Value &v) {
    if (v.is_user_defined()) {
        const UserDefinedValue *c = v.get_user_defined_content();
        if (c->type_desc.get_type_name() == std::string_view(typeid(T).name())) {
            return reinterpret_cast<const T *>(c->_data);
        }
    }
    return nullptr;
}


}

#endif /* KISSJSON_USER_DEFINED_H_ */
