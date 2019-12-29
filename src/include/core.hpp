#ifndef ined(IIRAN_LEASP_CORE_HPP)
#define IIRAN_LEASP_CORE_HPP

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#define CSTR_FILE_SPLIT "/"
#define C_FILE_SPLIT *((const char*)CSTR_FILE_SPLIT)[0]
#define CSTR_NEW_LINE "\n"

#define IIRAN_OK 0
#define IIRAN_IO -1
#define IIRAN_INVALID_ARG -2
#define IIRAN_NETWORK -3
#define IIRAN_TIMEOUT -4
#define IIRAN_PEER_CLOSE -5

#endif  // IIRAN_LEASP_CORE_HPP
