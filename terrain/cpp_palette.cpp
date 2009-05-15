/**
* @brief a class that holds a texture palette (basicly, an index accoicated with a texture)
*
* Unfortunaly, this uses a std::map class, which means that hasIndex is slow.
* A fix would be to use a class that has fast lookups for both key/value (both are keys).
* Or something better than that. Whatever.
*
* Yeah, so this is a bit like a std::map
*/
class TexturePalette {
public:
    inline bool hasTexture(int index) {
        return (mPalette.find(index) != mPalette.end());
    }

    /**
    * Not a great function. Very slow :(
    */
    inline bool hasIndex(const std::string& texture) {
        for ( Palette::iterator i = mPalette.begin(); i != mPalette.end(); ++i) {
            if ( i->second == texture )
                return true;
        }
        return false;
    }
    inline int getIndex(const std::string& texture) {
        for ( Palette::iterator i = mPalette.begin(); i != mPalette.end(); ++i) {
            if ( i->second == texture )
                return i->first;
        }
        return -1;
    }
    inline int getOrAddIndex(const std::string& texture) {
        if ( hasIndex(texture) )
            return getIndex(texture);
        return addTexture(texture);
    }

    inline const std::string& getTexture(int index) {
        return mPalette[index];
    }
    inline void setTexture(int index, std::string texture) {
        mPalette[index] = texture;
    }
    /**
    * @todo add proper error thing rather than assert(0)
    */
    inline int addTexture(const std::string& texture) {
        for ( int i = 0; i >= 0; i++ ) { //this loop is not infinate, as it will go to -2^31
            if ( mPalette.find(i) != mPalette.end() )
                continue;
            mPalette[i] = texture;
            return i;
        }
        assert(0); //this should never happen. Seeing as we can assign about 2^31 images
    }

    inline std::map<int, std::string>& getPalette() {
        return mPalette;
    }
private:
    typedef std::map<int, std::string> Palette;
    Palette mPalette;

    friend class boost::serialization::access;
    /**
    * @brief saves the palette
    */
    template<class Archive>
    inline void serialize(Archive& ar, const unsigned int version){
        ar &mPalette;
    }
};

BOOST_CLASS_TRACKING(TexturePalette, boost::serialization::track_never);
