#ifndef JSON_CONVERTOR_H__
#define JSON_CONVERTOR_H__

#include <string>
#include <vector>
#include <sstream>

#include "rapidjson/document.h"

#define BEGIN_NAMESPACE(ns) namespace ns {
#define END_NAMESPACE(ns) }

#define DELETE_OBJECT(p) if ((p)) { delete (p); (p) = NULL; } do {} while (0)
#define CALL_BACK(path, ok, msg, val) if (callback) { callback(path, ok, msg, val); } do {} while (0)

#define LOADJSON_ARGUMENT_HPP const rapidjson::Value& json, const char* path = NULL, funLoadJsonCB callback = NULL
#define SAVEJSON_ARGUMENT_HPP rapidjson::Value& json, rapidjson::Document::AllocatorType& allocator, const char* path = NULL, funSaveJsonCB callback = NULL
#define LOADJSON_ARGUMENT_CPP const rapidjson::Value& json, const char* path, funLoadJsonCB callback
#define SAVEJSON_ARGUMENT_CPP rapidjson::Value& json, rapidjson::Document::AllocatorType& allocator, const char* path, funSaveJsonCB callback

// simplify definition os jstruct, define default constructor, disable copy/= constructor, define Load/Save Json methods.
#define END_JSTRUCT(st) \
	st();\
	~st();\
	void LoadJson(LOADJSON_ARGUMENT_HPP);\
	void SaveJson(SAVEJSON_ARGUMENT_HPP);\
	private:\
	st& operator=(const st&);\
 
// fix for some gcc version
// st(const st&);\

// simlify the implemetation for Load/Save Json method. Note the argument name must match.
#define EXTRACT_JSON_VALUE(field) extract_json_value(field, json, path, #field, callback);
#define EXTRACT_JSON_OBJECT(field) extract_json_value(&field, json, path, #field, callback);
#define EXTRACT_JSON_VALUE2(field, key) extract_json_value(field, json, path, key, callback);
#define EXTRACT_JSON_OBJECT2(field, key) extract_json_value(&field, json, path, key, callback);
#define INJECT_JSON_VALUE(field) inject_json_value(field, json, path, #field, allocator, callback);
#define INJECT_JSON_VALUE2(field, key) inject_json_value(field, json, path, key, allocator, callback);

const int SHORT_STRING_BUFF = 32;

typedef void (*funLoadJsonCB) (const char* path, bool ok, const char* msg, const char* val);
typedef void (*funSaveJsonCB) (const char* path, bool ok, const char* msg, const char* val);

inline const char* named_basic_type(const std::string& val) { return "string"; }
inline const char* named_basic_type(const int& val) { return "int"; }
inline const char* named_basic_type(const double& val) { return "double"; }
inline const char* named_basic_type(const bool& val) { return "bool"; }

template <typename T>
std::string stringfy_basic_value(const T& val)
{
	std::stringstream ss;
	ss << val;
	return ss.str();
}

bool convert_json_value(std::string& dest, const rapidjson::Value& json);
bool convert_json_value(int& dest, const rapidjson::Value& json);
bool convert_json_value(double& dest, const rapidjson::Value& json);
bool convert_json_value(bool& dest, const rapidjson::Value& json);

// check if json is object and has specific field.
// return ture if check passed and output the joined full path.
bool pre_check_extract(std::string& joinPath, const rapidjson::Value& json, const char* path, const char* field, funLoadJsonCB callback = NULL);

/*
 extract a scalar json value to cpp value, such as int double string bool
 \param dest, cpp value
 \param json, a json object
 \param path, the path of json node in its dmo, NULL for root
 \param field, json[field] will extract value to dest
 \param callback, when success or fial to extract value, can call a function to
        do something such as log.
*/
template <typename T>
void extract_json_value(T& dest, const rapidjson::Value& json, const char* path, const char* field, funLoadJsonCB callback = NULL)
{
	std::string strPath;
	if (!pre_check_extract(strPath, json, path, field, callback))
	{
		return;
	}

	const rapidjson::Value& child = json[field];
	if (!convert_json_value(dest, child))
	{
		if (callback)
		{
			std::string msg("cannot convert to expected type: ");
			msg += named_basic_type(dest);
			callback(strPath.c_str(), false, msg.c_str(), NULL);
		}
		return;
	}

	if (callback)
	{
		std::string sval = stringfy_basic_value(dest);
		callback(strPath.c_str(), true, "OK", sval.c_str());
	}
	return;
}

