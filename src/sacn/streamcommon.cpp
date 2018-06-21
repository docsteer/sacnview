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
  PackB2(p, RLP_PREAMBLE_SIZE);                   
  p += 2;

  //root post-amble size
  PackB2(p, RLP_POSTAMBLE_SIZE);                    
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
  PackB4(p, ROOT_VECTOR);
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
  PackB4(p, FRAMING_VECTOR);
  p += 4;
	
  //framing source name
  strncpy((char*)p, source_name, SOURCE_NAME_SIZE);
  p[SOURCE_NAME_SIZE - 1] = '\0'; 
  p += SOURCE_NAME_SIZE;  
  //We currently null terminate to be safe.

  //framing priority
  PackB1(p, priority);
  p += 1;

  //reserved
  PackB2(p, reserved);
  p += 2;
   
  //framing sequence number
  PackB1(p, 0); //We leave the sequence number set to NONE                   
  p += 1;  
  
  PackB1(p, options);
  p += 1;

  //framing universe
  PackB2(p, universe);
  p += 2;

  //dmp flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, STREAM_HEADER_SIZE - DMP_FLAGS_AND_LENGTH_ADDR 
		 + slot_count, false); 
  p += 2;
   
  //DMP vector
  PackB1(p, DMP_VECTOR);
  p += 1;

  //DMP address and data type
  PackB1(p, ADDRESS_AND_DATA_FORMAT);
  p += 1;

  //DMP first property address
  PackB2(p, 0); //THIS WILLBE START CODE IN OLD
  p += 2;
  
  //DMP address increment
  PackB2(p, ADDRESS_INC);
  p += 2; 

  //DMP property value count -- Includes the one byte for the start code
  PackB2(p, slot_count + 1);
  p += 2;
   
  //start code for ratified standard
  PackB1(p, start_code);
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
  PackB2(p, RLP_PREAMBLE_SIZE);                   
  p += 2;

  //root post-amble size
  PackB2(p, RLP_POSTAMBLE_SIZE);                    
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
  PackB4(p, DRAFT_ROOT_VECTOR);
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
  PackB4(p, FRAMING_VECTOR);
  p += 4;
	
  //framing source name
  strncpy((char*)p, source_name, DRAFT_SOURCE_NAME_SIZE);
  p[DRAFT_SOURCE_NAME_SIZE - 1] = '\0'; 
  p += DRAFT_SOURCE_NAME_SIZE;  
  //We currently null terminate to be safe.

  //framing priority
  PackB1(p, priority);
  p += 1;
   
  //framing sequence number
  PackB1(p, 0); //We leave the sequence number set to NONE                   
  p += 1;  

  //framing universe
  PackB2(p, universe);
  p += 2;

  //dmp flags and length
  VHD_PackFlags(p, false, false, false);
  VHD_PackLength(p, DRAFT_STREAM_HEADER_SIZE 
		 - DRAFT_DMP_FLAGS_AND_LENGTH_ADDR + slot_count, false); 
  p += 2;
   
  //DMP vector
  PackB1(p, DMP_VECTOR);
  p += 1;

  //DMP address and data type
  PackB1(p, ADDRESS_AND_DATA_FORMAT);
  p += 1;

  //DMP first property address
  PackB1(p, 0); //this is just padding
  p += 1;
  PackB1(p, start_code); 
  p += 1;
  
  //DMP address increment
  PackB2(p, ADDRESS_INC);
  p += 2; 

  //DMP property value count
  PackB2(p, slot_count);
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
          PackB1(pbuf + DRAFT_SEQ_NUM_ADDR, seq);
    }
    else if (pbuf)
    {
        PackB1(pbuf + SEQ_NUM_ADDR, seq);
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
  
  root_vector = UpackB4(pbuf + ROOT_VECTOR_ADDR);
  
  if(root_vector == ROOT_VECTOR)
  {
      return VerifyStreamHeader(pbuf, buflen, source_cid, source_space, 
				priority, start_code, reserved, sequence, 
				options, universe, slot_count, pdata);
  }
  else if(root_vector == DRAFT_ROOT_VECTOR)
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
  if(UpackB2(pbuf) != RLP_PREAMBLE_SIZE)
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
  if(UpackB4(pbuf + FRAMING_VECTOR_ADDR) != FRAMING_VECTOR)
  {
      return false;
  }
  if(UpackB2(pbuf + RESERVED_ADDR) != RESERVED_VALUE)
  { 
      return false;
  }
  if(UpackB1(pbuf + DMP_VECTOR_ADDR) != DMP_VECTOR)
  {
      return false;
  }
  if(UpackB1(pbuf + DMP_ADDRESS_AND_DATA_ADDR) 
     != ADDRESS_AND_DATA_FORMAT)
  {
      return false;
  }
  if(UpackB2(pbuf + FIRST_PROPERTY_ADDRESS_ADDR) 
     != DMP_FIRST_PROPERTY_ADDRESS_FORCE)
  {
      return false;
  }
  if(UpackB2(pbuf + ADDRESS_INC_ADDR) != ADDRESS_INC)
  {
      return false;
  }
   
  /* Init the parameters */
  source_cid.Unpack(pbuf + CID_ADDR);
  
  strncpy(source_space, (char*)(pbuf + SOURCE_NAME_ADDR), SOURCE_NAME_SIZE);
  source_space[SOURCE_NAME_SIZE-1] = '\0';
  priority = UpackB1(pbuf + PRIORITY_ADDR);
  start_code = UpackB1(pbuf + START_CODE_ADDR);
  reserved = UpackB2(pbuf + RESERVED_ADDR);
  sequence = UpackB1(pbuf + SEQ_NUM_ADDR);
  options = UpackB1(pbuf + OPTIONS_ADDR);
  universe = UpackB2(pbuf + UNIVERSE_ADDR);
  slot_count = UpackB2(pbuf + PROP_COUNT_ADDR) - 1;  //The property value count includes the start code byte
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
  if(UpackB2(pbuf) != RLP_PREAMBLE_SIZE)
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
  if(UpackB4(pbuf + FRAMING_VECTOR_ADDR) != FRAMING_VECTOR)
  {
      return false;
  }
  if(UpackB1(pbuf + DRAFT_DMP_VECTOR_ADDR) != DMP_VECTOR)
  {
      return false;
  }
  if(UpackB1(pbuf + DRAFT_DMP_ADDRESS_AND_DATA_ADDR) 
     != ADDRESS_AND_DATA_FORMAT)
  {
      return false;
  }
  if(UpackB2(pbuf + DRAFT_ADDRESS_INC_ADDR) != ADDRESS_INC)
  {
      return false;
  }
  // this check is removed, as one proposal allows larger buffer sizes
  //      if(UpackB2(pbuf + 88) > 512)
  //      return false;
  //   
  
  /* Init the parameters */
  source_cid.Unpack(pbuf + CID_ADDR);
  
  strncpy(source_space, (char*)(pbuf + SOURCE_NAME_ADDR), 
	  DRAFT_SOURCE_NAME_SIZE);
  source_space[DRAFT_SOURCE_NAME_SIZE-1] = '\0';
  priority = UpackB1(pbuf + DRAFT_PRIORITY_ADDR);
  if(priority == 0)
	  priority = 100;  //The default priority if the source isn't using priority.
  start_code = UpackB1(pbuf + (DRAFT_FIRST_PROPERTY_ADDRESS_ADDR) + 1);
  sequence = UpackB1(pbuf + DRAFT_SEQ_NUM_ADDR);
  universe = UpackB2(pbuf + DRAFT_UNIVERSE_ADDR);
  slot_count = UpackB2(pbuf + DRAFT_PROP_COUNT_ADDR);
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
    if(!pbuf)
        return false;
    return DRAFT_ROOT_VECTOR == UpackB4(pbuf + ROOT_VECTOR_ADDR);
}

