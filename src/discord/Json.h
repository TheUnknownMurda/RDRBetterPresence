#pragma once

// Tiny JSON writer, enough for the Discord IPC payloads we produce.
// (No parser: we only inspect incoming frames with simple substring checks.)

namespace Json
{
	inline std::string Escape(std::string_view s)
	{
		std::string out;
		out.reserve(s.size() + 8);
		for (unsigned char c : s)
		{
			switch (c)
			{
				case '"':  out += "\\\""; break;
				case '\\': out += "\\\\"; break;
				case '\n': out += "\\n"; break;
				case '\r': out += "\\r"; break;
				case '\t': out += "\\t"; break;
				default:
					if (c < 0x20)
					{
						char buf[8];
						snprintf(buf, sizeof(buf), "\\u%04x", c);
						out += buf;
					}
					else
					{
						out += (char)c;
					}
			}
		}
		return out;
	}

	class Object
	{
	public:
		Object& String(std::string_view key, std::string_view value)
		{
			Sep();
			m_out += '"'; m_out += Escape(key); m_out += "\":\""; m_out += Escape(value); m_out += '"';
			return *this;
		}

		Object& Int(std::string_view key, long long value)
		{
			Sep();
			m_out += '"'; m_out += Escape(key); m_out += "\":"; m_out += std::to_string(value);
			return *this;
		}

		Object& Bool(std::string_view key, bool value)
		{
			Sep();
			m_out += '"'; m_out += Escape(key); m_out += "\":"; m_out += value ? "true" : "false";
			return *this;
		}

		// Insert an already-serialized JSON value (object / array).
		Object& Raw(std::string_view key, std::string_view json)
		{
			Sep();
			m_out += '"'; m_out += Escape(key); m_out += "\":"; m_out += json;
			return *this;
		}

		Object& Nested(std::string_view key, const Object& obj)
		{
			return Raw(key, obj.Str());
		}

		bool Empty() const { return m_out.empty(); }

		std::string Str() const { return "{" + m_out + "}"; }

	private:
		void Sep()
		{
			if (!m_out.empty()) m_out += ',';
		}

		std::string m_out;
	};

	class Array
	{
	public:
		Array& Item(const Object& obj)
		{
			if (!m_out.empty()) m_out += ',';
			m_out += obj.Str();
			return *this;
		}

		bool Empty() const { return m_out.empty(); }

		std::string Str() const { return "[" + m_out + "]"; }

	private:
		std::string m_out;
	};
}
