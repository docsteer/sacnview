// Copyright 2016 Tom Barthel-Steer
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

/*streamcommon.cpp.  
Implementation of the common streaming ACN packing and parsing functions*/
#include <string.h>

#include "streamcommon.h"
#include "defpack.h"
#include "VHD.h"



/*
 * Given a buffer, initialize the header, based on the data slot count, 
 * cid, etc. The buffer must be at least STREAM_HEADER_SIZE bytes long
 */
void InitStreamHeader(quint8* pbuf, const CID &source_cid, 
		      const char* source_name, quint8 priority, quint16 reserved,
		      quint8 options, quint8 start_code, quint16 universe, 
		      quint16 slot_count) 
{
  if(!pbuf)
     return;

  quint8* p = pbuf;

  //root preamble size
  PackBUint16(p, RLP_PREAMBLE_SIZE);                   
  p += 2;

  //root post-amble size
  PackBUint16(p, RLP_POSTAMBLE_SIZE);                    
  p += 2;

  //root ACN packet identifier
  memcpy(p, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE);   
  p += ACN_IDENTIFIER_SIZE;
   
  //root flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, STREAM_HEADER_SIZE - RLP_PREAMBLE_SIZE 
		 + slot_count, false); 
  p += 2;

  //root vector
  PackBUint32(p, VECTOR_ROOT_E131_DATA);
  p += 4;

  //root CID
  source_cid.Pack(p);              
  p += CID::CIDBYTES;

  //framing flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, STREAM_HEADER_SIZE - FRAMING_FLAGS_AND_LENGTH_ADDR 
		 + slot_count, false); 
  p += 2;
   
  //framing vector
  PackBUint32(p, VECTOR_E131_DATA_PACKET);
  p += 4;
	
  //framing source name
  strncpy((char*)p, source_name, SOURCE_NAME_SIZE);
  p[SOURCE_NAME_SIZE - 1] = '\0'; 
  p += SOURCE_NAME_SIZE;  
  //We currently null terminate to be safe.

  //framing priority
  PackBUint8(p, priority);
  p += 1;

  //reserved
  PackBUint16(p, reserved);
  p += 2;
   
  //framing sequence number
  PackBUint8(p, 0); //We leave the sequence number set to NONE                   
  p += 1;  
  
  PackBUint8(p, options);
  p += 1;

  //framing universe
  PackBUint16(p, universe);
  p += 2;

  //dmp flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, STREAM_HEADER_SIZE - DMP_FLAGS_AND_LENGTH_ADDR 
		 + slot_count, false); 
  p += 2;
   
  //DMP vector
  PackBUint8(p, VECTOR_DMP_SET_PROPERTY);
  p += 1;

  //DMP address and data type
  PackBUint8(p, DMP_ADDRESS_AND_DATA_FORMAT);
  p += 1;

  //DMP first property address
  PackBUint16(p, 0); //THIS WILLBE START CODE IN OLD
  p += 2;
  
  //DMP address increment
  PackBUint16(p, ADDRESS_INC);
  p += 2; 

  //DMP property value count -- Includes the one byte for the start code
  PackBUint16(p, slot_count + 1);
  p += 2;
   
  //start code for ratified standard
  PackBUint8(p, start_code);
  p += 1;
}

/*
 * Given a buffer, initialize the header, based on the data slot count, 
 * cid, etc. The buffer must be at least STREAM_HEADER_SIZE bytes long
 * This function is included to support legacy code from before 
 * ratification of the standard.
 */
