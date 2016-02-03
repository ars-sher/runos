/*
 * Copyright 2015 Applied Research Center for Computer Networks
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LearningSwitch.hh"
#include "Controller.hh"
#include "Topology.hh"
#include "Switch.hh"
#include "STP.hh"
#include "NATHelper.hh"

#include <set>
#include <chrono>
#include <thread>
#include <fstream>

REGISTER_APPLICATION(LearningSwitch, {"controller", "switch-manager", "topology", "stp", ""})

void LearningSwitch::init(Loader *loader, const Config &config)
{
    Controller* ctrl = Controller::get(loader);
    topology = Topology::get(loader);
    switch_manager = SwitchManager::get(loader);
    stp = STP::get(loader);

    ctrl->registerHandler(this);

    parseNATConfig("nat-settings.json");
}

OFMessageHandler::Action LearningSwitch::Handler::processMiss(OFConnection* ofconn, Flow* flow) {
    const Switch *sw = app->switch_manager->getSwitch(ofconn);

    if (sw && app->isNATSwitch(sw) && app->isTCPPacket(flow)) {
        flow->setFlags(Flow::Disposable);
//        LOG(INFO) << "A  packet has arrived at the NAT switch with dpid " << sw->id() << " to port " << flow->loadInPort();
        Socket src_socket(flow->loadIPv4Src(), flow->loadTCPSrc());
        Socket dst_socket(flow->loadIPv4Dst(), flow->loadTCPDst());
        if (app->isOutComingPacket(flow)) {
            Socket issuedNATSocket = *app->natMappings.processOutcoming(src_socket, dst_socket);
            flow->add_action(new of13::SetFieldAction(new of13::IPv4Src(issuedNATSocket.ip)));
            flow->add_action(new of13::SetFieldAction(new of13::TCPSrc(issuedNATSocket.port)));
            LOG(INFO) << "The packet is passed behind NAT, issued socket is " << issuedNATSocket;
        }
        else {
            const Socket *issuedLocalSocketPtr = app->natMappings.processIncoming(src_socket, dst_socket);
            if (issuedLocalSocketPtr) {
                flow->add_action(new of13::SetFieldAction(new of13::IPv4Dst(issuedLocalSocketPtr->ip)));
                flow->add_action(new of13::SetFieldAction(new of13::TCPDst(issuedLocalSocketPtr->port)));
                LOG(INFO) << "The packet is passed inside NAT, redirected to local socket " << *issuedLocalSocketPtr;
            }
            else {
                LOG(INFO) << "Not found any record for packet from external network; dropping it";
                return Stop;
            }
        }
    }
//    else if (sw && app->isTCPPacket(flow)) {
//        LOG(INFO) << "A packet has arrived at non-NAT switch " << sw->id() << " to port " << flow->loadInPort();
//    }
    return processMissLearningSwitch(ofconn, flow);
}

/* NAT part */
void LearningSwitch::parseNATConfig(const std::string &config_file) {
    std::ifstream nat_config_stream(config_file);
    std::string config((std::istreambuf_iterator<char>(nat_config_stream)), std::istreambuf_iterator<char>());
    std::string parse_error = "";
    auto nat_config_json = json11::Json::parse(config, parse_error);
    if (parse_error != "" || !nat_config_json.is_object()) {
        LOG(INFO) << "Error during nat settings json parse: " << parse_error;
        throw NATSettingsJsonParseError();
    }

    LOG(INFO) << "initializing NAT";
    uint32_t nat_switch_dpid = 2;
    std::set<uint32_t> nat_switch_local_port_ids = {1};
    double timeout = 15;
    std::set<IPAddress, IPv4AddressComparator> nat_ips;
    std::set<uint16_t> nat_ports;

    auto nat_dpid_json = nat_config_json["nat-dpid"];
    if (!nat_dpid_json.is_number() || (nat_dpid_json.int_value() < 1))
        LOG(WARNING) << "NAT config should contain numerical 'nat-dpid' field, setting default parameter = " <<
                nat_switch_dpid;
    else
        nat_switch_dpid = (uint32_t)nat_dpid_json.int_value();

    auto nat_timeout_json = nat_config_json["timeout"];
    if (!nat_timeout_json.is_number() || (nat_timeout_json.int_value() < 1))
        LOG(WARNING) << "NAT config should contain numerical 'timeout' field, setting default parameter = " <<
        timeout;
    else
        timeout = (uint32_t)nat_timeout_json.int_value();

    auto nat_switch_local_port_ids_json = nat_config_json["nat-switch-local-ports"];
    if (!nat_switch_local_port_ids_json.is_array()) {
        LOG(WARNING) << "NAT config should contain numerical array of nat switch local ports, setting default"
                                " parameter = {1}";
    }
    else {
        auto nat_switch_local_port_ids_array = nat_switch_local_port_ids_json.array_items();
        nat_switch_local_port_ids.clear();
        for (auto it : nat_switch_local_port_ids_array) {
            if (it.is_number() && it.number_value() >= 1)
                nat_switch_local_port_ids.insert((uint32_t)it.number_value());
        }
    }

    auto nat_ips_json = nat_config_json["nat-ips"];
    if (!nat_ips_json.is_array()) {
        LOG(WARNING) << "NAT config must contain array of NAT IPs";
        throw NATSettingsJsonParseError();
    }
    else {
        auto nat_ips_array = nat_ips_json.array_items();
        for (auto it : nat_ips_array) {
            if (!it.is_string() || AppObject::uint32_t_ip_to_string(IPAddress(it.string_value()).getIPv4()) != it.string_value())
                continue;
            nat_ips.insert(IPAddress(it.string_value()));
        }
    }

    auto nat_ports_json = nat_config_json["nat-ports"];
    if (!nat_ports_json.is_array()) {
        LOG(WARNING) << "NAT config must contain array of nat ports";
        throw NATSettingsJsonParseError();
    }
    else {
        auto nat_ports_array = nat_ports_json.array_items();
        for (auto it : nat_ports_array) {
            if (it.is_number() && it.number_value() >= 1 && it.number_value() <= 65535)
                nat_ports.insert((uint16_t)it.number_value());
        }
    }

    natSwitchId = nat_switch_dpid;
    natSwitchLocalPorts = nat_switch_local_port_ids;
    natMappings = NATMappings(nat_ips, nat_ports, timeout);

    LOG(INFO) << natMappings.socketStorage;
    LOG(INFO) << "timeout is " << natMappings.timeout;

//    IPAddress local_ip1 = IPAddress(IPAddress::IPv4from_string("192.168.1.1"));
//    IPAddress local_ip2 = IPAddress(IPAddress::IPv4from_string("192.168.1.2"));
//    IPAddress local_ip3 = IPAddress(IPAddress::IPv4from_string("192.168.1.3"));
//
//    IPAddress gl_ip1 = IPAddress(IPAddress::IPv4from_string("77.37.250.168"));
//    IPAddress gl_ip2 = IPAddress(IPAddress::IPv4from_string("77.37.250.169"));
//    IPAddress gl_ip3 = IPAddress(IPAddress::IPv4from_string("77.37.250.170"));
//
//    IPAddress nat_ip1 = IPAddress(IPAddress::IPv4from_string("10.0.0.200"));
//    IPAddress nat_ip2 = IPAddress(IPAddress::IPv4from_string("10.0.0.201"));
//    IPAddress nat_ip3 = IPAddress(IPAddress::IPv4from_string("10.0.0.202"));
//
//    nat_ips.insert(IPAddress(nat_ip1));
//    nat_ips.insert(IPAddress(nat_ip2));
//    nat_ips.insert(IPAddress(nat_ip3));
//    nat_ports.insert(1025);
//    nat_ports.insert(1026);
//    nat_ports.insert(1027);
//    natSwitchLocalPorts.insert(1);
//    natSwitchLocalPorts.insert(2);
//
//
//    Socket issuedSocket1 = *natMappings.processOutcoming(Socket(local_ip1, 9000), Socket(gl_ip1, 80));
//    LOG(INFO) << "Issued socket 1 is " << issuedSocket1 << std::endl;
//    Socket issuedSocket2 = *natMappings.processOutcoming(Socket(local_ip1, 9000), Socket(gl_ip1, 90));
//    LOG(INFO) << "Issued socket 2 is " << issuedSocket2 << std::endl;
//    Socket issuedSocket3 = *natMappings.processOutcoming(Socket(local_ip2, 9000), Socket(gl_ip1, 80));
//    LOG(INFO) << "Issued socket 3 is " << issuedSocket3 << std::endl;
//    LOG(INFO) << natMappings;
//    const Socket *localSocket_ptr = natMappings.processIncoming(Socket(gl_ip1, 80), Socket(nat_ip1, 1025));
//    if (localSocket_ptr)
//        LOG(INFO) << "Given local socket: " << *localSocket_ptr;
//    else
//        LOG(INFO) << "Packed dropped";
//
////    std::this_thread::sleep_for(std::chrono::seconds(20));
//    localSocket_ptr = natMappings.processIncoming(Socket(gl_ip1, 80), Socket(nat_ip1, 1025));
//    if (localSocket_ptr)
//        LOG(INFO) << "Given local socket: " << *localSocket_ptr;
//    else
//        LOG(INFO) << "Packed dropped";
}

