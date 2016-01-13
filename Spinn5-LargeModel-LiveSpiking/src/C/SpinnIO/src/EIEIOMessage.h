//####################################################################//
//SpinnIO - An Open-Source library for spike io to/from a spinnaker board																													
//
//																																																				//
//Copyright (c) <2015>, BABEL project, Plymouth University in collaboration with Manchester University

#include <stdint.h>
#include <cstdlib>
#include <string.h>
#include <new>

#define ERR_OK 0
#define ERR_SPIKE_ALLOC 1
#define ERR_BUF_ALLOC 2
#define ERR_COMMAND_SET 4
#define ERR_BAD_COMMAND 8
#define ERR_UNKNOWN_TYPE 16
#define ERR_WRITE_ON_CMD 32

#define MAX_COMMAND 0x3FFF
#define MAX_SPIKE_COUNT 63

#define RAW_HDR_COUNT_MASK 0xFF
#define RAW_HDR_TAG_MASK 0x300
#define RAW_HDR_TYPE_MASK 0xC00
#define RAW_HDR_T_MASK 0x1000
#define RAW_HDR_D_MASK 0x2000
#define RAW_HDR_F_MASK 0x4000
#define RAW_HDR_P_MASK 0x8000

#define RAW_HDR_COUNT_POS 0
#define RAW_HDR_TAG_POS 8
#define RAW_HDR_TYPE_POS 10
#define RAW_HDR_T_POS 12
#define RAW_HDR_D_POS 13
#define RAW_HDR_F_POS 14
#define RAW_HDR_P_POS 15

#define OUT_MASK 0xFFFFFFFF
#define VIRTUAL_KEY 0x70800

#ifndef SPINNIO_EIEIOMESSAGE_H_
#define SPINNIO_EIEIOMESSAGE_H_

namespace spinnio
{


// Message Types:
// TYPE_16_K  - 16 bit keys only
// TYPE_16_KP - alternating 16 bit keys and payloads
// TYPE_32_K  - 32 bit keys only
// TYPE_32_KP - alternating 32 bit keys and payloads

typedef enum messageType {TYPE_16_K=0, TYPE_16_KP=1, 
                          TYPE_32_K=2, TYPE_32_KP=3} messageType;

// Message Formats:
// BASIC   - data message
// COMMAND - command message
// PREFIX_L - OR prefix with lower halfword
// PREFIX_U - OR prefix with upper halfword

typedef enum messageFormat {BASIC=0, COMMAND=1, 
                            PREFIX_L=2, PREFIX_U=3} messageFormat;

struct eieio_header{
    bool p;            // Key prefix
    messageFormat f;  // Message format (see above)
    bool d;            // Payload prefix
    bool t;            // Payload Timestamp
    messageType type; // Message type (see above)
    char tag;
    uint8_t count;
};

struct eieio_message{
    eieio_header header;
    unsigned char* data;
};


typedef struct spike
{
  unsigned int key;
  unsigned int payload;
} spike;

typedef struct shortSpike
{
  unsigned short key;
  unsigned short payload;
} shortSpike;

class EIEIOMessage {

public:
    EIEIOMessage(uint8_t, messageFormat, messageType, char, bool, bool, bool);
    EIEIOMessage();
    virtual ~EIEIOMessage();
    void writeMessageData(unsigned char, int);
    int writeMessage(spike*, unsigned char);
    uint8_t* getMessageBuffer();
    void setCount(uint8_t);
    void setKeyPrefix(bool);
    void setPayloadPrefix(bool);
    void setTimestamp(bool);
    void setFormat(messageFormat);
    void setType(messageType);
    void setTag(char);

protected:
private:
    uint16_t rawHeader;
    uint8_t* messageBuffer;
    uint16_t addressPrefix;
    uint32_t dataPrefix;
    //eieio_header messageHeader;
    //unsigned char* messageData;
    struct eieio_message* message;
    int messageBufferLength() const;
    int error;
    spike* spikes;

};

}
#endif /* SPINNIO_EIEIOMESSAGE_H_ */
