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
#include "veins/modules/utility/Consts80211p.h"
#include "veins/modules/mac/ieee80211p/Mac1609_4.h"
#include "dcc/Beacon_m.h"
#include "dcc/GymConnection.h"

Define_Module(veins::dcc::DCCApp);


namespace veins {
namespace dcc {

void DCCApp::initialize(int stage)
{
    BaseApplLayer::initialize(stage);

    if (stage == 0) {
        // set up beaconing timer
        rescheduleBeacon(currentBeaconInterval(), 0);

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
            // prune old entries
            simtime_t windowEnd = simTime() - std::max(par("rampUpWindow").doubleValue(), par("rampDownWindow").doubleValue());
            while (channelBusyHistory.front().first < windowEnd) {
                channelBusyHistory.erase(channelBusyHistory.begin());
            }
        };
        signalManager.subscribeCallback(getParentModule(), Mac1609_4::sigChannelBusy, channelBusyCallback);

        // get config from the Gym
        auto& gymCon = *veins::FindModule<GymConnection*>::findGlobalModule();
        relaxedToActiveThreshold = gymCon.getConfig()[0];
        activeToRelaxedThreshold = gymCon.getConfig()[1];
        activeToRestrictiveThreshold = gymCon.getConfig()[2];
        restrictiveToActiveThreshold = gymCon.getConfig()[3];
    }
}

void DCCApp::finish()
{
    EV_TRACE << "Finish called for DCCApp of " << getParentModule()->getFullPath() << ".\n";
}

void DCCApp::handleSelfMsg(cMessage* msg)
{
    timerManager.handleMessage(msg);
}

void DCCApp::beacon()
{
    // just some demo content
    auto* beacon = new Beacon();

    beacon->setRecipientAddress(LAddress::L2BROADCAST());
    beacon->setBitLength(par("headerLength").intValue());
    beacon->setSenderPos(mobility->getPositionAt(simTime()));
    beacon->setSenderSpeed(mobility->getCurrentSpeed());
    beacon->setSenderId(getParentModule()->getFullPath().c_str());
    std::stringstream stateStringStream;
    stateStringStream << state;
    beacon->setSenderState(stateStringStream.str().c_str());
    beacon->setPsid(-1);
    beacon->setChannelNumber(static_cast<int>(Channel::cch));
    beacon->addBitLength(par("beaconLengthBits").intValue());
    beacon->setUserPriority(par("beaconUserPriority").intValue());

    sendDown(beacon);
}

void DCCApp::handleLowerMsg(cMessage* msg)
{
    auto* beacon = check_and_cast<Beacon*>(msg);
    std::string senderId{beacon->getSenderId()};
    EV_INFO << "Received beacon from " << senderId << "(" << beacon->getSenderState() << ") at " << getParentModule()->getFullPath() << "; ";
    if (neighbors.find(senderId) == neighbors.end()) {
        EV_INFO << "previously unknown\n";
    }
    else {
        EV_INFO << "last info from " << (simTime() - neighbors[senderId].timestamp).inUnit(SIMTIME_MS) << "ms ago.\n";
    }
    neighbors[senderId] = { senderId, beacon->getSenderPos(), beacon->getSenderSpeed(), simTime() };
    cancelAndDelete(msg);
}

double DCCApp::channelBusyRatio(simtime_t windowSize) const
{
    if (channelBusyHistory.empty()) {
        EV_TRACE << "Channel busy history empty, considering as busy\n";
        return 1.0;
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
            currentTime = windowEnd;
            break;
        }
        if (channelBusy) { // channel is busy
            busyTime += currentTime - recordTime;
        }
        currentTime = recordTime;
    }

    // make up for missing time if we don't have enough history -- we treat unknown time as busy time
    busyTime += std::max(SimTime(0), currentTime - windowEnd);

    EV_TRACE << "Channel busy time was " << busyTime << " for window " << windowSize << "\n";
    return busyTime / windowSize;
}

simtime_t DCCApp::currentBeaconInterval() const
{
    switch (state) {
        case State::relaxed:
            return par("beaconIntervalRelaxed").doubleValue();
            break;
        case State::active:
            return par("beaconIntervalActive").doubleValue();
            break;
        case State::restrictive:
            return par("beaconIntervalRestrictive").doubleValue();
            break;
    }
}

void DCCApp::rescheduleBeacon(simtime_t beaconInterval, TimerManager::TimerHandle handle)
{
    if (handle != 0) {
        timerManager.cancel(handle);
    }
    beaconHandle = timerManager.create(
        TimerSpecification([this]() { this->beacon(); })
        .relativeStart(uniform(0, beaconInterval))
        .interval(beaconInterval)
    );
}

void DCCApp::sampleDCC()
{
    double channelBusyRatioUp = channelBusyRatio(par("rampUpWindow"));
    double channelBusyRatioDown = channelBusyRatio(par("rampDownWindow"));
    EV_TRACE << "DCC state check. "
        << "Ramp-Up/Ramp-Down Busy Ratio: " << channelBusyRatioUp << " / " << channelBusyRatioDown << ".\n";

    switch (state) {
        case State::relaxed:
            if (channelBusyRatioUp >= relaxedToActiveThreshold) switchToState(State::active);
            break;
        case State::active:
            if (channelBusyRatioUp >= activeToRestrictiveThreshold) switchToState(State::restrictive);
            if (channelBusyRatioDown < activeToRelaxedThreshold) switchToState(State::relaxed);
            break;
        case State::restrictive:
            if (channelBusyRatioDown < restrictiveToActiveThreshold) switchToState(State::active);
            break;
    }
}

void DCCApp::switchToState(State newState)
{
    EV_INFO << "DCC state switch: " << state << " -> " << newState << "\n";
    state = newState;

    // re-schedule next beacon
    auto beaconInterval = currentBeaconInterval();
    rescheduleBeacon(beaconInterval, beaconHandle);
}

std::ostream& operator<<(std::ostream& os, DCCApp::State state)
{
    switch(state)
    {
        case DCCApp::State::relaxed: os << "RELAXED"; break;
        case DCCApp::State::active: os << "ACTIVE"; break;
        case DCCApp::State::restrictive: os << "RESTRICTIVE"; break;
        default    : os.setstate(std::ios_base::failbit);
    }
    return os;
}

} // namespace dcc
} // namespace veins
