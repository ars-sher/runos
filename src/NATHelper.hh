#pragma once
#include "Common.hh"
#include "TupleHashImplementation.hh"

#include <unordered_map>
#include <set>

struct IPv4AddressComparator {
    bool operator() ( IPAddress firstIP,  IPAddress secondIP) const {
        return firstIP.getIPv4() < secondIP.getIPv4();
    }
};

struct Socket {
    IPAddress ip;
    uint16_t port;
    Socket(IPAddress ip_, uint16_t port_) : ip(ip_), port(port_) {}
    Socket() = default;
    bool operator==(const Socket & s2) const;
};

namespace std {
    template <>
    struct hash<IPAddress> {
        size_t operator()(const IPAddress i) const {
            IPAddress tmp_i = IPAddress(i); // horrible
            return hash<uint32_t >()(tmp_i.getIPv4());
        }
    };

    template <>
    struct hash<Socket> {
        size_t operator()(const Socket s) const {
            return ((hash<IPAddress>()(s.ip)) ^ (hash<uint16_t>()(s.port) << 1)) >> 1;
        }
    };
}


// stores available NAT sockets
class SocketStorage {
public:
    SocketStorage(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports);
    SocketStorage() = default;
    Socket issueSocket();
    void returnSocket(Socket s);
};


// stores active mappings
class NATMappings {
    SocketStorage socketStorage;
    typedef std::tuple<Socket, IPAddress> NATMappingPrimaryKey ; // tuple<src_ip, src_port, dst_ip>
    // tuple<nat_ip, nat_port, map<dst_port, creation_time>
    typedef std::tuple<Socket, std::unordered_map<uint16_t, time_t>> NATMappingValue;
    std::unordered_map<NATMappingPrimaryKey, NATMappingValue> mappings;


    NATMappingValue& getRow(NATMappingPrimaryKey);
public:
    NATMappings(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports);
    NATMappings() = default;
    // returns NAT IP and port for incoming packet.
    Socket processOutcoming(Socket localSocket, Socket globalSocket);
    Socket processIncoming(Socket globalSocket, Socket NATSocket);
    void wipeOutExpiredMappings();
};
