#ifndef GAME_MWMECHANICS_AISEQUENCE_H
#define GAME_MWMECHANICS_AISEQUENCE_H

#include <list>

#include <components/esm/loadnpc.hpp>

namespace MWWorld
{
    class Ptr;
}

namespace MWMechanics
{
    class AiPackage;
    
    /// \brief Sequence of AI-packages for a single actor
    class AiSequence
    {
            std::list<AiPackage *> mPackages;

            bool mDone;

            void copy (const AiSequence& sequence);

            // The type of AI package that ran last
            int mLastAiPackage;

        public:
        
            AiSequence();
            
            AiSequence (const AiSequence& sequence);
            
            AiSequence& operator= (const AiSequence& sequence);
            
            virtual ~AiSequence();

            int getTypeId() const;
            ///< @see enum AiPackage::TypeId

            int getLastRunTypeId() const { return mLastAiPackage; }
            ///< Get the typeid of the Ai package that ran last, NOT the currently "active" Ai package that will be run in the next frame.
            /// This difference is important when an Ai package has just finished and been removed.

            bool getCombatTarget (std::string &targetActorId) const;
            ///< Return true and assign target if combat package is currently
            /// active, return false otherwise

            void stopCombat();
            ///< Removes all combat packages until first non-combat or stack empty.

            void stopPersue();
            ///< Removes all persue packages until first non-persue or stack empty.
            
            bool isPackageDone() const;
            ///< Has a package been completed during the last update?
            
            void execute (const MWWorld::Ptr& actor,float duration);
            ///< Execute package.
            
            void clear();
            ///< Remove all packages.

            void stack (const AiPackage& package);
            ///< Add \a package to the front of the sequence (suspends current package)
            
            void queue (const AiPackage& package);
            ///< Add \a package to the end of the sequence (executed after all other packages have been
            /// completed)

            AiPackage* getActivePackage();
            ///< return the current active package. If there is no active package, throw an exeption

            void fill (const ESM::AIPackageList& list);
    };
}

#endif
