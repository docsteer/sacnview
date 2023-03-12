// Copyright 2016 Tom Steer
// http://www.tomsteer.net
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Parts of this file from Electronic Theatre Controls Inc, License info below
//
// Copyright (c) 2015 Electronic Theatre Controls, http://www.etcconnect.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

/*streamcommon.h  The common functions both a client or server may need,
 * mostly dealing with packing or parsing the streaming ACN header*/

#ifndef _STREAMCOMMON_H_
#define _STREAMCOMMON_H_

#include "CID.h"
#include "ipaddr.h"

/* 
 * a description of the address space being used
 */
// Root
#define PREAMBLE_SIZE_ADDR 0
#define POSTAMBLE_SIZE_ADDR 2
#define ACN_IDENTIFIER_ADDR 4
#define ROOT_FLAGS_AND_LENGTH_ADDR 16
#define ROOT_VECTOR_ADDR 18
#define CID_ADDR 22
// Framing
#define FRAMING_FLAGS_AND_LENGTH_ADDR 38
#define FRAMING_VECTOR_ADDR 40
#define SOURCE_NAME_ADDR 44
#define PRIORITY_ADDR 108
#define SYNC_ADDR 109
#define SEQ_NUM_ADDR 111
#define OPTIONS_ADDR 112
#define UNIVERSE_ADDR 113
// DMP
#define DMP_FLAGS_AND_LENGTH_ADDR 115
#define DMP_VECTOR_ADDR 117
#define DMP_ADDRESS_AND_DATA_ADDR 118
#define FIRST_PROPERTY_ADDRESS_ADDR 119
#define ADDRESS_INC_ADDR 121
#define PROP_COUNT_ADDR 123
#define START_CODE_ADDR 125
#define PROP_VALUES_ADDR (START_CODE_ADDR + 1)
// Universe Synchronization
#define SYNC_FLAGS_AND_LENGTH_ADDR FRAMING_FLAGS_AND_LENGTH_ADDR
#define SYNC_VECTOR_ADDR FRAMING_VECTOR_ADDR
#define SYNC_SEQ_NUM_ADDR 44
#define SYNC_SYNCHRONIZATION_ADDRESS 45
#define SYNC_RESERVED 47
// Universe Discovery
#define DISCO_FLAGS_AND_LENGTH_ADDR 112
#define DISCO_VECTOR_ADDR 114
#define DISCO_PAGE_ADDR 118
#define DISCO_LAST_PAGE_ADDR 119
#define DISCO_LIST_UNIVERSE_ADDR 120

//for support of the early draft:
#define DRAFT_SOURCE_NAME_ADDR SOURCE_NAME_ADDR
#define DRAFT_PRIORITY_ADDR 76
#define DRAFT_SEQ_NUM_ADDR 77
#define DRAFT_UNIVERSE_ADDR 78
#define DRAFT_DMP_FLAGS_AND_LENGTH_ADDR 80
#define DRAFT_DMP_VECTOR_ADDR 82
#define DRAFT_DMP_ADDRESS_AND_DATA_ADDR 83
#define DRAFT_FIRST_PROPERTY_ADDRESS_ADDR 84
#define DRAFT_ADDRESS_INC_ADDR 86
#define DRAFT_PROP_COUNT_ADDR 88
#define DRAFT_PROP_VALUES_ADDR 90

/*
 * common sizes
 */
//You'd think this would be 125, wouldn't you?  
//But it's not.  It's not because the start code is squeaked in
//right before the actual DMX512-A data.
#define STREAM_HEADER_SIZE 126
#define SOURCE_NAME_SIZE 64
#define RLP_PREAMBLE_SIZE 16
#define RLP_POSTAMBLE_SIZE 0
#define ACN_IDENTIFIER_SIZE 12
#define DMX_SLOT_MAX 512

//for support of the early draft
#define DRAFT_STREAM_HEADER_SIZE 90
#define DRAFT_SOURCE_NAME_SIZE 32

/*
 * data definitions
 */
// Root
#define ACN_IDENTIFIER "ASC-E1.17\0\0\0"	
#define VECTOR_ROOT_E131_DATA 0x00000004
#define VECTOR_ROOT_E131_EXTENDED 0x00000008
// Draft v0.2
#define VECTOR_ROOT_E131_DATA_DRAFT 0x00000003

// Framing
#define VECTOR_E131_DATA_PACKET 0x00000002
#define VECTOR_E131_EXTENDED_SYNCHRONIZATION 0x00000001
#define VECTOR_E131_EXTENDED_DISCOVERY 0x00000002

// DMP
#define VECTOR_DMP_SET_PROPERTY 0x02
#define DMP_ADDRESS_AND_DATA_FORMAT 0xa1
#define DMP_FIRST_PROPERTY_ADDRESS_FORCE 0
#define ADDRESS_INC 1

// Universe Discovery Layer
#define VECTOR_UNIVERSE_DISCOVERY_UNIVERSE_LIST 0x00000001

#define NOT_SYNCHRONIZED_VALUE 0
#define NO_OPTIONS_VALUE 0

/*
 *  Options
 */
#define PREVIEW_DATA_OPTION 0x80 // Bit 7
#define STREAM_TERMINATED_OPTION 0x40 // Bit 6
#define FORCE_SYCHRONIZATION_OPTION 0x20 // Bit 5

/***/

//The well-known streaming ACN port (currently the ACN port)
#define STREAM_IP_PORT 5568

