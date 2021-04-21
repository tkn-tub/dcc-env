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

#pragma once

#include "veins/veins.h"

#include <unordered_map>

#include "veins/base/modules/BaseApplLayer.h"
#include "veins/base/utils/Coord.h"
#include "veins/modules/utility/SignalManager.h"
#include "veins/modules/utility/TimerManager.h"

namespace veins {

class BaseMobility;

namespace dcc {

class DCCApp : public BaseApplLayer {
public:
    ~DCCApp() override = default;

    // OMNeT++ module interface implementation
    void initialize(int stage) override;
    void finish() override;
    void handleSelfMsg(cMessage* msg) override;

    // CAM sending
    void beacon();
    void handleLowerMsg(cMessage* msg) override;

    double channelBusyRatio(simtime_t windowSize) const;

    enum class State {
        relaxed,
        active,
        restrictive,
    };

    struct Neighbor {
        std::string vehicleId;
        Coord position;
        Coord speed;
        simtime_t timestamp;
    };

protected:
    SignalManager signalManager;
    TimerManager timerManager{this};
    BaseMobility* mobility;
    std::unordered_map<std::string, Neighbor> neighbors;

private:
    std::vector<std::pair<simtime_t, bool>> channelBusyHistory;
    TimerManager::TimerHandle beaconHandle = 0;
    State state = State::restrictive;

    double relaxedToActiveThreshold;     // threshold for channelBusyRatio to go from relaxed to active
    double activeToRelaxedThreshold;     // threshold for channelBusyRatio to go from active to relaxed
    double activeToRestrictiveThreshold; // threshold for channelBusyRatio to go from active to restrictive 
    double restrictiveToActiveThreshold; // threshold for channelBusyRatio to go from restrictive to active 

    simtime_t currentBeaconInterval() const;
    void rescheduleBeacon(simtime_t beaconInterval, TimerManager::TimerHandle handle=0);
    void sampleDCC();
    void switchToState(State newState);
};

std::ostream& operator<<(std::ostream& os, DCCApp::State state);

} // namespace dcc
} // namespace veins
