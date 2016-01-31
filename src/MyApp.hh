#pragma once

#include "Application.hh"
#include "Loader.hh"
#include "OFMessageHandler.hh"

class MyApp : public Application, public OFMessageHandlerFactory {
SIMPLE_APPLICATION(MyApp, "myapp")
public:
    void init(Loader* loader, const Config& config) override;
    std::string orderingName() const override { return "mac-filtering"; }
    bool isPostreq(const std::string &name) const override {return (name == "forwarding"); }
    std::unique_ptr<OFMessageHandler> makeOFMessageHandler() override {return new Handler();}
public slots:
    void onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr);

private:
    class Handler: public OFMessageHandler {
    public:
        Action processMiss(OFConnection* ofconn, Flow* flow) override;
    };
};