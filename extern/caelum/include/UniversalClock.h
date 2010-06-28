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

#ifndef UNIVERSALCLOCK_H
#define	UNIVERSALCLOCK_H

#include "CaelumPrerequisites.h"

namespace Caelum {

    /** The system's time model.
     *  This class is responsible of keeping track of current astronomical time
     *  and syncronising with ogre time.
     *
     *  It maintains a snapshot point: At mCurrentTime == 0 julian day was mJulianDayBase.
     *  At any time the julian day can be calculated from mCurrentTime and mJulianDayBase.
     *  This increases precission; mCurrentTime is tracked in seconds while mJulianDayBase
     *  uses days. It would be silly to track the current time in days.
     */
    class CAELUM_EXPORT UniversalClock
    {
	private:
        /// Astronomical julian day at mCurrentTime = 0;
		LongReal mJulianDayBase;

        /// Seconds since mJulianDayBase.
        LongReal mCurrentTime;

		/// Seconds since mJulianDayBase at last update.
		LongReal mLastUpdateTime;

        /// Time scale.
        Ogre::Real mTimeScale;

	public:
        /** Number of seconds per day; exactly 60*60*24.
         */
        static const LongReal SECONDS_PER_DAY;

		/** Constructor.
		 */
		UniversalClock ();

		/** Sets the time scale.
		 * @param scale The new time scale. If negative, time will move backwards; 2.0 means double speed...
		 */
		void setTimeScale (const Ogre::Real scale);

		/** Gets the time scale.
		 *  @return The current time scale. Defaults to 1.
		 */
		Ogre::Real getTimeScale () const;

		/** Updates the clock.
		 *  @param time The time to be added to the clock. It will beaffected by the time scale.
		 */
		void update (const Ogre::Real time);

        /** Set the current time as a julian day.
         *  Set the current time as a julian day, which you build using one
         *  of the static getJulianDayFromXXX functions.
         *  Defaults to J2000 (noon january 1st)
         */
        void setJulianDay(LongReal value);

        /** Set the current time as a gregorian date.
         *  This is here as an easy to use function.
         */
        void setGregorianDateTime(
                int year, int month, int day,
                int hour, int minute, double second);

        /** Get current julian day.
         */
        LongReal getJulianDay() const;

        /** Get the difference in julian day between this and the last update.
         *  This is most likely very small and unprecise.
         */
        LongReal getJulianDayDifference() const;

        /** Get the current julian second (getJulianDay * SECONDS_PER_DAY)
         *  This is most likely very very large and unprecise.
         */
        LongReal getJulianSecond() const;

        /** Get the difference in seconds between this and the last update.
         *  This is what you want for per-frame updates.
         */
        LongReal getJulianSecondDifference() const;
    };
}

#endif //UNIVERSALCLOCK_H
