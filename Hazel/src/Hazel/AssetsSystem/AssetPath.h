#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace Engine
{
	class AssetPath
	{
	public:
		AssetPath() = default;

		static std::optional<AssetPath> Parse(std::string_view path);

		const std::string& String() const { return m_Path; }
		bool Empty() const { return m_Path.empty(); }

		bool operator==(const AssetPath& other) const { return m_Path == other.m_Path; }

	private:
		explicit AssetPath(std::string path)
			: m_Path(std::move(path)) {}

		std::string m_Path;
	};
}
