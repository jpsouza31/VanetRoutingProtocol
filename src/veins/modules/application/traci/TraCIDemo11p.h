//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
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

#pragma once

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"
#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"
#include <vector>
#include <map>
#include <math.h>
#include <algorithm>
#include <stdio.h>

namespace veins {

/**
 * @brief
 * A tutorial demo for TraCI. When the car is stopped for longer than 10 seconds
 * it will send a message out to other cars containing the blocked road id.
 * Receiving cars will then trigger a reroute via TraCI.
 * When channel switching between SCH and CCH is enabled on the MAC, the message is
 * instead send out on a service channel following a Service Advertisement
 * on the CCH.
 *
 * @author Christoph Sommer : initial DemoApp
 * @author David Eckhoff : rewriting, moving functionality to DemoBaseApplLayer, adding WSA
 *
 */

class VEINS_API TraCIDemo11p : public DemoBaseApplLayer {
public:
    void initialize(int stage) override;
    void finish() override;

protected:
    simtime_t lastDroveAt;
    bool sentMessage;
    int currentSubscribedServiceId;
    int rsuIds[4];
    std::map<int, Coord> rsuCoords;
    std::map<int, std::map<int,Coord>> routingMap;
    std::map<int, std::map<int,simtime_t>> routingMapLastBeaconReceivedTime;
    int messageSendInterval;
    simtime_t timeToSendMessage;
    int messageReceivedCount;
    int messageGeneratedCount;
    int replicatedMessagesCount;
    int receivedMessagesCount;

    double delay;
    std::map<int, std::map<int, int>> cControlMap;
    std::map<int, std::map<int, int>> sControlMap;
    std::map<int, std::map<int, simtime_t>> tControlMap;
    std::map<int, std::map<int, std::vector<simtime_t>>> lControlMap;

    int queueSize;
    int queue;
    std::string nNos;
    std::string qSize;
    std::string runNumber;
    int stackOverflowNumber;

protected:
    void onWSM(BaseFrame1609_4* wsm) override;
    void onWSA(DemoServiceAdvertisment* wsa) override;
    void onBSM(DemoSafetyMessage* bsm) override;

    void handleSelfMsg(cMessage* msg) override;
    void handlePositionUpdate(cObject* obj) override;

    TraCIDemo11pMessage* createMessage (int to, int senderId, std::string dado, int messageId, int finalAddress, simtime_t beginTime);
//    TraCIDemo11pMessage* createRoutingMessage (int to, int senderId, int finalAddress, std::string dado, int messageId, simtime_t beginTime);
    void createAndSendMessage (int to, int senderId, int messageId, int finalAddress, simtime_t beginTime);
    void sendRoutingMessage(int rsuId, int senderAddress, simtime_t beginTime, int messageId);

    int getNeighborsNumber();
    bool isADenseRoad();
    std::vector<int> insertSorted(std::map<int, Coord> myMap, Coord currentPos);

    int getQueueSize();
    void addTaskSizeToQueue(int value);
    void removeTaskSizeToQueue(int value);
    int getCurrentQueueSize();
};

} // namespace veins
