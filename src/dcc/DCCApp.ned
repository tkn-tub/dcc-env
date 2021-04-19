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

package dcc;
import org.car2x.veins.base.modules.IBaseApplLayer;
import org.car2x.veins.modules.application.ieee80211p.BaseWaveApplLayer;

simple DCCApp like IBaseApplLayer
{
    parameters:
        @class(veins::dcc::DCCApp);

        // beacon parameters
        int headerLength = default(88bit) @unit(bit); //header length of the application
        int beaconLengthBits = default(256bit) @unit(bit); //the length of a beacon packet
        int beaconUserPriority = default(7); //the user priority (UP) of the beacon messages

        // DCC parameters
        double beaconIntervalRelaxed = default(0.1s) @unit(s); //the intervall between 2 beacon messages in the relaxed state
        double beaconIntervalActive = default(0.4s) @unit(s); //the intervall between 2 beacon messages in the active state
        double beaconIntervalRestrictive = default(1s) @unit(s); //the intervall between 2 beacon messages in the restrictied state
        double stateCheckInterval = default(1s) @unit(s); // the interval at which the state machine checks for changes
        double rampUpWindow = default(1s) @unit(s); // window size to compute channel busy ratio for becoming more restrictive
        double rampDownWindow = default(5s) @unit(s); // window size to compute channel busy ratio for becoming more relaxed

        double relaxedToActiveThreshold = default(0.15); // threshold for channelBusyRatio to go from relaxed to active
        double activeToRelaxedThreshold = default(0.15); // threshold for channelBusyRatio to go from active to relaxed
        double activeToRestrictiveThreshold = default(0.4); // threshold for channelBusyRatio to go from active to restrictive
        double restrictiveToActiveThreshold = default(0.4); // threshold for channelBusyRatio to go from restrictive to active
    gates:
        input lowerLayerIn; // from mac layer
        output lowerLayerOut; // to mac layer
        input lowerControlIn;
        output lowerControlOut;
}
