#include "NATHelper.hh"
#include "TupleHashImplementation.hh"

#include <unordered_map>


SocketStorage::SocketStorage(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports) {
  LOG(INFO) << "Initializing socket storage";
}

bool Socket::operator==(const Socket &s2) const {
    return (ip == s2.ip && port == s2.port);
}

NATMappings::NATMappings(std::set<IPAddress, IPv4AddressComparator> nat_ips, std::set<uint16_t> nat_ports)
: socketStorage(nat_ips, nat_ports) { }

//Socket NATMappings::processOutcoming(Socket localSocket, Socket globalSocket) {
//    std::unordered_map<NATMappingPrimaryKey, NATMappingValue>::iterator result_row = mappings.find(std::make_pair);
//  if (result_row != mappings.end()) {
//  }
//}

NATMappings::NATMappingValue &NATMappings::getRow(NATMappings::NATMappingPrimaryKey key) {
    return mappings[key];
}

