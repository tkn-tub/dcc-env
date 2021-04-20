//
// Copyright (C) 2020 Dominik S. Buse <buse@ccs-labs.org>, Max Schettler <schettler@ccs-labs.org>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
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

#include "dcc/GymConnection.h"

Define_Module(GymConnection);

void GymConnection::initialize()
{
    // do we want to use this at all?
    if (!par("enable")) {
        return;
    }

    // determine host and port from params and ENV variables
    std::string host = par("host");
    int port = par("port");
    if (host == "") { // param empty, check environment
        if (std::getenv("VEINS_GYM_HOST") != nullptr) {
            host = std::getenv("VEINS_GYM_HOST");
        } else {
            // param and environment empty, fail
            throw omnetpp::cRuntimeError("Gym host not configured! Change your ini file or set VEINS_GYM_HOST");
        }
    }

    if (port < 0) { // param "empty", check environment
        if (std::getenv("VEINS_GYM_PORT") != nullptr) {
            port = std::atoi(std::getenv("VEINS_GYM_PORT"));
        } else {
            // param and environment empty, fail
            throw omnetpp::cRuntimeError("Gym port not configured! Change your ini file or set VEINS_GYM_PORT");
        }
    }

    EV_INFO << "Connecting to server 'tcp://" << host << ":" << port << "'\n";
    socket.connect("tcp://" + host + ":" + std::to_string(port));

    veinsgym::proto::Request init_request;
    *(init_request.mutable_init()->mutable_observation_space_code()) = par("observation_space").stdstringValue();
    *(init_request.mutable_init()->mutable_action_space_code()) = par("action_space").stdstringValue();
    communicate(init_request); // ignore (empty) reply

    // now talk to the agent again to get the first action
    EV_INFO << "GymConnection asking the agent for the initial config\n";
    veinsgym::proto::Request action_request;
    action_request.set_id(0);
    auto *values = action_request.mutable_step()->mutable_observation()->mutable_box()->mutable_values();
    // *values = {observation.begin(), observation.end()};
    action_request.mutable_step()->mutable_reward()->mutable_box()->mutable_values()->Add();
    // request.mutable_step()->mutable_reward()->mutable_box()->set_values(0, reward);
    auto reply = communicate(action_request);
    EV_INFO << "GymConnection got action values: ";
    size_t index = 0;
    for (auto value: reply.action().box().values()) {
        config[index] = value;
        ++index;
        EV_INFO << value << ", ";
    }
    EV_INFO << std::endl;
    ASSERT(index == 4);

    // set up regular communication timer
    timerManager.create(
        veins::TimerSpecification([this]() { this->update(); })
        .relativeStart(1.0)
        .interval(1.0)
    );
}

void GymConnection::update()
{
    EV_INFO << "GymConnection communicating with the agent in a regular interval\n";
    veinsgym::proto::Request action_request;
    action_request.set_id(simTime().inUnit(SIMTIME_S));
    auto *values = action_request.mutable_step()->mutable_observation()->mutable_box()->mutable_values();
    // *values = {observation.begin(), observation.end()};
    action_request.mutable_step()->mutable_reward()->mutable_box()->mutable_values()->Add();
    // request.mutable_step()->mutable_reward()->mutable_box()->set_values(0, reward);
    auto reply = communicate(action_request);
    // TODO: add observations and maybe use reply
}

std::array<double, 4> GymConnection::getConfig() const
{
    return config;
}

void GymConnection::handleMessage(cMessage* msg)
{
    timerManager.handleMessage(msg);
}

void GymConnection::finish()
{
    EV_TRACE << "Finish called for GymConnection.\n";
}

veinsgym::proto::Reply GymConnection::communicate(veinsgym::proto::Request request)
{
    veinsgym::proto::Reply reply;
    // do we want to use this at all?
    if (par("enable")) {
        std::string request_msg = request.SerializeAsString();
        socket.send(zmq::message_t(request_msg.data(), request_msg.size()), zmq::send_flags::none);
        zmq::message_t response_msg;
        socket.recv(response_msg);
        std::string response(static_cast<char*>(response_msg.data()), response_msg.size());
        reply.ParseFromString(response);
    }
    return reply;
}
