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

#ifndef CAELUM__POINT_STARFIELD_H
#define CAELUM__POINT_STARFIELD_H

#include "CaelumPrerequisites.h"
#include "CameraBoundElement.h"
#include "PrivatePtr.h"
#include "FastGpuParamRef.h"

namespace Caelum
{
    /** POD for bright star catalogue entries.
     *  Only J2000 position and magnitude included.
     */
    struct BrightStarCatalogueEntry {
        signed char rasc_hour;
        signed char rasc_min;
        float rasc_sec;
        signed char decl_deg;
        signed char decl_min;
        float decl_sec;
        float magn;
    };

    /// There are exactly 9110 stars in our version of the BSC.
    const int BrightStarCatalogueSize = 9110;

    /// Hardcoded bright star catalogue (BrightStarCatalogue.cpp)
    extern const BrightStarCatalogueEntry BrightStarCatalogue[BrightStarCatalogueSize];

    /** Point starfield class.
     *  An Ogre::ManualObject is used for drawing because billboards are too slow.
     *
     *  Stars are sized in pixels; this seems to work a lot better than relative sizes.
     *  Stars could be made even smaller but it would require hinting (nudging pixel
     *  coordinates to match actual screen pixels). Points are hard.
     * 
     *  Loading a bright-star catalogue is supported but star positions are
     *  likely only correct relative to each other. External rotation is probably wrong.
     */
    class CAELUM_EXPORT PointStarfield:
            public CameraBoundElement
    {
    public:
	    /** Constructor.
         *  By default this loads some reasonable defaults and the
         *  bright star catalogue.
         */
	    PointStarfield (
                Ogre::SceneManager *sceneMgr,
			    Ogre::SceneNode *caelumRootNode,
                bool initWithCatalogue = true);

	    /// Destructor.
	    virtual ~PointStarfield ();

        /// Struct representing one star inside PointStarfield.
        struct Star {
            Ogre::Degree RightAscension;
            Ogre::Degree Declination;
            Ogre::Real Magnitude;
        };

        /// A vector of Star
        typedef std::vector<Star> StarVector;

        /** Get a reference to the vector of stars.
         *  You can freely modify this; but you need to updateStars when you're done.
         */
        inline StarVector& getStarVector () { return mStars; }

        /** You must call this if you change the star vector by hand.
         */
        void notifyStarVectorChanged ();

        /// Clear any and all stars.
        void clearAllStars ();

        /** Add a bunch of random stars.
         */
        void addRandomStars (int count);

        /** Add one star from the bright star catalogue.
         */
        void addStar (const BrightStarCatalogueEntry &entry);

        /** Add stars from the bright star catalogue.
         *  @param count Number of stars to add (in order of brightness).
         */
        void addBrightStarCatalogue (int count = BrightStarCatalogueSize);

    private:
        /// Cloned material
        PrivateMaterialPtr mMaterial;

	    /// Node for the starfield
	    PrivateSceneNodePtr mNode;
        
        /// Manual object for drawing.
        PrivateManualObjectPtr mManualObj;

        /// Star data.
        std::vector<Star> mStars;

        Ogre::Real mMinPixelSize, mMaxPixelSize, mMag0PixelSize;
        Ogre::Real mMagnitudeScale;

        Ogre::Degree mObserverLatitude, mObserverLongitude;

        bool mValidGeometry;
		void invalidateGeometry();
		void ensureGeometry();

    public:
	    /** Update function; called from CaelumSystem::updateSubcomponents
            @param time Time of the day.
	     */
	    void _update (float time);

        /** Magnitude power scale.
         *  Star magnitudes are logarithming; one magnitude difference
         *  means a star is 2.512 times brighter.
         *  This property allows tweaking that value.
         */
        inline void setMagnitudeScale (Ogre::Real value) { mMagnitudeScale = value; }
        inline Ogre::Real getMagnitudeScale () const { return mMagnitudeScale; }

        inline void setMag0PixelSize (Ogre::Real value) { mMag0PixelSize = value; }
        inline Ogre::Real getMag0PixelSize () const { return mMag0PixelSize; }

        inline void setMinPixelSize (Ogre::Real value) { mMinPixelSize = value; }
        inline Ogre::Real getMinPixelSize () const { return mMinPixelSize; }

        inline void setMaxPixelSize (Ogre::Real value) { mMaxPixelSize = value; }
        inline Ogre::Real getMaxPixelSize () const { return mMaxPixelSize; }

        void setObserverLatitude (Ogre::Degree value);
        inline Ogre::Degree getObserverLatitude () const { return mObserverLatitude; }

        void setObserverLongitude (Ogre::Degree value);
        inline Ogre::Degree getObserverLongitude () const { return mObserverLongitude; }

    private:
        Ogre::Degree mObserverPositionRebuildDelta;

    public:
        /** Moving the observer position around causes a starfield rebuild.
         *  Default value (DEFAULT_OBSERVER_POSITION_REBUILD_DELTA) is 0.1
         *  degrees which is equivalent to around 170 meters on the earth.
         *
         *  This only matters if you compute the observer position every
         *  frame. Caelum doesn't contain code for that.
         */
        inline Ogre::Degree getObserverPositionRebuildDelta () const {
            return mObserverPositionRebuildDelta;
        }
        inline void setObserverPositionRebuildDelta (Ogre::Degree value) {
            mObserverPositionRebuildDelta = value;
        }

	    static const Ogre::Degree DEFAULT_OBSERVER_POSITION_REBUILD_DELTA;

        /// Material used to draw all the points.
	    static const Ogre::String STARFIELD_MATERIAL_NAME;

	    /// Handle camera change.
	    virtual void notifyCameraChanged (Ogre::Camera *cam);

    protected:
        /// Handle far radius.
        virtual void setFarRadius (Ogre::Real radius);

    public:
        void setQueryFlags (uint flags) { mManualObj->setQueryFlags (flags); }
        uint getQueryFlags () const { return mManualObj->getQueryFlags (); }
        void setVisibilityFlags (uint flags) { mManualObj->setVisibilityFlags (flags); }
        uint getVisibilityFlags () const { return mManualObj->getVisibilityFlags (); }

    private:
        struct Params {
            void setup(Ogre::GpuProgramParametersSharedPtr vpParams);
        
            Ogre::GpuProgramParametersSharedPtr vpParams;
            FastGpuParamRef mag_scale;
            FastGpuParamRef mag0_size;
            FastGpuParamRef min_size;
            FastGpuParamRef max_size;
            FastGpuParamRef aspect_ratio;
        } mParams;
    };
}

#endif // CAELUM__POINT_STARFIELD_H
