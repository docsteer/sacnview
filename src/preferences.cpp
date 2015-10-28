// Copyright (c) 2015 Tom Barthel-Steer, http://www.tomsteer.net
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

#include <Qt>
#include "preferences.h"

// The base color to generate pastel shades for sources
static const QColor mixColor = QColor("coral");

Preferences *Preferences::m_instance = NULL;

Preferences::Preferences()
{

}

Preferences *Preferences::getInstance()
{
    if(!m_instance)
    {
        m_instance = new Preferences();
    }

    return m_instance;
}


QNetworkInterface Preferences::networkInterface() const
{
    return m_interface;
}

void Preferences::setNetworkInterface(const QNetworkInterface &value)
{
    if(m_interface.name()!=value.name())
    {
        m_interface = value;
        //emit networkInterfaceChanged();
    }
}

QColor Preferences::colorForCID(const CID &cid)
{
    if(m_cidToColor.contains(cid))
        return m_cidToColor[cid];

    int red = qrand() % 255;
    int green = qrand() % 255;
    int blue = qrand() % 255;

    red = (red + mixColor.red()) / 2;
    green = (green + mixColor.green()) / 2;
    blue = (blue + mixColor.blue()) / 2;

    QColor newColor = QColor::fromRgb(red, green, blue);
    m_cidToColor[cid] = newColor;
    return newColor;
}
