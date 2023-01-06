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

#include "veins/modules/application/traci/TraCIDemo11p.h"

#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

using namespace veins;

Define_Module(veins::TraCIDemo11p);

TraCIDemo11pMessage* TraCIDemo11p::createMessage(int to, int senderId, std::string dado, int messageId, int finalAddress, simtime_t beginTime = NULL)
{
    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    populateWSM(wsm, to);
    wsm->setFinalAddress(finalAddress);
    wsm->setSenderAddress(senderId);
    wsm->setDemoData(dado.c_str());
    wsm->setMessageId(messageId);
    if (beginTime == NULL)
        wsm->setBeginTime(simTime());
    else
        wsm->setBeginTime(beginTime);
    return wsm;
}

void TraCIDemo11p::createAndSendMessage(int to, int senderId, int messageId, int finalAddress, simtime_t beginTime = NULL)
{
    std::string dado = "Mensagem para o node " + std::to_string(to) + " enviado do " + std::to_string(myId);
    TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    populateWSM(wsm, to);
    wsm->setFinalAddress(finalAddress);
    wsm->setSenderAddress(senderId);
    wsm->setDemoData(dado.c_str());
    wsm->setMessageId(messageId);
    if (beginTime == NULL)
        wsm->setBeginTime(simTime());
    else
        wsm->setBeginTime(beginTime);
    sendDown(wsm);
}

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        messageSendInterval = 20;
        timeToSendMessage = simTime() + messageSendInterval;
        rsuIds[0] = 13;
        rsuIds[1] = 18;
        rsuIds[2] = 23;
        rsuIds[3] = 28;
        rsuCoords[13] = Coord(1000, 500, 3);
        rsuCoords[18] = Coord(1500, 500, 3);
        rsuCoords[23] = Coord(1000, 1000, 3);
        rsuCoords[28] = Coord(1500, 1000, 3);
        messageGeneratedCount = 0;
        replicatedMessagesCount = 0;
        queue = 0;
        queueSize = 16000000;
    }
}

void TraCIDemo11p::finish()
{
    DemoBaseApplLayer::finish();
    std::ofstream out;
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/200/geradas.txt", std::ios::app);
    out << messageGeneratedCount << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/200/replicadas.txt", std::ios::app);
    out << replicatedMessagesCount << std::endl;
    out.close();
//    std::cout << "FOI GERADA ESSA QUANTIDADE DE MENSAGENS DO NODE " << myId << ": " << messageGeneratedCount << endl;
//    std::cout << "FOI GERADA ESSA QUANTIDADE DE MENSAGENS REPLICADAS DO NODE " << myId << ": " << replicatedMessagesCount << endl;
}

