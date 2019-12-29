#include "socket.hpp"
namespace iiran {

// create default socket
Socket::Socket() {
  memset(&m_sock_addr, 0, sizeof(sockaddr_in));
  m_sock_addr_len = sizeof(m_sock_addr);
  clean_peer_addr();
}
// create a listening socket
Socket::Socket(uint32_t port, int32_t timeout) : Socket() {
  setup_listen_socket_addr(port);
  m_timeout = timeout;
}

// create a connecting socket
Socket::Socket(const char* hostname, int32_t port, int32_t timeout) : Socket() {
  Socket::setup_socket_addr(port, Socket::get_addr_from_hostname(hostname));
  m_timeout = timeout;
}

// create from existing socket
Socket::Socket(int* other_sock, int32_t timeout) : Socket() {
  m_fd = *other_sock;
  int code = IIRAN_OK;
  // retrieve peer addr from existing socket
  code = getsockname(m_fd, (sockaddr*)&m_sock_addr, &m_sock_addr_len);
  if (code != IIRAN_OK) {
    printf("Failed to get socket name, errno = %d", errno);
  } else {
    m_init = true;

    code = getpeername(m_fd, (sockaddr*)&m_peer_sock_addr, &m_sock_addr_len);
    if (code != IIRAN_OK) {
      printf("Failed to get peer name, errno = %d", errno);
    }
  }
  m_timeout = timeout;
}

int32_t Socket::init_socket() {
  int32_t code = IIRAN_OK;
  if (!m_init) {
    clean_peer_addr();
    m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_fd != -1) {
      m_init = true;
      Socket::set_socket_timeout(m_fd, m_timeout / 1000);

      // macOS nosig
      int32_t temp{1};
      setsockopt(m_fd, SOL_SOCKET, SO_NOSIGPIPE, &temp, sizeof(temp));

    } else {
      code = IIRAN_NETWORK;
      printf("Failed to initalize socket, errno = %d", errno);
    }
  }

  return code;
}

// extend expire-close time
int32_t Socket::set_socket_li(int32_t on_off, int linger) {
  int32_t code = IIRAN_OK;
  struct linger _linger;
  _linger.l_onoff = on_off;
  _linger.l_linger = linger;
  code = setsockopt(m_fd, SOL_SOCKET, SO_LINGER, (const char*)&_linger,
                    sizeof(_linger));
  return code;
}

void Socket::set_addr(const char* hostname, uint32_t port) {
  clean_peer_addr();
  setup_socket_addr(port, Socket::get_addr_from_hostname(hostname));
}

int32_t Socket::bind_listen() {
  int32_t code = IIRAN_OK;
  int32_t temp = 1;
  code = setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&temp, sizeof(temp));
  if (code != IIRAN_OK) {
    printf("Failed to setsockpt SO_REUSEADDR, errno = %d", errno);
  }
  code = set_socket_li(1, DEFAULT_LINGER_SEC);
  if (code != IIRAN_OK) {
    printf("Failed to setsockopt SO_LINGER, errno = %d", errno);
  }
  code = ::bind(m_fd, (struct sockaddr*)&m_sock_addr, m_sock_addr_len);
  if (code != IIRAN_OK) {
    printf("Failed to bind socket, errno = %d", errno);
    code = IIRAN_NETWORK;
    goto error;
  }
  code = listen(m_fd, SOMAXCONN);
  if (code != IIRAN_OK) {
    printf("Failed to listen socket, errno = %d", errno);
    code = IIRAN_NETWORK;
    goto error;
  }
done:
  return code;
error:
  close();
  goto done;
}