/*The start codes we'll commonly use*/
//The payload is up to 512 1-byte dmx values
#define STARTCODE_DMX 0
//The payload is the per-channel priority (0-200),
//where 0 means "ignore my values on this channel"
#define STARTCODE_PRIORITY 0xDD     

// Special universes
#define E131_DISCOVERY_UNIVERSE 64214

// Timings
#define E131_UNIVERSE_DISCOVERY_INTERVAL 10000 //10 Seconds
#define E131_NETWORK_DATA_LOSS_TIMEOUT 2500 // 2.5 Seconds
#define E131_DATA_KEEP_ALIVE_INTERVAL_MIN 800 // 800 mS
#define E131_DATA_KEEP_ALIVE_INTERVAL_MAX 1000 // 1000 mS
// We'll go with min +25% of the difference between max and min
static constexpr float E131_DATA_KEEP_ALIVE_FREQUENCY = 1000 /
        (static_cast<float>(E131_DATA_KEEP_ALIVE_INTERVAL_MIN) +
         (static_cast<float>(E131_DATA_KEEP_ALIVE_INTERVAL_MAX) - static_cast<float>(E131_DATA_KEEP_ALIVE_INTERVAL_MIN)) / 4);


/*** Functions ***/

/*
 * Given a buffer, initialize the header, based on the data slot count, 
 * cid, etc. The buffer must be at least STREAM_HEADER_SIZE bytes long
 */
void InitStreamHeader(quint8* pbuf, const CID &source_cid, 
              const char* source_name, quint8 priority, quint16 synchronization,
		      quint8 options, quint8 start_code, quint16 universe, 
		      quint16 slot_count);
/*
 * Given a buffer, initialize the header, based on the data slot count, 
 * cid, etc. The buffer must be at least STREAM_HEADER_SIZE bytes long
 * This function is included to support legacy code from before 
 * ratification of the standard.
 */
void InitStreamHeaderForDraft(quint8* pbuf, const CID &source_cid,
                              const char* source_name, quint8 priority, quint16 reserved,
                              quint8 options, quint8 start_code, quint16 universe,
                              quint16 slot_count);

/* Given an initialized buffer, change the sequence number to... */
void SetStreamHeaderSequence(quint8* pbuf, quint8 seq, bool draft);

/*
 * Given a buffer, validate that the stream header is correct.  If this returns
 * true, the header is validated, and the necessary values are filled in.  
 * source_space must be of size SOURCE_NAME_SPACE.
 * pdata is the offset into the buffer where the data is stored
 */
enum e_ValidateStreamHeader
{
    StreamHeader_Invalid,
    StreamHeader_Draft,
    StreamHeader_Ratified,
    StreamHeader_Extended,
    StreamHeader_Pathway_Secure,
    StreamHeader_Unknown
};
e_ValidateStreamHeader ValidateStreamHeader(
        quint8* pbuf, uint buflen,
        quint32 &root_vector,
        CID &source_cid, char* source_sp, quint8 &priority,
        quint8 &start_code, quint16 &synchronization, quint8 &sequence,
        quint8 &options, quint16 &universe,
        quint16 &slot_count, quint8* &pdata);

/*
 * helper function that does the actual validation of a header
 * that carries the post-ratification root vector
 */
bool VerifyStreamHeader(
        quint8 *pbuf, uint buflen,
        CID &source_cid, char* source_name, quint8 &priority,
        quint8 &start_code, quint16 &synchronization, quint8 &sequence,
        quint8 &options, quint16 &universe,
        quint16 &slot_count, quint8* &pdata);
/*
 * helper function that does the actual validation of a header
 * that carries the early draft's root vector
 * This function is included to support legacy code from before 
 * ratification of the standard.
 */
bool VerifyStreamHeaderForDraft(
        quint8* pbuf, uint buflen,
        CID &source_cid, char* source_space, quint8 &priority,
        quint8 &start_code, quint8 &sequence,
        quint16 &universe, quint16 &slot_count,
        quint8* &pdata);

/*
 * Returns true if contains draft root vector value
 */
bool isDraft(quint8* pbuf);

/*
 * Helper function
 * Check framing vector
 * Returns true if vector is expected type
 */
bool checkFramingVector(quint8* pbuf, quint8 expectedVector);

/*
 * Helper function
 * Set a bit in the option field
 */
void setOptionBit(quint8 mask, quint8* pbuf, bool value);

/*
 * Helper function
 * Get a bit in the option field
 */
bool getOptionBit(quint8 mask, quint8* pbuf);

/* 
 * toggles the preview_data bit of the options field to either 1 or 0
 */
void SetPreviewData(quint8* pbuf, bool preview);

/*
 * Returns the preview_data bit of the options field
 */
bool GetPreviewData(quint8* pbuf);

/* 
 * toggles the stream_terminated bit of the options field to either 1 or 0
 */
void SetStreamTerminated(quint8* pbuf, bool terminated);

/* 
 * returns the stream_terminated bit of the options field
 */
bool GetStreamTerminated(quint8* pbuf);

/*
 * toggles the Force_Synchronization bit of the options field to either 1 or 0
 */
void SetForceSync(quint8* pbuf, bool terminated);

/*
 * returns the Force_Synchronization bit of the options field
 */
bool GetForceSync(quint8* pbuf);

/*Fills in the multicast address and port (not netiface) to use for listening to or sending on a universe*/
void GetUniverseAddress(quint16 universe, CIPAddr& addr);


#endif //_STREAMCOMMON_H_

