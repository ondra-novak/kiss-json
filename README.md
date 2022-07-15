# KISSJSOIN 

KISS - Keep It Simple Stupd

KISSJSON is simplified version of JSON parser/serializer/manipulator inspired from ImtJSON. It's not only about parsing and serializing. It exposes interface to store and manipulate with values parsed from text-JSON which ability to serialize them back to text file.

Whole library is header only. It is separated to several files that can be incuded as
needed.

* **value.h** - contains definition of a class  `Value`
* **serializer.h** - contains serializer
* **parser.h** - contains parser

## Key features

* **header only** - no library
* **internal immutability** - all modification are made using copy-on-write. Each value act as separate instance even if it is internally shared. Because of this, accessing content of values is MT Safe without additional locking. This is especially useful while traversing containers. 
* **no cycles** - you can't create cycles in JSON structure. There is no special check, it is simple impossible.
* **no pointers** - no pointers are visible on public interface
* **no nullptr** - access to unitialized variable can't cause SIGSEGV
* **standard iterators** - iterating through containers are done using standard iterators. 
* **operator [] ** - to access containers' values
* **numbers stored as strings** - parser doesn't 'parses' numbers, just checks validity and stores the numbers as strings. This allows to serialize these numbers in original form without loosing precision. Conversion to number is performed once the number is requested. Numbers used to create json value is also converted to string immeditaly, and serializer only copies the string to the output. It also enables **get_string()** function to work on numbers.
* **statefull parser** - Parser acts as output stream. You put characters to the parser until the value is fully parsed. This helps to parse JSON transfered through for example network stream, when JSON file arrives as set of incomplette messages
* **statefull serializer** - Serializer acts as input stream. You read charactes from the serializer until eof appear. This helps to serialize JSON to network stream. You can interrupt serializing when output buffer is full and continue after buffer is flushed to an output stream.

## Create value

```
kjson::Value number(10);
kjson::Value string("Hello world");
kjson::Value object({
        {"key1",1},
        {"key2",2},
        {"subkey",{
            {"subkey1",1},
            {"subkey2",2},
        }}
   });
kjson::Value array(kjson::Value::array{1,2,3,4,5});
```

- To construct array, you need to specify `Value::array` object, which helps to constructor array using initializer list
- Alternatively, you can use `Value::object` to constructor object, however, the class `Value` automatically expects object


### Create array programatically 

```
kjson::Value empty (kjson::Value::array());

std::vector<int> data={1,2,3,4,5}

kjson::Value v1 (kjson::Value::array(data));

kjson::Value v2 (kjson::Value::array(data, [](int i) {
  //... map function
  return kjson::Value(i)
}));
kjson::Value v3 (kjson::Value::array(10, [](std::size_t idx) {

    //... generator
    return kjson::Value(idx);
}));

```

## Create object programatically

```
kjson::Value empty (kjson::Value::object());

std::vector<kjson::Value> data={
        kjson::Value("key1",1),
        kjson::Value("key2",2)
};

kjson::Value v1 (kjson::Value::object(data));

kjson::Value v2 (kjson::Value::object(data, [](kjson::Value x) {
  //... map function
  return x
}));

kjson::Value v3 (kjson::Value::object(10, [](std::size_t idx) {
    //... generator
    // return ....
}));

```

## Modyfing values

Elemental values can be modified by simple assign new value

```
kjson::Value a ("hello");
a = "world"
a = 10
a = true
```

The same applied to whole containers (objects and arrays). 

Different situation is while content of a container is modified. Because every write to a container creates modified copy, there are no traditional setters to change single value
of the container. There are several functions to modify multiple items in a single call

### Modyfing objects

#### function merge()

Merges two objects into one. Keys from other objects replaces keys of first object. 

```
kjson::Value v{
    {"key1","a"},
    {"key2",10},
 };
 
v.merge({
    {"key2",11}
    {"key3",true}
    });
    
    
//result is key1="a", key2=11, key3=true
```

Merge an be done recursively, so you are able to modify sub-objects

`Value::merge(object, merge_op)`

- **Merge::flat** - merge only first level
- **Merge::recursive** - merge also sub-objects

**Deleting keys using merge** - KissJson uses value **undefined** to mark keys for deletion. To constructuct such value, just pass empty constructor


```
v.merge({
    {"key1",kjson::Value()}
    });
//result is key2=11, key3=true
```

#### Transorming object - transform_object()

Function transform_object() accepts function which converts one value of the object to another, similar to map function for an array

```
v.transform_object([](Value x) {
    return x.get_int()+1;
});

```
### Modyfing arrays

#### function append()

```
kjson::Value x(kjson::Value::array{1,2,3,4});
x.append({10,20,30,40})
```

#### function push()

```
kjson::Value x(kjson::Value::array{1,2,3,4});
x.push(5)
```

Note: The function push can be slow, because it always make a copy of the whole container

#### function transform()

```
kjson::Value x(kjson::Value::array{1,2,3,4});
x.transform(... map function...)
```

Applies map function to every item on array. This function also allow to return `undefined` to remove items

#### function transform_flatten()

Works similar as `.transform()` but flatens array after mapping is applied. This allows to insert new items, which are inserted as arrays (so intead returning one item, you can return array of items. These items are eventually inserted at the given place)

#### function slice()

Creates sub-array

#### function pop()

Removes last item from the array


## How keys are stored

Keys are bound to values. Object is only container which contain keyed and ordered values (ordered by their key). There is function, which retrieves key of a value when the content of the object is enumerated

```
for (Value v: object) {
    cout << v.get_key() << '=' << v.get_string() << endl;
}
```



