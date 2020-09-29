#ifndef _JSON_OPERATOR_H__
#define _JSON_OPERATOR_H__

#include <string>
#include "rapidjson/document.h"

namespace jsop
{
	bool has_path_error();
}

#define JVAL rapidjson::Value
#define JDOC rapidjson::Document

/// path operator which is from divide operator, implement json path pointer, but focus to query value
/// return a reference to sub-tree of the current json value, or
/// return a static null json value if occur path error.
/// example:
///   for json root = {"subname": {"subsub": ??, ...}, "subarray": [0, {"value": ??, ...}, ...]}
///   sub_tree = json_root / "subname" / "subsub";
///   sub_val  = json_root / "subarray" / 1 / "value";
///   sub_tree = json_root / "subname/subsub";
///   sub_val  = json_root / "subarray/1/value";
const JVAL& operator/ (const JVAL& json, const std::string& path);
const JVAL& operator/ (const JVAL& json, const char* path);
const JVAL& operator/ (const JVAL& json, size_t index);
const JVAL& operator/ (const JVAL& json, int index);

inline
JVAL& operator/ (JVAL& json, const std::string& path)
{
	return const_cast<JVAL&>(const_cast<const JVAL&>(json) / path);
}
inline
JVAL& operator/ (JVAL& json, const char* path)
{
	return const_cast<JVAL&>(const_cast<const JVAL&>(json) / path);
}
inline
JVAL& operator/ (JVAL& json, size_t index)
{
	return const_cast<JVAL&>(const_cast<const JVAL&>(json) / index);
}
inline
JVAL& operator/ (JVAL& json, int index)
{
	return const_cast<JVAL&>(const_cast<const JVAL&>(json) / index);
}

/// conversion operator which is from bit or operator(|), conver a generic scalar json value to c++ value
/// mainly int or doule or string.
/// example:
///   int i = json | 0;
///   int d = json | 0.0;
///   std::string s = json | "";
#define AS_INT | 0
#define AS_DOUBLE | 0.0
#define AS_STRING | ""
int operator|(const JVAL& json, int defval);
double operator|(const JVAL& json, double defval);
std::string operator|(const JVAL& json, const char* defval);
std::string operator|(const JVAL& json, const std::string& defval);

inline
bool operator! (const JVAL& json)
{
	return json.IsNull();
}

#endif // !_JSON_OPERATOR_H__