bool LearningSwitch::isNATSwitch(const Switch *sw) {
    return sw->id() == natSwitchId;
}

bool LearningSwitch::isOutComingPacket(Flow *flow) {
    return natSwitchLocalPorts.find(flow->loadInPort()) != natSwitchLocalPorts.end();
}

bool LearningSwitch::isTCPPacket(Flow *flow) {
//    LOG(INFO) << "Try to guess is it NAT? ethtype is " << flow->loadEthType() << "A";
    if (flow->loadEthType() != 0x0800 || flow -> loadIPProto() != 0x06) {
        return false;
    } else if ((AppObject::uint32_t_ip_to_string(flow->loadIPv4Src().getIPv4()) == "0.0.0.0") || (AppObject::uint32_t_ip_to_string(flow->loadIPv4Src().getIPv4()) == "255.255.255.255")) {
        return false;
    }
    return true;
}


/* LearningSwitch part */
std::pair<bool, switch_and_port>
LearningSwitch::findAndLearn(const switch_and_port& where,
                             const EthAddress& eth_src,
                             const EthAddress& eth_dst)
{
    std::pair<bool, switch_and_port> ret;
    ret.first = false;

    auto it_src = db.find(eth_src);
    auto it_dst = db.find(eth_dst);

    if (it_dst != db.end()) {
        ret.first = true;
        ret.second = it_dst->second;
    }

    // Learn new MAC or update old entry
    // TODO: implement migrations
    if (it_src == db.end()) {
        db_lock.lock();
        db[eth_src] = where;
        db_lock.unlock();

        LOG(INFO) << eth_src.to_string() << " seen at "
            << FORMAT_DPID << where.dpid << ':' << where.port;
    }

    return ret;
}

