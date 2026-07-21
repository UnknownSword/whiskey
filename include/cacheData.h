#pragma once

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <functional>
#include <cctype>
#include <utils/stringCast.h>
using namespace std;


struct CacheData{
	// name of the Cache
	string name;
	// map child name to child Cache
	map<string, CacheData> childs;
	// map value name to value
	map<string, string> values;
	
	CacheData() = default;
	CacheData(string file_path){ cache_get(file_path); };
	
	void clear(){
		name.clear();
		childs.clear();
		values.clear();
	}
	
	void set_name(string new_name){
		name = new_name;
	}
	bool contains_child(string name){
		return childs.contains(name);
	}
	bool contains_value(string name){
		return values.contains(name);
	}
	CacheData& get_child(string child_name){
		return childs[child_name];
	}
	
	template<typename T>
	void set_value(string key, T val){
		values[key] = to_string(val);
	}
	void set_value(string key, string val){
		values[key] = val;
	}
	void set_value(string key, const char* val){
		values[key] = string(val);
	}
	
	template<typename T>
	T get_value(string name){
		return string_cast<T>(values[name]);
	}
	
	void cache_get(string file_path){
		FILE* file = fopen(file_path.c_str(), "r");
		if(file == NULL){
			return ;
		}
		clear();
		
		fseek(file, 0, SEEK_END);
		long file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		string content(file_size, '\0');
		fread(content.data(), 1, file_size, file);
		fclose(file);
		
		size_t pos = 0;
		size_t len = content.size();
		
		auto skip_ws = [&](){
			while(pos < len && isspace(content[pos]))
				pos++;
		};
		
		auto read_quoted = [&]() -> string {
			skip_ws();
			if(pos >= len || content[pos] != '"') return "";
			pos++;
			string s;
			while(pos < len && content[pos] != '"')
				s += content[pos++];
			if(pos < len) pos++;
			return s;
		};
		
		auto read_value = [&]() -> string {
			skip_ws();
			string s;
			while(pos < len && content[pos] != ';')
				s += content[pos++];
			while(!s.empty() && isspace(s.back()))
				s.pop_back();
			if(pos < len) pos++;
			return s;
		};
		
		function<void(CacheData&, CacheData&)> parse_block =
			[&](CacheData& target, CacheData& scope){
			while(pos < len){
				skip_ws();
				if(pos >= len || content[pos] == '}') break;
				
				if(content[pos] == '('){
					pos++;
					string child_name = read_quoted();
					skip_ws();
					if(pos >= len) break;
					
					if(content[pos] == ')'){
						pos++;
						skip_ws();
						if(pos < len && content[pos] == '{'){
							pos++;
							CacheData child;
							child.name = child_name;
							parse_block(child, scope);
							target.childs[child_name] = child;
							skip_ws();
							if(pos < len && content[pos] == '}') pos++;
						}else if(pos < len && content[pos] == ';'){
							pos++;
							if(scope.childs.contains(child_name))
								target.childs[child_name] = scope.childs[child_name];
						}
					}
				}else if(content[pos] == '['){
					pos++;
					string key = read_quoted();
					skip_ws();
					if(pos < len && content[pos] == ']') pos++;
					skip_ws();
					if(pos < len && content[pos] == '=') pos++;
					skip_ws();
					string val = read_value();
					if(val == "main")
						target.name = val;
					else
						target.values[key] = val;
				}else{
					pos++;
				}
			}
		};
		
		parse_block(*this, *this);
	}
	
	void cache_put(string file_path){
		FILE* file = fopen(file_path.c_str(), "w");
		if(file == NULL) return;
		cache_write(file, 0);
		fclose(file);
	}
	
	void cache_write(FILE* file, int indent){
		string tab(indent, '\t');
		for(auto& [k, v] : values)
			fprintf(file, "%s[\"%s\"] = %s;\n", tab.c_str(), k.c_str(), v.c_str());
		
		for(auto& [child_name, child] : childs){
			fprintf(file, "%s(\"%s\"){\n", tab.c_str(), child_name.c_str());
			child.cache_write(file, indent + 1);
			fprintf(file, "%s}\n", tab.c_str());
		}
	}
	
};


