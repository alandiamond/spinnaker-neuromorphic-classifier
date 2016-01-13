//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University																																								

#include "EIEIOMessage.h"
#include <stdio.h>

using namespace std;


namespace spinnio
{

EIEIOMessage::~EIEIOMessage() {
}

// Constructor - create basic packet with nothing set

EIEIOMessage::EIEIOMessage() 
{
    this->message = new eieio_message();
    this->messageBuffer = 0;
    this->addressPrefix = 0x0;
    this->dataPrefix = 0;
    this->spikes = 0;
    this->error = ERR_OK;
}

// Constructor - create basic packet with defaults in header unless
// supplied and empty data
EIEIOMessage::EIEIOMessage(uint8_t count = 0, messageFormat format = BASIC, messageType type = TYPE_32_K, char tag = 0, bool keyPrefix = false, bool payloadPrefix = false, bool timestamp = false) 
{
    this->message = new eieio_message();
    this->message->header.count = count;
    this->message->header.p = keyPrefix;
    this->message->header.f = format;
    this->message->header.d = payloadPrefix;
    this->message->header.t = timestamp;
    this->message->header.type = type;
    this->message->header.tag = tag;

    this->messageBuffer = 0;
    this->addressPrefix = 0x0;
    this->dataPrefix = 0;
    this->spikes = 0;
    this->error = ERR_OK;
}

// Write the message data in 
void EIEIOMessage::writeMessageData(unsigned char bufferData, int numBytes){

   this->message->data = (unsigned char *) malloc(numBytes);
   memcpy(this->message->data, &bufferData, numBytes);
   
}

// Write the whole message into a buffer for sending from a spike
// list
int EIEIOMessage::writeMessage(spike* spikeList, unsigned char numSpikes){

    shortSpike tempSpike16;

    // set up the raw header with appropriate bits set
    this->rawHeader = (numSpikes << RAW_HDR_COUNT_POS) & RAW_HDR_COUNT_MASK;
    this->rawHeader|= (this->message->header.tag << RAW_HDR_TAG_POS) & RAW_HDR_TAG_MASK;
    this->rawHeader |= (this->message->header.type << RAW_HDR_TYPE_POS) & RAW_HDR_TYPE_MASK;
    if (this->message->header.t) this->rawHeader |= RAW_HDR_T_MASK;
    if (this->message->header.d) this->rawHeader |= RAW_HDR_D_MASK;


    if (this->message->header.f > COMMAND)
    {
        this->rawHeader |= RAW_HDR_P_MASK;
        if (this->message->header.f == PREFIX_U) this->rawHeader |= RAW_HDR_F_MASK;
    }

    // try to allocate the message buffer
    try
    {
      this->messageBuffer = new uint8_t[messageBufferLength()];
    }
    catch (std::bad_alloc)
    {
        this->error |= ERR_BUF_ALLOC;
        this->messageBuffer = 0;
        return 0;
    }

    // Copy the raw header into the message buffer 
    uint8_t* messageBufferPointer = this->messageBuffer; 
    memcpy(messageBufferPointer, &(this->rawHeader), 2);
    messageBufferPointer += 2;

    if (this->message->header.f >= PREFIX_L)
    {
        memcpy(messageBufferPointer, &(this->addressPrefix), 2);
        messageBufferPointer += 2;
    }
    if (this->message->header.d)
    {
        if (this->message->header.type >= TYPE_32_K)
        {
            memcpy(messageBufferPointer, &(this->dataPrefix), 4);
            messageBufferPointer += 4;
        }
        else
        {
            uint16_t dataPrefix16 = static_cast<uint16_t>(this->dataPrefix);
            memcpy(messageBufferPointer, &dataPrefix16, 2);
            messageBufferPointer +=2;
        }
    }

    // write spike data into buffer

    this->spikes = spikeList;
    switch (this->message->header.type)
    {
        case TYPE_16_K:
	     for (unsigned char s = 0; s < numSpikes; s++) 
	     {
                  tempSpike16.key = this->spikes[s].key;
	          memcpy(messageBufferPointer, &(tempSpike16.key), 2);
                  messageBufferPointer +=2;
             }
	     break;
        case TYPE_16_KP:
	     for (unsigned char s = 0; s < numSpikes; s++) 
	     {
                  tempSpike16.key = this->spikes[s].key;
                  tempSpike16.payload = this->spikes[s].payload;
	          memcpy(messageBufferPointer, &(tempSpike16.key), 2);
                  messageBufferPointer += 2;
                  memcpy(messageBufferPointer, &(tempSpike16.payload), 2);
                  messageBufferPointer += 2;
             }
             break;
        case TYPE_32_K:
	     for (unsigned char s = 0; s < numSpikes; s++) 
	     {
                  memcpy(messageBufferPointer, &(this->spikes[s].key), 4);
                  messageBufferPointer += 4;
             }
             break;
        case TYPE_32_KP:
	     for (unsigned char s = 0; s < numSpikes; s++) 
	     {
                  memcpy(messageBufferPointer, &(this->spikes[s].key), 4);
                  messageBufferPointer += 4;
                  memcpy(messageBufferPointer, &(this->spikes[s].payload), 4);
                  messageBufferPointer += 4;
             }
             break;
        default:
             error |= ERR_UNKNOWN_TYPE;
             this->spikes = 0;
             this->messageBuffer = 0;
             return 0;
    }
    this->spikes = 0;
    // finally return the message buffer length
    return messageBufferLength(); 

}

void EIEIOMessage::setCount(uint8_t count){
   this->message->header.count = count;
}

void EIEIOMessage::setKeyPrefix(bool p){
   this->message->header.p = p;
}

void EIEIOMessage::setPayloadPrefix(bool d){
   this->message->header.d = d;
}

void EIEIOMessage::setTimestamp(bool t){
   this->message->header.t = t;
}

void EIEIOMessage::setFormat(messageFormat f){
   this->message->header.f = f;
}

void EIEIOMessage::setType(messageType type){
   this->message->header.type = type;
}

void EIEIOMessage::setTag(char tag){
   this->message->header.tag = tag;
}

uint8_t* EIEIOMessage::getMessageBuffer(){

   return this->messageBuffer;


}

int EIEIOMessage::messageBufferLength() const
{
    int hdr_len = 2;
    if (this->message->header.f >= 2) hdr_len += 2;
    if (this->message->header.d) hdr_len += 2*(this->message->header.type & 0x2);
    switch (this->message->header.type)
    {
       case TYPE_16_K: return hdr_len + 2 * this->message->header.count;
       case TYPE_16_KP: 
       case TYPE_32_K: return hdr_len + 4 * this->message->header.count;
       case TYPE_32_KP: return hdr_len + 8 * this->message->header.count;
       default: return 0; 
    }
}


}// namespace spinnio
