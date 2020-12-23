/*
 * Node.cc
 *
 *  Created on: 2020Äê12ÔÂ23ÈÕ
 *      Author: Lenovo
 */

#include "Node.h"
#define DEBUG
#ifdef DEBUG
#include<iostream>
#include<fstream>
using namespace std;

#endif
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<stdlib.h>

vector<Node*> Node::datanodev;
double startTime;

Define_Module(Node);

Node::Node()
{
    networkInitializationEvent = DataGenerationEvent = DataQueueEmptyEvent = DataPacketAccessEvent
    = DIFS_CheckEvent = CW_CheckEvent = DataPacketTransmissionEvent
    = TransmissionStatusOperationEvent = CWStatusMinusOneEvent = randomChooseMinislotEvent = NULL;
}

Node::~Node(){
    cancelAndDelete(networkInitializationEvent);
    cancelAndDelete(DataGenerationEvent);
    cancelAndDelete(DataQueueEmptyEvent );
    cancelAndDelete(DataPacketAccessEvent);
    cancelAndDelete(DIFS_CheckEvent);
    cancelAndDelete(CW_CheckEvent);
    cancelAndDelete(DataPacketTransmissionEvent);
    cancelAndDelete(TransmissionStatusOperationEvent);
    cancelAndDelete(CWStatusMinusOneEvent);
    cancelAndDelete(randomChooseMinislotEvent);


}

void Node::initialize()
{
    cout<<"Initializing Start!" << endl;
    cModule* parent = this->getParentModule();
    this->dataNodeNum = parent->par("dataNodeNum");
    this->rounds = parent->par("rounds");
    this->networkWidth = parent->par("width");
    this->networkHeight = parent->par("height");
    this->queueSize = parent->par("queueSize");
    this->DIFS = parent->par("DIFS");
    this->DIFS_checkInterval = parent->par("DIFS_checkInterval");
    this->CW_checkInterval = parent->par("CW_checkInterval");
    this->CWmin = parent->par("CWmin");
    this->datapackettransmissiontime = parent->par("data_packet_transmission_time");
    this->retryLimit = parent->par("retryLimit");
    this->backoffStageLimit = parent->par("backoffStageLimit");
    this->lamda = parent->par("lamda");
    this->datapacketSize = parent->par("packetSize");
    this->networkSpeed = parent->par("networkSpeed");
    this->x = par("x");
    this->y = par("y");
    this->setGateSize("in",this->dataNodeNum+3);
    this->setGateSize("out",this->dataNodeNum+3);
    this->getDisplayString().setTagArg("i", 1, "black");

    datanodev.push_back(this);
    networkInitializationEvent = new cMessage("Network_Initialization");
    DataGenerationEvent = new cMessage("Data_Generation");
    DataQueueEmptyEvent = new cMessage("Data_Queue_Empty");
    DataPacketAccessEvent = new cMessage("Data_Packet_Access");
    DIFS_CheckEvent = new cMessage("DIFS_Check");
    CW_CheckEvent = new cMessage("CW_Check");
    DataPacketTransmissionEvent = new cMessage("Data_Packet_Transmission");
    TransmissionStatusOperationEvent = new cMessage("Transmission_Status_Operation");
    CWStatusMinusOneEvent = new cMessage("CW_Status_Minus_One");
    randomChooseMinislotEvent = new cMessage("random_Choose_Minislot");

    networkInitializationEvent->setKind(EV_NETWORKINITIALIZATION);
    this->scheduleAt(simTime() + SLOT_TIME, networkInitializationEvent);
    cout << "Node Initialization Finished!" << endl;

}

