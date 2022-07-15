//============================================================================
// Name        : kissjson.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "kissjson.h"
#include "parser.h"
#include "serializer.h"

using namespace std;

int main() {

    using namespace kissjson;


    Value sm = Value::from_string(R"json("\uD83D\uDC69")json");
    std::cout << sm.get_string() << std::endl;
    std::cout << sm.to_string(OutputType::ascii) << std::endl;

    std::string c = R"json(
        {"a":[1,3,-9,1.155e25,0.78e-51,-7158.01789644887e+001337891001,"ax",true,{"z":{},"y":[]},[],[3,"aey","ahoj\nnazdar"]],
         "\u0045":"\r\n\b\f\t\\Test\"lomeno\/trailer",
        "z":"\u006F",
        "u":"\uFFFF",
        "k":"\uabcd",
        "š":"+ěščřžýáíé"
        }
    )json";

    Parser p;
    for (char x: c) p.put_char(x);

    Value z = p.get_result();
    Value u = z["u"];

    std::cout << u.get_string() << std::endl;


    std::cout << z.to_string(OutputType::ascii) << std::endl;
    /*

    Value v = {
        {"aaa",10},
        {"bbb",42},
        {"x",true},
        {"obj",{
                {"z",-1}
        }},
        {"arr",Value::array{1,2,3,4,5}}
    };

    Value g = Value::array(std::vector<int>{1,2,3,4,5});


    v.set_keys({
        {"xyz",121},
        {"obj",{
                {"a",4}
        }},
        {"g",g}
    },Merge::recursive);



    std::cout << v.to_string() << std::endl;
*/

	return 0;
}