/* extract a json object to cpp struct
 * will call new T, require default constructor, 
 * and need LoadJson method to recursive fill content of T.
 * other parameters are similar as the previous function for scalar json value
 * */
template <typename T>
void extract_json_value(T** dest, const rapidjson::Value& json, const char* path, const char* field, funLoadJsonCB callback = NULL)
{
	if (NULL == dest)
	{
		CALL_BACK(path, false, "invalid dest pointer", NULL);
		return;
	}

	std::string strPath;
	if (!pre_check_extract(strPath, json, path, field, callback))
	{
		return;
	}

	const rapidjson::Value& child = json[field];
	if (!child.IsObject())
	{
		CALL_BACK(strPath.c_str(), false, "not an object", NULL);
		return;
	}

	*dest = new T;
	if (NULL == *dest)
	{
		CALL_BACK(strPath.c_str(), false, "fail to crate new object", NULL);
		return;
	}

	(*dest)->LoadJson(child, strPath.c_str(), callback);
	CALL_BACK(strPath.c_str(), true, "OK", "{object}");
	return;
}

/* extract a json array to cpp std::vector for each item is scalar value type.
 * */
template <typename T>
void extract_json_value(std::vector<T>& dest, const rapidjson::Value& json, const char* path, const char* field, funLoadJsonCB callback = NULL)
{
	std::string strPath;
	if (!pre_check_extract(strPath, json, path, field, callback))
	{
		return;
	}

	const rapidjson::Value& child = json[field];
	if (!child.IsArray())
	{
		CALL_BACK(strPath.c_str(), false, "not an array", NULL);
		return;
	}

	size_t length = child.Size();
	char buffer[SHORT_STRING_BUFF] = {0};
	for (size_t i = 0; i < length; ++i)
	{
		T item;
		snprintf(buffer, SHORT_STRING_BUFF, "/%d", i);
		std::string itemPath = strPath + buffer;
		if (!convert_json_value(item, child[i]))
		{
			if (callback)
			{
				std::string msg("cannot convert to expected type: ");
				msg += named_basic_type(item);
				callback(itemPath.c_str(), false, msg.c_str(), NULL);
			}
		}
		else
		{
			if (callback)
			{
				std::string sval = stringfy_basic_value(item);
				callback(itemPath.c_str(), true, "OK", sval.c_str());
			}
		}
		dest.push_back(item);
	}

	snprintf(buffer, SHORT_STRING_BUFF, "[%d items]", length);
	CALL_BACK(strPath.c_str(), true, "OK", buffer);
	return;
}

/* extract a json array to cpp std::vector for each item is another object
 * */
template <typename T>
void extract_json_value(std::vector<T*>& dest, const rapidjson::Value& json, const char* path, const char* field, funLoadJsonCB callback = NULL)
{
	std::string strPath;
	if (!pre_check_extract(strPath, json, path, field, callback))
	{
		return;
	}

	const rapidjson::Value& child = json[field];
	if (!child.IsArray())
	{
		CALL_BACK(strPath.c_str(), false, "not an array", NULL);
		return;
	}

	size_t length = child.Size();
	char buffer[SHORT_STRING_BUFF] = {0};
	for (size_t i = 0; i < length; ++i)
	{
		T* item = new T;
		snprintf(buffer, SHORT_STRING_BUFF, "/%d", i);
		std::string itemPath = strPath + buffer;
		if (NULL == item)
		{
			CALL_BACK(itemPath.c_str(), false, "cannot create new item object", NULL);
		}
		else
		{
			CALL_BACK(itemPath.c_str(), true, "OK", "{object}");
		}
		item->LoadJson(child[i], itemPath.c_str(), callback);
		dest.push_back(item);
	}

	snprintf(buffer, SHORT_STRING_BUFF, "[%d items]", length);
	CALL_BACK(strPath.c_str(), true, "OK", buffer);
	return;
}

template <typename T>
void store_json_value(const T& src, rapidjson::Value& json, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value val(src);
	json = val;
}
void store_json_value(const std::string& src, rapidjson::Value& json, rapidjson::Document::AllocatorType& allocator);

bool pre_check_inject(std::string& joinPath, const rapidjson::Value& json, const char* path, const char* field, funSaveJsonCB callback = NULL);

/* insert a scalar value src to object that will refer as json[field]
 * T is simple type int/double/value that do not require allocate more memory.
 */
