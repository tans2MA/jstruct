#include "convertor.h"
#include <stdio.h>
#include <algorithm>

bool convert_json_value(std::string& dest, const rapidjson::Value& json)
{
	if (json.IsString())
	{
		dest = json.GetString();
		return true;
	}

	return false;
}

bool convert_json_value(int& dest, const rapidjson::Value& json)
{
	if (json.IsInt())
	{
		dest = json.GetInt();
		return true;
	}
	else if (json.IsString()) // Backwards compatibility
	{
		std::string str_tmp = json.GetString();
		dest = atoi(str_tmp.c_str());
		return true;
	}

	return false;
}

bool convert_json_value(double& dest, const rapidjson::Value& json)
{
	if (json.IsDouble())
	{
		dest = json.GetDouble();
		return true;
	}
	else if (json.IsInt()) // Backwards compatibility
	{
		dest = (double)json.GetInt();
		return true;
	}
	else if (json.IsString())
	{
		std::string str_tmp = json.GetString();
		dest = atof(str_tmp.c_str());
		return true;
	}

	return false;
}

bool convert_json_value(bool& dest, const rapidjson::Value& json)
{
	if (json.IsBool())
	{
		dest = json.GetBool();
		return true;
	}
	else if (json.IsInt()) // Backwards compatibility
	{
		dest = (json.GetInt() == 0) ? false : true;
		return true;
	}
	else if (json.IsString())
	{
		std::string str_tmp = json.GetString();
		std::transform(str_tmp.begin(), str_tmp.end(), str_tmp.begin(), ::toupper); //  uppercase 
		dest = (str_tmp == "TRUE") ? true : false;
		return true;
	}

	return false;
}

bool pre_check_extract(std::string& joinPath, const rapidjson::Value& json, const char* path, const char* field, funLoadJsonCB callback)
{
	if (path && path[0])
	{
		joinPath = path;
	}
	else
	{
		joinPath = "/";
	}

	if (!json.IsObject())
	{
		CALL_BACK(joinPath.c_str(), false, "not object", NULL);
		return false;
	}

	if (NULL == field)
	{
		CALL_BACK(joinPath.c_str(), false, "try query empty field", NULL);
	}

	if (joinPath[joinPath.size()-1] != '/')
	{
		joinPath.append("/");
	}
	joinPath.append(field);


	if (!json.HasMember(field))
	{
		CALL_BACK(joinPath.c_str(), false, "field missing", NULL);
		return false;
	}

	return true;
}

bool pre_check_inject(std::string& joinPath, const rapidjson::Value& json, const char* path, const char* field, funSaveJsonCB callback)
{
	if (path && path[0])
	{
		joinPath = path;
	}
	else
	{
		joinPath = "/";
	}

	if (!json.IsObject())
	{
		CALL_BACK(joinPath.c_str(), false, "not object", NULL);
		return false;
	}

	if (NULL == field)
	{
		CALL_BACK(joinPath.c_str(), false, "try insert empty field", NULL);
	}

	if (joinPath[joinPath.size()-1] != '/')
	{
		joinPath.append("/");
	}
	joinPath.append(field);

	if (json.HasMember(field))
	{
		CALL_BACK(joinPath.c_str(), false, "field overwirte", NULL);
	}

	return true;
}

void store_json_value(const std::string& src, rapidjson::Value& json, rapidjson::Document::AllocatorType& allocator)
{
	rapidjson::Value val;
	val.SetString(src.c_str(), src.size(), allocator);
	json = val;
}

