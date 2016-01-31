#include <glog/logging.h>
#include "MyApp.hh"
#include "Controller.hh"

REGISTER_APPLICATION(MyApp, {"controller", ""})

void MyApp::init(Loader *loader, const Config& config)
{
    Controller* ctrl = Controller::get(loader);
    ctrl->registerHandler(this);
    QObject::connect(ctrl, &Controller::switchUp,
                     this, &MyApp::onSwitchUp);
    LOG(INFO) << "Hello, world!";
}

void MyApp::onSwitchUp(OFConnection* ofconn, of13::FeaturesReply fr)
{
    LOG(INFO) << "Look! This is a switch " << FORMAT_DPID << fr.auxiliary_id();
}

OFMessageHandler::Action MyApp::Handler::processMiss(OFConnection *ofconn, Flow *flow) {
    if (flow->match(of13::EthSrc("00:11:22:33:44:55"))) {
        return Stop;
    } else return Continue;
}
