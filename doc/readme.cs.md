# KISSJSON

Knihovna implementuje parsování, serializaci a manipulaci s JSON hodnotami v rámci programu v jazyce C++ (verze 17). Knihovna zavádí univerzální třídu `Value` která je k určena k uložení libovné JSON kompatibilní hodnoty 

## Organizace projektu

Hlavičkové soubory najdete přímo v kořeni projektu. Toto uspořádání umožňuje linkovat knihovnu jako `git submodule`

Ve složce `test` se nachází testy. 

* **enums.h** - obsahuje pouze definice enumů
* **core.h** - obsahuje deklaraci všech pomocných tříd
* **number2str.h** - pomocné funkce
* **value.h** - obsahuje deklaraci třídy `Value`
* **parser.h** - obsahuje parser
* **serialize.h** - obsahuje serializer
* **user_defined_core.h** - obsahuje deklaraci typů pro tvorbu vlastních Value-typů
* **user_defined.h** - nabízí funkce `create_value` a `cast_value` pro zabalení libovolného typu do `Value`
* **base64.h** - obsahuje funkce podpory kódování base64

### Vkládání do existujícího kódu

```
#include "value.h"        //mandatory
#include "parser.h"       //only if we need parser
#include "serializer.h"   //only if we need serializer
#include "user_defined.h" //only if we need user defined values
```

## Vlastnosti

* **pouze hlavičky** - není třeba linkovat žádnou knihovnu
* **interní immutabilita** - struktura dat je navržena tak, aby nebylo možné měnit již existující hodnoty. Pro pozměnění obsahu je třeba vytvořit kopii. (COW - copy on write). Díky tomuto pravidlu se veškeré kopírování provádí jako sdílení s garancí, že sdílený obsah nebude nepředvídatelně pozměněn. To automaticky zaručuje bezpečný přístup z více vláken současně
* **bezpečnost ve vícevláknovém prostředí** - Na každou proměnnou `Value` lze nahlížet jako na izolovaný objekt a to i přesto, že obsah může být sdílen. Procházení kontejnerů nevyžaduje zámky ani synchronizaci. Pouze změna samotné `Value` musí být prováděno pouze jedním vláknem současně.
* **žádné cykly** - Některé jiné JSON knihovny umožňují vytvářet rekurzivně zacyklé kontejnery. To se týká i objektů v samotném javascriptu. Nebezpečí cyklu zesložiťuje serializaci nebo procházení JSON struktrů nebo i správu paměti . V knihovně `KissJSON` nelze cykly vytvářet a tudíž program využívající tuto knihovnu nemusí cykly ve struktuře řešit.
* **žádné ukazatele** - Až na okrajové výjimky se při práci s knihovnou nepoužívají ukazatelé (na veřejném rozhraní). Neexistuje tedy nebezpečí pádu programu například při pokusu pracovat s neinicializovanou proměnnou nebo s hodnotou `null`
* **čísla jsou uložené jako řetězce** - tato vlastnost zrychluje parsování a serializaci a umožňuje serializaci hodnot, které byly načteny z JSON formátu a jsou uloženy bez modifikace, aniž by došlo ke ztráce přesnosti nebo k zaokrouhlovacím chybám při převodu na číslo a zpět.
* **programátorské rozhraní používá iterátory a operátor[]** - Práce s kontejnery se tak neliší od práce s vektory, mapami atd
* **alfanumerické řazení klíčů v objektu** - všechny objekty mají klíče řazeny alfanumericky. Nezachovává se pořadí klíčů
* **stavový parser a serializer** - parsování a serializování provádí objekty s vnitřním stavem, umožňuje to proces parsování nebo serializace přerušit například při použití asynchroních IO operací.
* **podpora binárních dat** - do `Value` lze vložit i binární obsah, který se následně uloží jako BASE64.

## Podporované typy

