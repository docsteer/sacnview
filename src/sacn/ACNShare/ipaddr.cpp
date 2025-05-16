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

// ipaddr.cpp: implementation of the CIPAddr class.
//
//////////////////////////////////////////////////////////////////////

#include <string.h>
#include "defpack.h"
#include <stdio.h>
#include "ipaddr.h"

//The default is an all zero address and port
CIPAddr::CIPAddr()
{
	memset(m_addr, 0, ADDRBYTES);
	m_port = 0;
	m_netid = NETID_INVALID;
}

//Construct from a port, v4 address, and network interface
CIPAddr::CIPAddr(netintid id, IPPort port, IPv4 addr)
{
	m_port = port;
	m_netid = id;
	memset(m_addr, 0, ADDRBYTES - sizeof(IPv4));
	PackBUint32(m_addr + ADDRBYTES - sizeof(IPv4), addr);
}

//Construct from a port, v6 address, and network interface
CIPAddr::CIPAddr(netintid id, IPPort port, const quint8* addr)  
{
	m_netid = id;
	m_port = port;
	memcpy(m_addr, addr, ADDRBYTES);
}

CIPAddr::CIPAddr(const CIPAddr& addr)
{
	m_netid = addr.m_netid;
	m_port = addr.m_port;
	memcpy(m_addr, addr.m_addr, ADDRBYTES);
}

CIPAddr::CIPAddr(const QHostAddress &address)
{
    m_port = 0;
    m_netid = 0;
    memset(m_addr, 0, ADDRBYTES - sizeof(IPv4));
    PackBUint32(m_addr + ADDRBYTES - sizeof(IPv4), address.toIPv4Address());
}

CIPAddr::~CIPAddr()
{

}

CIPAddr& CIPAddr::operator=(const CIPAddr& addr)
{
	m_netid = addr.m_netid;
	m_port = addr.m_port;
	memcpy(m_addr, addr.m_addr, ADDRBYTES);
	return *this;
}

void CIPAddr::SetNetInterface(netintid id)
{
	m_netid = id;
}

netintid CIPAddr::GetNetInterface() const
{
	return m_netid;
}

void CIPAddr::SetIPPort(IPPort port)
{
	m_port = port;
}

IPPort CIPAddr::GetIPPort() const
{
	return m_port;
}

bool CIPAddr::IsV4Address() const
{
	//The first 10 bytes of an embedded v4 address are all 0's
	//The next 2 bytes are either 0x0000 or 0xffff
	const quint8 tst[10] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	const unsigned short mask = 0xffff;
	return (0 == memcmp(tst, m_addr, 10)) &&
		   ((0 == *(reinterpret_cast<unsigned short*>(const_cast<quint8*>(m_addr + 10)))) ||
		    (mask == *(reinterpret_cast<unsigned short*>(const_cast<quint8*>(m_addr + 10)))));
}

void CIPAddr::SetV4Address(IPv4 addr)
{
	memset(m_addr, 0, ADDRBYTES - sizeof(IPv4));
	PackBUint32(m_addr + ADDRBYTES - sizeof(IPv4), addr);
}

IPv4 CIPAddr::GetV4Address() const
{
	return UpackBUint32(m_addr + ADDRBYTES - sizeof(IPv4));
}

void CIPAddr::SetV6Address(const quint8* addr)
{
	memcpy(m_addr, addr, ADDRBYTES);
}

const quint8* CIPAddr::GetV6Address() const
{
	return m_addr;
}

bool CIPAddr::IsMulticastAddress() const
{
	//A v4 address has it's upper byte between 224 and 239 inclusive.
	//A v6 address has an upmost byte of 0xff
	bool v4 = IsV4Address();
	return (v4 && (m_addr[12] >= 224) && (m_addr[12] <= 239)) ||
		   (!v4 && (m_addr[0] == 0xff));
}

bool operator<(const CIPAddr& a1, const CIPAddr& a2)
{
	int res = memcmp(a1.m_addr, a2.m_addr, CIPAddr::ADDRBYTES);
	return (0 > res) || 
		   ((0 == res) && (a1.m_port < a2.m_port)) ||
		   ((0 == res) && (a1.m_port == a2.m_port) && (a1.m_netid < a2.m_netid));
}

bool operator==(const CIPAddr& a1, const CIPAddr& a2)
{
	return (0 == memcmp(a1.m_addr, a2.m_addr, CIPAddr::ADDRBYTES)) &&
		   (a1.m_port == a2.m_port) && (a1.m_netid == a2.m_netid);
}

bool operator!=(const CIPAddr& a1, const CIPAddr& a2)
{
	return (0 != memcmp(a1.m_addr, a2.m_addr, CIPAddr::ADDRBYTES)) ||
		   (a1.m_port != a2.m_port) || (a1.m_netid != a2.m_netid);
}


///////////////////////////////////////////////////////////////////
// Static functions

//Returns an address based on the string. 
	//d.d.d.d		for ipv4 address
	//d.d.d.d:d		for ipv4 address and port
	//d.d.d.d:d,d	for ipv4 address: port, network interface
	//[x:x:x:x:x:x:x:x]		for ipv6 address
	//[x:x:x:x:x:x:x:x]:d	for ipv6 address and port
	//[x:x:x:x:x:x:x:x]:d,d	for ipv6 address: port, network interface
