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

class Solution {
public:

    bool bit(int n, int pos) {
        return (n >> pos) & 1;
    }

    bool test(int i, const string& s, const vector<int>& parenPos, string& res) {
        int pc = 0;
        int open = 0;
        int chars = 0;
        for(int j = 0; j < s.size(); j++) {
            while(pc < parenPos.size() && parenPos[pc] < j) {
                ++pc;
            }
            while(bit(i, j) && pc < parenPos.size() && j < s.size() && parenPos[pc] == j) {
                ++j;
                ++pc;
            }
            if(s[j] == '(') {
                ++open;
            } else if(s[j] == ')') {
                --open;
            }
            if(open < 0) {
                return false;
            }
            res[chars++] = s[j];
        }
        if(open != 0) {
            return false;
        }

        res.resize(chars);
        return true;
    }



    vector<string> removeInvalidParentheses(string s) {
        vector<int> parenPos;
        for(int i = 0; i < s.size(); ++i) {
            if(s[i] == '(' || s[i] == ')') {
                parenPos.push_back(i);
            }
        }
        string res;
        if(test(0, s, parenPos, res)) {
            cout << "0" << endl;
            return {s};
        }
        vector<int> poss;
        for(int i = 0; i < (1 << parenPos.size()); ++i) {
            poss.push_back(i);
        }

        auto comparator = [](int a, int b) { return __builtin_popcount(a) < __builtin_popcount(b); };
        sort(poss.begin(), poss.end(), comparator);
        int mn = -1;
        set<string> sols;
        for(int i = 0; i < poss.size(); ++i) {
            string res(s.size(), 0);
            if(mn != -1 && __builtin_popcount(poss[i]) > mn) {
                break;
            } else if(test(i, s, parenPos, res)) {
                cout << bitset<25>(poss[i]) << " - y - " << res << endl;
                mn = __builtin_popcount(poss[i]);
                sols.insert(res);
            } else {
                cout << bitset<25>(poss[i]) << " - n - " << res << endl;
            }
        }

        return vector<string>(sols.begin(), sols.end());

    }
};
