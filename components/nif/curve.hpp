#ifndef _NIF_KEYLIST_H_
#define _NIF_KEYLIST_H_

#include <cfloat>

namespace Nif
{

template <typename iterator , typename predicate>
void bubble_sort (iterator begin, iterator end, predicate const & in_order)
{
    if (end > begin)
    {
        for (iterator i = begin; i != end - 1; ++i)
        {
            if (in_order (*(i+0), *(i+1)))
                continue;

            for (iterator j = i; j >= begin; --j)
            {
                std::swap (*(j+0), *(j+1));

                if (in_order (*(j+0), *(j+1)))
                    break;
            }
        }
    }
}

template <typename value_type>
value_type linear_interpolate (float amount, value_type prev, value_type next)
{
    return prev + (next - prev) * amount;
}

inline
Ogre::Quaternion linear_interpolate (float amount, Ogre::Quaternion prev, Ogre::Quaternion next)
{
    return Ogre::Quaternion::nlerp (amount, prev, next);
}

template<typename value_type>
struct KeyT {

    static const size_t EncodedLength =
        NIFStream::handler <float>::EncodedLength +
        NIFStream::handler <value_type>::EncodedLength
        ;

    float mTime;
    value_type mValue;

    void extract (NIFStream &nif)
    {
        nif.uncheckedRead (mTime);
        nif.uncheckedRead (mValue);
    }

    static bool in_order (KeyT <value_type> const & l, KeyT <value_type> const & r)
    {
        return l.mTime < r.mTime;
    }

    template <typename derived_type>
    struct NIFStream_handler
    {
        static const bool FixedLength = true;
        static const size_t EncodedLength = derived_type::EncodedLength;
        static const bool FileCompatibleLayout = true;

        static void extract (NIFStream& Stream, KeyT <value_type> & Value)
        {
            static_cast <derived_type &> (Value).extract (Stream);
        }
    };
};

template <typename T>
struct LinearKeyT : KeyT <T>
{
    static T interpolate (LinearKeyT <T> * prev, LinearKeyT <T> * next, float amount)
    {
        return linear_interpolate (amount, prev->mValue, next->mValue);
    }
};

template <typename T>
struct QuadraticKeyT : KeyT <T>
{
    static const size_t EncodedLength =
        KeyT <T>::EncodedLength +
        NIFStream::handler <T>::EncodedLength * 2
        ;

    T mForwardValue;
    T mBackwardValue;

    static T interpolate (QuadraticKeyT <T> * prev, QuadraticKeyT <T> * next, float amount)
    {
        return linear_interpolate (amount, prev->mValue, next->mValue);
    }

    void extract (NIFStream &nif)
    {
        KeyT<T>::extract (nif);

        nif.uncheckedRead (mForwardValue);
        nif.uncheckedRead (mBackwardValue);
    }
};

template <typename T>
struct TbcKeyT : KeyT <T>
{
    static const size_t EncodedLength =
        KeyT <T>::EncodedLength +
        NIFStream::handler <float>::EncodedLength * 3
        ;

    float mTension;
    float mBias;
    float mContinuity;

    static T interpolate (TbcKeyT <T> * prev, TbcKeyT <T> * next, float amount)
    {
        return linear_interpolate (amount, prev->mValue, next->mValue);
    }

    void extract (NIFStream &nif)
    {
        KeyT<T>::extract (nif);

        nif.uncheckedRead (mTension);
        nif.uncheckedRead (mBias);
        nif.uncheckedRead (mContinuity);
    }
};

// register NIFStream extraction handlers for KeyT derivatives
template <typename T> struct NIFStream::handler < LinearKeyT    <T> > : KeyT <T>::template NIFStream_handler < LinearKeyT    <T> > {};
template <typename T> struct NIFStream::handler < QuadraticKeyT <T> > : KeyT <T>::template NIFStream_handler < QuadraticKeyT <T> > {};
template <typename T> struct NIFStream::handler < TbcKeyT       <T> > : KeyT <T>::template NIFStream_handler < TbcKeyT       <T> > {};

struct Curve
{
    static const int sLinearInterpolation    = 1;
    static const int sQuadraticInterpolation = 2;
    static const int sTBCInterpolation       = 3;
};

template<typename value_type>
struct CurveT : Curve {

