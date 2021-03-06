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

#pragma once

#include <mutex>
#include <unordered_map>
#include <set>

#include "Common.hh"
#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"
#include "ILinkDiscovery.hh"
#include "FluidUtils.hh"
#include "NATHelper.hh"
#include "Switch.hh"

class LearningSwitch : public Application, OFMessageHandlerFactory {
SIMPLE_APPLICATION(LearningSwitch, "learning-switch")
Q_OBJECT
/* LearningSwitch part */
public:
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override { return "forwarding"; }
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override 
    { return std::unique_ptr<OFMessageHandler>(new Handler(this)); }
    bool isPrereq(const std::string &name) const;


    std::pair<bool, switch_and_port>
    findAndLearn(const switch_and_port& where,
                 const EthAddress& src,
                 const EthAddress& dst);
signals:
    void newRoute(Flow* flow, std::string src, std::string dst, uint64_t dpid, uint32_t out_port);
private:
    /* LearningSwitch part */
    class Handler: public OFMessageHandler {
    public:
        Handler(LearningSwitch* app_) : app(app_) { }
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
        Action processMissLearningSwitch(OFConnection* ofconn, Flow* flow);
    private:
        LearningSwitch* app;
    };

    class Topology* topology;
    class SwitchManager* switch_manager;
    class STP* stp;

    std::mutex db_lock;
    std::unordered_map<EthAddress, switch_and_port> db;

/* NAT part */
private:
    uint32_t natSwitchId;
    std::set<uint32_t> natSwitchLocalPorts;
    NATMappings natMappings;

    void parseNATConfig(const std::string &config_file);
    bool isNATSwitch(const Switch* sw);
    bool isOutComingPacket(Flow *flow);
    bool isTCPPacket(Flow *flow);
};

struct NATSettingsJsonParseError : std::exception {};