CIPAddr CIPAddr::StringToAddr(const char* ptext)
{
	CIPAddr addr;
	quint8 addrbytes [ADDRBYTES + 1];  //The temporary buffer
	memset(addrbytes, 0, ADDRBYTES);
	
	//Ok.  Scan for the different possiblities -- Init the tmp vars
	int v41 = -1;
	int v42 = -1;
	int v43 = -1;
	int v44 = -1;
	int port = -1;
	int netint = -1;

	int num = sscanf(ptext, "%d.%d.%d.%d:%d,%d", &v41, &v42, &v43, &v44, &port, &netint);
	if(num >= 4)
	{
		addrbytes[12] = (quint8)v41;
		addrbytes[13] = (quint8)v42;
		addrbytes[14] = (quint8)v43;
		addrbytes[15] = (quint8)v44;
		addr.SetV6Address(addrbytes);

		if(num >= 5)
			addr.SetIPPort((IPPort)port);
		if(num == 6)
			addr.SetNetInterface(netint);
	}
	else //Try IPv6 -- Yes, this is pretty minimal.  And why didn't I have these in AsyncSocket instead, so I could just use the socket variant of parsing...  Because some apps wanted to use the concept of ip addresses without needing the actual sockets/communications...
	{	//I'm going to have to flesh out the IPV6 parsing, since I didn't want these tightly bound to the AsyncSocket library.
		int v61 = -1;
		int v62 = -1;
		int v63 = -1;
		int v64 = -1;
		int v65 = -1;
		int v66 = -1;
		int v67 = -1;
		int v68 = -1;
		num = sscanf(ptext, "[%x:%x:%x:%x:%x:%x:%x:%x]:%d,%d", &v61, &v62, &v63, &v64, &v65,&v66,&v67,&v68,&port, &netint);
		if(num >= 8)
		{
			PackBUint16(addrbytes, (quint16)v61);
			PackBUint16(addrbytes +2, (quint16)v62);
			PackBUint16(addrbytes +4, (quint16)v63);
			PackBUint16(addrbytes +6, (quint16)v64);
			PackBUint16(addrbytes +8, (quint16)v65);
			PackBUint16(addrbytes +10, (quint16)v66);
			PackBUint16(addrbytes +12, (quint16)v67);
			PackBUint16(addrbytes +14, (quint16)v68);
			addr.SetV6Address(addrbytes);
			if(num >= 9)
				addr.SetIPPort((IPPort)port);
			if(num == 10)
				addr.SetNetInterface(netint);
		}
	}
	return addr;
}

//Translates an address to a preallocated text string of ADDRSTRINGBYTES, including the terminating NULL
void CIPAddr::AddrIntoString(const CIPAddr& addr, char* ptxt, bool showport, bool showint)
{
	quint8 addrb [ADDRBYTES];  //The temporary buffer
	memcpy(addrb, addr.GetV6Address(), ADDRBYTES);

	//Depending on address type and params, print the right string
	if(addr.IsV4Address())
	{
		if(showint)
			snprintf(ptxt, ADDRSTRINGBYTES, "%d.%d.%d.%d:%d,%d", addrb[12], addrb[13], addrb[14], addrb[15], addr.GetIPPort(), addr.GetNetInterface());
		else if(showport)
			snprintf(ptxt, ADDRSTRINGBYTES, "%d.%d.%d.%d:%d", addrb[12], addrb[13], addrb[14], addrb[15], addr.GetIPPort());
		else
			snprintf(ptxt, ADDRSTRINGBYTES, "%d.%d.%d.%d", addrb[12], addrb[13], addrb[14], addrb[15]);
	}
	else
	{
		if(showint)
			snprintf(ptxt, ADDRSTRINGBYTES, "[%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X]:%d,%d",
					UpackBUint16(addrb), UpackBUint16(addrb + 2),
					UpackBUint16(addrb + 4), UpackBUint16(addrb + 6),
					UpackBUint16(addrb + 8), UpackBUint16(addrb + 10),
					UpackBUint16(addrb + 12), UpackBUint16(addrb + 14),
					addr.GetIPPort(), addr.GetNetInterface());
		else if(showport)
			snprintf(ptxt, ADDRSTRINGBYTES, "[%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X]:%d",
					UpackBUint16(addrb), UpackBUint16(addrb + 2),
					UpackBUint16(addrb + 4), UpackBUint16(addrb + 6),
					UpackBUint16(addrb + 8), UpackBUint16(addrb + 10),
					UpackBUint16(addrb + 12), UpackBUint16(addrb + 14),
					addr.GetIPPort());
		else
			snprintf(ptxt, ADDRSTRINGBYTES, "[%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X]",
					UpackBUint16(addrb), UpackBUint16(addrb + 2),
					UpackBUint16(addrb + 4), UpackBUint16(addrb + 6),
					UpackBUint16(addrb + 8), UpackBUint16(addrb + 10),
					UpackBUint16(addrb + 12), UpackBUint16(addrb + 14));
	}
}


QHostAddress CIPAddr::ToQHostAddress() const
{
    if(IsV4Address())
        return QHostAddress(this->GetV4Address());
    return QHostAddress();
}

