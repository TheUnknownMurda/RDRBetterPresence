#pragma once

// Direct implementation of the Discord local RPC protocol over the named pipe
// \\.\pipe\discord-ipc-N. No dependency on discord-rpc / the Game SDK.
//
// Frame layout: [opcode u32 LE][length u32 LE][JSON payload]

struct ActivityButton
{
	std::string label;
	std::string url;

	bool operator==(const ActivityButton&) const = default;
};

struct Activity
{
	std::string details;      // first line
	std::string state;        // second line
	std::string largeImage;   // asset key or https URL
	std::string largeText;
	std::string smallImage;
	std::string smallText;
	long long startTimestamp = 0;   // unix seconds; 0 = no timer
	std::vector<ActivityButton> buttons;

	bool operator==(const Activity&) const = default;
};

class DiscordIPC
{
public:
	explicit DiscordIPC(std::string clientId);
	~DiscordIPC();

	DiscordIPC(const DiscordIPC&) = delete;
	DiscordIPC& operator=(const DiscordIPC&) = delete;

	// Opens the pipe and performs the handshake. Returns true once READY is received.
	bool Connect();
	void Disconnect();
	bool IsConnected() const { return m_pipe != INVALID_HANDLE_VALUE && m_ready; }

	bool SetActivity(const Activity& activity);
	bool ClearActivity();

	// Process unsolicited frames (PING -> PONG, CLOSE -> disconnect). Non-blocking.
	void Pump();

	const std::string& LastError() const { return m_lastError; }

private:
	enum Opcode : uint32_t
	{
		Op_Handshake = 0,
		Op_Frame = 1,
		Op_Close = 2,
		Op_Ping = 3,
		Op_Pong = 4
	};

	bool OpenPipe();
	bool WriteFrame(uint32_t opcode, const std::string& json);
	// Waits up to timeoutMs for a complete frame. Returns false on timeout / broken pipe.
	bool ReadFrame(uint32_t& opcode, std::string& json, DWORD timeoutMs);
	bool SendCommandAndWait(const std::string& json);
	void HandleFrame(uint32_t opcode, const std::string& json);
	std::string NextNonce();

	HANDLE m_pipe = INVALID_HANDLE_VALUE;
	std::string m_clientId;
	std::string m_lastError;
	bool m_ready = false;
	unsigned m_nonce = 0;
};
