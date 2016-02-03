#pragma once
#include "Common.hh"
#include "TupleHashImplementation.hh"
#include "AppObject.hh"

#include <unordered_map>
#include <set>
#include <time.h>

struct IPv4AddressComparator {
    bool operator() ( IPAddress firstIP,  IPAddress secondIP) const {
        return firstIP.getIPv4() < secondIP.getIPv4();
    }
};

struct NATNoFreeSocketsError : std::exception { };

struct Socket {
    IPAddress ip;
    uint16_t port;
    Socket(IPAddress ip_, uint16_t port_) : ip(ip_), port(port_) {}
    Socket() = default;
    bool operator==(const Socket & s) const;
    bool operator<(const Socket & s) const;
};

inline std::ostream& operator<<(std::ostream &strm, const Socket &s) {
    IPAddress s_ip = IPAddress(s.ip); // ah, still that forgotten const
    return strm << AppObject::uint32_t_ip_to_string(s_ip.getIPv4()) << ":" << s.port;
}

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
    std::set<Socket> freeSockets;
public:
    SocketStorage(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports);
    SocketStorage() = default;
    Socket issueSocket();
    void returnSocket(Socket s);
    std::set<Socket> getFreeSockets() const {return freeSockets;} ;
};

inline std::ostream& operator<<(std::ostream &strm, const SocketStorage &s) {
    strm << "Current free sockets:" << std::endl;
    for (auto fs : s.getFreeSockets())
        strm << fs << std::endl;
    return strm;
}


// stores active mappings
class NATMappings {
    friend class LearningSwitch;
    SocketStorage socketStorage;
    typedef std::tuple<Socket, IPAddress> NATMappingPrimaryKey ; // tuple<src_ip, src_port, dst_ip>
    // tuple<nat_ip, nat_port, map<dst_port, creation_time>
    typedef std::tuple<Socket, std::unordered_map<uint16_t, time_t>> NATMappingValue;
    std::unordered_map<NATMappingPrimaryKey, NATMappingValue> mappings;
    // after timeout seconds record will be invalid
    double timeout; // should be const, but I don't to play with operator= now

    // returns NAT IP and Port from mappings or creates new if needed
    NATMappingValue& getNATIPAndPort(const NATMappingPrimaryKey &key);
    // mapping is valid if at least one of it's record is active; the method wipes out old mappings while iterating.
    bool isValidMapping(const NATMappingValue &nat_value);
    // check if the record is outdated
    bool isRecordValid(const time_t &creation_time) const;
    // add record if it doesn't exist
    void ensureRecordExistence(NATMappingValue& mapping, uint16_t dst_port);
    friend std::ostream& operator<<(std::ostream &strm, const NATMappings &s);

public:
    NATMappings(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports, double _timeout);
    NATMappings() : timeout(15) {}; // FIXME: why can't I forbid it?
    // returns NAT IP and port for incoming packet.
    const Socket * processOutcoming(Socket localSocket, Socket globalSocket);
    const Socket * processIncoming(Socket globalSocket, Socket NATSocket);
    void wipeOutExpiredMappings();
};

inline std::ostream& operator<<(std::ostream &strm, const NATMappings &s) {
    strm << "Current active records:" << std::endl;
    for (auto const &mp_it : s.mappings) {
        auto mapping_key = mp_it.first;
        auto mapping_value = mp_it.second;
        IPAddress dst_ip = std::get<1>(mapping_key);
        std::string dst_ip_string = AppObject::uint32_t_ip_to_string(dst_ip.getIPv4());
        for (auto const &rec_it: std::get<1>(mapping_value)) {
            strm << "(" << std::get<0>(mapping_key) << ", " << dst_ip_string << ") -> (";
            strm << std::get<0>(mapping_value) << " <" << rec_it.first << ", " << difftime(time(0), rec_it.second);
            strm << ">)" << std::endl;
        }
    }
    return strm;
}
