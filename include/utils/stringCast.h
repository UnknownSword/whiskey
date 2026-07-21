#pragma once

#include <string>
#include <stdexcept>
using namespace std;

template<typename T>
T string_cast(string s);

template<>
inline string string_cast(string s){
	return s;
}

template<>
inline int string_cast(string s){
	return stoi(s);
}

template<>
inline long string_cast(string s){
	return stol(s);
}

template<>
inline long long string_cast(string s){
	return stoll(s);
}

template<>
inline unsigned long string_cast(string s){
	return stoul(s);
}

template<>
inline unsigned long long string_cast(string s){
	return stoull(s);
}

template<>
inline float string_cast(string s){
	return stof(s);
}

template<>
inline double string_cast(string s){
	return stod(s);
}

template<>
inline long double string_cast(string s){
	return stold(s);
}

template<>
inline bool string_cast(string s){
	if(s == "true" || s == "1" || s == "yes") return true;
	if(s == "false" || s == "0" || s == "no") return false;
	throw invalid_argument("string_cast<bool>: invalid string \"" + s + "\"");
}

template<>
inline char string_cast(string s){
	if(s.size() != 1)
		throw invalid_argument("string_cast<char>: string must be exactly 1 character");
	return s[0];
}
