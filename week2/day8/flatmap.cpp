#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <chrono>
#include <random>



template <typename K, typename V>
class FlatMap{
public:
    void insert (const K& key, const V& value);
    V* find(const K& key);      // returns a pointer to value or nullptr when not found.
    const V* find(const K& key) const;
    void erase(const K& key);
    V& operator[](const K& key);
    size_t size() const;

private:
    std::vector<std::pair<K, V>> data_;

};

//find function.
template <typename K, typename V>
V* FlatMap<K,V>::find(const K& key){
    // The comparator: "given one stored pair and the key I'm searching for, 
    //is the pair's key less than the search key?"
    auto comp = [](const std::pair<K, V>& element, const K& k){
        return element.first < k;
    };

    //lower_bound parameters: first where to search, then what you're looking for, then how to compare.
    auto ite = std::lower_bound(data_.begin(), data_.end(), key, comp);

    if(ite != data_.end() && ite->first == key){
        return &(ite->second);
    }

    return nullptr;
    
}
template <typename K, typename V>
const V* FlatMap<K,V>::find(const K& key) const{
    // The comparator: "given one stored pair and the key I'm searching for, 
    //is the pair's key less than the search key?"
    auto comp = [](const std::pair<K, V>& element, const K& k){
        return element.first < k;
    };

    //lower_bound parameters: first where to search, then what you're looking for, then how to compare.
    auto ite = std::lower_bound(data_.begin(), data_.end(), key, comp);

    if(ite != data_.end() && ite->first == key){
        return &(ite->second);
    }

    return nullptr;
    
}

//insert function
template <typename K, typename V>
void FlatMap<K,V>::insert (const K& key, const V& value){
    auto comp = [](const std::pair<K, V>& element, const K& k){
        return element.first < k;
    };

    auto ite = std::lower_bound(data_.begin(), data_.end(), key, comp);

    if(ite != data_.end() && ite->first == key){
        ite->second = value;
    }
    else{
        data_.insert(ite, {key, value});
    }
}

//erase function.
template <typename K, typename V>
void FlatMap<K,V>::erase(const K& key){
    
    auto comp = [](const std::pair<K,V>& element, const K& k){
        return element.first < k;
    };

    auto ite = std::lower_bound(data_.begin(), data_.end(), key, comp);

    if(ite != data_.end() && ite->first == key){
        data_.erase(ite);
    }

}

// operator[] function.
template<typename K, typename V>
V& FlatMap<K,V>::operator[](const K& key){
    auto comp = [](const std::pair<K,V>& element, const K& k){
        return element.first < k;
    };

    auto ite = std::lower_bound(data_.begin(), data_.end(), key, comp);

    if(ite != data_.end() && ite->first == key){
        return ite->second;
    }
    else{
        auto new_ite = data_.insert(ite, {key, V{}});
        return new_ite->second;
    }
}

//size function.
template<typename K, typename V>
size_t FlatMap<K,V>::size() const{
    return data_.size();
}

//Things to understand:
/*
volatile long sink - when calling find(), prevents an aggresive compiler from deleting the entire loop as 'useless work'
in an attempt to optimize it making the benchmark measure nothing.

Random lookup order, separate from insertion order - looking up keys in the same order they were inserted
creates artificial cache-friendly access for the tree/hash structures. random lookup is better for real workload.

Warm-up pass before timing - The first time memory is touched we pay costs(page faults/cold caches) that aren't related to the
algorithm. Running it once before gives a fairer number.

FiXed RNG seed(42) - makes it reproducible, if we get a good or bad number we can rerun and get the same shuffle to double check.

*/

int main(){


    std::vector<size_t> sizes = {100, 10000, 1000000};

    for(size_t n: sizes){
        std::vector<int> keys(n);
        for(size_t i = 0; i < n; ++i){
            keys[i] = static_cast<int>(i);
        }

        std::mt19937 rng(42); //fixed seed, to reproduce run.
        std::shuffle(keys.begin(), keys.end(), rng);

        FlatMap<int, int> flat;
        std::map<int, int> tree;
        std::unordered_map<int, int> hash;

        for(int k: keys){
            flat.insert(k, k);
            tree[k] = k;
            hash[k] = k;
        }

        //build random lookup order, separate from insertion order.
        std::vector<int> lookups = keys;
        std::shuffle(lookups.begin(), lookups.end(), rng);

        volatile int64_t sink = 0; // prevents compiler from optimizing loop away.

        //warm-up pass.
        for(int k: lookups){
            sink += *flat.find(k);
        }

        auto t0 = std::chrono::steady_clock::now();
        for(int k: lookups){
            sink += *flat.find(k);
        }
        
        auto t1 = std::chrono::steady_clock::now();
        for(int k: lookups){
            sink += tree.find(k)->second;
        }

        auto t2 = std::chrono::steady_clock::now();
        for(int k: lookups){
            sink += hash.find(k)->second;
        }

        auto t3 = std::chrono::steady_clock::now();

        

        std::chrono::duration<double, std::milli> flat_ms = t1-t0;
        std::chrono::duration<double, std::milli> tree_ms = t2-t1;
        std::chrono::duration<double, std::milli> hash_ms = t3-t2;


        std::cout << " n = " << n << "\n"
                  << " FlatMap = " << flat_ms.count() << " ms" << "\n"
                  << " Map = " << tree_ms.count() << " ms" << "\n"
                  << " Unordered Map = " << hash_ms.count() << " ms" << "\n"
                  << " sink = "  << sink << "\n";
    }
}   


/*
RESULT:
 n = 100
 FlatMap = 0 ms
 Map = 0 ms
 Unordered Map = 0 ms
 sink = 19800
 n = 10000
 FlatMap = 1.004 ms
 Map = 0.996 ms
 Unordered Map = 0 ms
 sink = 199980000
 n = 1000000
 FlatMap = 119 ms
 Map = 520.564 ms
 Unordered Map = 49.509 ms
 sink = -1456759936

*/