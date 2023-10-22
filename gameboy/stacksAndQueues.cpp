//
// Created by jc on 08/10/23.
//

#include "stacksAndQueues.h"

#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <bitset>
#include <algorithm>
#include <cmath>
#include <array>

using namespace std;

#include <stdio.h>
#include <cstdlib>
#include <vector>
#include <deque>
#include <map>
#include <cstdint>
#include <cassert>
#include <list>
#include <cmath>

/*
    OrderBook is a level 3 orderbook.  Please fill out the stub below.
*/

using u64 = uint64_t;
using i64 = int64_t;

i64 PRECISION = 1e9;

struct Order {
    const int orderId;
    const char side;
    int size;
    i64 priceL;

    Order(int orderId, char side, int size, i64 price) : orderId{orderId}, side{side}, size{size}, priceL{price} {}


};

using OrderId = int;


using L2 = map<OrderId, Order>;
using SideLevels = list<L2>;

struct OrderIdLoc {
    char side;
    SideLevels::iterator l2;
    L2::iterator loc;

    OrderIdLoc(char side, SideLevels ::iterator l2, L2::iterator loc) : side{side}, l2{l2}, loc{loc} {
    }

};

class OrderBook {
    map<OrderId, OrderIdLoc> orderIdMap;
    SideLevels bidLevels;
    SideLevels askLevels;

public:

    OrderBook() : bidLevels(), askLevels(), orderIdMap() {
    }

    static i64 getPriceL(const L2 &bids) {
        assert(!bids.empty());
        return bids.begin()->second.priceL;
    }

    static double getPriceD(const L2 &bids) {
        assert(!bids.empty());
        return double(bids.begin()->second.priceL) / double(PRECISION);
    }

    static bool isAggressiveOrder(i64 price, SideLevels &levels, char side) {
        int dir = side == 'b' ? 1 : -1;
        return !levels.empty() && (getPriceL(*levels.begin()) - price) * dir < 0;
    }

    static i64 normPrice(double price) {
        return i64(PRECISION * price);
    }

    bool isBid(char side) {
        assert(side == 'b' || side == 's');
        return side == 'b';
    }

    void insertLevel(int orderId, char side, int size, i64 priceL) {

        SideLevels &levels = side == 'b' ? bidLevels : askLevels;

        auto compare = [side](i64 a, i64 b) {
            if (side == 'b') {
                return a > b;
            } else {
                return a < b;
            }
        };
        auto iter = levels.begin();
        for (; iter != levels.end(); ++iter) {
            i64 curPrice = getPriceL(*iter);
            if (compare(priceL, curPrice)) {
                L2 mp{{orderId, Order{orderId, side, size, priceL}}};

                auto insertPos = levels.emplace(iter, std::move(mp));
                auto res = insertPos->find(orderId);
                orderIdMap.emplace(orderId, OrderIdLoc{side, iter, res});
                return;
            } else if (priceL == curPrice) {
                auto res = iter->emplace(orderId, Order{orderId, side, size, priceL});
                if (!res.second) {
                    assert(false);
                }
                orderIdMap.emplace(orderId, OrderIdLoc{side, iter, res.first});
                return;
            }
        }

        assert(iter == levels.end());
        L2 mp{{orderId, Order{orderId, side, size, priceL}}};
        const SideLevels::iterator &insertPos = levels.emplace(iter, mp);
        const auto &res = insertPos->find(orderId);
        orderIdMap.emplace(orderId, OrderIdLoc{side, insertPos, res});

    }

    //adds order to order book
    void newOrder(int orderId, char side, int size, double priceD) {
        SideLevels &levelsOpp = getOppSideLevels(side);
        i64 priceL = normPrice(priceD);
        if (isAggressiveOrder(priceL, levelsOpp, side)) {
            assert(false);
        } else {
            insertLevel(orderId, side, size, priceL);
        }
    }

