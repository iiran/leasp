#if !defined(IIRAN_LEASP_SOCKET)
#define IIRAN_LEASP_SOCKET

#include "core.hpp"
#define SOCKET_DEFAULT_TIMEOUT 10000
#define SOCKET_DEFAULT_TIMEOUT_SEC (SOCKET_DEFAULT_TIMEOUT / 1000)
#define MAX_HOSTNAME 1000
#define MAX_SERVICENAME 1000
#define DEFAULT_LINGER_SEC 30
#define MAX_RECV_RETRIES 5
#define MAX_SELECT_WAIT_SEC SOCKET_DEFAULT_TIMEOUT / 1000000
#define MAX_SELECT_WAIT_USEC SOCKET_DEFAULT_TIMEOUT % 1000000

namespace iiran {
class Socket {
 private:
  int32_t m_fd{0};
  socklen_t m_sock_addr_len{0};
  socklen_t m_peer_sock_addr_len{0};
  struct sockaddr_in m_sock_addr {};
  struct sockaddr_in m_peer_sock_addr {};
  bool m_init{false};
  int32_t m_timeout{0};

 protected:
  uint32_t get_port(sockaddr_in* addr);
  int32_t get_addr(sockaddr_in* addr, char* address, uint32_t len);

 public:
  int32_t set_socket_li(int32_t on_off, int linger);
  void set_addr(const char* hostname, uint32_t port);

  Socket();
  explicit Socket(uint32_t port, int32_t timeout);
  explicit Socket(const char* hostname, int32_t port, int32_t timeout);
  explicit Socket(int* sock, int32_t timeout);
  virtual ~Socket() { close(); }
  int32_t init_socket();
  int32_t bind_listen();
  bool is_connected();
  int32_t send(const char* msg, int32_t len,
               int32_t timeout = SOCKET_DEFAULT_TIMEOUT, int32_t flag = 0);
  int32_t recv(char* msg, int32_t len, int32_t timeout = SOCKET_DEFAULT_TIMEOUT,
               int32_t flag = 0);
  int recv_now(char* msg, int32_t len,
               int32_t timeout = SOCKET_DEFAULT_TIMEOUT);
  int32_t connect();
  void close();
  int32_t accept(int* sock, struct sockaddr* addr, socklen_t addr_len,
                 int32_t timeout = SOCKET_DEFAULT_TIMEOUT);
  // nagle's algorithm: https://en.wikipedia.org/wiki/Nagle%27s_algorithm
  int32_t disable_nagle();
  uint32_t get_peer_port();
  int32_t get_peer_addr(char* addr, uint32_t len);
  uint32_t get_local_port();
  int32_t get_local_addr(char* addr, uint32_t len);
  int32_t set_timeout(int32_t sec);
  static int32_t get_hostname(char* name, int32_t len);
  // from /etc/services
  static int32_t get_port(char* servicename, uint32_t& port);

  /**
   *  BELOW HELPER
   */

  void clean_peer_addr() {
    memset(&m_peer_sock_addr, 0, sizeof(sockaddr_in));
    m_peer_sock_addr_len = sizeof(m_peer_sock_addr);
  }

  static void set_socket_timeout(int32_t fd, int32_t timeout_sec) {
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
  }

  void setup_listen_socket_addr(uint32_t port) {
    memset(&m_sock_addr, 0, sizeof(sockaddr_in));
    m_sock_addr.sin_family = AF_INET;
    m_sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_sock_addr.sin_port = htons(port);
    m_sock_addr_len = sizeof(m_sock_addr);
  };

  void setup_socket_addr(uint32_t port, uint32_t addr) {
    memset(&m_sock_addr, 0, sizeof(sockaddr_in));
    m_sock_addr.sin_family = AF_INET;
    m_sock_addr.sin_addr.s_addr = addr;
    m_sock_addr.sin_port = htons(port);
    m_sock_addr_len = sizeof(m_sock_addr);
  };

  static uint32_t get_addr_from_hostname(const char* hostname) {
    uint32_t addr{};
    // is hostname
    struct hostent* host = gethostbyname(hostname);
    if (host != nullptr) {
      addr = *((int*)host->h_addr_list[0]);
    } else {
      // is x.x.x.x
      addr = inet_addr(hostname);
    }
    return addr;
  }
};
}  // namespace iiran

#endif  // IIRAN_LEASP_SOCKET