void Node::handleMessage(cMessage* msg){
    if(msg->isSelfMessage()){
        switch(msg->getKind()){
            case EV_NETWORKINITIALIZATION:
                networkInitialization();
                break;
            case EV_DATAGENERATION:
                dataPacketGeneration();
                break;
            case EV_DATAPACKETACCESS:
                dataPacketAccess();
                break;
            case EV_DATAQUEUEEMPTY:
                IsDataQueueEmpty();
                break;
            case EV_DIFSCHECK:
                DIFScheck();
                break;
            case EV_CWCHECK:
                CWcheck();
                break;
            case EV_DATAPACKETTRANSMISSION:
                DataPacketTransmission();
                break;
            case EV_TRANSMISSIONSTATUSOPERATION:
                TransmissionStatusZero();
                break;
            case EV_CWSTATUSMINUSONE:
                CWStatusMinusOne();
                break;
        }
    }
    else{
        delete msg;
    }

}


void Node::finish(){
    int totalSentDataPackets = 0;
    double DataThroughput = 0.0;
    double endTime = SIMTIME_DBL(simTime());
    if(this->getId() == 5){
        for(int i = 0; i < datanodev.size(); i ++){
            totalSentDataPackets += datanodev[i]->sentdatapackets;
        }

        double runningTime = (double)(endTime - startTime);
        DataThroughput = (double)(totalSentDataPackets*this->datapacketSize)/(double)(runningTime*this->networkSpeed);
        cout << "runningTime " << runningTime << " , data throughput = " << DataThroughput << endl;
        cout << "The total number of data node is = " << this->dataNodeNum
                << " , the normalized data throughput is = " << DataThroughput << endl;
    }
}


void Node::networkInitialization(){
    cout << "Network Initialization Start~" << endl;
    this->cur_round = 0;
    this->dataqueue = 0;
    this->sentPackets = 0;
    this->lossPackets = 0;
    this->generatedPackets = 0;
    this->retryCounter = 0;
    this->CW_counter = 0;
    this->backoffStage = 0;
    this->DIFS_counter = 0;
    this->first_time_data_transmission = 0;
    this->generateddatapackets = 0;
    this->transmission_status = 0;
    this->losseddatapackets = 0;
    this->sentdatapackets = 0;
    Busy = 0;
    srand((unsigned)time(0));

    startTime = SIMTIME_DBL(simTime());

    dataPacketGeneration();

    this->destinationNode = getRandomDestination(this->getId());

    DataPacketAccessEvent->setKind(EV_DATAPACKETACCESS);
    this->scheduleAt(simTime() + SLOT_TIME, DataPacketAccessEvent);
    cout << "Network Initilization Finished~" << endl;


}

void Node::dataPacketGeneration(){
    if(this->dataQueue.empty() || (this->dataQueue.size() < this->queueSize)){
        VoicePacket* data = new VoicePacket();
        data->setSource_id(this->getId());
        data->setKind(MSG_DATA);
        data->setGeneration_time(simTime().dbl());
        this->dataQueue.push(data);
        this->generateddatapackets++;
    }
    double time = exponential(1/this->lamda);
    DataGenerationEvent->setKind(EV_DATAGENERATION);
    this->scheduleAt(simTime() + time, DataGenerationEvent);

}

void Node::dataPacketAccess(){
    label = 0;
    this->cur_round++;
    if(this->cur_round >= this->rounds){
        label = 1;
        endSimulation();
    }
    Busy = 0;
    IsDataQueueEmpty();
}


void Node::IsDataQueueEmpty(){
    if(!this->dataQueue.empty()){
        DIFS_CheckEvent->setKind(EV_DIFSCHECK);
        this->scheduleAt(simTime() + DIFS_checkInterval, DIFS_CheckEvent);
    }
    else{
        DataQueueEmptyEvent->setKind(EV_DATAQUEUEEMPTY);
        this->scheduleAt(simTime() + DIFS_checkInterval, DataQueueEmptyEvent);

    }
}

