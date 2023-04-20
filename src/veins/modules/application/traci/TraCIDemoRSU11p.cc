//
// Copyright (C) 2016 David Eckhoff <david.eckhoff@fau.de>
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

#include "veins/modules/application/traci/TraCIDemoRSU11p.h"

#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

using namespace veins;

Define_Module(veins::TraCIDemoRSU11p);

void TraCIDemoRSU11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    messageReceivedCount = 0;
    nNos = "200";
    qSize = "20000";
    runNumber = std::to_string(getEnvir()->getConfigEx()->getActiveRunNumber());
}

void TraCIDemoRSU11p::finish()
{
    DemoBaseApplLayer::finish();
    std::ofstream out;
    std::cout << "CHEGOU ESSA QUANTIDADE DE MENSAGEM NO RSU " << myId << ": " << messageReceivedCount << endl;
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/" + runNumber + "/entregues" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << messageReceivedCount << std::endl;
    out.close();
//    std::cout << "CHEGOU ESSA QUANTIDADE DE MENSAGEM NO RSU " << myId << ": " << messageReceivedCount << endl;
}

void TraCIDemoRSU11p::onWSA(DemoServiceAdvertisment* wsa)
{
    // if this RSU receives a WSA for service 42, it will tune to the chan
    if (wsa->getPsid() == 42) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
    }
}

void TraCIDemoRSU11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);
    bool isValidMessage = true;
    if ( myId == wsm->getFinalAddress()) {
        std::vector<int> receivedMessages = receivedMessagesMap[wsm->getSenderAddress()];
        for (std::vector<int>::iterator id = receivedMessages.begin(); id != receivedMessages.end(); id++) {
            if (*id == wsm->getMessageId()) {
                isValidMessage = false;
            }
        }
        if (isValidMessage) {
            receivedMessages.push_back(wsm->getMessageId());
            receivedMessagesMap[wsm->getSenderAddress()] = receivedMessages;
            wsm->setEndTime(simTime());
            messageReceivedCount++;
            simtime_t delay = wsm->getEndTime() - wsm->getBeginTime();
//           std::cout << "######" << delay << endl;
            std::ofstream out;
            out.open("/home/joao/Documentos/TCC/newProtocol/resultados/" + runNumber + "/delay" + qSize + "_" + nNos + ".txt", std::ios::app);
            out << delay << std::endl;
            out.close();
        }
    }
}
