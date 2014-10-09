#ifndef AISTATE_H
#define AISTATE_H

#include <typeinfo>
#include <stdexcept>

// c++11 replacement
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>


/// \brief stores an object of any class derived from Base
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
        BOOST_STATIC_ASSERT(boost::is_base_of<Base,Derived>::value);//,"DerivedClassStorage may only store derived classes");
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
    
    /// \brief gives away ownership of object. Throws exception if storage does not contain Derived or is empty.
    template< class Derived >
    Derived* moveOut()
    {
        assert_derived<Derived>();
        
        
        if(!mStorage)
            throw std::runtime_error("Cant move out: empty storage.");
        
        Derived* result = dynamic_cast<Derived*>(mStorage);
        
        if(!mStorage)
            throw std::runtime_error("Cant move out: wrong type requested.");
        
        return result;
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

namespace MWMechanics
{
    /// \brief base class for the temporary storage of AiPackages.
    /**
     * Each AI package with temporary values needs a AiPackageStorage class
     * which is derived from AiTemporaryBase. The CharacterController holds a container
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