* **null** - null hodnota - v rámci knihovny nemá žá
* **boolean** - lze uložit dvě hodnoty true/false
* **number** - nerozlišuje se zda jde o typ integer nebo float. Čísla se ukládají jako řetězec a při čtení se opět parsují na číslo, přitom záleží na požadovaném typu. Plovoucí čísla se ukládají s přesností na 14 desetinných míst (v 32bit prostředí na 9)
* **string** - řetězce mohou být ve formátu **utf-8**.
* **objekt** - představuje kontejner s asociativní klíči (typ řetězec), bez určeného pořadí. Kontejnery lze adresovat jak klíčem tak indexem. Lze použít range-for na procházení objektů. Interně jsou tyto kontejnery seřazené alfanumericky podle klíče
* **array** - představuje kontejner adresovaný indexem, pracuje se s ním podobně jako s vektorem
* **binary** - binární hodnoty se ukládájí jako typ **string**, pouze se značkou že jde o binární řetězec. Při serializaci se konvertuje na BASE64. Při parsování přichází jako **string** a je třeba ho nechat konvertovat ručne do binárního řetězec pomocí patřičného nástroje.
* **undefined** - Předstvauje neinicializovanou hodnotu (ani **null**) a objevuje se také při pokusu adresovat neexistující klíč, získat prvek mimo rozsah kontejneru, a při dalších situacích, které představují chybový stav. Tato hodnota se pak při převodu na C++ proměnnou chová deterministicky podle definice (viz dále)

## Filozofie přístupu.

* **typ** - je určen z příchozího formátu, nebo použitím správného konstruktoru.
* **přetypování** - každou hodnotu lze přetypovat na jiný typ a pokud přetypování je smysluplné, je vrácena adekvání hodnota. Například je možné volat `get_string()` na číselnou hodnotu a je obdržena řetězcová reprezentace čísla. Stejným způsobem lze požadovat `get_float()` na řetězcovou hodnotu a pokud ta obsahuje číslo, pak doje ke správné konverzi.
* **konverze až na požádání** - binární typ se nekonvertuje dokud to není požadováno.

## Namespace

Všechny třídy jsou definované v namespace `kjson`. Dále nebude uváděn

## Třída Value

### Konstrukce hodnot

* řetězec - `Value text("Ahoj světe")`
* boolean - `Value b(true)`
* číslo - `Value c(3.1415)`
* null - `Value n(nullptr)`
* undefined - `Value u()`

### Konstrukce objektu (inline)

Objekt lze konstrovat pomocí initializer listu.

(pozor, pole je nutné konstruovat jinak)

```
Value objekt = {
      {"klic","hodnota"},
      {"pi",3.14159265},
      {"is_earth_flat", false},
      {"nothing", nullptr}
      {"subobject", {
        {"hello", "world"}
        {"array", Array{1,2,3,10,true,"test string",{
                     {"object in array",true}
                 }, "array continues","here"}}
        }}
   };
```

### Konstrukce pole
    
Pole třeba konstruovat pomocí třídy `Array`, která dědí `Value`

```
Value a = Array{1,2,3,4,5,6};
```

Některé funkce očekávají typ `Array`, tam je možné konstruktor vynechat

```
Value b = a.concat({10,20,30}); //concat očekává pole
```

### Konstrukce objektu alternativně

Stejně pro `Array` existuje třída `Object`, kterou lze použít ke konstrukci objektu stejným způsobem

```
Value obj = Object{
       {"a",1},
       {"b",2},
       }
```

### Konstrukce pole z C++ kontejneru

Třída `Array` umí také zpracovat C++ kontejner, pokud kontejner má definovaný `begin()` a `end()`, definovaný  `std::distance` a hodnoty v kontejnerech lze převést na `Value`

```
std::vector<int> numbers({1,2,3,4,5});
Value a = Array(numbers);
```

Pokud kontejner obsahuje typy, které nelze převést na `Value` automaticky, lze přidat mapovací funkci

