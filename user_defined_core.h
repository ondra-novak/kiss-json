#ifndef KISSJSON_USER_DEFINED_CORE_H_
#define KISSJSON_USER_DEFINED_CORE_H_


namespace kjson {

struct UserDefinedValueTypeDesc;


class PNode;

///Structure contains necessary fields to store user defined value
struct UserDefinedValue { // @suppress("Miss copy constructor or assignment operator")
    /** Contains type descriptor. It is basically type of this
     * custom value. It is reference to value type descriptor, which
     * defines how the user defined value is handled
     */
    const UserDefinedValueTypeDesc &type_desc;

    /** Pointer to variable area, where the user value stores all its necessary data
     *
     * @note This field is initialized for initialization, but if the type doesn't
     * use variable area, the field can be used as general purpose pointer
     *
     * @note Pointer can be nullptr in case that user value was created without variable area
     *
     * */
    void *_data;
    /** Size of variable area
     *
     * @note This field is initialized for initialization, but if the type doesn't
     * use variable area, the field can be used as general purpose number
     *
     */
    std::size_t _size;
};

class Value;

///Describes new user defined type
/**
 * We use pointers to functions because value is detached out of definition, so this is
 * basically virtual function table
 */
struct UserDefinedValueTypeDesc { // @suppress("Miss copy constructor or assignment operator")

    ///Gets name of the user type
    /**
     * @return name of type
     * @note function is mandatory
     */
    std::string_view (*get_type_name)     ();

    ///Returns required size of variable area
    /**
     * @param args arguments passed to the function
     * @return required size, this area will be allocated during initialization
     *
     * @note function is optional, if not defined, assumes 0 as response
     */
    std::size_t      (*get_required_size)(void *args);
    ///Called during initialization
    /**
     * @param value contains already initialized instance, now ready for user-initialization
     *    (like a constructor).
     * @param args arguments (same arguments passed to get_required_size() )
     *
     * @note function is optional, if not defined, no initialization is performed
     */
    void             (*init)              (UserDefinedValue &value, void *args);

    ///Called before it is destroyed
    /**
     * @param value contains instance itself. Function should deinitialize the user-defined value
     *
     * @note do not deallocate variable area.
     *
     * @note function is optional, if not defined, no deinitialization is perfomed
     */
    void             (*deinit)            (UserDefinedValue &value);


    ///Converts value to string
    /**
     * @param value value instance
     * @return text representation of value. Note the string is returned as string_view. It
     * is required to keep this string reference for whole lifetime of the instance.
     *
     * @note Function can be set to nullptr, if not supported - in this case get_type_name() is called
     */
    std::string_view (*get_string)        (const UserDefinedValue &value);

    ///In case that user-defined value acts as container, this function should return count of items
    /**
     * @param value instance
     * @return count of items in container, zero if the value is not container
     *
     * @note Function can be set to nullptr, if not supported
     */
    std::size_t      (*get_conatiner_size)(const UserDefinedValue &value);
    ///Retrieves value from container referenced by index
    /**
     * @param value instance
     * @param index index
     * @return PNode of value, function can return nullptr if not supported
     *
     * @note Function can be set to nullptr, if not supported
     */
    PNode            (*find_by_index)     (const UserDefinedValue &value, std::size_t index);
    ///Retrieves value from container referenced by a key
    /**
     * @param value instance
     * @param index index
     * @return PNode of value, function can return nullptr if not supported
     *
     * @note Function can be set to nullptr, if not supported
     */
    PNode            (*find_by_key)       (const UserDefinedValue &value, const std::string_view &key);
};

}




#endif /* KISSJSON_USER_DEFINED_CORE_H_ */
