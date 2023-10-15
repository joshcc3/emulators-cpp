//
// Created by jc on 14/10/23.
//

#include "debug_code.h"
#include <iostream>
#include <vector>

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
    vector<int> v{};
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    v.clear();
    v.push_back(3);
    v.push_back(10);
    cout << v.size() << " " << v[0] << " " << v[1] << " " << v.capacity() << endl;
}