//
// Created by jc on 08/10/23.
//

#include "stacksAndQueues.h"

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <bitset>

using namespace std;

class Fancy {

public:

    vector<long long> vals;
    vector<long long> prods;
    vector<long long> sums;
    constexpr static long long mod = 1e9 + 7;

    Fancy() {
        sums.push_back(0);
        prods.push_back(1);
    }

    void append(int val) {
        vals.push_back(val);
        *sums.rbegin() = (*sums.rbegin() * *prods.rbegin()) % mod;
        sums.push_back(*sums.rbegin());
        prods.push_back((*prods.rbegin()));
    }

    void addAll(int inc) {
        *sums.rbegin() = ((*sums.rbegin() * *prods.rbegin())%mod + inc) % mod;
    }

    void multAll(int m) {
        *prods.rbegin() = (*prods.rbegin() * m) % mod;
    }

    long long findInverse(long long x, long long y) {
        long long z = 1;
        while (x % y != 0) {
            z += (mod - y) / y + 2;
            y = y - (mod - y) % y;
        }
        return z + x / y - 1;

    }

    int getIndex(int idx) {
        long long prod = findInverse(*prods.rbegin(), prods[idx]);
        long long inc = (*sums.rbegin() < sums[idx]) ? 1e9 + 7 : 0;
        long long sum = (((inc + *sums.rbegin()) - sums[idx])*prod) % mod;
        return ((prod * vals[idx]) % mod + sum) % mod;
    }
};

int main() {
    Fancy f;
    int vals_[] = {2, 3, 7, 2, 0, 3, 10, 2, 0, 1, 2};
    int *vals = vals_;
    f.append(2);
    f.addAll(3);
    f.append(7);
    f.multAll(2);
    cout << f.getIndex(0) << endl;
    f.addAll(3);
    f.append(10);
    f.multAll(2);
    cout << f.getIndex(0) << endl;
    cout << f.getIndex(1) << endl;
    cout << f.getIndex(2) << endl;

}