```
std::vector<int> numbers({1,2,3,4,5});
Value a = Array(numbers, [](int c){return Value(c*10);});
```

### Konstrukce objektu z C++ kontejneru

Pro konstrukci objektu je třeba přiřadit ke každé hodnotě klíč. Proto je potřeba vždy definovat mapovací funkci. K přiřazení klíče k hodnotě lze použít konstruktor `Value(<klíč>,<hodnota>)`

Následující příklad vytvoří objekt s klíči podle řetězcového vektoru a jako hodnota je použita délka klíče


```
std::vector<std::string> strs = {"hello", "hi", "world", "test", "foo", "" ,"bar", "xx", "a"};
Value obj = Object(strs, [](const std::string &key) {
        return Value(key, key.length);
        });
```

**Pozor:**  - Objekt nezachovává pořadí, výsledek je alfanumericky seřazený podle klíče

### Změny v kontejnerech

Díky interní immutabilitě každá změna znamená vytvoření kopie celého kontejneru. Knihovna disponuje nabídkou funkcí, které umožňují provádět více změn současně za provedení jedné kopie. Proto se zde nenachásí primitivní funkce set() která by umožňila měnit hodnoty po jedné. Nelze ani použít operátor přiřazení do operátoru[]

Následující kód nefunguje

```
Value obj = Object(); //create empty object
obj["key"]=42; //NOT compile
```

Ke změně objektů lze použít funkci `merge`. Ke změně pole lze použít `transform`, `transform_flatten`, `append`, nebo `splice` 

Pokud je potřeba manipulovat s jednotlivými prvky, doporučuje se překlopit kontejner do std::vector, provést požadované změny a poté překlopit zpět do kontejneru. Lze takto zpracovat i objekty, protože klíče jsou součástí hodnot (více funkce `get_key` a `bind_key`)

**Poznámka** - veškeré úpravy se vždy vztahují na aktuální proměnnou. Pokud jakákoliv jiná proměnná referencuje jakýkoliv vnořený kontejner, který je předmětem změny, pro něho se změna neprojeví, pořád bude mít k dispozici starý obsah

### Value::merge

Funkce merge slouží k modifikaci kontejneru typu `object`. Funkce připojí nové klíče do současného objektu. Klíče se stejným názvem jsou přepsány. Lze použít hodnotu `undefined` pro smazání klíče

```
Value obj = {
    {"untouched",1},
    {"will_be_replaced",42},
    {"will_be_deleted", 96},
    {"subobject",{
        {"a",1},
        {"b",3}
    }}
};

obj.merge({
   {"will_be_replaced",54},
   {"will_be_deleted",Value()},
   {"subobject",{
        {"b",Value()},
        {"c",10}
   }}
});
```

Výsledkem výše uvedeného kódu bude objekt

```
{
    "untouched":1,
    "will_be_replaced":54,
    "subobject": {
        "b":"undefined",
        "c":10
    }
}
```

Ve výchozím nastavení se slučují pouze klíče první úrovně. Proto subobject je kompletně nahrazen. Hodnota `undefined`, pokud není použita ke smazání hodnoty, se serializuje jako řetězec "undefined".

Pro slučování rekurzivně je třeba volání modifikovat

```
obj.merge({
   {"will_be_replaced",54},
   {"will_be_deleted",Value()},
   {"subobject",{
        {"b",Value()},
        {"c",10}
   }}
}, Merge::recursive);
```

Výsledkem této úpravy bude objekt

```
{
    "untouched":1,
    "will_be_replaced":54,
    "subobject": {
        "a":1,
        "c":10
    }
}
```

### Value::transform

Funkce `transform` slouží k modifikaci existujícího pole 

```
Value a = {1,2,3,4,5};
a.transform([](Value x){return x.get_int()*10;}); //multiply all item ten times
```


Výsledkem je pole [10,20,30,40,50]

Pokud funkce vrátí `undefined`, pak je předmětná hodnota smazána

