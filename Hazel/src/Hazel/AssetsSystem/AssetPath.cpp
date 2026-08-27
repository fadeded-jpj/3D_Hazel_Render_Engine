#include "hzpch.h"
#include "AssetPath.h"

namespace Engine
{
	std::optional<AssetPath> AssetPath::Parse(std::string_view input)
	{
		if (input.empty())
			return std::nullopt;

		std::string source(input);
		std::replace(source.begin(), source.end(), '\\', '/');
		if (source.front() != '/')
			source.insert(source.begin(), '/');

		std::string normalized;
		normalized.reserve(source.size());

		size_t cursor = 1;
		while (cursor <= source.size())
		{
			const size_t separator = source.find('/', cursor);
			const size_t end = separator == std::string::npos ? source.size() : separator;
			const std::string_view segment(source.data() + cursor, end - cursor);

			if (!segment.empty() && segment != ".")
			{
				if (segment == "..")
					return std::nullopt;

				normalized.push_back('/');
				normalized.append(segment);
			}

			if (separator == std::string::npos)
				break;
			cursor = separator + 1;
		}

		if (normalized.empty())
			return std::nullopt;

		return AssetPath(std::move(normalized));
	}
}
