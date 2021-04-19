//
// Copyright (C) 2021 Dominik S. Buse <buse@ccs-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "dcc/DCCApp.h"

#include "veins/base/utils/FindModule.h"
#include "veins/base/modules/BaseMobility.h"
#include "veins/modules/mobility/traci/TraCIMobility.h"
#include "veins/modules/messages/DemoSafetyMessage_m.h"
#include "veins/modules/utility/Consts80211p.h"
#include "veins/modules/mac/ieee80211p/Mac1609_4.h"

Define_Module(veins::dcc::DCCApp);


namespace veins {
namespace dcc {

void DCCApp::initialize(int stage)
{
    BaseApplLayer::initialize(stage);

    if (stage == 0) {
        // set up beaconing timer
        auto triggerBeacon = [this]() { this->beacon(); };
        auto timerSpecBeacon = TimerSpecification(triggerBeacon)
            .relativeStart(uniform(0, par("beaconIntervalRelaxed")))
            .interval(par("beaconIntervalRelaxed"));
        beaconHandle = timerManager.create(timerSpecBeacon);

        // set up sampling timer
        timerManager.create(
            TimerSpecification([this]() { this->sampleDCC(); })
            .relativeStart(uniform(0, par("stateCheckInterval")))
            .interval(par("stateCheckInterval"))
        );

        // find mobility submodule
        auto mobilityModules = getSubmodulesOfType<TraCIMobility>(getParentModule());
        ASSERT(mobilityModules.size() == 1);
        mobility = mobilityModules.front();

        // register to channel busy/idle change signals
        auto channelBusyCallback = [this](veins::SignalPayload<bool> payload) {
            channelBusyHistory.emplace_back(simTime(), payload.p);
            // TODO: remove old entries!
        };
        signalManager.subscribeCallback(getParentModule(), Mac1609_4::sigChannelBusy, channelBusyCallback);
    }
}

void DCCApp::finish()
{
}

void DCCApp::handleSelfMsg(cMessage* msg)
{
    timerManager.handleMessage(msg);
}

void DCCApp::beacon()
{
    // just some demo content
    auto* dsm = new DemoSafetyMessage();

    dsm->setRecipientAddress(LAddress::L2BROADCAST());
    dsm->setBitLength(par("headerLength").intValue());
    dsm->setSenderPos(mobility->getPositionAt(simTime()));
    dsm->setSenderSpeed(mobility->getCurrentSpeed());
    dsm->setPsid(-1);
    dsm->setChannelNumber(static_cast<int>(Channel::cch));
    dsm->addBitLength(par("beaconLengthBits").intValue());
    dsm->setUserPriority(par("beaconUserPriority").intValue());

    sendDown(dsm);
}

void DCCApp::handleLowerMsg(cMessage* msg)
{
    EV_INFO << "Received beacon.\n";
    cancelAndDelete(msg);
}

double DCCApp::channelBusyRatio(simtime_t windowSize) const
{
    if (channelBusyHistory.empty()) {
        // Note: considering no data as busy is harder!
        //       If the recorded history was shorter than the window size, we would need to make up for that.
        //       By considereing no data as idle, this comes for free.
        EV_TRACE << "Channel busy history empty, considering channel idle\n";
        return 0.0;
    }

    simtime_t busyTime = 0;
    simtime_t currentTime = simTime();
    const simtime_t windowEnd = simTime() - windowSize;
    for (auto&& iter = channelBusyHistory.rbegin(); iter != channelBusyHistory.rend(); ++iter) {
        const simtime_t recordTime = iter->first;
        const bool channelBusy = iter->second;

        if (recordTime <= windowEnd) {
            // end of window reached, take last section and return
            if (channelBusy) { // channel is busy
                busyTime += currentTime - windowEnd;
            }
            break;
        }
        if (channelBusy) { // channel is busy
            busyTime += currentTime - recordTime;
        }
        currentTime = recordTime;
    }
    EV_TRACE << "Channel busy time was " << busyTime << " for window " << windowSize << "\n";
    return busyTime / windowSize;
}

void DCCApp::sampleDCC()
{
    double channelBusyRatioUp = channelBusyRatio(par("rampUpWindow"));
    double channelBusyRatioDown = channelBusyRatio(par("rampDownWindow"));
    EV_INFO << "DCC state check. "
        << "Ramp-Up/Ramp-Down Busy Ratio: " << channelBusyRatioUp << " / " << channelBusyRatioDown << ".\n";

    switch (state) {
        case State::RELAXED:
            if (channelBusyRatioUp >= par("relaxedToActiveThreshold").doubleValue()) switchToState(State::ACTIVE);
            break;
        case State::ACTIVE:
            if (channelBusyRatioUp >= par("activeToRestrictiveThreshold").doubleValue()) switchToState(State::RESTRICTIVE);
            if (channelBusyRatioDown < par("activeToRelaxedThreshold").doubleValue()) switchToState(State::RELAXED);
            break;
        case State::RESTRICTIVE:
            if (channelBusyRatioDown < par("restrictiveToActiveThreshold").doubleValue()) switchToState(State::ACTIVE);
            break;
    }
}

void DCCApp::switchToState(State newState)
{
    EV_INFO << "DCC state switch: " << state << " -> " << newState << "\n";
    state = newState;

    // TODO: re-schedule next beacon
}

std::ostream& operator<<(std::ostream& os, DCCApp::State state)
{
    switch(state)
    {
        case DCCApp::State::RELAXED: os << "RELAXED"; break;
        case DCCApp::State::ACTIVE: os << "ACTIVE"; break;
        case DCCApp::State::RESTRICTIVE: os << "RESTRICTIVE"; break;
        default    : os.setstate(std::ios_base::failbit);
    }
    return os;
}

} // namespace dcc
} // namespace veins