void TraCIDemo11p::onBSM(DemoSafetyMessage* bsm)
{
    std::map<int, Coord> myRoutingMap = routingMap[myId];
    myRoutingMap[bsm->getSenderID()] = bsm->getSenderPos();
    routingMap[myId] = myRoutingMap;

    std::map<int, simtime_t> myRoutingMapLastBeaconReceivedTime = routingMapLastBeaconReceivedTime[myId];
    myRoutingMapLastBeaconReceivedTime[bsm->getSenderID()] = simTime();
    routingMapLastBeaconReceivedTime[myId] = myRoutingMapLastBeaconReceivedTime;
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    if (getCurrentQueueSize() + wsm->getByteLength() < getQueueSize()) {
        addTaskSizeToQueue(wsm->getByteLength());
        if (wsm->getBeginTime() + 0.5 > simTime()) {
            if (wsm->getSenderAddress() != myId) {

                    std::map<int, int> cControl = cControlMap[wsm->getSenderAddress()];
                    std::map<int, int> sControl = sControlMap[wsm->getSenderAddress()];
                    std::map<int, simtime_t> tControl = tControlMap[wsm->getSenderAddress()];
                    std::map<int, std::vector<simtime_t>> lControl = lControlMap[wsm->getSenderAddress()];

                    int c = cControl[wsm->getMessageId()];
                    int s = sControl[wsm->getMessageId()];
                    simtime_t t = tControl[wsm->getMessageId()];
                    std::vector<simtime_t> l = lControl[wsm->getMessageId()];

                    if (t == NULL) t = 0;

                    if (t == 0) {
                        c = 1;
                        s = 0;
                        t = simTime();


                        TraCIDemo11pMessage* newWsm = new TraCIDemo11pMessage();
                        populateWSM(newWsm, wsm->getFinalAddress());
                        newWsm->setFinalAddress(wsm->getFinalAddress());
                        newWsm->setSenderAddress(wsm->getSenderAddress());
                        newWsm->setDemoData(wsm->getDemoData());
                        newWsm->setMessageId(wsm->getMessageId());
                        newWsm->setBeginTime(wsm->getBeginTime());
                        newWsm->setKind(SEND_AID_MESSAGE);

                        delay = uniform(0, par("randomDelayTimeMax").doubleValue());
                        if (sendBeaconEvt->isScheduled()) cancelEvent(sendBeaconEvt);
                        scheduleAt(simTime() + delay, newWsm);
                    } else {
                        c++;
                        simtime_t t1 = simTime();
                        l.push_back(t1 - t);
                        t = t1;
                    }

                    cControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = c;
                    sControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = s;
                    tControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = t;
                    lControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = l;
                }
        }
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    if (msg->getKind() == SEND_AID_MESSAGE) {
        TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg);
        int senderId = wsm->getSenderAddress();
        int messageId = wsm->getMessageId();
        int finalAddress = wsm->getFinalAddress();

        std::map<int, int> cControl = cControlMap[wsm->getSenderAddress()];
        std::map<int, int> sControl = sControlMap[wsm->getSenderAddress()];
        std::map<int, simtime_t> tControl = tControlMap[wsm->getSenderAddress()];
        std::map<int, std::vector<simtime_t>> lControl = lControlMap[wsm->getSenderAddress()];

        int c = cControl[wsm->getMessageId()];
        int s = sControl[wsm->getMessageId()];
        simtime_t t = tControl[wsm->getMessageId()];
        std::vector<simtime_t> l = lControl[wsm->getMessageId()];

        if (c == 1) {
            t = 0;
            l.clear();
            replicatedMessagesCount++;
            sendRoutingMessage(wsm->getFinalAddress(), wsm->getSenderAddress(), wsm->getBeginTime(), wsm->getMessageId());
        } else {
            simtime_t interArrivalTimeBase = delay / c;
            for (std::vector<simtime_t>::iterator i = l.begin(); i != l.end(); i++) {
                if (interArrivalTimeBase - (*i) > 0) {
                    s--;
                } else {
                    s++;
                }
            }
            if (s > 0) {
                t = 0;
                l.clear();
                replicatedMessagesCount++;
                sendRoutingMessage(wsm->getFinalAddress(), wsm->getSenderAddress(), wsm->getBeginTime(), wsm->getMessageId());
            }
        }

        cControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = c;
        sControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = s;
        tControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = t;
        lControlMap[wsm->getSenderAddress()][wsm->getMessageId()] = l;
        removeTaskSizeToQueue(wsm->getByteLength());
    }

    if (!sendBeaconEvt->isScheduled()) {
        DemoSafetyMessage* bsm = new DemoSafetyMessage();
        populateWSM(bsm);
        sendDown(bsm);
        scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
    }
}