    typedef KeyT <value_type>          BaseKey;
    typedef TbcKeyT <value_type>       TcbKey;
    typedef LinearKeyT <value_type>    LinearKey;
    typedef QuadraticKeyT <value_type> QuadraticKey;

    union keys {
        LinearKey*    Linear;
        QuadraticKey* Quadratic;
        TcbKey*       Tcb;
    };

    class interpolator;

    int mInterpolationType;
    size_t  mSize;
    keys    mKeys;

    value_type sample (float time) const;

    KeyT <value_type> const * const & keyAtIndex (size_t Index) const
    {
        switch (mInterpolationType)
        {
        case sLinearInterpolation:    return mKeys.Linear    + Index;
        case sQuadraticInterpolation: return mKeys.Quadratic + Index;
        case sTBCInterpolation:       return mKeys.Tcb       + Index;
        }
    }

    void read(NIFStream *nif, bool force=false)
    {
        size_t count = nif->getInt();

        mSize = 0;

        if(count > 0 || force)
        {
            mInterpolationType = nif->getInt();

            assert (mInterpolationType >= sLinearInterpolation && mInterpolationType <= sTBCInterpolation);

            if (count > 0)
            {
                if(mInterpolationType == sLinearInterpolation)
                    read_keys (nif, mKeys.Linear, count);
                else if(mInterpolationType == sQuadraticInterpolation)
                    read_keys (nif, mKeys.Quadratic, count);
                else if(mInterpolationType == sTBCInterpolation)
                    read_keys (nif, mKeys.Tcb, count);
                else
                    nif->file->warn("Unhandled interpolation type: "+Ogre::StringConverter::toString(mInterpolationType));
            }
        }
        else
            mInterpolationType = sLinearInterpolation;
    }

    CurveT () { init (); }
    CurveT (CurveT <value_type> const & k) { init (k); }
    //CurveT (CurveT <value_type> && k) { init (); swap (std::move (k)); }
    ~CurveT () { dest (); }

    operator bool () const { return mSize > 0; }

    //void operator = (CurveT<value_type> && k) { swap(k); }
    void operator = (CurveT<value_type> const & k) { dest (); init (k); }

    void swap (CurveT<value_type> & k)
    {
        std::swap (mSize, k.mSize);
        std::swap (mInterpolationType, k.mInterpolationType);
        std::swap (mKeys, k.mKeys);
    }

private:

    void init ()
    {
        mSize = 0;
    }

    void init (CurveT<value_type> const & k)
    {
        mInterpolationType = k.mInterpolationType;
        switch (mInterpolationType)
        {
        default:
        case sLinearInterpolation:
            mKeys.Linear = new LinearKey [k.mSize];
            memcpy (mKeys.Linear, k.mKeys.Linear, sizeof (LinearKey) * k.mSize);
            mSize = k.mSize;
            break;
        case sQuadraticInterpolation:
            mKeys.Quadratic = new QuadraticKey [k.mSize];
            memcpy (mKeys.Quadratic, k.mKeys.Quadratic, sizeof (QuadraticKey) * k.mSize);
            mSize = k.mSize;
            break;
        case sTBCInterpolation:
            mKeys.Tcb = new TcbKey [k.mSize];
            memcpy (mKeys.Tcb, k.mKeys.Tcb, sizeof (TcbKey) * k.mSize);
            mSize = k.mSize;
            break;
        }
    }

    void dest ()
    {
        if (mSize > 0)
        {
            switch (mInterpolationType)
            {
            case sLinearInterpolation:      delete mKeys.Linear;    break;
            case sQuadraticInterpolation:   delete mKeys.Quadratic; break;
            case sTBCInterpolation:         delete mKeys.Tcb;       break;
            }
        }
    }

