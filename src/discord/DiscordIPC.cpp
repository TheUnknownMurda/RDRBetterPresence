#include "pch.h"
#include "DiscordIPC.h"
#include "Json.h"
#include "../Log.h"

namespace
{
	constexpr size_t kMaxFrameSize = 64 * 1024;

	bool Contains(const std::string& haystack, const char* needle)
	{
		return haystack.find(needle) != std::string::npos;
	}

	// Trim a string for the 128-char limits Discord enforces on details/state
	// (and the 2-char minimum: shorter strings make Discord reject the whole activity).
	std::string ClampText(const std::string& s)
	{
		if (s.empty()) return s;
		if (s.size() == 1) return s + " ";
		if (s.size() <= 128) return s;

		// Cut on a UTF-8 boundary.
		size_t cut = 125;
		while (cut > 0 && (static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80) --cut;
		return s.substr(0, cut) + "...";
	}
}

DiscordIPC::DiscordIPC(std::string clientId)
	: m_clientId(std::move(clientId))
{
}

DiscordIPC::~DiscordIPC()
{
	Disconnect();
}

bool DiscordIPC::OpenPipe()
{
	for (int i = 0; i < 10; ++i)
	{
		char name[64];
		snprintf(name, sizeof(name), "\\\\.\\pipe\\discord-ipc-%d", i);

		HANDLE h = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (h != INVALID_HANDLE_VALUE)
		{
			m_pipe = h;
			Log::Info("Connected to %s", name);
			return true;
		}

		DWORD err = GetLastError();
		if (err == ERROR_PIPE_BUSY)
		{
			// Another client is mid-connect; give it a moment and retry the same slot once.
			if (WaitNamedPipeA(name, 500))
			{
				--i;
				continue;
			}
		}
	}

	m_lastError = "Discord is not running (no discord-ipc-* pipe found)";
	return false;
}

bool DiscordIPC::Connect()
{
	Disconnect();

	if (!OpenPipe())
	{
		return false;
	}

	Json::Object handshake;
	handshake.Int("v", 1).String("client_id", m_clientId);

	if (!WriteFrame(Op_Handshake, handshake.Str()))
	{
		m_lastError = "Failed to send handshake";
		Disconnect();
		return false;
	}

	uint32_t opcode = 0;
	std::string json;
	if (!ReadFrame(opcode, json, 5000))
	{
		m_lastError = "No handshake response from Discord";
		Disconnect();
		return false;
	}

	if (opcode == Op_Close)
	{
		// Typical payload: {"code":4000,"message":"Invalid Client ID"}
		m_lastError = "Discord rejected the handshake: " + json;
		Disconnect();
		return false;
	}

	if (opcode != Op_Frame || !Contains(json, "\"READY\""))
	{
		m_lastError = "Unexpected handshake response: " + json;
		Disconnect();
		return false;
	}

	m_ready = true;
	Log::Info("Discord RPC handshake complete (client %s)", m_clientId.c_str());
	return true;
}

void DiscordIPC::Disconnect()
{
	if (m_pipe != INVALID_HANDLE_VALUE)
	{
		if (m_ready)
		{
			// Best effort: tell Discord we're leaving so the presence disappears immediately.
			WriteFrame(Op_Close, "{}");
		}
		CloseHandle(m_pipe);
		m_pipe = INVALID_HANDLE_VALUE;
	}
	m_ready = false;
}

bool DiscordIPC::WriteFrame(uint32_t opcode, const std::string& json)
{
	if (m_pipe == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	std::string frame;
	frame.resize(8 + json.size());
	uint32_t length = static_cast<uint32_t>(json.size());
	memcpy(&frame[0], &opcode, 4);
	memcpy(&frame[4], &length, 4);
	memcpy(&frame[8], json.data(), json.size());

	DWORD written = 0;
	if (!WriteFile(m_pipe, frame.data(), static_cast<DWORD>(frame.size()), &written, nullptr) || written != frame.size())
	{
		m_lastError = "WriteFile failed (" + std::to_string(GetLastError()) + ")";
		return false;
	}
	return true;
}

bool DiscordIPC::ReadFrame(uint32_t& opcode, std::string& json, DWORD timeoutMs)
{
	if (m_pipe == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

	auto waitFor = [&](DWORD bytesNeeded) -> bool
	{
		for (;;)
		{
			DWORD available = 0;
			if (!PeekNamedPipe(m_pipe, nullptr, 0, nullptr, &available, nullptr))
			{
				m_lastError = "Pipe closed by Discord";
				return false;
			}
			if (available >= bytesNeeded)
			{
				return true;
			}
			if (std::chrono::steady_clock::now() >= deadline)
			{
				return false;
			}
			Sleep(10);
		}
	};

	if (!waitFor(8))
	{
		return false;
	}

	uint32_t header[2];
	DWORD read = 0;
	if (!ReadFile(m_pipe, header, 8, &read, nullptr) || read != 8)
	{
		m_lastError = "ReadFile (header) failed";
		return false;
	}

	opcode = header[0];
	uint32_t length = header[1];
	if (length > kMaxFrameSize)
	{
		m_lastError = "Frame too large: " + std::to_string(length);
		return false;
	}

	json.assign(length, '\0');
	size_t total = 0;
	while (total < length)
	{
		if (!waitFor(1))
		{
			return false;
		}
		DWORD chunk = 0;
		if (!ReadFile(m_pipe, &json[total], static_cast<DWORD>(length - total), &chunk, nullptr))
		{
			m_lastError = "ReadFile (payload) failed";
			return false;
		}
		total += chunk;
	}
	return true;
}

std::string DiscordIPC::NextNonce()
{
	return std::to_string(++m_nonce);
}

bool DiscordIPC::SendCommandAndWait(const std::string& json)
{
	if (!IsConnected())
	{
		return false;
	}

	if (!WriteFrame(Op_Frame, json))
	{
		Disconnect();
		return false;
	}

	// Discord answers every command with a frame carrying the same nonce. We only
	// need to spot errors; other unsolicited frames are handled on the way.
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(3000);
	while (std::chrono::steady_clock::now() < deadline)
	{
		uint32_t opcode = 0;
		std::string reply;
		if (!ReadFrame(opcode, reply, 500))
		{
			if (m_pipe == INVALID_HANDLE_VALUE || Contains(m_lastError, "closed"))
			{
				Disconnect();
				return false;
			}
			continue;
		}

		if (opcode == Op_Frame && Contains(reply, "\"cmd\":\"SET_ACTIVITY\""))
		{
			if (Contains(reply, "\"evt\":\"ERROR\""))
			{
				m_lastError = "SET_ACTIVITY rejected: " + reply;
				Log::Warning("%s", m_lastError.c_str());
				return false;
			}
			return true;
		}

		HandleFrame(opcode, reply);
		if (!IsConnected())
		{
			return false;
		}
	}

	// No reply is not fatal (Discord occasionally stays silent under load); the presence was sent.
	return true;
}

bool DiscordIPC::SetActivity(const Activity& a)
{
	Json::Object activity;
	if (!a.details.empty()) activity.String("details", ClampText(a.details));
	if (!a.state.empty())   activity.String("state", ClampText(a.state));

	if (a.startTimestamp > 0)
	{
		Json::Object ts;
		ts.Int("start", a.startTimestamp);
		activity.Nested("timestamps", ts);
	}

	Json::Object assets;
	if (!a.largeImage.empty()) assets.String("large_image", a.largeImage);
	if (!a.largeText.empty())  assets.String("large_text", ClampText(a.largeText));
	if (!a.smallImage.empty()) assets.String("small_image", a.smallImage);
	if (!a.smallText.empty())  assets.String("small_text", ClampText(a.smallText));
	if (!assets.Empty())       activity.Nested("assets", assets);

	Json::Array buttons;
	for (size_t i = 0; i < a.buttons.size() && i < 2; ++i)
	{
		if (a.buttons[i].label.empty() || a.buttons[i].url.empty()) continue;
		Json::Object b;
		b.String("label", a.buttons[i].label).String("url", a.buttons[i].url);
		buttons.Item(b);
	}
	if (!buttons.Empty()) activity.Raw("buttons", buttons.Str());

	activity.Bool("instance", false);

	Json::Object args;
	args.Int("pid", GetCurrentProcessId()).Nested("activity", activity);

	Json::Object cmd;
	cmd.String("cmd", "SET_ACTIVITY").Nested("args", args).String("nonce", NextNonce());

	Log::Debug("SET_ACTIVITY %s", cmd.Str().c_str());
	return SendCommandAndWait(cmd.Str());
}

bool DiscordIPC::ClearActivity()
{
	Json::Object args;
	args.Int("pid", GetCurrentProcessId());

	Json::Object cmd;
	cmd.String("cmd", "SET_ACTIVITY").Nested("args", args).String("nonce", NextNonce());
	return SendCommandAndWait(cmd.Str());
}

void DiscordIPC::HandleFrame(uint32_t opcode, const std::string& json)
{
	switch (opcode)
	{
		case Op_Ping:
			WriteFrame(Op_Pong, json);
			break;
		case Op_Close:
			Log::Warning("Discord closed the connection: %s", json.c_str());
			m_ready = false;
			Disconnect();
			break;
		default:
			// READY / other events - nothing to do.
			break;
	}
}

void DiscordIPC::Pump()
{
	if (!IsConnected())
	{
		return;
	}

	for (int guard = 0; guard < 8 && IsConnected(); ++guard)
	{
		DWORD available = 0;
		if (!PeekNamedPipe(m_pipe, nullptr, 0, nullptr, &available, nullptr))
		{
			Log::Warning("Discord pipe closed (Discord quit?)");
			m_ready = false;
			Disconnect();
			return;
		}
		if (available < 8)
		{
			return;
		}

		uint32_t opcode = 0;
		std::string json;
		if (!ReadFrame(opcode, json, 200))
		{
			return;
		}
		HandleFrame(opcode, json);
	}
}