void TraCIDemo11p::sendRoutingMessage(int rsuId, int senderAddress, simtime_t beginTime, int messageId)
{
    std::map<int, Coord> myRoutingMap = routingMap[myId];
    bool alreadySendMessage = false;
    int mySortedRoutingMapByDistanceToDestinyArray[myRoutingMap.size()];
    int mySortedRoutingMapByDistanceToDestinyArrayIndex = 0;

    for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
        if (it->first == rsuId) {
            createAndSendMessage(rsuId, senderAddress, messageId, rsuId, beginTime);
            alreadySendMessage = true;
            break;
        }
    }
    if (!alreadySendMessage) {
        Coord rsuCoord = rsuCoords[rsuId];
        float currentDistance = curPosition.distance(rsuCoord);
        for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
            int targetId = it->first;
            Coord targetNodeCoord = it->second;
            float targetDistanceToRsu = targetNodeCoord.distance(rsuCoord);
            if (currentDistance > targetDistanceToRsu+10) {
                if (!isADenseRoad()) {
                    createAndSendMessage(targetId, senderAddress, messageId, rsuId, beginTime);
                } else {
                    mySortedRoutingMapByDistanceToDestinyArrayIndex = insertSorted(mySortedRoutingMapByDistanceToDestinyArray, mySortedRoutingMapByDistanceToDestinyArrayIndex, targetDistanceToRsu, myRoutingMap.size(), targetId);
                }
            }
        }
        if (isADenseRoad()) {
            int indexToStart = mySortedRoutingMapByDistanceToDestinyArrayIndex - std::ceil(mySortedRoutingMapByDistanceToDestinyArrayIndex * 0.3);
            for (int i = indexToStart; i < mySortedRoutingMapByDistanceToDestinyArrayIndex; i++) {
                int targetId = mySortedRoutingMapByDistanceToDestinyArray[i];
                createAndSendMessage(targetId, senderAddress, messageId, rsuId, beginTime);
            }
        }
    }
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    simtime_t currentTime = simTime();
    std::map<int, Coord> myRoutingMap = routingMap[myId];
    std::map<int, simtime_t> myRoutingMapLastBeaconReceivedTime = routingMapLastBeaconReceivedTime[myId];
    std::map<int, simtime_t> myRoutingMapLastBeaconReceivedTimeAux = myRoutingMapLastBeaconReceivedTime;

    for (auto it = myRoutingMapLastBeaconReceivedTimeAux.begin(); it != myRoutingMapLastBeaconReceivedTimeAux.end(); ++it) {
        if (currentTime > it->second + 10) {
            myRoutingMap.erase(it->first);
            myRoutingMapLastBeaconReceivedTime.erase(it->first);
        }
    }

    int mySortedRoutingMapByDistanceToDestinyArray[myRoutingMap.size()];
    int mySortedRoutingMapByDistanceToDestinyArrayIndex = 0;


    routingMap[myId] = myRoutingMap;
    routingMapLastBeaconReceivedTime[myId] = myRoutingMapLastBeaconReceivedTime;

    bool alreadySendMessage = false;
    DemoBaseApplLayer::handlePositionUpdate(obj);
    if (myId % 5 == 0 && simTime() == timeToSendMessage) {
        messageGeneratedCount++;
        int rsuId = rsuIds[rand() % 4];
        Coord rsuCoord = rsuCoords[rsuId];
        // Se a RSU esta ao alcance do nó, deve enviar diretamente
        for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
            if (it->first == rsuId) {
                createAndSendMessage(rsuId, myId, messageGeneratedCount, rsuId, NULL);
                alreadySendMessage = true;
                break;
            }
        }
        if (!alreadySendMessage) {
            float currentDistance = curPosition.distance(rsuCoord);
            for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
                Coord targetNodeCoord = it->second;
                int targetId = it->first;
                float targetDistanceToRsu = targetNodeCoord.distance(rsuCoord);
                if (currentDistance > targetDistanceToRsu+10) {
                    if (!isADenseRoad()) {
                        createAndSendMessage(targetId, myId, messageGeneratedCount, rsuId, NULL);
                    } else {
                        mySortedRoutingMapByDistanceToDestinyArrayIndex = insertSorted(mySortedRoutingMapByDistanceToDestinyArray, mySortedRoutingMapByDistanceToDestinyArrayIndex, targetDistanceToRsu, myRoutingMap.size(), targetId);
                    }
                }
            }
            if (isADenseRoad()) {
                int indexToStart = mySortedRoutingMapByDistanceToDestinyArrayIndex - std::ceil(mySortedRoutingMapByDistanceToDestinyArrayIndex * 0.3);
                for (int i = indexToStart; i < mySortedRoutingMapByDistanceToDestinyArrayIndex; i++) {
                    int targetId = mySortedRoutingMapByDistanceToDestinyArray[i];
                    createAndSendMessage(targetId, myId, messageGeneratedCount, rsuId, NULL);
                }
            }
        }
        timeToSendMessage = timeToSendMessage + messageSendInterval;
    }
}

int TraCIDemo11p::getNeighborsNumber() {
    return routingMap[myId].size();
}

bool TraCIDemo11p::isADenseRoad() {
    return getNeighborsNumber() > 5;
}

int getArraySize(int sizeInBytes) {
    return sizeInBytes / sizeof(int);
}

int TraCIDemo11p::insertSorted(int arr[], int n, int key, int capacity, int nodeID)
{
    // Cannot insert more elements if n is already
    // more than or equal to capacity
    if (n >= capacity)
        return n;

    int i;
    for (i = n - 1; (i >= 0 && arr[i] > key); i--)
        arr[i + 1] = arr[i];

    arr[i + 1] = nodeID;

    return (n + 1);
}

int TraCIDemo11p::getQueueSize() {
    return queueSize;
}

void TraCIDemo11p::addTaskSizeToQueue(int value) {
    queue = queue + value;
}

void TraCIDemo11p::removeTaskSizeToQueue(int value) {
    queue = queue - value;
}

int TraCIDemo11p::getCurrentQueueSize() {
    return queue;
}