/* 
 * toggles the preview_data bit of the options field to either 1 or 0
 */
void SetPreviewData(quint8* pbuf, bool preview)
{
  if (isDraft(pbuf)) return;
  if(pbuf)
  {
    quint8 current_options = UpackB1(pbuf + OPTIONS_ADDR);
    //sets "bit 7" to the value of preview
    current_options = (current_options & ~(1 << 7)) | (preview << 7);
    PackB1(pbuf + OPTIONS_ADDR, current_options);
  }
}

/* 
 * toggles the stream_terminated  bit of the options field to either 1 or 0
 */
void SetStreamTerminated(quint8* pbuf, bool terminated)
{
  if (isDraft(pbuf)) return;
  if(pbuf)
  {
    quint8 current_options = UpackB1(pbuf + OPTIONS_ADDR);
    //sets "bit 6" to the value of terminated
    current_options = (current_options & ~(1 << 6)) | (terminated << 6);
    PackB1(pbuf + OPTIONS_ADDR, current_options);
  }
}

/* 
 * returns the stream_terminated  bit of the options field
 */
bool GetStreamTerminated(quint8* pbuf)
{
  if (isDraft(pbuf)) return false;
  if(!pbuf)
     return false;
  int current_options = UpackB1(pbuf + OPTIONS_ADDR);
  return ((current_options & 0x40) == 0x40);
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
	PackB2(addrbuf + 14, universe);
	addr.SetV6Address(addrbuf);
}

