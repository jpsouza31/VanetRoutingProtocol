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
    populateWSM(wsm);
    wsm->setFinalAddress(finalAddress);
    wsm->setSenderAddress(senderId);
    wsm->setMessageLength(1);
//    wsm->setByteLength(500000);
    wsm->setDemoData(dado.c_str());
    wsm->setMessageId(messageId);
    wsm->setSerial(to);
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
    wsm->setMessageLength(500000);
//    wsm->setByteLength(500000);
    wsm->setDemoData(dado.c_str());
    wsm->setMessageId(messageId);
    wsm->setSerial(to);
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
        messageSendInterval = 15;
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
        receivedMessagesCount = 0;
        queue = 0;
        queueSize = 20000;
        nNos = "200";
        qSize = "20000";
        runNumber = std::to_string(getEnvir()->getConfigEx()->getActiveRunNumber());
        stackOverflowNumber= 0;
    }
}

void TraCIDemo11p::finish()
{
    DemoBaseApplLayer::finish();
    std::ofstream out;
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/" + runNumber + "/geradas" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << messageGeneratedCount << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/" + runNumber + "/replicadas" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << replicatedMessagesCount << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/" + runNumber + "/overflow" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << stackOverflowNumber << std::endl;
    out.close();
    out.open("/home/joao/Documentos/TCC/newProtocol/resultados/" + runNumber + "/recebidas" + qSize + "_" + nNos + ".txt", std::ios::app);
    out << receivedMessagesCount << std::endl;
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
    if (wsm->getBeginTime() + 0.2 > simTime()) {
        receivedMessagesCount++;
        int rsuId = wsm->getFinalAddress();
        int senderAddress = wsm->getSenderAddress();
        int messageId = wsm->getMessageId();
        simtime_t  beginTime = wsm->getBeginTime();
        bool alreadySendMessage = false;
        if (wsm->getSerial() == myId) {
            std::map<int, Coord> myRoutingMap = routingMap[myId];
            for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
                if (it->first == rsuId) {
                    createAndSendMessage(rsuId, senderAddress, messageId, rsuId, beginTime);
                    alreadySendMessage = true;
    //                std::cout << "AQUI" << endl;
                    break;
                }
            }


    //        std::cout << "HIHIHIHI: " << wsm->getSerial() << " - " << myId << endl;
    //            if (wsm->getBeginTime() + 0.5 > simTime()) {
            if (!alreadySendMessage) {
                if ((getCurrentQueueSize() + 1) < getQueueSize()) {
                      addTaskSizeToQueue(1);
    //                std::cout << "AQUI 0" << endl;
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

                            TraCIDemo11pMessage* newWsm = createMessage(wsm->getSerial(), wsm->getSenderAddress(), wsm->getDemoData(), wsm->getMessageId(), wsm->getFinalAddress(), wsm->getBeginTime());
                            newWsm->setKind(SEND_AID_MESSAGE);

                            delay = uniform(0, par("randomDelayTimeMax").doubleValue());
                            if (sendBeaconEvt->isScheduled()) cancelEvent(sendBeaconEvt);
    //                        std::cout << "AQUI 2: " << delay << endl;
                            scheduleAt(simTime() + delay, newWsm);
    //                    }
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
    //                }
                } else {
                    stackOverflowNumber++;
                }
            }
        }
    } else {
    //        std::cout << "AQUI"<< endl;
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
        removeTaskSizeToQueue(1);
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
//    std::cout << "ASHAPIUBSPIUABSIASBIASBAS " << endl;
    std::map<int, Coord> myRoutingMap = routingMap[myId];
    bool alreadySendMessage = false;
    int mySortedRoutingMapByDistanceToDestinyArray[myRoutingMap.size()];
    int mySortedRoutingMapByDistanceToDestinyArrayIndex = 0;

//    for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
//        if (it->first == rsuId) {
//            createAndSendMessage(rsuId, senderAddress, messageId, rsuId, beginTime);
//            alreadySendMessage = true;
//            break;
//        }
//    }
    if (!alreadySendMessage) {
        std::vector<int> vec;
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
                    vec.push_back(targetId);
                }
            }
        }

        if (isADenseRoad()) {
            std::vector<int> sortedVec;
            std::vector<int> arr = insertSorted(myRoutingMap, curPosition);
            for (const auto &item : arr) {
                for (const auto &elem : vec) {
                    if (item == elem) {
                        sortedVec.push_back(elem);
                    }
                }
            }

            int indexToStart = sortedVec.size() - std::ceil(sortedVec.size() * 0.3);
            for (auto it = sortedVec.begin() + indexToStart; it != sortedVec.end(); ++it) {
                createAndSendMessage(*it, senderAddress, messageId, rsuId, beginTime);
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
        if (currentTime > it->second + 6) {
            myRoutingMap.erase(it->first);
            myRoutingMapLastBeaconReceivedTime.erase(it->first);
        }
    }


    routingMap[myId] = myRoutingMap;
    routingMapLastBeaconReceivedTime[myId] = myRoutingMapLastBeaconReceivedTime;

    bool alreadySendMessage = false;

    DemoBaseApplLayer::handlePositionUpdate(obj);
//    if (myId == 40) {
//        std::cout << "AQUII 0 -> " << timeToSendMessage << endl;
//        std::cout << "AQUII 1 -> " << timeToSendMessage - 1 << endl;
//        std::cout << "AQUII 2 -> " << simTime() << endl;
//        std::cout << "AQUII 3 -> " << timeToSendMessage + 1 << endl;
//    }
    if (myId % 4 == 0 && simTime() > 65 && simTime() == timeToSendMessage) {

        std::vector<int> vec;
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
//        std::cout << "AQUII 1 -> " << alreadySendMessage << endl;
        if (!alreadySendMessage) {
            float currentDistance = curPosition.distance(rsuCoord);
            for (auto it = myRoutingMap.begin(); it != myRoutingMap.end(); ++it) {
                Coord targetNodeCoord = it->second;
                int targetId = it->first;
                float targetDistanceToRsu = targetNodeCoord.distance(rsuCoord);
                if (currentDistance > targetDistanceToRsu+10) {
                    if (!isADenseRoad()) {
//                        std::cout << "DIRETO "<< endl;
                        createAndSendMessage(targetId, myId, messageGeneratedCount, rsuId, NULL);
                    } else {
                        vec.push_back(targetId);
                    }
                }
            }
            if (isADenseRoad()) {
                std::vector<int> sortedVec;
                std::vector<int> arr = insertSorted(myRoutingMap, curPosition);
//                std::cout << "AQUII 1 -> " << vec.size() << endl;
                for (const auto &item : arr) {
                    for (const auto &elem : vec) {
                        if (item == elem) {
                            sortedVec.push_back(elem);
                        }
                    }
                }

                int indexToStart = sortedVec.size() - std::ceil(sortedVec.size() * 0.3);
                for (auto it = sortedVec.begin(); it != sortedVec.end(); ++it) {
//                    std::cout << "AQUI "<< *it << endl;
                    createAndSendMessage(*it, myId, messageGeneratedCount, rsuId, NULL);
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

std::vector<int> TraCIDemo11p::insertSorted(std::map<int, Coord> myMap, Coord currentPos) {

    std::map<int, float> distanceMap;

    for (auto it = myMap.begin(); it != myMap.end(); ++it) {
        distanceMap[it->first] = currentPos.distance(it->second);
    }

    std::vector<std::pair<int, float>> vec;
    std::vector<int> arr;

    for (const auto &elem : distanceMap) {
        vec.push_back(elem);
      }

    std::sort(vec.begin(), vec.end(), [](const auto &a, const auto &b) {
       return a.second < b.second;
     });

    for (const auto &elem : vec) {
//        std::cout << elem.second << " ";
        arr.push_back(elem.first);
    }

    return arr;
}

//int TraCIDemo11p::insertSorted(int arr[], int n, std::map<int, Coord> key, int capacity, int nodeID, Coord curPosition)
//{
//    // Cannot insert more elements if n is already
//    // more than or equal to capacity
//    if (n >= capacity)
//        return n;
//
//    for (auto it = key.begin(); it != key.end(); ++it) {
//        std::cout << "AQUII 1 -> " << it->first << " - " << curPosition.distance(it->second) << endl;
//    }
//    std::cout << "AKSJHDSA 1-> " << curPosition.distance(key[nodeID]) << endl;
//    std::cout << "AKSJHDSA 2-> " << nodeID << endl;
//
//    float newDistance = key[nodeID].distance(curPosition);
//
//    int i;
//    for (i = n - 1; (i >= 0 && key[i].distance(curPosition) > newDistance); i--)
//        arr[i + 1] = arr[i];
//
//    for (int j = 0; i < sizeof(arr); j++) {
//        std::cout << "HIHIHI -> " << arr[j] << endl;
//    }
//
//    arr[i + 1] = nodeID;
//
//    std::cout << " ======== " << sizeof(arr) << endl;
//
//    for (int j = 0; i < sizeof(arr); j++) {
//        std::cout << "YRYRYRYR -> " << arr[j] << endl;
//    }
//
//    return (n + 1);
//}

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
