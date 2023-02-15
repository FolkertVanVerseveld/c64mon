#pragma once

#if _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

typedef int SOCKET;

#define INVALID_SOCKET (-1)
#endif

#include <cstdint>
#include <atomic>
#include <stdexcept>
#include <string>
#include <utility>

class Net final {
public:
	Net();
	~Net();
};

/** Error to indicate the associated socket has been closed. For TCP, this means the other end has send a shutdown and recv/send will not work anymore. */
class SocketClosedError final : public std::runtime_error {
public:
	explicit SocketClosedError(const std::string &s) : std::runtime_error(s.c_str()) {}
	explicit SocketClosedError(const char *msg) : std::runtime_error(msg) {}
};

class TcpSocket final {
	std::atomic<int> s;
public:
	TcpSocket();
	~TcpSocket();

	void connect(const char *address, uint16_t port);

	// data exchange
	// NOTE tries indicates number of attempts. use tries=0 for infinite retries.
	// NOTE the template send/recv version use a different tries default value than the non-template versions. its value is chosen randomly.
	// NOTE the template send/recv version also throw if an incomplete object is sent/received. if you don't want this, use the generic send/recv functions.

	int try_send(const void *ptr, int len, unsigned tries) noexcept;
	int send(const void *ptr, int len, unsigned tries=1);

	template<typename T> int send(const T *ptr, int len, unsigned tries=5) {
		int out = send((const void*)ptr, len * sizeof * ptr, tries);
		if (out % sizeof * ptr)
			throw std::runtime_error("wsa: send failed: incomplete object sent");
		return out / sizeof * ptr;
	}

	void send_fully(const void *ptr, int len);

	template<typename T> void send_fully(const T *ptr, int len) {
		send_fully((void*)ptr, len * sizeof * ptr);
	}

	int try_recv(void *dst, int len, unsigned tries) noexcept;
	int recv(void *dst, int len, unsigned tries=1);

	template<typename T> int recv(T *ptr, int len, unsigned tries=5) {
		int in = recv((void*)ptr, len * sizeof * ptr, tries);
		if (in % sizeof * ptr)
			throw std::runtime_error("wsa: recv failed: incomplete object received");
		return in / sizeof * ptr;
	}

	void recv_fully(void *dst, int len);

	template<typename T> void recv_fully(T *ptr, int len) {
		recv_fully((void*)ptr, len * sizeof * ptr);
	}

	// operator overloading

	TcpSocket &operator=(TcpSocket &&other) noexcept
	{
		s.store(other.s.load());
		other.s.store((int)INVALID_SOCKET);
		return *this;
	}
};
