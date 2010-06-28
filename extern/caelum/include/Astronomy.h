/*
This file is part of Caelum.
See http://www.ogre3d.org/wiki/index.php/Caelum 

Copyright (c) 2008 Caelum team. See Contributors.txt for details.

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

#ifndef CAELUM__ASTRONOMY_H
#define CAELUM__ASTRONOMY_H

#include "CaelumPrerequisites.h"

namespace Caelum
{
    /** Static class with astronomy routines.
     *  This class contains various astronomical routines useful in Caelum.
     *
     *  Most of the formulas are from http://stjarnhimlen.se/comp/ppcomp.html
     *  That site contains much more than was implemented here; it has code
     *  for determining the positions of all the planets. Only the sun and
     *  moon are actually useful for caelum.
     *
     *  The formulas are isolated here in pure procedural code for easier
     *  testing (Tests are done as assertions in the demo).
     *
     *  Precision is vital here, so this class uses Caelum::LongReal(double)
     *  instead of Ogre::Real(float) for precission. All angles are in degrees
     *  unless otherwise mentioned. Ogre::Degree and Ogre::Radian use
     *  Ogre::Real and should be avoided here.
     */
    class CAELUM_EXPORT Astronomy
    {
    private:
        Astronomy() {}

        static const LongReal PI;

        /** Normalize an angle to the 0, 360 range.
         *  @param x The angle to normalize
         */
        static LongReal normalizeDegrees (LongReal x);

        /// Convert radians to degrees.
        static LongReal radToDeg (LongReal x);

        /// Convert degrees to radians.
        static LongReal degToRad (LongReal x);

        static LongReal sinDeg (LongReal x);
        static LongReal cosDeg (LongReal x);
        static LongReal atan2Deg (LongReal y, LongReal x);

    public:
        /// January 1, 2000, noon
        static const LongReal J2000;

        /** Convert from ecliptic to ecuatorial spherical coordinates, in radians.
         *  @param lon Ecliptic longitude
         *  @param lat Ecliptic latitude
         *  @param rasc Right ascension
         *  @param decl Declination
         *  @warning: This function works in radians.
         */
		static void convertEclipticToEquatorialRad (
                LongReal lon, LongReal lat,
                LongReal& rasc, LongReal& decl);

		static void convertRectangularToSpherical (
                LongReal x, LongReal y, LongReal z,
                LongReal &rasc, LongReal &decl, LongReal &dist);

        static void convertSphericalToRectangular (
                LongReal rasc, LongReal decl, LongReal dist,
                LongReal &x, LongReal &y, LongReal &z);

        /** Convert from equatorial to horizontal coordinates.
         *  This function converts from angles relative to the earth's equator
         *  to angle relative to the horizon at a given point.
         *  @param jday Astronomical time as julian day.
         *  @param longitude Observer's longitude in degrees east.
         *  @param latitude Observer's latitude in degrees north.
         *  @param rasc Object's right ascension.
         *  @param decl Object's declination.
         *  @param azimuth Object's azimuth (clockwise degrees from true north).
         *  @param altitude Object's altitude (degrees above the horizon).
         */
        static void convertEquatorialToHorizontal (
                LongReal jday,
                LongReal longitude, LongReal latitude,
                LongReal rasc,      LongReal decl,
                LongReal &azimuth,  LongReal &altitude);

        /** Get the sun's position in the sky in, relative to the horizon.
         *  @param jday Astronomical time as julian day.
         *  @param longitude Observer longitude
         *  @param latitude Observer latitude
         *  @param azimuth Astronomical azimuth, measured clockwise from North = 0.
         *  @param altitude Astronomical altitude, elevation above the horizon.
         */
        static void getHorizontalSunPosition (
                LongReal jday,
                LongReal longitude, LongReal latitude,
                LongReal &azimuth, LongReal &altitude);

        static void getHorizontalSunPosition (
                LongReal jday,
                Ogre::Degree longitude, Ogre::Degree latitude,
                Ogre::Degree &azimuth, Ogre::Degree &altitude);

        /// Gets the moon position at a specific time in ecliptic coordinates
        /// @param lon: Ecliptic longitude, in radians.
        /// @param lat: Ecliptic latitude, in radians.
		static void getEclipticMoonPositionRad (
                LongReal jday,
                LongReal &lon,
                LongReal &lat);

        static void getHorizontalMoonPosition (
                LongReal jday,
                LongReal longitude, LongReal latitude,
                LongReal &azimuth, LongReal &altitude);
		static void getHorizontalMoonPosition (
                LongReal jday,
                Ogre::Degree longitude, Ogre::Degree latitude,
                Ogre::Degree &azimuth, Ogre::Degree &altitude);

        /** Get astronomical julian day from normal gregorian calendar.
         *  From wikipedia: the integer number of days that have elapsed
         *  since the initial epoch defined as
         *  noon Universal Time (UT) Monday, January 1, 4713 BC
         *  @note this is the time at noon, not midnight.
         */
        static int getJulianDayFromGregorianDate (
                int year, int month, int day); 

        /** Get astronomical julian day from normal gregorian calendar.
         *  Calculate julian day from a day in the normal gregorian calendar.
         *  Time should be given as UTC.
         *  @see http://en.wikipedia.org/wiki/Julian_day
         */
        static LongReal getJulianDayFromGregorianDateTime (
                int year, int month, int day,
                int hour, int minute, LongReal second); 

        /** Get astronomical julian day from normal gregorian calendar.
         *  @see above (I don't know the proper doxygen syntax).
         */
        static LongReal getJulianDayFromGregorianDateTime (
                int year, int month, int day,
                LongReal secondsFromMidnight); 

        /// Get gregorian date from integer julian day.
        static void getGregorianDateFromJulianDay (
                int julianDay, int &year, int &month, int &day);

        /// Get gregorian date time from floating point julian day.
        static void getGregorianDateTimeFromJulianDay (
                LongReal julianDay, int &year, int &month, int &day,
                int &hour, int &minute, LongReal &second);

        /// Get gregorian date from floating point julian day.
        static void getGregorianDateFromJulianDay (
                LongReal julianDay, int &year, int &month, int &day);

        /** Enter high-precission floating-point mode.
         *
         *  By default Direct3D decreases the precission of ALL floating
         *  point calculations, enough to stop Caelum's astronomy routines
         *  from working correctly.
         *  
         *  To trigger this behaviour in a standard ogre demo select the
         *  Direct3D render system and set "Floating-point mode" to
         *  "Fastest". Otherwise it's not a problem.
         *          
         *  It can be fixed by changing the precission only inside caelum's
         *  astronomy routines using the _controlfp function. This only works
         *  for MSVC on WIN32; This is a no-op on other compilers.
         *
         *  @note: Must be paired with restoreFloatingPointMode.
         *  @return Value to pass to restoreFloatingModeMode.
         */
        static int enterHighPrecissionFloatingPointMode ();

        /** Restore old floating point precission.
         *  @see enterHighPrecissionFloatingPointMode.
         */
        static void restoreFloatingPointMode (int oldMode);
    };

    /** Dummy class to increase floting point precission in a block
     *  This class will raise precission in the ctor and restore it
     *  in the destructor. During it's lifetime floating-point
     *  precission will be increased.
     *
     *  To use this class just create a instance on the stack at the start of a block.
     *
     *  @see Astronomy::enterHighPrecissionFloatingPointMode
     */ 
    class CAELUM_EXPORT ScopedHighPrecissionFloatSwitch
    {
    private:
        int mOldFpMode;

    public:
        inline ScopedHighPrecissionFloatSwitch() {
            mOldFpMode = Astronomy::enterHighPrecissionFloatingPointMode ();
        }

        inline ~ScopedHighPrecissionFloatSwitch() {
            Astronomy::restoreFloatingPointMode (mOldFpMode);
        }
    };
}

#endif // CAELUM__ASTRONOMY_H