void InitStreamHeaderForDraft(quint8* pbuf, const CID &source_cid, 
			      const char* source_name, quint8 priority, 
                  quint16 /*reserved*/, quint8 /*options*/, quint8 start_code,
			      quint16 universe, quint16 slot_count) 
{
  if(!pbuf)
     return;

  quint8* p = pbuf;

  //root preamble size
  PackBUint16(p, RLP_PREAMBLE_SIZE);                   
  p += 2;

  //root post-amble size
  PackBUint16(p, RLP_POSTAMBLE_SIZE);                    
  p += 2;

  //root ACN packet identifier
  memcpy(p, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE);   
  p += ACN_IDENTIFIER_SIZE;
   
  //root flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, DRAFT_STREAM_HEADER_SIZE - RLP_PREAMBLE_SIZE 
		 + slot_count, false); 
  p += 2;

  //root vector
  PackBUint32(p, VECTOR_ROOT_E131_DATA_DRAFT);
  p += 4;

  //root CID
  source_cid.Pack(p);              
  p += CID::CIDBYTES;

  //framing flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, DRAFT_STREAM_HEADER_SIZE 
		 - FRAMING_FLAGS_AND_LENGTH_ADDR  + slot_count, false); 
  p += 2;
   
  //framing vector
  PackBUint32(p, VECTOR_E131_DATA_PACKET);
  p += 4;
	
  //framing source name
  strncpy((char*)p, source_name, DRAFT_SOURCE_NAME_SIZE);
  p[DRAFT_SOURCE_NAME_SIZE - 1] = '\0'; 
  p += DRAFT_SOURCE_NAME_SIZE;  
  //We currently null terminate to be safe.

  //framing priority
  PackBUint8(p, priority);
  p += 1;
   
  //framing sequence number
  PackBUint8(p, 0); //We leave the sequence number set to NONE                   
  p += 1;  

  //framing universe
  PackBUint16(p, universe);
  p += 2;

  //dmp flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, DRAFT_STREAM_HEADER_SIZE 
		 - DRAFT_DMP_FLAGS_AND_LENGTH_ADDR + slot_count, false); 
  p += 2;
   
  //DMP vector
  PackBUint8(p, VECTOR_DMP_SET_PROPERTY);
  p += 1;

  //DMP address and data type
  PackBUint8(p, DMP_ADDRESS_AND_DATA_FORMAT);
  p += 1;

  //DMP first property address
  PackBUint8(p, 0); //this is just padding
  p += 1;
  PackBUint8(p, start_code); 
  p += 1;
  
  //DMP address increment
  PackBUint16(p, ADDRESS_INC);
  p += 2; 

  //DMP property value count
  PackBUint16(p, slot_count);
  p += 2;
}

/* 
 * Given an initialized buffer, change the sequence number to... 
 * This function is included to support legacy code from before 
 * ratification of the standard.
 */
void SetStreamHeaderSequence(quint8* pbuf, quint8 seq, bool draft)
{
    if(draft && pbuf)
    {
          PackBUint8(pbuf + DRAFT_SEQ_NUM_ADDR, seq);
    }
    else if (pbuf)
    {
        PackBUint8(pbuf + SEQ_NUM_ADDR, seq);
    }
}

/*
 * Given a buffer, validate that the stream header is correct.  If this returns
 * true, the header is validated, and the necessary values are filled in.  
 * source_space must be of size SOURCE_NAME_SPACE.
 * pdata is the offset into the buffer where the data is stored
 */
bool ValidateStreamHeader(quint8* pbuf, uint buflen, CID &source_cid, 
			  char* source_space, quint8 &priority, 
			  quint8 &start_code, quint16 &reserved, 
			  quint8 &sequence, quint8 &options, quint16 &universe,
			  quint16 &slot_count, quint8* &pdata)
{
  if(!pbuf)
     return false;

  int root_vector = 0;
  
  root_vector = UpackBUint32(pbuf + ROOT_VECTOR_ADDR);
  
  if(root_vector == VECTOR_ROOT_E131_DATA)
  {
      return VerifyStreamHeader(pbuf, buflen, source_cid, source_space, 
				priority, start_code, reserved, sequence, 
				options, universe, slot_count, pdata);
  }
  else if(root_vector == VECTOR_ROOT_E131_DATA_DRAFT)
  {
      return VerifyStreamHeaderForDraft(pbuf, buflen, source_cid, 
					source_space, priority, start_code, 
					sequence, universe, slot_count, pdata);
  }
  else
  {
      return false;
  }
}

/*
 * helper function that does the actual validation of a header
 * that carries the post-ratification root vector
 */
