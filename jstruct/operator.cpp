#include "operator.h"
#include "convertor.h"
#include "rapidjson/pointer.h"

namespace jsop
{

static bool s_path_error = false;
bool has_path_error()
{
	return s_path_error;
}

const JVAL& null_value()
{
	static JVAL json;
	return json;
}

inline
const JVAL& fail_path()
{
	s_path_error = true;
	return null_value();
}

inline
const JVAL& success_path(const JVAL& ret)
{
	s_path_error = false;
	return ret;
}

} // end of namespace jsop

const JVAL& operator/ (const JVAL& json, const std::string& path)
{
	return operator/(json, path.c_str());
}

const JVAL& operator/ (const JVAL& json, const char* path)
{
	if (path == NULL || path[0] == '\0')
	{
		return jsop::fail_path();
	}

	if (!json.IsObject() && !json.IsArray())
	{
		return jsop::fail_path();
	}

	if (json.IsObject() && json.HasMember(path))
	{
		return jsop::success_path(json[path]);
	}

	bool any_slash = false;
	bool all_digit = true;
	for (size_t i = 0; ; i++)
	{
		char c = path[i];
		if (c == '\0')
		{
			break;
		}
		if (c == '/')
		{
			any_slash = true;
			all_digit = false;
			break;
		}
		if (c <= '0' || c >= '9')
		{
			all_digit = false;
		}
	}

	if (json.IsArray() && all_digit)
	{
		size_t index = (size_t) atoi(path);
		if (json.Size() > index)
		{
			return jsop::success_path(json[index]);
		}
	}
	
	if (any_slash)
	{
		std::string fixPath;
		if (path[0] == '/')
		{
			fixPath = path;
		}
		else
		{
			fixPath = std::string("/") + std::string(path);
		}
		rapidjson::Pointer jpath(fixPath.c_str(), fixPath.size());
		const rapidjson::Value *pVal = jpath.Get(json);
		if (pVal)
		{
			return jsop::success_path(*pVal);
		}
	}

	return jsop::fail_path();
}

const JVAL& operator/ (const JVAL& json, size_t index)
{
	if (json.IsArray() && json.Size() > index)
	{
		return jsop::success_path(json[index]);
	}
	return jsop::fail_path();
}

const JVAL& operator/ (const JVAL& json, int index)
{
	if (index < 0)
	{
		return jsop::fail_path();
	}
	return operator/(json, size_t(index));
}

////////////////////////////////////////

int operator|(const JVAL& json, int defval)
{
	int val;
	if (convert_json_value(val, json))
	{
		return val;
	}
	return defval;
}

double operator|(const JVAL& json, double defval)
{
	double val;
	if (convert_json_value(val, json))
	{
		return val;
	}
	return defval;
}

std::string operator|(const JVAL& json, const char* defval)
{
	std::string val;
	if (convert_json_value(val, json))
	{
		return val;
	}
	return defval;
}

std::string operator|(const JVAL& json, const std::string& defval)
{
	std::string val;
	if (convert_json_value(val, json))
	{
		return val;
	}
	return defval;
}

