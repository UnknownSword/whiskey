#ifndef UTILS_RANDOM_H
#define UTILS_RANDOM_H

#include <random>
#include <chrono>
#include <cstdint>

inline std::mt19937& getRng() {
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    return rng;
}

template<typename T>
T random(T min, T max) {
    if constexpr (std::is_integral_v<T>) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(getRng());
    } else {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(getRng());
    }
}

inline uint64_t random_seed(uint64_t seed){
	uint64_t ret = seed;
	
	static vector<uint64_t> primes = {
		0x94f,
		0x955,
		0x959,	
		0xf6323,
		0xf6325,
		0x98a011,
		0x98a01f,
		0x3b9aca07,
		0x3b9aca09,
		0x3b9aca15,
		0x2540bed63,
		0x2540bed67,
		0xe8d4a53095,
		0xe8d4a530af,
		0x5af3107a638f,
		0x5af3107a63a7,
		0x5af3107a63c1,
		0x5af3107a63df,
		0x38d7ea4c6a10d,
		0x38d7ea4c6a115,
		0x16345785d8a2083,
		0x16345785d8a20d3,
		0x16345785d8a20ef,
		0x16345785d8a20fb
	};
	for(int i=0 ; i < primes.size() ; i++){
		ret += primes[(ret + i) % primes.size()];
		ret ^= primes[(ret * i) % primes.size()];
		ret += primes[(primes[ret % primes.size()] ^ i) % primes.size()];
		ret << ( ret & 3 );
	}
	for(uint64_t i : primes){
		ret += i;
		ret *= primes[i % primes.size()];
		ret += i * i ;
		ret ^= i;
	}
	return ret;
}

inline uint64_t random_seed(const string& s){
	uint64_t ret = 0x2383;
	for(uint64_t i=0 ; i < s.size() ; i++){
		ret ^= random_seed(s[i]) * random_seed(i);
		ret += random_seed(s[i]) * random_seed(i*i);
	}
	return ret;
}

#endif /* UTILS_RANDOM_H */
