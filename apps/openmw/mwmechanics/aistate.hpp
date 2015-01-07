#ifndef AISTATE_H
#define AISTATE_H

#include <typeinfo>
#include <stdexcept>

// c++11 replacement
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace MWMechanics
{

    /** \brief stores one object of any class derived from Base.
     *  Requesting a certain dereived class via get() either returns
     * the stored object if it has the correct type or otherwise replaces
     * it with an object of the requested type.
     */
    template< class Base >
    class DerivedClassStorage
    {              
    private:
        Base* mStorage;
        
        // assert that Derived is derived from Base. 
        template< class Derived >
        void assert_derived()
        {
            // c++11:
            // static_assert( std::is_base_of<Base,Derived> , "DerivedClassStorage may only store derived classes" );
            
            // boost:
            BOOST_STATIC_ASSERT((boost::is_base_of<Base,Derived>::value));//,"DerivedClassStorage may only store derived classes");
        }
        
        //if needed you have to provide a clone member function
        DerivedClassStorage( const DerivedClassStorage& other );
        DerivedClassStorage& operator=( const DerivedClassStorage& );
        
    public:
        /// \brief returns reference to stored object or deletes it and creates a fitting
        template< class Derived >
        Derived& get()
        {
            assert_derived<Derived>();
            
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
        void store( const Derived& payload )
        {
            assert_derived<Derived>();
            if(mStorage)
                delete mStorage;
            mStorage = new Derived(payload);
        }
        
        /// \brief takes ownership of the passed object
        template< class Derived >
        void moveIn( Derived* p )
        {
            assert_derived<Derived>();
            if(mStorage)
                delete mStorage;
            mStorage = p;
        }
        
        bool empty() const
        {
            return mStorage == NULL;
        }
        
        const std::type_info& getType() const
        {
            return typeid(mStorage);
        }
        
        
        DerivedClassStorage():mStorage(NULL){};
        ~DerivedClassStorage()
        {
            if(mStorage)
                delete mStorage;
        };
        
        
        
    };


    /// \brief base class for the temporary storage of AiPackages.
    /**
     * Each AI package with temporary values needs a AiPackageStorage class
     * which is derived from AiTemporaryBase. The Actor holds a container
     * AiState where one of these storages can be stored at a time.
     * The execute(...) member function takes this container as an argument.
     * */
    struct AiTemporaryBase
    {
        virtual ~AiTemporaryBase(){};
    };
    
    /// \brief Container for AI package status.
    typedef DerivedClassStorage<AiTemporaryBase> AiState;
}

#endif // AISTATE_H