    template <typename T>
    void read_keys (NIFStream *nif, T * & store, size_t count)
    {
        store = new T [count];

        mSize = count;

        nif->getArray (store, count);

        //NOTE: Is this really necessary? It seems reasonable to assume that
        //      animation data is already sorted by time...
        //      verified no out of order frames in GOTY edition
        bubble_sort (store, store+count, T::in_order);
    }
};

template<typename value_type>
class CurveT<value_type>::interpolator
{
    template <typename key_type>
    struct impl
    {
        key_type *Cur, *End;

        void init (key_type * Beg, size_t Len)
        {
            if (Len > 0)
            {
                Cur = Beg;
                End = Beg + Len - 1;
            }
            else
            {
                Cur = End = NULL;
            }
        }

        bool hasData () const
        {
            return Cur && Cur <= End;
        }

        value_type valueAt (float time)
        {
            while ((Cur < End) && (time >= Cur [1].mTime))
                ++Cur;

            if (Cur < End)
            {
                if (time > Cur->mTime)
                {
                    key_type * Nxt = Cur + 1;

                    float offset = time - Cur->mTime;
                    float length = Nxt->mTime - Cur->mTime;

                    return key_type::interpolate (Cur, Nxt, offset / length);
                }
                else
                    return Cur->mValue;
            }
            else
                return End->mValue;
        }

        float curTime () const
        {
            return (Cur != NULL) ? Cur->Time : FLT_MIN;
        }

        float nextTime () const
        {
            return Cur < End ? (Cur + 1)->mTime : FLT_MAX;
        }
    };

public:

    int mInterpolationType;
    union {
        impl <LinearKey>    Linear;
        impl <QuadraticKey> Quadratic;
        impl <TcbKey>       Tcb;
    };

    interpolator (CurveT <value_type> const & Curve)
    {
        mInterpolationType = Curve.mInterpolationType;

        switch (mInterpolationType)
        {
        default:
        case Curve::sLinearInterpolation:    Linear   .init (Curve.mKeys.Linear,    Curve.mSize); break;
        case Curve::sQuadraticInterpolation: Quadratic.init (Curve.mKeys.Quadratic, Curve.mSize); break;
        case Curve::sTBCInterpolation:       Tcb      .init (Curve.mKeys.Tcb,       Curve.mSize); break;
        }
    }

    // return true if there is any value(s) in this curve
    float hasData () const
    {
        switch (mInterpolationType)
        {
        default:
        case Curve::sLinearInterpolation:    return Linear   .hasData ();
        case Curve::sQuadraticInterpolation: return Quadratic.hasData ();
        case Curve::sTBCInterpolation:       return Tcb      .hasData ();
        }
    }

    // return the timestamp of the next key-frame, or FLT_MAX if
    // there are no more key-frames, valid if hasData returns false
    float nextTime () const
    {
        switch (mInterpolationType)
        {
        default:
        case Curve::sLinearInterpolation:    return Linear   .nextTime ();
        case Curve::sQuadraticInterpolation: return Quadratic.nextTime ();
        case Curve::sTBCInterpolation:       return Tcb      .nextTime ();
        }
    }

    // return the value of the curve at the specified time
    // the passed in time should never exceed the result of
    // nextTime, not valid if hasData returns false
    value_type valueAt (float time)
    {
        switch (mInterpolationType)
        {
        default:
        case Curve::sLinearInterpolation:    return Linear   .valueAt (time);
        case Curve::sQuadraticInterpolation: return Quadratic.valueAt (time);
        case Curve::sTBCInterpolation:       return Tcb      .valueAt (time);
        }
    }
};

template<typename value_type>
value_type CurveT<value_type>::sample (float time) const
{
    interpolator i (*this);
    return i.valueAt (time);
}

typedef CurveT<float> FloatCurve;
typedef CurveT<Ogre::Vector3> Vector3Curve;
typedef CurveT<Ogre::Vector4> Vector4Curve;
typedef CurveT<Ogre::Quaternion> QuaternionCurve;

}

#endif
