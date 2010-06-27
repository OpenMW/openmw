/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2006-2008 Caelum team. See Contributors.txt for details.

Caelum is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Caelum is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Caelum. If not, see <http://www.gnu.org/licenses/>.
*/

#include "CaelumPrecompiled.h"
#include "UniversalClock.h"
#include "Astronomy.h"

namespace Caelum
{
    const Caelum::LongReal UniversalClock::SECONDS_PER_DAY = 86400.0;

    UniversalClock::UniversalClock () {
        setJulianDay (Astronomy::J2000);        
	    setTimeScale (1.0);
    }

    void UniversalClock::setJulianDay (Caelum::LongReal value) {
        mJulianDayBase = value;
        mCurrentTime = 0;
        mLastUpdateTime = 0;
    }

    void UniversalClock::setGregorianDateTime(
            int year, int month, int day,
            int hour, int minute, double second)
    {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        setJulianDay(Astronomy::getJulianDayFromGregorianDateTime(year, month, day, hour, minute, second));
    }

    LongReal UniversalClock::getJulianDay () const
    {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        Caelum::LongReal res = mJulianDayBase + mCurrentTime / SECONDS_PER_DAY;
        return res;
    }

    LongReal UniversalClock::getJulianDayDifference () const {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        return (mCurrentTime - mLastUpdateTime) / SECONDS_PER_DAY;
    }

    LongReal UniversalClock::getJulianSecond () const {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        LongReal res = mJulianDayBase * SECONDS_PER_DAY + mCurrentTime;
        return res;
    }

    LongReal UniversalClock::getJulianSecondDifference () const {
        ScopedHighPrecissionFloatSwitch precissionSwitch;
        return mCurrentTime - mLastUpdateTime;
    }

    void UniversalClock::setTimeScale (const Ogre::Real scale) {
	    mTimeScale = scale;
    }

    Ogre::Real UniversalClock::getTimeScale () const {
	    return mTimeScale;
    }

    void UniversalClock::update (const Ogre::Real time) {
        mLastUpdateTime = mCurrentTime;
        mCurrentTime += time * mTimeScale;
    }
}

