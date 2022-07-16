#ifndef AISTATE_H
#define AISTATE_H

#include "aistatefwd.hpp"
#include "aitemporarybase.hpp"

namespace MWMechanics
{

    /** \brief stores one object of any class derived from Base.
     *  Requesting a certain derived class via get() either returns
     * the stored object if it has the correct type or otherwise replaces
     * it with an object of the requested type.
     */
    template< class Base >
    class DerivedClassStorage
    {              
    private:
        Base* mStorage;
        
        //if needed you have to provide a clone member function
        DerivedClassStorage( const DerivedClassStorage& other );
        DerivedClassStorage& operator=( const DerivedClassStorage& );
        
    public:
        /// \brief returns reference to stored object or deletes it and creates a fitting
        template< class Derived >
        Derived& get()
        {
            Derived* result = dynamic_cast<Derived*>(mStorage);
            
            if(!result)
            {
                if(mStorage)
                    delete mStorage;
                mStorage = result = new Derived();
            }
            
            //return a reference to the (new allocated) object 
            return *result;
        }

        template< class Derived >
        void copy(DerivedClassStorage& destination) const
        {
            Derived* result = dynamic_cast<Derived*>(mStorage);
            if (result != nullptr)
                destination.store<Derived>(*result);
        }
        
        template< class Derived >
        void store( const Derived& payload )
        {
            if(mStorage)
                delete mStorage;
            mStorage = new Derived(payload);
        }
        
        /// \brief takes ownership of the passed object
        template< class Derived >
        void moveIn( Derived* p )
        {
            if(mStorage)
                delete mStorage;
            mStorage = p;
        }

        DerivedClassStorage():mStorage(nullptr){}
        ~DerivedClassStorage()
        {
            if(mStorage)
                delete mStorage;
        }
    };
}

#endif // AISTATE_H
