#include "NATHelper.hh"
#include "TupleHashImplementation.hh"
#include <iostream>
#include <time.h>

#include <unordered_map>


SocketStorage::SocketStorage(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports) {
    LOG(INFO) << "Initializing socket storage";
    for (auto i : nat_ips) {
        for (auto p : nat_ports) {
            freeSockets.insert(Socket(i, p));
        }
    }
}

Socket SocketStorage::issueSocket() {
    auto it = freeSockets.begin();
    Socket s;
    if (it == freeSockets.end())
        throw NATNoFreeSocketsError();
    else
        s = *it;
    freeSockets.erase(s);
    return s;
}

void SocketStorage::returnSocket(Socket s) {
  freeSockets.insert(s);
}

bool Socket::operator==(const Socket &s) const {
    return (ip == s.ip && port == s.port);
}

bool Socket::operator<(const Socket &s) const {
    IPAddress s1_ip_tmp = IPAddress(ip);
    IPAddress s2_ip_tmp = IPAddress(s.ip);
    long s1_value = s1_ip_tmp.getIPv4()*65537 + port;
    long s2_value = s2_ip_tmp.getIPv4()*65537 + s.port;
    return s1_value < s2_value;
}


NATMappings::NATMappings(std::set<IPAddress, IPv4AddressComparator> nat_ips,
                         std::set<uint16_t> nat_ports, double _timeout) :
        socketStorage(nat_ips, nat_ports), timeout(_timeout) {}

const Socket * NATMappings::processOutcoming(Socket localSocket, Socket globalSocket) {
    NATMappingValue &nat_value = getNATIPAndPort(std::make_tuple(localSocket, globalSocket.ip));
    const Socket * nat_socket_ptr = &std::get<0>(nat_value);
    ensureRecordExistence(nat_value, globalSocket.port);
    return nat_socket_ptr;
}

const Socket *NATMappings::processIncoming(Socket globalSocket, Socket NATSocket) {
    auto mp_it = mappings.begin();
    while (mp_it != mappings.end()) {
        const NATMappingPrimaryKey &key = mp_it->first;
        const NATMappingValue &nat_value = mp_it->second;
        if (!isValidMapping(nat_value))
            mp_it = mappings.erase(mp_it);
        else {
            if (std::get<1>(key) == globalSocket.ip && std::get<0>(nat_value) == NATSocket) {
                auto const &rec_map = std::get<1>(nat_value);
                auto const &rec_it = rec_map.find(globalSocket.port);
                if (rec_it != rec_map.end())
                    return &std::get<0>(key);
            }
            ++mp_it;
        }
    }
    return nullptr;
}

NATMappings::NATMappingValue &NATMappings::getNATIPAndPort(const NATMappings::NATMappingPrimaryKey &key) {
    auto it = mappings.find(key);
    if (it != mappings.end()) {
        if (isValidMapping(it->second))
            return it->second;
        else
            mappings.erase(key);
    }
    Socket newNATSocket = socketStorage.issueSocket();
    mappings.insert(std::make_pair(
            key,
            std::make_pair(newNATSocket, std::unordered_map<uint16_t, time_t>())));
    return mappings[key];
}

bool NATMappings::isValidMapping(const NATMappings::NATMappingValue &nat_value) {
    bool is_valid = false;
    auto record_map = std::get<1>(nat_value);
    auto it = record_map.begin();
    while (it != record_map.end()) {
        if (!isRecordValid(it->second))
            it = record_map.erase(it);
        else {
            ++it;
            is_valid = true;
        }
    }
    return is_valid;
}

bool NATMappings::isRecordValid(const time_t &creation_time) const {
    time_t curr_time = time(0);
    return difftime(curr_time, creation_time) < timeout;
}

void NATMappings::ensureRecordExistence(NATMappings::NATMappingValue& mapping, uint16_t dst_port) {
    std::unordered_map<uint16_t, time_t> &record_map = std::get<1>(mapping);
    auto it = record_map.find(dst_port);
    if (it != record_map.end()) {
        if (isRecordValid(it->second))
            return;
        else
            record_map.erase(dst_port);
    }
    record_map.insert(std::make_pair(dst_port, time(0)));
}

void NATMappings::wipeOutExpiredMappings() {
    auto it = mappings.begin();
    while (it != mappings.end()) {
        if (!isValidMapping(it->second))
            it = mappings.erase(it);
        else
            ++it;
    }
}