OFMessageHandler::Action LearningSwitch::Handler::processMissLearningSwitch(OFConnection* ofconn, Flow* flow)
{
    static EthAddress broadcast("ff:ff:ff:ff:ff:ff");

    // Get required fields
    EthAddress eth_src = flow->loadEthSrc();
    uint32_t   in_port = flow->loadInPort();
    EthAddress eth_dst = flow->loadEthDst();
    uint32_t out_port = 0;

    if (eth_src == broadcast) {
        DLOG(WARNING) << "Broadcast source address, dropping";
        return Stop;
    }

    // Observe our point
    Switch* sw = app->switch_manager->getSwitch(ofconn);
    if (sw) {
        switch_and_port where;
        where.dpid = sw->id();
        where.port = in_port;

        // Learn
        bool target_found;
        switch_and_port target;
        std::tie(target_found, target)
                = app->findAndLearn(where, eth_src, eth_dst);

        // Find path
        if (target_found) {
            if (where.dpid != target.dpid) {
                data_link_route route = app->topology->computeRoute(where.dpid, target.dpid);
                if (route.size() > 0) {
                    out_port = route[0].port;
                } else {
                    LOG(WARNING) << "Path between " << FORMAT_DPID << where.dpid 
                        << " and " << FORMAT_DPID << target.dpid << " not found";
                }
            } else {
                out_port = target.port;
            }
            emit app->newRoute(flow, eth_src.to_string(), eth_dst.to_string(), where.dpid, out_port);
        }
    }
    else
        return Stop;

    // Forward
    if (out_port) {
        flow->idleTimeout(60);
        flow->timeToLive(5*60);
        flow->add_action(new of13::OutputAction(out_port, 0));
        return Continue;
    } else {
        DVLOG(5) << "Flooding for address " << eth_dst.to_string();

        flow->setFlags(Flow::Disposable);
        STPPorts ports = app->stp->getSTP(sw->id());
        if (!ports.empty()) {
            for (auto port : ports) {
                if (port != in_port)
                    flow->add_action(new of13::OutputAction(port, 128));
            }
        }
        return Continue;
    }
}

bool LearningSwitch::isPrereq(const std::string &name) const
{
    return name == "link-discovery";
}