    void modifyOrder(int oldOrderId, int orderId, char side, int newSize, double newPrice) {
        deleteOrder(oldOrderId);
        newOrder(orderId, side, newSize, newPrice);
    }

    //replaces order with different order
    void reduceOrder(int orderId, int size) {
        const auto &elem = orderIdMap.find(orderId);
        assert(elem != orderIdMap.end());
        auto &pos = elem->second;
        assert(pos.loc->second.size >= size);
        pos.loc->second.size = size;
        if (pos.loc->second.size <= 0) {
            deleteOrder(orderId);
        }
    }

    //deletes order from orderbook
    void deleteOrder(int orderId) {
        const auto &elem = orderIdMap.find(orderId);
        assert(elem != orderIdMap.end());
        // TODO - clean up the order id map as well.
        // clean up the level as well if the row is empty;
        auto &l2 = elem->second.l2;
        char side = l2->begin()->second.side;
        l2->erase(elem->second.loc);
        if(l2->empty()) {
            getSideLevels(elem->second.side).erase(l2);
        }
    }


    //returns the number of levels on the side
    int getNumLevels(char side) {
        return int(getSideLevels(side).size());
    }

    //returns the price of a level.  Level 0 is top of book.
    double getLevelPrice(char side, int level) {

        SideLevels &levels = getSideLevels(side);
        auto iter = levels.begin();
        for(int i = 0; iter != levels.end() && i < level; ++i, ++iter);
        if(iter == levels.end()) {
            return std::nan("");
        } else {
            return getPriceD(*iter);
        }
    }

    //returns the size of a level.
    int getLevelSize(char side, int level) {
        // TODO - optimize this.
        SideLevels &levels = getSideLevels(side);
        auto iter = levels.begin();
        for(int i = 0; iter != levels.end() && i < level; ++i, ++iter);
        if(iter == levels.end()) {
            return std::nan("");
        }
        const auto &mp = *iter;
        int res = 0;
        for (const auto &e: mp) {
            res += e.second.size;
        }
        return res;
    }

    //returns the number of orders contained in price level.
    int getLevelOrderCount(char side, int level) {
        SideLevels &levels = getSideLevels(side);
        auto iter = levels.begin();
        for(int i = 0; iter != levels.end() && i < level; ++i, ++iter);
        if(iter == levels.end()) {
            return std::nan("");
        }
        return int(iter->size());
    }

    SideLevels &getSideLevels(char side) {
        return side == 'b' ? bidLevels : askLevels;
    }

    SideLevels &getOppSideLevels(char side) {
        return side == 'b' ? askLevels : bidLevels;
    }
};

/*
    Do not change main function
*/

int main(int argc, char *argv[]) {
    char instruction;
    char side;
    int orderId;
    double price;
    int size;
    int oldOrderId;

//    FILE *file = fopen(std::getenv("OUTPUT_PATH"), "w");
    FILE *file = fopen("/home/jc/CLionProjects/gb_emulator/data/output.txt", "w");

    OrderBook book;
    int counter = 0;
    while (scanf("%c %c %d %d %lf %d\n", &instruction, &side, &orderId, &size, &price, &oldOrderId) != EOF) {
        ++counter;
        switch (instruction) {
            case 'n':
                book.newOrder(orderId, side, size, price);
                break;
            case 'r':
                book.reduceOrder(orderId, size);
                break;
            case 'm':
                book.modifyOrder(oldOrderId, orderId, side, size, price);
                break;
            case 'd':
                book.deleteOrder(orderId);
                break;
            case 'p':
                fprintf(file, "%.02lf\n", book.getLevelPrice(side, orderId));
                break;
            case 's':
                fprintf(file, "%d\n", book.getLevelSize(side, orderId));
                break;
            case 'l':
                fprintf(file, "%d\n", book.getNumLevels(side));
                break;
            case 'c':
                fprintf(file, "%d\n", book.getLevelOrderCount(side, orderId));
                break;
            default:
                fprintf(file, "invalid input\n");
        }
    }
    fclose(file);
    return 0;
}