void Node::DIFScheck(){
    if((Busy == 0)  && (DIFS_counter < 5)){
        DIFS_counter++;
        if(DIFS_counter == 5){
            DIFS_counter = 0;
            if(first_time_data_transmission == 0){
                transmission_status = 1;
                DataPacketTransmissionEvent->setKind(EV_DATAPACKETTRANSMISSION);
                this->scheduleAt(simTime() + SLOT_TIME, DataPacketTransmissionEvent);

            }
            else if(first_time_data_transmission == 1){
                if(CW_counter == -1){
                    CW_counter = (int)uniform(1,CWmin*getBackoffTime(backoffStage));
                }
                CW_CheckEvent->setKind(EV_CWCHECK);
                this->scheduleAt(simTime() + CW_checkInterval, CW_CheckEvent);
            }
        }
        else{
            DIFS_CheckEvent->setKind(EV_DIFSCHECK);
            this->scheduleAt(simTime() + DIFS_checkInterval, DIFS_CheckEvent);
        }
    }
    else{
        DIFS_counter = 0;
        first_time_data_transmission = 1;

        DIFS_CheckEvent->setKind(EV_DIFSCHECK);
        this->scheduleAt(simTime() - DIFS_checkInterval + datapackettransmissiontime + DIFS_checkInterval, DIFS_CheckEvent);
    }
}

int Node::getBackoffTime(int backoffstage){
    int backofftime = 1;
    backofftime = (int)pow(2,(double)backoffstage);
    return backofftime;
}


void Node::CWcheck(){
    if(Busy == 0 && CW_counter != 0){
        CW_counter--;
        if(CW_counter != 0){
            CW_CheckEvent->setKind(EV_CWCHECK);
            this->scheduleAt(simTime() + CW_checkInterval, CW_CheckEvent);

        }
        else{
            DataPacketTransmissionEvent->setKind(EV_DATAPACKETTRANSMISSION);
            this->scheduleAt(simTime() + SLOT_TIME, DataPacketTransmissionEvent);
        }

    }
    else if(Busy == 1 && CW_counter != 0){
        DIFS_CheckEvent->setKind(EV_DIFSCHECK);
        this->scheduleAt(simTime() - CW_checkInterval + datapackettransmissiontime + DIFS_checkInterval, DIFS_CheckEvent);
    }
}


void Node::DataPacketTransmission(){
    Busy = 1;
    int collision_indication = 0;
    if(first_time_data_transmission == 0){
        first_time_data_transmission = 1;
        for(int i = 0; i < datanodev.size(); i ++ ){
            if(datanodev[i]->transmission_status == 1 && datanodev[i]->getId() != this->getId()){
                this->retryCounter++;
                collision_indication = 1;
                break;
            }
        }
        TransmissionStatusOperationEvent->setKind(EV_TRANSMISSIONSTATUSOPERATION);
        this->scheduleAt(simTime() + SLOT_TIME, TransmissionStatusOperationEvent);
    }
    else{
        for(int i = 0; i < datanodev.size(); i ++ ){
            if(datanodev[i]->CW_counter == 0 && datanodev[i]->getId() != this->getId()){
                this->retryCounter++;
                this->backoffStage++;
                collision_indication = 1;
                break;
            }
        }
        CWStatusMinusOneEvent->setKind(EV_CWSTATUSMINUSONE);
        this->scheduleAt(simTime() + SLOT_TIME, CWStatusMinusOneEvent);
    }
    if(collision_indication == 1){
        if(backoffStage > backoffStageLimit){
            backoffStage = backoffStageLimit;
        }
        if(retryCounter > retryLimit){
            this->retryCounter = 0;
            this->backoffStage = 0;
            this->dataQueue.pop();
            this->losseddatapackets++;
        }
    }
    else{
        this->dataQueue.pop();
        this->sentdatapackets++;
        this->backoffStage = 0;
        this->retryCounter = 0;
    }
    DataPacketAccessEvent->setKind(EV_DATAPACKETACCESS);
    this->scheduleAt(simTime() - SLOT_TIME + datapackettransmissiontime, DataPacketAccessEvent);

}

Node* Node::getRandomDestination(int nodeId){
    int id;
    while(true){
        id = uniform(0,datanodev.size());
        if(datanodev[id]->getId() != nodeId){
            return datanodev[id];
        }
    }
}

void Node::TransmissionStatusZero(){
    this->transmission_status = 0;
}

void Node::CWStatusMinusOne(){
    this->CW_counter = -1;
}