```
Value a = {1,2,3,4,5};
a.transform([](Value x){return x.get_int() == 2?Value():x;}); //remove value 2
```
Výsledkem je pole [1,3,4,5]


### Value::transform_flatten

Funkce `transform_flatten` pracuje stejně jako `transform` pouze s tou změnou, že pokud je výsledkem 2D pole, je převedeno na 1D pole (obecně je vždy snížen rozměr výsledného pole o 1, tedy 3D pole je sníženo na 2D, atd) - Cílem této úpravy je umožnit během transformace přepsat jeden prvek vícero prvky

```
Value a = {1,2,3,4,5};
a.transform_flatten([](Value x) -> Value {return x.get_int() == 2?Array(10,20,30):x;}); //replace 2
```
Výsledkem je pole [1,10,20,30,3,4,5]

Je třeba dát pozor, pokud zdrojové pole je vícerozměrné, je třeba v mapovací funkci všechna pole vložit do nového jednoprvkového pole, aby při flatten nedošlo k jejich rozbalení do jednoho rozměru.

### Value::append

Funkce `append` obyčejně přidá na konec prvky z druhého pole

```
Value x = {1,2,3,4};
x.append({10,20,30})
```
Výsledek je [1,2,3,10,20,30];


### Value::splice

Funkce `splice` funguje podobně jako stejnojmenná funkce v Javascriptu

`splice(<index>,<count>,<prvky>)`

Funkce odstraní z pole prvky od indexu <index> v počtu <count>. Místo těchto prvků vloží nové prvky. Pokud je rozsah <count> nulový, jsou prvky vloženy na zadaný index. Pokud nejsou zadané žádné prvky, pak dojde pouze ke smazání rozsahu

````
a.splice(1,4) - delete from 1 four items
a.splice(2,1,{10,20}) - replace 1 item at index by two items 10,20
a.splice(3,0,{"a","b","c"}) - insert array at index 3
```

### Serializace

Serializaci lze provádět pomocí třídi `Serializer`. Nicméně rozhranní `Value` nabízí funkce pro jednoduché použití serializeru

```
Value v = ...;

std::string s = v.to_string(); //returns serialized version of Value
```

```
Value v = ...;

v.to_stream(std::cout);  //serialize to an output stream

````

Serializace do vlastního streamu docílíte použitím lambda funkce

```
Value v = ...;

v.serialize([&](char c){
    send_character_to_user_defined_streeam(c);
});
```

#### Specifikace formátu

Serializaci lze provádět ve dvou formátech
* **OutputFormat::ascii** - omezí výsledný charset na 7-bit ascii
* **OutputFormat::utf8** - uloží ve formátu utf-8

Ve výchozím stavu se používá **utf8** pro funkci to_string() a **ascii** v ostatních případech. Nicméně lze formát nastavit parametrem. Formát **ascii** použijete všude tam, kde není dopředu garantováno zpracování dat ve formátu **utf8**. V tomto režimu jsou všechny unicode znaky přepsány na sekvenci \uXXXX.

**Poznámka** - Pro použití funkcí serializace je třeba vložit `#include "serializer.h"` i když se přímo nepoužívá


### Parsování JSON

Parsování lze provádět pomocí třídi `Parser`. Nicméně rozhranní `Value` nabízí funkce pro jednoduché použití parseru

```
Value v = Value::from_string("...");
```

```
Value v = Value::from_stream(std::cin);
```

