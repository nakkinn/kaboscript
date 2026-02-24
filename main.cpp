//実行コマンド　g++ main.cpp string_utils.cpp virtual_machine.cpp compiler.cpp -o main && main

#include <iostream>

#include "string_utils.hpp"
#include "virtual_machine.hpp"
#include "compiler.hpp"



const char* jack_script0 = R"(

class Calc {                                                                                  int val;                                                                            
    static int count;                                                                                                                                                             
    constructor new(int v) {                                                            
        val = v;                                                                        
        count++;
    }

    method add(int n) {
        val += n;
        return val;
    }

    method get() { return val; }

    function getCount() { return count; }
}

class Main {
    function main() {

        // 算術・比較
        Output.printi(3 + 4);    Output.printc(' ');  // 7
        Output.printi(10 - 3);   Output.printc(' ');  // 7
        Output.printi(3 * 4);    Output.printc(' ');  // 12
        Output.printi(10 > 3);   Output.printc(' ');  // 1
        Output.printi(10 == 9);  Output.printc(' ');  // 0

        // ローカル変数・制御構文
        int sum = 0;
        for(int i = 1; i <= 5; i++) {
            sum += i;
        }
        Output.printi(sum);  Output.printc(' ');  // 15

        // 配列
        int arr[3];
        arr[0] = 10; arr[1] = 20; arr[2] = 30;
        Output.printi(arr[1]);  Output.printc(' ');  // 20

        // constructor + method + static
        Calc c1;
        Calc c2;
        c1 = Calc.new(10);
        c2 = Calc.new(20);
        c1.add(5);
        Output.printi(c1.get());          Output.printc(' ');  // 15
        Output.printi(c2.get());          Output.printc(' ');  // 20
        Output.printi(Calc.getCount());   Output.printc(' ');  // 2

        return 0;
    }
}


)";


VM vm;
Compiler compiler;

int main(){

    char vm_script[100000] = "";
    char output[10000] = "";

    compiler.run(jack_script0, vm_script, 100000);

    if(vm_script[0]=='E'){
        std::cout << vm_script << std::endl;
    }else{
        vm.run(vm_script, output, 10000);
        std::cout << output << std::endl;
    }



    //     std::cout << "### stack ###" << std::endl;
    //     char s1[] = " : ";
    //     for(int i=0; i<40; i++){
    //         if(i==vm.sp) s1[1] = '#';
    //         else    s1[1] = ':';
    //         std::cout << i << s1 << vm.stack[i] << std::endl;
    //     }

    //     std::cout << std::endl;
    //     std::cout << "### heap ###" << std::endl;
    //     for(int i=0; i<20; i++){
    //         std::cout << i << " : " << vm.heap[i] << std::endl;
    //     }

    //     std::cout << std::endl;
    //     std::cout << "### static ###" << std::endl;
    //     for(int i=0; i<20; i++){
    //         std::cout << i << " : " << vm.statics[i] << std::endl;
    //     }

    //     std::cout << "\noutput : \n" << output << std::endl;

    // }


    return 0;
}