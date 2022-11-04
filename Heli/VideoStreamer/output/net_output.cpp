/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (C) 2020, Raspberry Pi (Trading) Ltd.
 *
 * net_output.cpp - send output over network.
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <stdexcept>

#include "net_output.hpp"

NetOutput::NetOutput(const std::string& address,
                     unsigned short port) : Output()
{
    saddr_ = {};
    saddr_.sin_family = AF_INET;
    saddr_.sin_port = htons(port);
    if (inet_aton(address.c_str(), &saddr_.sin_addr) == 0)
        throw std::runtime_error("inet_aton failed for " + address);

    fd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd_ < 0)
        throw std::runtime_error("unable to open udp socket");

    saddr_ptr_ = (const sockaddr*) &saddr_; // sendto needs these for udp
    sockaddr_in_size_ = sizeof(sockaddr_in);
}

NetOutput::~NetOutput()
{
    close(fd_);
}

// Maximum size that sendto will accept.
constexpr size_t MAX_UDP_SIZE = 65507;

void NetOutput::outputBuffer(void* mem, size_t size, int64_t /*timestamp_us*/, uint32_t /*flags*/)
{
    size_t max_size = saddr_ptr_ ? MAX_UDP_SIZE : size;
    for (auto* ptr = (uint8_t*) mem; size;)
    {
        size_t bytes_to_send = std::min(size, max_size);
        if (sendto(fd_, ptr, bytes_to_send, 0, saddr_ptr_, sockaddr_in_size_) < 0)
            throw std::runtime_error("failed to send data on socket");
        ptr += bytes_to_send;
        size -= bytes_to_send;
    }
}