bool VerifyStreamHeader(quint8* pbuf, uint buflen, CID &source_cid, 
			char* source_space, quint8 &priority, 
			quint8 &start_code, quint16 &reserved, quint8 &sequence, 
			quint8 &options, quint16 &universe,
			quint16 &slot_count, quint8* &pdata)
{
  if(!pbuf)
     return false;
  
  /* Do a little packet validation */
  if(buflen < STREAM_HEADER_SIZE)
  {
      return false;
  } 
  if(UpackBUint16(pbuf) != RLP_PREAMBLE_SIZE)
  {
      return false;
  }
  if(memcmp(pbuf 
	    + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE) != 0)
  {
      return false;
  }
  //don't have to check the root vector, since that test has already
  //been performed.
  if(UpackBUint32(pbuf + FRAMING_VECTOR_ADDR) != VECTOR_E131_DATA_PACKET)
  {
      return false;
  }
  if(UpackBUint16(pbuf + RESERVED_ADDR) != RESERVED_VALUE)
  { 
      return false;
  }
  if(UpackBUint8(pbuf + DMP_VECTOR_ADDR) != VECTOR_DMP_SET_PROPERTY)
  {
      return false;
  }
  if(UpackBUint8(pbuf + DMP_ADDRESS_AND_DATA_ADDR) 
     != DMP_ADDRESS_AND_DATA_FORMAT)
  {
      return false;
  }
  if(UpackBUint16(pbuf + FIRST_PROPERTY_ADDRESS_ADDR) 
     != DMP_FIRST_PROPERTY_ADDRESS_FORCE)
  {
      return false;
  }
  if(UpackBUint16(pbuf + ADDRESS_INC_ADDR) != ADDRESS_INC)
  {
      return false;
  }
   
  /* Init the parameters */
  source_cid.Unpack(pbuf + CID_ADDR);
  
  strncpy(source_space, (char*)(pbuf + SOURCE_NAME_ADDR), SOURCE_NAME_SIZE);
  source_space[SOURCE_NAME_SIZE-1] = '\0';
  priority = UpackBUint8(pbuf + PRIORITY_ADDR);
  start_code = UpackBUint8(pbuf + START_CODE_ADDR);
  reserved = UpackBUint16(pbuf + RESERVED_ADDR);
  sequence = UpackBUint8(pbuf + SEQ_NUM_ADDR);
  options = UpackBUint8(pbuf + OPTIONS_ADDR);
  universe = UpackBUint16(pbuf + UNIVERSE_ADDR);
  slot_count = UpackBUint16(pbuf + PROP_COUNT_ADDR) - 1;  //The property value count includes the start code byte
  pdata = pbuf + STREAM_HEADER_SIZE;
  
  /*Do final length validation*/
  if((pdata + slot_count) > (pbuf + buflen))
    return false;
  
  return true;
}

/*
 * helper function that does the actual validation of a header
 * that carries the early draft's root vector
 * This function is included to support legacy code from before 
 * ratification of the standard.
 */
bool VerifyStreamHeaderForDraft(quint8* pbuf, uint buflen, CID &source_cid, 
				char* source_space, quint8 &priority, 
				quint8 &start_code, quint8 &sequence, 
				quint16 &universe, quint16 &slot_count, 
				quint8* &pdata)
{
  if(!pbuf)
     return false;
  
  /* Do a little packet validation */
  if(buflen < DRAFT_STREAM_HEADER_SIZE)
  {
      return false;
  } 
  if(UpackBUint16(pbuf) != RLP_PREAMBLE_SIZE)
  {
      return false;
  }
  if(memcmp(pbuf 
	    + ACN_IDENTIFIER_ADDR, ACN_IDENTIFIER, ACN_IDENTIFIER_SIZE) != 0)
  {
      return false;
  }
  //don't have to check the root vector, since that test has already
  //been performed.
  if(UpackBUint32(pbuf + FRAMING_VECTOR_ADDR) != VECTOR_E131_DATA_PACKET)
  {
      return false;
  }
  if(UpackBUint8(pbuf + DRAFT_DMP_VECTOR_ADDR) != VECTOR_DMP_SET_PROPERTY)
  {
      return false;
  }
  if(UpackBUint8(pbuf + DRAFT_DMP_ADDRESS_AND_DATA_ADDR) 
     != DMP_ADDRESS_AND_DATA_FORMAT)
  {
      return false;
  }
  if(UpackBUint16(pbuf + DRAFT_ADDRESS_INC_ADDR) != ADDRESS_INC)
  {
      return false;
  }
  // this check is removed, as one proposal allows larger buffer sizes
  //      if(UpackBUint16(pbuf + 88) > 512)
  //      return false;
  //   
  
  /* Init the parameters */
  source_cid.Unpack(pbuf + CID_ADDR);
  
  strncpy(source_space, (char*)(pbuf + SOURCE_NAME_ADDR), 
	  DRAFT_SOURCE_NAME_SIZE);
  source_space[DRAFT_SOURCE_NAME_SIZE-1] = '\0';
  priority = UpackBUint8(pbuf + DRAFT_PRIORITY_ADDR);
  if(priority == 0)
	  priority = 100;  //The default priority if the source isn't using priority.
  start_code = UpackBUint8(pbuf + (DRAFT_FIRST_PROPERTY_ADDRESS_ADDR) + 1);
  sequence = UpackBUint8(pbuf + DRAFT_SEQ_NUM_ADDR);
  universe = UpackBUint16(pbuf + DRAFT_UNIVERSE_ADDR);
  slot_count = UpackBUint16(pbuf + DRAFT_PROP_COUNT_ADDR);
  pdata = pbuf + DRAFT_STREAM_HEADER_SIZE;
  
  /*Do final length validation*/
  if((pdata + slot_count) > (pbuf + buflen))
    return false;
  
  return true;
}

