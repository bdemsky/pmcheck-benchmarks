#include <iostream>
#include <chrono>
#include <random>
#include <thread>
#include <assert.h>
using namespace std;
#include "Tree.h"


extern "C" {
    void * getRegionFromID(uint ID);
    void setRegionFromID(uint ID, void *ptr);
}

typedef struct thread_data {
    uint32_t id;
    ART_ROWEX::Tree *tree;
} thread_data_t;


void loadKey(TID tid, Key &key) {
    return ;
}

ART_ROWEX::Tree *tree = NULL;
Key ** Keys = NULL;
uint64_t *counters = NULL;

void run(char **argv) {
    std::cout << "Simple Example of P-ART" << std::endl;
    uint64_t n = std::atoll(argv[1]);
    uint64_t *keys = new uint64_t[n];

    // Generate keys
    for (uint64_t i = 0; i < n; i++) {
        keys[i] = i + 1;
    }

    int num_thread = atoi(argv[2]);
    if(getRegionFromID(0) != NULL && getRegionFromID(1) != NULL && getRegionFromID(2) != NULL) {
        tree = (ART_ROWEX::Tree*)getRegionFromID(0);
        tree->recoverFromCrash();
        assert(tree);
        Keys = (Key **) getRegionFromID(1);
        assert(Keys);
        counters = (uint64_t *) getRegionFromID(2);
        assert(counters);
    } else {
        tree = new ART_ROWEX::Tree(loadKey, num_thread);
        setRegionFromID(0, tree);
        Keys = new Key*[n];
        for(uint64_t i=0; i< n; i++){
#ifdef BUGFIX
            PMCHECK::clflush((char*)&Keys[i], sizeof(Key), false, true);//b1
#endif
            Keys[i] = Key::make_leaf(keys[i], sizeof(uint64_t), keys[i]);
#ifndef BUGFIX
            PMCHECK::clflush((char*)&Keys[i], sizeof(Key), false, true);//b1
#endif
        }
        PMCHECK::clflush((char*)Keys, sizeof(Key*)*n, false, true);
        setRegionFromID(1, Keys);
        //Make sure counters and hashtable aren't in the same line:
        // 64 bytes + n*sizeof(uint64_t) + 64 bytes.
        counters = (uint64_t *)calloc(n + 16, sizeof(uint64_t));
        counters = &counters[8];
        PMCHECK::clflush((char*)counters, sizeof(uint64_t)*n, false, true);
        setRegionFromID(2, counters);
    }

    thread_data_t *tds = (thread_data_t *) calloc(num_thread, sizeof(thread_data_t));
    std::atomic<int> next_thread_id(0);

    {
        // Build tree
        auto starttime = std::chrono::system_clock::now();
        auto func = [&]() {
            int thread_id = next_thread_id.fetch_add(1);
            tds[thread_id].id = thread_id;
            tds[thread_id].tree = tree;

            uint64_t start_key = n / num_thread * (uint64_t)thread_id;
            uint64_t end_key = start_key + n / num_thread;
            auto t = tree->getThreadInfo(thread_id);

            uint64_t index = start_key;
            // First read to see the actual values made out to the memory
            for (; index < start_key + counters[thread_id]; index++) {
                uint64_t *val = reinterpret_cast<uint64_t *> (tree->lookup(Keys[index], t));
                if (val == NULL || *val != keys[index]) {
                    uint64_t readVal = val == NULL? (uint64_t) -1 : *val;
                    std::cout << "wrong value read: " << readVal << " expected:" << keys[index] << std::endl;
                    //This write did not make out to the memory. So, we need to start inserting from here ... 
                    break;
                }
            }

            if (index < end_key) {
              uint64_t *val = reinterpret_cast<uint64_t *> (tree->lookup(Keys[index], t));
              if (val != NULL)
                index++;
            }

            
            for (uint64_t i = index; i < end_key; i++) {
                tree->insert(Keys[i], t);
                counters[thread_id]++;
                PMCHECK::clflush((char*)&counters[thread_id], sizeof(counters[thread_id]), false, true);
            }
        };
        std::vector<std::thread> thread_group;

        for (int i = 0; i < num_thread; i++)
            thread_group.push_back(std::thread{func});

        for (int i = 0; i < num_thread; i++)
            thread_group[i].join();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - starttime);
        printf("Throughput: insert,%ld,%f ops/us\n", n, (n * 1.0) / duration.count());
        printf("Elapsed time: insert,%ld,%f sec\n", n, duration.count() / 1000000.0);
    }

    delete[] keys;
}


int main(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: %s [n] [nthreads]\nn: number of keys (integer)\nnthreads: number of threads (integer)\n", argv[0]);
        return 1;
    }
    run(argv);
    return 0;
}

