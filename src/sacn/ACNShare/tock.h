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

/*  tock.h

  Provides a standard definition of a tock and a ttimer.

  A tock is the number of milliseconds since a platform-specific date.
  Tocks are never used directly, rather the difference between two tocks 
  (latest - previous) are used to determine the passage of time.  It is
  assumed that tocks always move forward, and that the base date is not
  different for any instance of the compiled tock, nor does the base date change.  
  
  Tocks are guaranteed to have a millisecond granularity.
  Tock comparisons correctly handle clock rollover.

  A ttimer is a simple abstraction for typical timer usage, which is
  setting a number of milliseconds to time out, and then telling whether
  or not the ttimer has expired.
*/

#ifndef _TOCK_H_
#define _TOCK_H_

//tock requires the default types
#ifndef _DEFTYPES_H_
#error "#include error: tock.h requires deftypes.h"
#endif

class tock;
class ttimer;

//These functions comprise the tock API

//Initializes the tock layer.  Only needs to be called once per application,
//but can be called multiple times as long as there is an equal number of
//Tock_StopLib() calls.
bool Tock_StartLib();  
tock Tock_GetTock();   //Gets a tock representing the current time
void Tock_StopLib();   //Shuts down the tock layer.

//This is the actual tock 
class tock
{
public:
	//construction and copying
	tock();
	tock(uint4 ms);
	tock(const tock& t);
	tock& operator=(const tock& t);

	//Returns the number of milliseconds that this tock represents
	uint4 Getms();  

	//Used sparingly, but sets the number of milliseconds that this tock represents
	void Setms(uint4 ms);

protected:
	int4 v;  //Signed, so the wraparound calculations will work
	
	friend bool operator>(const tock& t1, const tock& t2);
	friend bool operator>=(const tock& t1, const tock& t2);
	friend bool operator==(const tock& t1, const tock& t2);
	friend bool operator!=(const tock& t1, const tock& t2);
	friend bool operator<(const tock& t1, const tock& t2);
	friend bool operator<=(const tock& t1, const tock& t2);
	friend uint4 operator-(const tock& t1, const tock& t2);
};

//The class used for simple expiration tracking
class ttimer
{
public:
	//construction/setup
	ttimer();				//Will immediately time out if timeout isn't set
	ttimer(int4 ms);	//The number of milliseconds before the timer will time out
	void SetInterval(int4 ms);	//Sets a new timeout interval (in ms) and resets the timer
	int4 GetInterval();			//Returns the current timeout interval (in ms)

	void Reset();	//Resets the timer, using the current timeout interval
	bool Expired();  //Returns true if the timer has expired.
					 //Call Reset() to use this timer again for a new interval.
protected:
	int4 interval;
	tock tockout;
};


//---------------------------------------------------------------------------------
//	Implementation

/*ttimer implementation*/
inline ttimer::ttimer():interval(0) {Reset();}
inline ttimer::ttimer(int4 ms):interval(ms) {Reset();}
inline void ttimer::SetInterval(int4 ms) {interval = ms; Reset();}
inline int4 ttimer::GetInterval() {return interval;}
inline void ttimer::Reset() {tockout.Setms(Tock_GetTock().Getms() + interval);}
inline bool ttimer::Expired() {return Tock_GetTock() > tockout;}

/*tock implementation*/
inline tock::tock():v(0) {}
inline tock::tock(uint4 ms):v(ms) {};
inline tock::tock(const tock& t) {v = t.v;}
inline tock& tock::operator=(const tock& t) {v = t.v; return *this;}

inline uint4 tock::Getms() {return v;}
inline void tock::Setms(uint4 ms) {v = ms;}
	
inline bool operator>(const tock& t1, const tock& t2)  {return t1.v - t2.v > 0;}
inline bool operator>=(const tock& t1, const tock& t2) {return t1.v - t2.v >= 0;}
inline bool operator==(const tock& t1, const tock& t2) {return t1.v - t2.v == 0;}
inline bool operator!=(const tock& t1, const tock& t2) {return t1.v - t2.v != 0;}
inline bool operator<(const tock& t1, const tock& t2)  {return t2.v - t1.v > 0;}
inline bool operator<=(const tock& t1, const tock& t2) {return t2.v - t1.v >= 0;}
inline uint4 operator-(const tock& t1, const tock& t2) {return t1.v - t2.v;}


#endif /*_TOCK_H*/
