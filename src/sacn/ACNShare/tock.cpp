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

/* tock.cpp
  
   Implementation of the platforms-specific part of the tock library.
*/

#include "tock.h"
#include <QElapsedTimer>

static QElapsedTimer timer;

//Initializes the tock layer.  Only needs to be called once per application
bool Tock_StartLib()
{
    timer.start();
    return true;
}

//Gets a tock representing the current time
tock Tock_GetTock()
{
    return tock(std::chrono::nanoseconds(timer.nsecsElapsed()));
}

//Shuts down the tock layer.
void Tock_StopLib()
{

}

tock::tock():v(0) {}

template <typename Rep, typename Period>
tock::tock(std::chrono::duration<Rep, Period> duration)
{
    v = duration;
}

tock::resolution_t tock::Get() const
{
    return v;
}

void tock::Set(tock::resolution_t time)
{
    v = time;
}

bool operator>(const tock& t1, const tock& t2)
{
    return t1.v.count() - t2.v.count() > 0;
}

bool operator>=(const tock& t1, const tock& t2)
{
    return t1.v.count() - t2.v.count() >= 0;
}

bool operator==(const tock& t1, const tock& t2)
{
    return t1.v.count() - t2.v.count() == 0;
}

bool operator!=(const tock& t1, const tock& t2)
{
    return t1.v.count() - t2.v.count() != 0;
}

bool operator<(const tock& t1, const tock& t2)
{
    return t2.v.count() - t1.v.count() > 0;
}

bool operator<=(const tock& t1, const tock& t2)
{
    return t2.v.count() - t1.v.count() >= 0;
}

ttimer::ttimer():interval(0)
{
    Reset();
}

template <typename Rep, typename Period>
ttimer::ttimer(std::chrono::duration<Rep, Period> interval) :
    interval(interval)
{
    Reset();
}

void ttimer::SetInterval(tock::resolution_t interval)
{
    this->interval = interval;
    Reset();
}

tock::resolution_t ttimer::GetInterval() const
{
    return interval;
}

void ttimer::Reset()
{
    tockout.Set(Tock_GetTock().Get() + interval);
}

bool ttimer::Expired() const
{
    return (Tock_GetTock().Get()) >= tockout.Get();
}

bool operator==(const ttimer& t1, const ttimer& t2)
{
    return ((t1.tockout == t2.tockout) && (t1.interval == t2.interval));
}

bool operator!=(const ttimer& t1, const ttimer& t2)
{
    return !(t1 == t2);
}
