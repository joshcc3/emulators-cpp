//
// Created by jc on 14/10/23.
//

#include "debug_code.h"
#include <iostream>

using namespace std;

class A {
public:
    A() {

    }

    bool b;
    void f() {
        b = false;
    }

    void f() const {
        const_cast<bool&>(b) = true;
    }

};


int main() {
    const A a;
    a.f();
    cout << a.b << endl;
}