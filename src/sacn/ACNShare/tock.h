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

  A tock is the number of nanoseconds since a platform-specific date.
  Tocks are never used directly, rather the difference between two tocks 
  (latest - previous) are used to determine the passage of time.  It is
  assumed that tocks always move forward, and that the base date is not
  different for any instance of the compiled tock, nor does the base date change.  
  
  Tocks are guaranteed to have a nanosecond granularity.
  Tock comparisons correctly handle clock rollover.

  A ttimer is a simple abstraction for typical timer usage, which is
  setting a number of milliseconds to time out, and then telling whether
  or not the ttimer has expired.
*/

#ifndef _TOCK_H_
#define _TOCK_H_

#include <QtGlobal>
#include <chrono>

class tock;
class ttimer;

//These functions comprise the tock API

//Initializes the tock layer.  Only needs to be called once per application,
//but can be called multiple times as long as there is an equal number of
//Tock_StopLib() calls.
bool Tock_StartLib();
tock Tock_GetTock(); //Gets a tock representing the current time
void Tock_StopLib(); //Shuts down the tock layer.

//This is the actual tock
class tock
{
public:

    typedef std::chrono::nanoseconds resolution_t;

    //construction and copying
    tock();
    tock(const tock &) = default;
    tock(tock &&) = default;
    tock & operator=(const tock &) = default;
    tock & operator=(tock &&) = default;

    template<typename Rep, typename Period>
    tock(std::chrono::duration<Rep, Period> duration)
    {
        v = duration;
    }

    //Returns the number of nanoseconds that this tock represents
    resolution_t Get() const;

    //Used sparingly, but sets the number of nanoseconds that this tock represents
    void Set(resolution_t time);

protected:

    resolution_t v;

    friend bool operator>(const tock & t1, const tock & t2);
    friend bool operator>=(const tock & t1, const tock & t2);
    friend bool operator==(const tock & t1, const tock & t2);
    friend bool operator!=(const tock & t1, const tock & t2);
    friend bool operator<(const tock & t1, const tock & t2);
    friend bool operator<=(const tock & t1, const tock & t2);
    friend quint32 operator-(const tock & t1, const tock & t2);
};

//The class used for simple expiration tracking
class ttimer
{
public:

    //construction/setup
    ttimer(); //Will immediately time out if timeout isn't set
    ttimer(const ttimer &) = default;
    ttimer(ttimer &&) = default;
    ttimer & operator=(const ttimer &) = default;
    ttimer & operator=(ttimer &&) = default;
    template<typename Rep, typename Period>
    ttimer(std::chrono::duration<Rep, Period> interval); //The duration before the timer will time out

    void SetInterval(tock::resolution_t interval); //Sets a new timeout interval and resets the timer

    tock::resolution_t GetInterval() const; //Returns the current timeout interval

    void Reset(); //Resets the timer, using the current timeout interval

    //Returns true if the timer has expired
    //Call Reset() to use this timer again for a new interval.
    bool Expired() const;

    friend bool operator==(const ttimer & t1, const ttimer & t2);
    friend bool operator!=(const ttimer & t1, const ttimer & t2);

protected:

    tock::resolution_t interval;
    tock tockout;
};
#endif /*_TOCK_H*/