Parsování z vlastního streamu lze zajistit lambda funkcí. Funkce musí vrátit další znak ze streamu, pokud narazí na konec streamu, musí vrátit hodnotu -1 (očekává se `int()`

```
Value v = Value::parse([&]()->int{
    return get_from_user_stream();
 });
 
```

#### Chyby při parsování a nevalidní JSON

Chyby v parsovaní jsou vyhazovány jako výjimky `ParseError`


## Získávání dat z `Value`

### Zjištění stavu

* `Value::defined()` - vrací **true**, pokud proměnná má hodnotu, přičemž `null` se považuje za hodnotu
* `Value::has_value()` - vrací **true**, pokud proměnná má hodnotu a není to `null` - tedy z hlediska interpretace obsahu z vnější strany kdy `null` se nepovažuje za hodnotu (interně hodnotou je)
* `Value::is_null()` - vrací **true**, pokud proměnná má právě hodnotu `null`.
* `Value::is_container()` - vrací **true**, pokud je v proměnné kontejner, například `object` nebo `array`, to znamená, že obsahuje víc prvků a je nutné jej iterovat
* `Value::is_object()` - vrací **true**, pokud proměnná obsahuje JSON object
* `Value::is_array()` - vrací **true**, pokud proměnná obsahuje JSON array
* `Value::is_string()` - vrací **true**, pokud proměnná obsahuje řetězec (utf-8)
* `Value::is_binary_string()` - vrací **true**, pokud proměnná obsahuje binární řetězec. Tento typ není možné obdržet parsováním existujícího JSON souboru, protože všechny binární řetězce jsou uloženy jako BASE64 řetězce. Teprve požadavkem na převod na binární řetězec lze obdržet proměnnou obsahující binární řetězec. Při serializaci se binární řetězec uloží jako BASE64 řetězec
* `Value::is_number()` - vrací **true**, pokud proměnná obsahuje číslo
* `Value::is_bool()` - vrací **true**, pokud proměnná obsahuje boolean hodnotu
* `Value::is_user_defined()` - vrací **true**, pokud proměnná obsahuje hodnotu uživatelem definovaného typu. - Pozor tyto hodnoty nelze serializovat
* `Value::is_copy_of(x)` - vrací true, pokud proměnná `x` má hodnotu, která vznikla jako přímá kopie hodnoty `this` proměnné. Přitom se neporovnává obsah, pouze skutečnost, zda jde o kopii. Tedy dvě proměnné se stejným obsahem nemusí být kopíí. Existuje několik výjimek a týká se primitivních hodnot jako jsou boolean hodnoty (**true/false**), hodnoty **null** a **undefined** a specifické hodnoty jako prázdný string, hodnota 0, prázdné pole a prázdný objekt. Tyto hodnoty mají staticky alokovanou instanci a všechny tak vznikají jako kopie této instance. Smyslem optimalizace je pro tyto hodnoty ušetřit extra paměť a provádění alokací. Takže dvě prázné pole nebo dvě hodnoty null budou vždy na `is_copy_of` vracet **true**, protože všechny jsou kopií jedné staticky alokované instance.
* `Value::size()` - vrací počet prvků v kontejneru, nebo 0, pokud se nejedná o kontejner.
* `Value::empty()` - vrací **true**, pokud je kontejner prázdný, jinak vrací false. Pro hodnoty, které nejsou kontejnerem se vrací **true**, protože takové hodnoty se chovají jako prázdný kontejner
* `Value::get_type()` - umožní zjistit typ a vrací jako enum `ValueType` - například `ValueType::string`

### Čtení hodnot

* `Value::get_string()` - vrací hodnotu jako řetězec, pokud je to možné. Pro typ `string` vrací přímo obsah řetězce, pro ostatní typy může vracet textovou reprezentaci. Platí pro `number` a pro `boolean`. Pro `null` vrací prázdný řetězec a pro `undefined` vrací "undefined". Pro kontejnery jako pole a objekt neprovádí serializaci, pouze vrací `[]` resp `{}` v případě že kontejner je prázdny a `[...]` resp `{...}` pokud není prázdný
* `Value::get_int()`
* `Value::get_long()`
* `Value::get_long_long()`
* `Value::get_unsigned_int()`
* `Value::get_unsigned_long()`
* `Value::get_unsigned_long_long()`
* `Value::get_float()`
* `Value::get_double()` - vrací číslo v patřičném typu. Lze použít i na řetězcové hodnoty, pokud obsahují číslo. Pokud nelze řetězec převést na číslo, vrací nulu. Pro `boolean` vrací 1 pro **true** a 0 pro **false**. Pro ostatní typy vrací nulu.
* `Value::get_bool()` - vrací hodnoto `boolean` proměnné. V případě použití na ostatní typy, vrací **true** pokud proměnná obsahuje neprázdnou hodnotu, jinak **false**. Například prázdný řetězec vrací **false**, neprázdný řetězec **true**. Stejné platí pro pole a objekt. Pro číslo vrací **true** pokud není hodnotou nula, pro nulu vrací **false**. Hodnoty `null` a `undefined` vrací **false**
* `Value::get_binary()` - převede `string` na binární string a vrátí jej v podobě objektu Binary. Na něj lze zavolat `Binary::get_string()` a získat tak binární podobu řetězce. Není-li v řetězci platný BASE64, převod se zastaví a výsledný binární string nemusí být kompletní nebo může být vrácen prázdný. Funkce používá `Value::get_string()` jako vstupní string. Nemá smysl funkci volat na jiné typy, než `string`
* `Value::get_array()` - přetypuje `Value` na `Array`. Je potřeba pro funkce, které očekávají jako vstupní parametr právě typ `Array`. Lze přetypovat `objekt` na `Array`, pak jakákoliv operace s takovou hodnotou vede na výsledek `array`. Hodnoty které nejsou kontejnerem se přetypují na prázdné pole
* `Value::get_object()` - přetypuje `Value` na `Object`. Lze použít pouze na `object` jinak vrací prázdný objekt
* `Value::get_user_defined_content()` - vrací pointer na uživatelem definovaný obsah u hodnot typu `user_defined`. Pokud proměnná neobsahuje hodnotu tohoto typu, vrátí **nullptr**

### Procházení kontejnerů

Kontejnery je možné procházet stejně, jako STL kontejnery. `Value` obsahuje defince funkci `begin()` a `end()`, a vlastní iterátor `Value::iterator` který patří do kategorie `random_access_iterator`. Iterátory lze použít na **range-for**

```
Value v = Array{10,20,30,40,50};

for(const Value &x: v) {
    cout << v.get_int() << endl;
}
```

**Pozor** - iterátorem nelze měnit obsah kontejneru. Iterátor je vždy konstantní. 

**Pozor** - Neukládejte vrácenou referenci. Vždy ukládejte kopii `Value`. Kopírování je rychlé. Protože veřejné rozhraní nevrací žádné reference, k dodržení rozhraní iterátoru odkazuje vrácená reference na dočasnou proměnnou, která obsahuje hodnotu daného iterátoru

### Procházení objektů a práce s klíči

Typ `object` je také kontejner a lze jeji procházet stejně. Položky v objektu jsou složeny s klíče a hodnoty. Tato dvojice je též uložena jako `Value` a hodnotu klíče je nutné vyzvednou funkcí `Value::get_key()`

```
Value v = {
    {"a one",1},
    {"a two",2},
    {"a three", 3},
    {"a four", 4},
    {"shall we take it from the top, boys", 0}
   };
  

for(const Value &x: v) {
    cout << v.get_key() << "=" << v.get_int() << endl;
}
```

Každá hodnota může mít přiřazený klíč i když se momentálně nenachází v objektu. Dokonce i v poli může mít klíč (tento klíč se však nebude serializovat). Přiřazení klíče k objektu lze provést několika způsoby

* Konstruktor `Value(<key>,<value>` - lze zavol

```
Value a("a one",1);
cout << a.get_key() << endl; //prints "a one"
```

* V objektu má každá hodnota klíč, a proto jeden ze způsobů je vytvořit objekt s klíčem a hodnotou
* Funkce `Value::bind_key()`

```
Value a(1);
a.bind_key("a one")
```

Obecně všechny hodnoty mají klíč, pokud není žádný přiřazený, pak se uvažuje klíč "" (prázdný řetězec). Funkce `Value::unbind_key()` změní přiřazený klíč na prázdný řetězec.

Přiřazování klíčů je opět úpravou hodnoty a platí, že změna se projeví jen v aktuální proměnné. Nelze tedy takto změnit klíč v existujícím objektu. Pro takovou úpravu je třeba použít `Value::merge` ve které je starý klíč smazán a nahrazen novým.


```
Value a = 1;
Value b = a;
a.bind_key("a one");
cout << a.get_key() << endl; //prints "o one"
cout << b.get_key() << endl; //prints empty line
```

```
Value v = {
    {"a one",1},
    {"a two",2},
    {"a three", 3},
    {"a four", 4},
    {"shall we take it from the top, boys", 0}
   };

Value z = a["a one"];
z.bind_key("a zero")

cout << z.get_key() << endl; //prints "a zero"
cout << v.to_string() << endl; //prints whole object where "a one" is not modified.

v.merge({
   {"a one", Value()},
   {"a zero", z}
});

cout << v.to_string() << endl; //prints whole object with changed "a one" to "a zero"

```

Přiřazování klíčů mimo `object` má význam při konstrukci objektu z libovolného C++ kontejneru, kdy je třeba předaným hodnotám přiřadit klíč. 

```
std::vector<std::string> strs = {"hello", "hi", "world", "test", "foo", "" ,"bar", "xx", "a"};
Value obj = Object(strs, [](const std::string &key) {
        return Value(key, key.length());
        });
```



## Vnitřní struktura

Proměnná `Value` je ve skutečnosti pouze ukazatelem na instance třídy `Node`

```
     Value
┌──────────────┐             Node
│              │        ┌──────────────┐
│       ───────┼──────► │   _cntr      │ ref.counter
│              │        ├──────────────┤
└──────────────┘        │   _type      │ ValueType
                        ├──────────────┤
                        │              │
                        ├───  type  ───┤
                        │   specific   │ 3x ptr size
                        ├───        ───┤
                        │              │
                        ├──────────────┤
                        │   variable   │
                        │   sized      │ 0..n bytes
                        ~   area       ~
                        │              │
                        └──────────────┘
```
### struktura null a undefined

```
   Node
┌──────────┐
│  _cntr   │
├──────────┤
│  _type   │  null / undefined
├──────────┤
│          │
│          │
│  unused  │
│          │
│          │
└──────────┘
```
Hodnoty null a undefined jsou předalokované a každá je tedy singletonem

### struktura boolean

```
   Node
┌──────────┐
│  _cntr   │
├──────────┤
│  _type   │  boolean
├──────────┤
│true/false│
├──────────┤
│  unused  │
│          │
│          │
└──────────┘
```
Obě hodnoty (true/false) jsou uložené jako singleton

### čísla a řetězce

```
          Node
    ┌───────────────┐
    │     _cntr     │
    ├───────────────┤
    │     _type     │ number/string
    ├───────────────┤
    │               │
┌───┼─ string_view ─┤
│   │               │
│   ├───────────────┤
│   │    encoding   │ utf-8/binary
│   ├───────────────┤
└──►│               │
    │   text-       │
    ~     -buffer   ~
    │               │
    │               │
    └───────────────┘
```
Čísla se ukládají jako řetězce. Samotný textový obsah je uložen ve variabilní části

### pole a objekty

```
          Node
    ┌───────────────┐
    │     _cntr     │
    ├───────────────┤
    │     _type     │ array/object
    ├───────────────┤                          ┌────────┐
┌───┤  _ptr begin   │                    ┌────►│        │
│   ├───────────────┤                    │     │  Node  │
│   │    _count     │                    │     │        │
│   ├───────────────┤                    │     └────────┘
│   │     nullptr   │                    │
│   ├───────────────┤                    │     ┌────────┐
└──►│  1st PNode    ├────────────────────┘   ┌►│        │
    ├───────────────┤                        │ │  Node  │
    │  2nd PNode    ├────────────────────────┘ │        │
    ├───────────────┤                          └────────┘
    │  3th PNode    ├────────────────────┐
    ├───────────────┤                    │     ┌────────┐
    │               │                    └────►│        │
    ~     .....     ~                          │  Node  │
    │               │                          │        │
    ├───────────────┤                          └────────┘
    │  nth PNode    │
    └───────────────┘
```
Prvky kontejneru se ukládají ve variabilně veliké oblasti. Pro objekty platí, že klíče jsou uloženy spolu s hodnotou a tedy nejsou uloženy extra v kontejneru. Rozdíl mezi polem a objektem je tedy pouze ve způsobu řazení a hledání dat

Specifický způsob uložení je ukládání `slices`, tedy podrozsahy kontejnerů

```
      Node
   ┌─────────┐
   │ _cntr   │
   ├─────────┤
   │ _type   │  array/object
   ├─────────┤
┌──┤ _ptrbeg │
│  ├─────────┤
├──┤ _count  │                Node
│  ├─────────┤          ┌───────────────┐
│  │ _owner  ├─────────►│     _cntr     │
│  └─────────┘          ├───────────────┤
│                       │     _type     │ array/object
│                       ├───────────────┤               ┌────────┐
│                   ┌───┤  _ptr begin   │         ┌────►│        │
│                   │   ├───────────────┤         │     │  Node  │
│                   │   │    _count     │         │     │        │
│                   │   ├───────────────┤         │     └────────┘
│                   │   │     nullptr   │         │
│                   │   ├───────────────┤         │     ┌────────┐
│                   └──►│  1st PNode    ├─────────┘   ┌►│        │
│                       ├───────────────┤             │ │  Node  │
└────────────────┬─────►│  2nd PNode    ├─────────────┘ │        │
                 │      ├───────────────┤               └────────┘
                 │      │  3th PNode    ├─────────┐
                 │      ├───────────────┤         │     ┌────────┐
                 │      │               │         └────►│        │
                 │      ~     .....     ~               │  Node  │
                 └────► │               │               │        │
                        ├───────────────┤               └────────┘
                        │  nth PNode    │
                        └───────────────┘
```

### Hodnoty s přidruženým klíčem

Klíč je samostatný objekt, který se odkazuje na hodnotu, ke které je přidružen. Z hlediska rozhraní se chová transparentně, tedy jakékoliv operace s klíčem se forwardují na hodnotu, jediný způsob jak přistoupit přímo na klíč je přes `get_key` a `bind_key`

```
          Node
    ┌───────────────┐
    │     _cntr     │
    ├───────────────┤
    │     _type     │ key
    ├───────────────┤               ┌────────┐
    │               │         ┌────►│        │
┌───┼─ string_view ─┤         │     │  Node  │
│   │               │         │     │        │
│   ├───────────────┤         │     └────────┘
│   │     PNode     ├─────────┘
│   ├───────────────┤
└──►│               │
    │   text-       │
    │      buffer   │
    ~               ~
    │               │
    └───────────────┘
```

### Uživatelem definovaný typ
```
      Node
┌───────────────┐
│     _cntr     │
├───────────────┤
│     _type     │ user_defined
├───────────────┤
│  _type_desc   ├─────┐     ┌────────────────┐
├───────────────┤     └────►│                │
│  void *_data  ├───┐       │                │
├───────────────┤   │       │  type          │
│     _size     │   │       │    descriptor  │
├───────────────┤   │       │                │
│               │◄──┘       │  (statically   │
│   optional    │           │     allocated) │
│   varaiable   │           │                │
~   sized area  ~           │                │
│               │           │                │
└───────────────┘           └────────────────┘
```