int32_t Socket::send(const char* msg, int32_t len,
                     int32_t timeout = SOCKET_DEFAULT_TIMEOUT,
                     int32_t flag = 0) {
  int32_t code = IIRAN_OK;
  int32_t max_fd = m_fd;
  struct timeval max_select_time;
  fd_set fds;

  max_select_time.tv_sec = MAX_SELECT_WAIT_SEC;
  max_select_time.tv_usec = MAX_SELECT_WAIT_USEC;
  if (len == 0) {
    return code;
  }

  // wait untile socket ready
  while (true) {
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);

    // fd can send msg?
    code = select(max_fd + 1, nullptr, &fds, nullptr,
                  timeout >= 0 ? &max_select_time : nullptr);
    if (code == 0) {
      // timeout
      code = IIRAN_TIMEOUT;
      goto done;
    } else if (code == EINTR) {
      continue;
    } else if (code < 0) {
      code = IIRAN_NETWORK;
      printf("Failed to select from socket, code = %d", errno);
      goto error;
    }

    if (FD_ISSET(m_fd, &fds)) {
      break;
    }
  }
  // fd is ready
  while (len > 0) {
    // MSG_NOSIGNAL: Requests not to send SIGPIPE on errors on stream oriented
    // sockets when the other end breaks the connection.
    // the EPIPE error is still returned
    // code = ::send(m_fd, msg, len, MSG_NOSIGNAL | flag);
    // macOS donot has SO_NOSIGPIPE
    int32_t send_len = ::send(m_fd, msg, len, flag);
    if (code == -1) {
      printf("Failed to send, code = %d", errno);
      code = IIRAN_NETWORK;
      goto error;
    }
    len -= send_len;
    msg += send_len;
  }
done:
  return code;
error:
  goto done;
}

bool Socket::is_connected() {
  int32_t code = IIRAN_OK;
  code = ::send(m_fd, "", 0, 0);
  if (code < 0) {
    return false;
  }
  return true;
}

int32_t Socket::recv(char* msg, int32_t len,
                     int32_t timeout = SOCKET_DEFAULT_TIMEOUT,
                     int32_t flag = 0) {
  int32_t code = IIRAN_OK;
  if (len == 0) {
    return code;
  }
  int32_t retry = 0;
  int max_fd = m_fd;
  struct timeval max_select_time;
  fd_set fds;
  max_select_time.tv_sec = MAX_SELECT_WAIT_SEC;
  max_select_time.tv_usec = MAX_SELECT_WAIT_USEC;
  while (true) {
    FD_ZERO(&fds);
    FD_SET(m_fd, &fds);
    int32_t select_code = select(max_fd + 1, &fds, nullptr, nullptr,
                                 timeout >= 0 ? &max_select_time : nullptr);

    // timeout?
    if (select_code == 0) {
      code = IIRAN_TIMEOUT;
      goto done;
    } else if (select_code == EINTR) {
      continue;
    } else if (select_code < 0) {
      printf("Failed to select from socket, code = %d", errno);
      code = IIRAN_NETWORK;
      goto error;
    }
    // is this fd's msg?
    if (FD_ISSET(m_fd, &fds)) {
      break;
    }
  }
  while (len > 0) {
    int32_t recv_len = ::recv(m_fd, msg, len, flag);
    if (recv_len > 0) {
      // just peek?
      if (flag & MSG_PEEK) {
        goto done;
      }
      len -= recv_len;
      msg += recv_len;
    } else if (recv_len == 0) {
      printf("Peer unexpected shutdown");
      code = IIRAN_PEER_CLOSE;
      goto error;
    } else {
      int32_t errcode = errno;
      if ((errcode == EAGAIN || errcode == EWOULDBLOCK) && timeout > 0) {
        printf("Recv timeout, code = %d", errcode);
        code = IIRAN_NETWORK;
        goto error;
      } else if ((errcode == EINTR) && (retry < MAX_RECV_RETRIES)) {
        retry++;
        continue;
      } else {
        printf("Recv() Failed , code = %d", errcode);
        code = IIRAN_NETWORK;
        goto error;
      }
    }
  }
  code = IIRAN_OK;
done:
  return code;
error:
  goto done;
}
int32_t Socket::connect() { int32_t code = IIRAN_OK; }
}  // namespace iiran