/*
 * Returns true if contains draft root vector value
 */
bool isDraft(quint8* pbuf)
{
    if(!pbuf) return false;
    return VECTOR_ROOT_E131_DATA_DRAFT == UpackBUint32(pbuf + ROOT_VECTOR_ADDR);
}

/*
 * Helper function
 * Check framing vector
 * Returns true if vector is expected type
 */
bool checkFramingVector(quint8* pbuf, quint8 expectedVector)
{
    if (!pbuf) return false;
    return (UpackBUint32(pbuf + expectedVector) != expectedVector);
}

/*
 * Helper function
 * Set a bit in the option field
 */
void setOptionBit(quint8 mask, quint8* pbuf, bool value)
{
    if (isDraft(pbuf)) return;
    if(!pbuf) return;

    quint8 current_options = UpackBUint8(pbuf + OPTIONS_ADDR);
    if (value)
        current_options |= mask;
    else
        current_options &= ~mask;

    PackBUint8(pbuf + OPTIONS_ADDR, current_options);
}

/*
 * Helper function
 * Get a bit in the option field
 */
bool getOptionBit(quint8 mask, quint8* pbuf)
{
    if (isDraft(pbuf)) return false;
    if(!pbuf) return false;
    int current_options = UpackBUint8(pbuf + OPTIONS_ADDR);
    return ((current_options & mask) == mask);
}


/* 
 * toggles the preview_data bit of the options field to either 1 or 0
 */
void SetPreviewData(quint8* pbuf, bool preview)
{
    if (checkFramingVector(pbuf, VECTOR_E131_DATA_PACKET))
        return setOptionBit(PREVIEW_DATA_OPTION, pbuf, preview);
}

/*
 * Returns the preview_data bit of the options field
 */
bool GetPreviewData(quint8* pbuf)
{
    if (checkFramingVector(pbuf, VECTOR_E131_DATA_PACKET))
        return getOptionBit(PREVIEW_DATA_OPTION, pbuf);
    else
        return false;
}

/* 
 * toggles the stream_terminated  bit of the options field to either 1 or 0
 */
void SetStreamTerminated(quint8* pbuf, bool terminated)
{
    if (checkFramingVector(pbuf, VECTOR_E131_DATA_PACKET))
        return setOptionBit(STREAM_TERMINATED_OPTION, pbuf, terminated);
}

/* 
 * returns the stream_terminated  bit of the options field
 */
bool GetStreamTerminated(quint8* pbuf)
{
    if (checkFramingVector(pbuf, VECTOR_E131_DATA_PACKET))
        return getOptionBit(STREAM_TERMINATED_OPTION, pbuf);
    else
        return false;
}


/*
 * toggles the Force_Synchronization bit of the options field to either 1 or 0
 */
void SetForceSync(quint8* pbuf, bool terminated)
{
    if (checkFramingVector(pbuf, VECTOR_E131_DATA_PACKET))
        return setOptionBit(FORCE_SYCHRONIZATION_OPTION, pbuf, terminated);
}

/*
 * returns the Force_Synchronization bit of the options field
 */
bool GetForceSync(quint8* pbuf)
{
    if (checkFramingVector(pbuf, VECTOR_E131_DATA_PACKET))
        return getOptionBit(FORCE_SYCHRONIZATION_OPTION, pbuf);
    else
        return false;
}

/*Fills in the multicast address and port (not netiface) to use for listening to or sending on a universe*/
void GetUniverseAddress(quint16 universe, CIPAddr& addr)
{
	addr.SetIPPort(STREAM_IP_PORT);
	//TODO: IPv6 support
	quint8 addrbuf [CIPAddr::ADDRBYTES];
	memset(addrbuf, 0, CIPAddr::ADDRBYTES);
	addrbuf[12] = 239;
	addrbuf[13] = 255;
	PackBUint16(addrbuf + 14, universe);
	addr.SetV6Address(addrbuf);
}