template <typename T>
void inject_json_value(T& src, rapidjson::Value& json, const char* path, const char* field, rapidjson::Document::AllocatorType& allocator, funSaveJsonCB callback = NULL)
{
	std::string strPath;
	if (!pre_check_inject(strPath, json, path, field, callback))
	{
		return;
	}

	rapidjson::Value key;
	key.SetString(field, allocator);

	rapidjson::Value val;
	store_json_value(src, val, allocator);
	json.AddMember(key, val, allocator);

	if (callback)
	{
		std::string sval = stringfy_basic_value(src);
		callback(strPath.c_str(), true, "OK", sval.c_str());
	}
	return;
}

// insert a object T to json[feild]
template <typename T>
void inject_json_value(T* src, rapidjson::Value& json, const char* path, const char* field, rapidjson::Document::AllocatorType& allocator, funSaveJsonCB callback = NULL)
{
	if (NULL == src)
	{
		CALL_BACK(path, false, "ignore NULL object pointer", NULL);
		return;
	}

	std::string strPath;
	if (!pre_check_inject(strPath, json, path, field, callback))
	{
		return;
	}

	rapidjson::Value key;
	key.SetString(field, allocator);

	rapidjson::Value val;
	val.SetObject();
	src->SaveJson(val, allocator, strPath.c_str(), callback);
	json.AddMember(key, val, allocator);

	CALL_BACK(strPath.c_str(), true, "OK", "{object}");

	return;
}

// insert a object T to json[feild]
template <typename T>
void inject_json_value(std::vector<T>& src, rapidjson::Value& json, const char* path, const char* field, rapidjson::Document::AllocatorType& allocator, funSaveJsonCB callback = NULL)
{
	if (src.empty())
	{
		CALL_BACK(path, false, "ignore empty array", NULL);
		return;
	}

	std::string strPath;
	if (!pre_check_inject(strPath, json, path, field, callback))
	{
		return;
	}

	rapidjson::Value key;
	rapidjson::Value val;
	key.SetString(field, allocator);
	val.SetArray();

	size_t length = src.size();
	char buffer[SHORT_STRING_BUFF] = {0};
	for (size_t i = 0; i < length; ++i)
	{
		rapidjson::Value item;
		store_json_value(src[i], item, allocator);
		val.PushBack(item, allocator);
		if (callback)
		{
			snprintf(buffer, SHORT_STRING_BUFF, "/%d", i);
			std::string itemPath = strPath + buffer;
			std::string sval = stringfy_basic_value(src[i]);
			callback(itemPath.c_str(), true, "OK", sval.c_str());
		}
	}

	json.AddMember(key, val, allocator);

	if (callback)
	{
		snprintf(buffer, SHORT_STRING_BUFF, "[%d items]", length);
		callback(strPath.c_str(), true, "OK", buffer);
	}

	return;
}

template <typename T>
void inject_json_value(std::vector<T*>& src, rapidjson::Value& json, const char* path, const char* field, rapidjson::Document::AllocatorType& allocator, funSaveJsonCB callback = NULL)
{
	if (src.empty())
	{
		CALL_BACK(path, false, "ignore empty array", NULL);
		return;
	}

	std::string strPath;
	if (!pre_check_inject(strPath, json, path, field, callback))
	{
		return;
	}

	rapidjson::Value key;
	rapidjson::Value val;
	key.SetString(field, allocator);
	val.SetArray();

	size_t length = src.size();
	char buffer[SHORT_STRING_BUFF] = {0};
	for (size_t i = 0; i < length; ++i)
	{
		rapidjson::Value item;
		snprintf(buffer, SHORT_STRING_BUFF, "/%d", i);
		std::string itemPath = strPath + buffer;
		if (NULL != src[i])
		{
			item.SetObject();
			src[i]->SaveJson(item, allocator, itemPath.c_str(), callback);
			CALL_BACK(itemPath.c_str(), true, "OK", "{object}");
		}
		else
		{
			CALL_BACK(itemPath.c_str(), false, "is null object", NULL);
		}
		val.PushBack(item, allocator);
	}

	json.AddMember(key, val, allocator);

	if (callback)
	{
		snprintf(buffer, SHORT_STRING_BUFF, "[%d items]", length);
		callback(strPath.c_str(), true, "OK", buffer);
	}

	return;
}

#endif /* end of include guard: JSON_CONVERTOR_H__ */
