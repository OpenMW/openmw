// Modified from OpenGUI under lenient license
// Original copyright details and licensing below:
// OpenGUI (http://opengui.sourceforge.net)
// This source code is released under the BSD License

// Permission is given to the Ogre project to use the contents of file within its
// source and binary applications, as well as any derivative works, in accordance
// with the terms of any license under which Ogre is or will be distributed.
//
// Ogre may relicense its copy of this file, as well as any OpenGUI released updates
// to this file, under any terms that it deems fit, and is not required to maintain
// the original BSD licensing terms of this file, however OpenGUI retains the right
// to present its copy of this file under the terms of any license under which
// OpenGUI is distributed.
//
// Ogre is not required to release to OpenGUI any future changes that it makes to
// this file, and understands and agrees that any such changes that are released
// back to OpenGUI will become available under the terms of any license under which
// OpenGUI is distributed.
//
// For brevity, this permission text may be removed from this file if desired.
// The original record kept within the SourceForge (http://sourceforge.net/) tracker
// is sufficient.
//
// - Eric Shorkey (zero/zeroskill) <opengui@rightbracket.com> [January 20th, 2007]

#ifndef __MYGUI_U_STRING_H__
#define __MYGUI_U_STRING_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_Diagnostic.h"
#include "MyGUI_LogManager.h"

// these are explained later
#include <iterator>
#include <string>
#include <stdexcept>

// this pragma used to avoid warnings from some advanced gcc warnings flags
#if MYGUI_COMPILER == MYGUI_COMPILER_GNUC
#pragma GCC system_header
#endif

// Workaround for VC7:
//      when build with /MD or /MDd, VC7 have both std::basic_string<unsigned short> and
// basic_string<__wchar_t> instantiated in msvcprt[d].lib/MSVCP71[D].dll, but the header
// files tells compiler that only one of them is over there (based on /Zc:wchar_t compile
// option). And since this file used both of them, causing compiler instantiating another
// one in user object code, which lead to duplicate symbols with msvcprt.lib/MSVCP71[D].dll.
//
#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC && (1300 <= MYGUI_COMP_VER && MYGUI_COMP_VER <= 1310)

# if defined(_DLL_CPPLIB)

namespace std
{
    template class _CRTIMP2 basic_string<unsigned short, char_traits<unsigned short>,
	    allocator<unsigned short> >;

    template class _CRTIMP2 basic_string<__wchar_t, char_traits<__wchar_t>,
	    allocator<__wchar_t> >;
}

# endif // defined(_DLL_CPPLIB)

#endif  // MYGUI_COMPILER == MYGUI_COMPILER_MSVC && MYGUI_COMP_VER == 1300


namespace MyGUI
{

	/* READ THIS NOTICE BEFORE USING IN YOUR OWN APPLICATIONS
	=NOTICE=
	This class is not a complete Unicode solution. It purposefully does not
	provide certain functionality, such as proper lexical sorting for
	Unicode values. It does provide comparison operators for the sole purpose
	of using UString as an index with std::map and other operator< sorted
	containers, but it should NOT be relied upon for meaningful lexical
	operations, such as alphabetical sorts. If you need this type of
	functionality, look into using ICU instead (http://icu.sourceforge.net/).

	=REQUIREMENTS=
	There are a few requirements for proper operation. They are fairly small,
	and shouldn't restrict usage on any reasonable target.
	* Compiler must support unsigned 16-bit integer types
	* Compiler must support signed 32-bit integer types
	* wchar_t must be either UTF-16 or UTF-32 encoding, and specified as such
	    using the WCHAR_UTF16 macro as outlined below.
	* You must include <iterator>, <string>, and <wchar>. Probably more, but
	    these are the most obvious.

	=REQUIRED PREPROCESSOR MACROS=
	This class requires two preprocessor macros to be defined in order to
	work as advertised.
	INT32 - must be mapped to a signed 32 bit integer (ex. #define INT32 int)
	UINT16 - must be mapped to an unsigned 16 bit integer (ex. #define UINT32 unsigned short)

	Additionally, a third macro should be defined to control the evaluation of wchar_t:
	WCHAR_UTF16 - should be defined when wchar_t represents UTF-16 code points,
	    such as in Windows. Otherwise it is assumed that wchar_t is a 32-bit
		integer representing UTF-32 code points.
	*/

	// THIS IS A VERY BRIEF AUTO DETECTION. YOU MAY NEED TO TWEAK THIS
#ifdef __STDC_ISO_10646__
// for any compiler that provides this, wchar_t is guaranteed to hold any Unicode value with a single code point (32-bit or larger)
// so we can safely skip the rest of the testing
#else // #ifdef __STDC_ISO_10646__
#if defined( __WIN32__ ) || defined( _WIN32 )
#define WCHAR_UTF16 // All currently known Windows platforms utilize UTF-16 encoding in wchar_t
#else // #if defined( __WIN32__ ) || defined( _WIN32 )
#if WCHAR_MAX <= 0xFFFF // this is a last resort fall back test; WCHAR_MAX is defined in <wchar.h>
#define WCHAR_UTF16 // best we can tell, wchar_t is not larger than 16-bit
#endif // #if WCHAR_MAX <= 0xFFFF
#endif // #if defined( __WIN32__ ) || defined( _WIN32 )
#endif // #ifdef __STDC_ISO_10646__


// MYGUI_IS_NATIVE_WCHAR_T means that wchar_t isn't a typedef of
// uint16 or uint32.
#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC

// Don't define wchar_t related functions since it'll duplicate
// with UString::code_point related functions when compile
// without /Zc:wchar_t, because in this case both of them are
// a typedef of uint16.
# if defined(_NATIVE_WCHAR_T_DEFINED)
#   define MYGUI_IS_NATIVE_WCHAR_T      1
# else
#   define MYGUI_IS_NATIVE_WCHAR_T      0
# endif

#else   // MYGUI_COMPILER != MYGUI_COMPILER_MSVC

// Assumed wchar_t is natively for other compilers
#   define MYGUI_IS_NATIVE_WCHAR_T     1

#endif  // MYGUI_COMPILER == MYGUI_COMPILER_MSVC

	//! A UTF-16 string with implicit conversion to/from std::string and std::wstring
	/*! This class provides a complete 1 to 1 map of most std::string functions (at least to my
	knowledge). Implicit conversions allow this string class to work with all common C++ string
	formats, with specialty functions defined where implicit conversion would cause potential
	problems or is otherwise unavailable.

	Some additional functionality is present to assist in working with characters using the
	32-bit UTF-32 encoding. (Which is guaranteed to fit any Unicode character into a single
	code point.) \b Note: Reverse iterators do not have this functionality due to the
	ambiguity that surrounds working with UTF-16 in reverse. (Such as, where should an
	iterator point to represent the beginning of a surrogate pair?)


	\par Supported Input Types
	The supported string types for input, and their assumed encoding schemes, are:
	- std::string (UTF-8)
	- char* (UTF-8)
	- std::wstring (autodetected UTF-16 / UTF-32 based on compiler)
	- wchar_t* (autodetected UTF-16 / UTF-32 based on compiler)


	\see
	- For additional information on UTF-16 encoding: http://en.wikipedia.org/wiki/UTF-16
	- For additional information on UTF-8 encoding: http://en.wikipedia.org/wiki/UTF-8
	- For additional information on UTF-32 encoding: http://en.wikipedia.org/wiki/UTF-32
	*/
	class UString
	{
		// constants used in UTF-8 conversions
		static const unsigned char _lead1 = 0xC0;      //110xxxxx
		static const unsigned char _lead1_mask = 0x1F; //00011111
		static const unsigned char _lead2 = 0xE0;      //1110xxxx
		static const unsigned char _lead2_mask = 0x0F; //00001111
		static const unsigned char _lead3 = 0xF0;      //11110xxx
		static const unsigned char _lead3_mask = 0x07; //00000111
		static const unsigned char _lead4 = 0xF8;      //111110xx
		static const unsigned char _lead4_mask = 0x03; //00000011
		static const unsigned char _lead5 = 0xFC;      //1111110x
		static const unsigned char _lead5_mask = 0x01; //00000001
		static const unsigned char _cont = 0x80;       //10xxxxxx
		static const unsigned char _cont_mask = 0x3F;  //00111111

	public:
		//! size type used to indicate string size and character positions within the string
		typedef size_t size_type;
		//! the usual constant representing: not found, no limit, etc
		static const size_type npos = ~(size_t)0;

		//! a single 32-bit Unicode character
		typedef uint32 unicode_char;

		//! a single UTF-16 code point
		typedef uint16 code_point;

		//! value type typedef for use in iterators
		typedef code_point value_type;

		typedef std::basic_string<code_point> dstring; // data string

		//! string type used for returning UTF-32 formatted data
		typedef std::basic_string<unicode_char> utf32string;

		//! This exception is used when invalid data streams are encountered
		class invalid_data: public std::runtime_error
		{ /* i don't know why the beautifier is freaking out on this line */
		public:
			//! constructor takes a string message that can be later retrieved by the what() function
			explicit invalid_data( const std::string& _Message ): std::runtime_error( _Message )
			{
				/* The thing is, Bob, it's not that I'm lazy, it's that I just don't care. */
			}
		};

		//#########################################################################
		//! base iterator class for UString
		class _base_iterator: public std::iterator<std::random_access_iterator_tag, value_type>
		{ /* i don't know why the beautifier is freaking out on this line */
			friend class UString;
		protected:
			_base_iterator()
			{
				mString = 0;
			}

			void _seekFwd( size_type c )
			{
				mIter += c;
			}
			void _seekRev( size_type c )
			{
				mIter -= c;
			}
			void _become( const _base_iterator& i )
			{
				mIter = i.mIter;
				mString = i.mString;
			}
			bool _test_begin() const
			{
				return mIter == mString->mData.begin();
			}
			bool _test_end() const
			{
				return mIter == mString->mData.end();
			}
			size_type _get_index() const
			{
				return mIter - mString->mData.begin();
			}
			void _jump_to( size_type index )
			{
				mIter = mString->mData.begin() + index;
			}

			unicode_char _getCharacter() const
			{
				size_type current_index = _get_index();
				return mString->getChar( current_index );
			}
			int _setCharacter( unicode_char uc )
			{
				size_type current_index = _get_index();
				int change = mString->setChar( current_index, uc );
				_jump_to( current_index );
				return change;
			}

			void _moveNext()
			{
				_seekFwd( 1 ); // move 1 code point forward
				if ( _test_end() ) return; // exit if we hit the end
				if ( _utf16_surrogate_follow( mIter[0] ) )
				{
					// landing on a follow code point means we might be part of a bigger character
					// so we test for that
					code_point lead_half = 0;
					//NB: we can't possibly be at the beginning here, so no need to test
					lead_half = mIter[-1]; // check the previous code point to see if we're part of a surrogate pair
					if ( _utf16_surrogate_lead( lead_half ) )
					{
						_seekFwd( 1 ); // if so, then advance 1 more code point
					}
				}
			}
			void _movePrev()
			{
				_seekRev( 1 ); // move 1 code point backwards
				if ( _test_begin() ) return; // exit if we hit the beginning
				if ( _utf16_surrogate_follow( mIter[0] ) )
				{
					// landing on a follow code point means we might be part of a bigger character
					// so we test for that
					code_point lead_half = 0;
					lead_half = mIter[-1]; // check the previous character to see if we're part of a surrogate pair
					if ( _utf16_surrogate_lead( lead_half ) )
					{
						_seekRev( 1 ); // if so, then rewind 1 more code point
					}
				}
			}

			dstring::iterator mIter;
			UString* mString;
		};

		//#########################################################################
		// FORWARD ITERATORS
		//#########################################################################
		class _const_fwd_iterator; // forward declaration

		//! forward iterator for UString
		class _fwd_iterator: public _base_iterator
		{ /* i don't know why the beautifier is freaking out on this line */
			friend class _const_fwd_iterator;
		public:
			_fwd_iterator() { }
			_fwd_iterator( const _fwd_iterator& i )
			{
				_become( i );
			}

			//! pre-increment
			_fwd_iterator& operator++()
			{
				_seekFwd( 1 );
				return *this;
			}
			//! post-increment
			_fwd_iterator operator++( int )
			{
				_fwd_iterator tmp( *this );
				_seekFwd( 1 );
				return tmp;
			}

			//! pre-decrement
			_fwd_iterator& operator--()
			{
				_seekRev( 1 );
				return *this;
			}
			//! post-decrement
			_fwd_iterator operator--( int )
			{
				_fwd_iterator tmp( *this );
				_seekRev( 1 );
				return tmp;
			}

			//! addition operator
			_fwd_iterator operator+( size_type n )
			{
				_fwd_iterator tmp( *this );
				tmp._seekFwd( n );
				return tmp;
			}
			//! addition operator
			_fwd_iterator operator+( difference_type n )
			{
				_fwd_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekRev( -n );
				else
					tmp._seekFwd( n );
				return tmp;
			}
			//! subtraction operator
			_fwd_iterator operator-( size_type n )
			{
				_fwd_iterator tmp( *this );
				tmp._seekRev( n );
				return tmp;
			}
			//! subtraction operator
			_fwd_iterator operator-( difference_type n )
			{
				_fwd_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekFwd( -n );
				else
					tmp._seekRev( n );
				return tmp;
			}

			//! addition assignment operator
			_fwd_iterator& operator+=( size_type n )
			{
				_seekFwd( n );
				return *this;
			}
			//! addition assignment operator
			_fwd_iterator& operator+=( difference_type n )
			{
				if ( n < 0 )
					_seekRev( -n );
				else
					_seekFwd( n );
				return *this;
			}
			//! subtraction assignment operator
			_fwd_iterator& operator-=( size_type n )
			{
				_seekRev( n );
				return *this;
			}
			//! subtraction assignment operator
			_fwd_iterator& operator-=( difference_type n )
			{
				if ( n < 0 )
					_seekFwd( -n );
				else
					_seekRev( n );
				return *this;
			}

			//! dereference operator
			value_type& operator*() const
			{
				return *mIter;
			}

			//! dereference at offset operator
			value_type& operator[]( size_type n ) const
			{
				_fwd_iterator tmp( *this );
				tmp += n;
				return *tmp;
			}
			//! dereference at offset operator
			value_type& operator[]( difference_type n ) const
			{
				_fwd_iterator tmp( *this );
				tmp += n;
				return *tmp;
			}

			//! advances to the next Unicode character, honoring surrogate pairs in the UTF-16 stream
			_fwd_iterator& moveNext()
			{
				_moveNext();
				return *this;
			}
			//! rewinds to the previous Unicode character, honoring surrogate pairs in the UTF-16 stream
			_fwd_iterator& movePrev()
			{
				_movePrev();
				return *this;
			}
			//! Returns the Unicode value of the character at the current position (decodes surrogate pairs if needed)
			unicode_char getCharacter() const
			{
				return _getCharacter();
			}
			//! Sets the Unicode value of the character at the current position (adding a surrogate pair if needed); returns the amount of string length change caused by the operation
			int setCharacter( unicode_char uc )
			{
				return _setCharacter( uc );
			}
		};


		//#########################################################################
		//! const forward iterator for UString
		class _const_fwd_iterator: public _base_iterator
		{ /* i don't know why the beautifier is freaking out on this line */
		public:
			_const_fwd_iterator() { }
			_const_fwd_iterator( const _const_fwd_iterator& i )
			{
				_become( i );
			}
			_const_fwd_iterator( const _fwd_iterator& i )
			{
				_become( i );
			}

			//! pre-increment
			_const_fwd_iterator& operator++()
			{
				_seekFwd( 1 );
				return *this;
			}
			//! post-increment
			_const_fwd_iterator operator++( int )
			{
				_const_fwd_iterator tmp( *this );
				_seekFwd( 1 );
				return tmp;
			}

			//! pre-decrement
			_const_fwd_iterator& operator--()
			{
				_seekRev( 1 );
				return *this;
			}
			//! post-decrement
			_const_fwd_iterator operator--( int )
			{
				_const_fwd_iterator tmp( *this );
				_seekRev( 1 );
				return tmp;
			}

			//! addition operator
			_const_fwd_iterator operator+( size_type n )
			{
				_const_fwd_iterator tmp( *this );
				tmp._seekFwd( n );
				return tmp;
			}
			//! addition operator
			_const_fwd_iterator operator+( difference_type n )
			{
				_const_fwd_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekRev( -n );
				else
					tmp._seekFwd( n );
				return tmp;
			}
			//! subtraction operator
			_const_fwd_iterator operator-( size_type n )
			{
				_const_fwd_iterator tmp( *this );
				tmp._seekRev( n );
				return tmp;
			}
			//! subtraction operator
			_const_fwd_iterator operator-( difference_type n )
			{
				_const_fwd_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekFwd( -n );
				else
					tmp._seekRev( n );
				return tmp;
			}

			//! addition assignment operator
			_const_fwd_iterator& operator+=( size_type n )
			{
				_seekFwd( n );
				return *this;
			}
			//! addition assignment operator
			_const_fwd_iterator& operator+=( difference_type n )
			{
				if ( n < 0 )
					_seekRev( -n );
				else
					_seekFwd( n );
				return *this;
			}
			//! subtraction assignment operator
			_const_fwd_iterator& operator-=( size_type n )
			{
				_seekRev( n );
				return *this;
			}
			//! subtraction assignment operator
			_const_fwd_iterator& operator-=( difference_type n )
			{
				if ( n < 0 )
					_seekFwd( -n );
				else
					_seekRev( n );
				return *this;
			}

			//! dereference operator
			const value_type& operator*() const
			{
				return *mIter;
			}

			//! dereference at offset operator
			const value_type& operator[]( size_type n ) const
			{
				_const_fwd_iterator tmp( *this );
				tmp += n;
				return *tmp;
			}
			//! dereference at offset operator
			const value_type& operator[]( difference_type n ) const
			{
				_const_fwd_iterator tmp( *this );
				tmp += n;
				return *tmp;
			}

			//! advances to the next Unicode character, honoring surrogate pairs in the UTF-16 stream
			_const_fwd_iterator& moveNext()
			{
				_moveNext();
				return *this;
			}
			//! rewinds to the previous Unicode character, honoring surrogate pairs in the UTF-16 stream
			_const_fwd_iterator& movePrev()
			{
				_movePrev();
				return *this;
			}
			//! Returns the Unicode value of the character at the current position (decodes surrogate pairs if needed)
			unicode_char getCharacter() const
			{
				return _getCharacter();
			}

			//! difference operator
			friend size_type operator-( const _const_fwd_iterator& left, const _const_fwd_iterator& right );
			//! equality operator
			friend bool operator==( const _const_fwd_iterator& left, const _const_fwd_iterator& right );
			//! inequality operator
			friend bool operator!=( const _const_fwd_iterator& left, const _const_fwd_iterator& right );
			//! less than
			friend bool operator<( const _const_fwd_iterator& left, const _const_fwd_iterator& right );
			//! less than or equal
			friend bool operator<=( const _const_fwd_iterator& left, const _const_fwd_iterator& right );
			//! greater than
			friend bool operator>( const _const_fwd_iterator& left, const _const_fwd_iterator& right );
			//! greater than or equal
			friend bool operator>=( const _const_fwd_iterator& left, const _const_fwd_iterator& right );

		};

		//#########################################################################
		// REVERSE ITERATORS
		//#########################################################################
		class _const_rev_iterator; // forward declaration
		//! forward iterator for UString
		class _rev_iterator: public _base_iterator
		{ /* i don't know why the beautifier is freaking out on this line */
			friend class _const_rev_iterator;
		public:
			_rev_iterator() { }
			_rev_iterator( const _rev_iterator& i )
			{
				_become( i );
			}

			//! pre-increment
			_rev_iterator& operator++()
			{
				_seekRev( 1 );
				return *this;
			}
			//! post-increment
			_rev_iterator operator++( int )
			{
				_rev_iterator tmp( *this );
				_seekRev( 1 );
				return tmp;
			}

			//! pre-decrement
			_rev_iterator& operator--()
			{
				_seekFwd( 1 );
				return *this;
			}
			//! post-decrement
			_rev_iterator operator--( int )
			{
				_rev_iterator tmp( *this );
				_seekFwd( 1 );
				return tmp;
			}

			//! addition operator
			_rev_iterator operator+( size_type n )
			{
				_rev_iterator tmp( *this );
				tmp._seekRev( n );
				return tmp;
			}
			//! addition operator
			_rev_iterator operator+( difference_type n )
			{
				_rev_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekFwd( -n );
				else
					tmp._seekRev( n );
				return tmp;
			}
			//! subtraction operator
			_rev_iterator operator-( size_type n )
			{
				_rev_iterator tmp( *this );
				tmp._seekFwd( n );
				return tmp;
			}
			//! subtraction operator
			_rev_iterator operator-( difference_type n )
			{
				_rev_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekRev( -n );
				else
					tmp._seekFwd( n );
				return tmp;
			}

			//! addition assignment operator
			_rev_iterator& operator+=( size_type n )
			{
				_seekRev( n );
				return *this;
			}
			//! addition assignment operator
			_rev_iterator& operator+=( difference_type n )
			{
				if ( n < 0 )
					_seekFwd( -n );
				else
					_seekRev( n );
				return *this;
			}
			//! subtraction assignment operator
			_rev_iterator& operator-=( size_type n )
			{
				_seekFwd( n );
				return *this;
			}
			//! subtraction assignment operator
			_rev_iterator& operator-=( difference_type n )
			{
				if ( n < 0 )
					_seekRev( -n );
				else
					_seekFwd( n );
				return *this;
			}

			//! dereference operator
			value_type& operator*() const
			{
				return mIter[-1];
			}

			//! dereference at offset operator
			value_type& operator[]( size_type n ) const
			{
				_rev_iterator tmp( *this );
				tmp -= n;
				return *tmp;
			}
			//! dereference at offset operator
			value_type& operator[]( difference_type n ) const
			{
				_rev_iterator tmp( *this );
				tmp -= n;
				return *tmp;
			}
		};
		//#########################################################################
		//! const reverse iterator for UString
		class _const_rev_iterator: public _base_iterator
		{ /* i don't know why the beautifier is freaking out on this line */
		public:
			_const_rev_iterator() { }
			_const_rev_iterator( const _const_rev_iterator& i )
			{
				_become( i );
			}
			_const_rev_iterator( const _rev_iterator& i )
			{
				_become( i );
			}
			//! pre-increment
			_const_rev_iterator& operator++()
			{
				_seekRev( 1 );
				return *this;
			}
			//! post-increment
			_const_rev_iterator operator++( int )
			{
				_const_rev_iterator tmp( *this );
				_seekRev( 1 );
				return tmp;
			}

			//! pre-decrement
			_const_rev_iterator& operator--()
			{
				_seekFwd( 1 );
				return *this;
			}
			//! post-decrement
			_const_rev_iterator operator--( int )
			{
				_const_rev_iterator tmp( *this );
				_seekFwd( 1 );
				return tmp;
			}

			//! addition operator
			_const_rev_iterator operator+( size_type n )
			{
				_const_rev_iterator tmp( *this );
				tmp._seekRev( n );
				return tmp;
			}
			//! addition operator
			_const_rev_iterator operator+( difference_type n )
			{
				_const_rev_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekFwd( -n );
				else
					tmp._seekRev( n );
				return tmp;
			}
			//! subtraction operator
			_const_rev_iterator operator-( size_type n )
			{
				_const_rev_iterator tmp( *this );
				tmp._seekFwd( n );
				return tmp;
			}
			//! subtraction operator
			_const_rev_iterator operator-( difference_type n )
			{
				_const_rev_iterator tmp( *this );
				if ( n < 0 )
					tmp._seekRev( -n );
				else
					tmp._seekFwd( n );
				return tmp;
			}

			//! addition assignment operator
			_const_rev_iterator& operator+=( size_type n )
			{
				_seekRev( n );
				return *this;
			}
			//! addition assignment operator
			_const_rev_iterator& operator+=( difference_type n )
			{
				if ( n < 0 )
					_seekFwd( -n );
				else
					_seekRev( n );
				return *this;
			}
			//! subtraction assignment operator
			_const_rev_iterator& operator-=( size_type n )
			{
				_seekFwd( n );
				return *this;
			}
			//! subtraction assignment operator
			_const_rev_iterator& operator-=( difference_type n )
			{
				if ( n < 0 )
					_seekRev( -n );
				else
					_seekFwd( n );
				return *this;
			}

			//! dereference operator
			const value_type& operator*() const
			{
				return mIter[-1];
			}

			//! dereference at offset operator
			const value_type& operator[]( size_type n ) const
			{
				_const_rev_iterator tmp( *this );
				tmp -= n;
				return *tmp;
			}
			//! dereference at offset operator
			const value_type& operator[]( difference_type n ) const
			{
				_const_rev_iterator tmp( *this );
				tmp -= n;
				return *tmp;
			}

			//! difference operator
			friend size_type operator-( const _const_rev_iterator& left, const _const_rev_iterator& right );
			//! equality operator
			friend bool operator==( const _const_rev_iterator& left, const _const_rev_iterator& right );
			//! inequality operator
			friend bool operator!=( const _const_rev_iterator& left, const _const_rev_iterator& right );
			//! less than
			friend bool operator<( const _const_rev_iterator& left, const _const_rev_iterator& right );
			//! less than or equal
			friend bool operator<=( const _const_rev_iterator& left, const _const_rev_iterator& right );
			//! greater than
			friend bool operator>( const _const_rev_iterator& left, const _const_rev_iterator& right );
			//! greater than or equal
			friend bool operator>=( const _const_rev_iterator& left, const _const_rev_iterator& right );
		};
		//#########################################################################

		typedef _fwd_iterator iterator;                     //!< iterator
		typedef _rev_iterator reverse_iterator;             //!< reverse iterator
		typedef _const_fwd_iterator const_iterator;         //!< const iterator
		typedef _const_rev_iterator const_reverse_iterator; //!< const reverse iterator


		//!\name Constructors/Destructor
		//@{
		//! default constructor, creates an empty string
		UString()
		{
			_init();
		}
		//! copy constructor
		UString( const UString& copy )
		{
			_init();
			mData = copy.mData;
		}
		//! \a length copies of \a ch
		UString( size_type length, const code_point& ch )
		{
			_init();
			assign( length, ch );
		}
		//! duplicate of nul-terminated sequence \a str
		UString( const code_point* str )
		{
			_init();
			assign( str );
		}
		//! duplicate of \a str, \a length code points long
		UString( const code_point* str, size_type length )
		{
			_init();
			assign( str, length );
		}
		//! substring of \a str starting at \a index and \a length code points long
		UString( const UString& str, size_type index, size_type length )
		{
			_init();
			assign( str, index, length );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! duplicate of nul-terminated \c wchar_t array
		UString( const wchar_t* w_str )
		{
			_init();
			assign( w_str );
		}
		//! duplicate of \a w_str, \a length characters long
		UString( const wchar_t* w_str, size_type length )
		{
			_init();
			assign( w_str, length );
		}
#endif
		//! duplicate of \a wstr
		UString( const std::wstring& wstr )
		{
			_init();
			assign( wstr );
		}
		//! duplicate of nul-terminated C-string \a c_str (UTF-8 encoding)
		UString( const char* c_str )
		{
			_init();
			assign( c_str );
		}
		//! duplicate of \a c_str, \a length characters long (UTF-8 encoding)
		UString( const char* c_str, size_type length )
		{
			_init();
			assign( c_str, length );
		}
		//! duplicate of \a str (UTF-8 encoding)
		UString( const std::string& str )
		{
			_init();
			assign( str );
		}
		//! destructor
		~UString()
		{
			_cleanBuffer();
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Utility functions
		//@{
		//! Returns the number of code points in the current string
		size_type size() const
		{
			return mData.size();
		}
		//! Returns the number of code points in the current string
		size_type length() const
		{
			return size();
		}
		//! Returns the number of Unicode characters in the string
		/*! Executes in linear time. */
		size_type length_Characters() const
		{
			const_iterator i = begin(), ie = end();
			size_type c = 0;
			while ( i != ie )
			{
				i.moveNext();
				++c;
			}
			return c;
		}
		//! returns the maximum number of UTF-16 code points that the string can hold
		size_type max_size() const
		{
			return mData.max_size();
		}
		//! sets the capacity of the string to at least \a size code points
		void reserve( size_type size )
		{
			mData.reserve( size );
		}
		//! changes the size of the string to \a size, filling in any new area with \a val
		void resize( size_type num, const code_point& val = 0 )
		{
			mData.resize( num, val );
		}
		//! exchanges the elements of the current string with those of \a from
		void swap( UString& from )
		{
			mData.swap( from.mData );
		}
		//! returns \c true if the string has no elements, \c false otherwise
		bool empty() const
		{
			return mData.empty();
		}
		//! returns a pointer to the first character in the current string
		const code_point* c_str() const
		{
			return mData.c_str();
		}
		//! returns a pointer to the first character in the current string
		const code_point* data() const
		{
			return c_str();
		}
		//! returns the number of elements that the string can hold before it will need to allocate more space
		size_type capacity() const
		{
			return mData.capacity();
		}
		//! deletes all of the elements in the string
		void clear()
		{
			mData.clear();
		}
		//! returns a substring of the current string, starting at \a index, and \a num characters long.
		/*! If \a num is omitted, it will default to \c UString::npos, and the substr() function will simply return the remainder of the string starting at \a index. */
		UString substr( size_type index, size_type num = npos ) const
		{
			// this could avoid the extra copy if we used a private specialty constructor
			dstring data = mData.substr( index, num );
			UString tmp;
			tmp.mData.swap( data );
			return tmp;
		}
		//! appends \a val to the end of the string
		void push_back( unicode_char val )
		{
			code_point cp[2];
			size_t c = _utf32_to_utf16( val, cp );
			if ( c > 0 ) push_back( cp[0] );
			if ( c > 1 ) push_back( cp[1] );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! appends \a val to the end of the string
		void push_back( wchar_t val )
		{
			// we do this because the Unicode method still preserves UTF-16 code points
			mData.push_back( static_cast<unicode_char>( val ) );
		}
#endif
		//! appends \a val to the end of the string
		/*! This can be used to push surrogate pair code points, you'll just need to push them
		one after the other. */
		void push_back( code_point val )
		{
			mData.push_back( val );
		}
		//! appends \a val to the end of the string
		/*! Limited to characters under the 127 value barrier. */
		void push_back( char val )
		{
			mData.push_back( static_cast<code_point>( val ) );
		}
		//! returns \c true if the given Unicode character \a ch is in this string
		bool inString( unicode_char ch ) const
		{
			const_iterator i, ie = end();
			for ( i = begin(); i != ie; i.moveNext() )
			{
				if ( i.getCharacter() == ch )
					return true;
			}
			return false;
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Stream variations
		//@{
		//! returns the current string in UTF-8 form within a std::string
		const std::string& asUTF8() const
		{
			_load_buffer_UTF8();
			return *m_buffer.mStrBuffer;
		}
		//! returns the current string in UTF-8 form as a nul-terminated char array
		const char* asUTF8_c_str() const
		{
			_load_buffer_UTF8();
			return m_buffer.mStrBuffer->c_str();
		}
		//! returns the current string in UTF-32 form within a utf32string
		const utf32string& asUTF32() const
		{
			_load_buffer_UTF32();
			return *m_buffer.mUTF32StrBuffer;
		}
		//! returns the current string in UTF-32 form as a nul-terminated unicode_char array
		const unicode_char* asUTF32_c_str() const
		{
			_load_buffer_UTF32();
			return m_buffer.mUTF32StrBuffer->c_str();
		}
		//! returns the current string in the native form of std::wstring
		const std::wstring& asWStr() const
		{
			_load_buffer_WStr();
			return *m_buffer.mWStrBuffer;
		}
		//! returns the current string in the native form of a nul-terminated wchar_t array
		const wchar_t* asWStr_c_str() const
		{
			_load_buffer_WStr();
			return m_buffer.mWStrBuffer->c_str();
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Single Character Access
		//@{
		//! returns a reference to the element in the string at index \c loc
		code_point& at( size_type loc )
		{
			return mData.at( loc );
		}
		//! returns a reference to the element in the string at index \c loc
		const code_point& at( size_type loc ) const
		{
			return mData.at( loc );
		}
		//! returns the data point \a loc evaluated as a UTF-32 value
		/*! This function will will only properly decode surrogate pairs when \a loc points to the index
		of a lead code point that is followed by a trailing code point. Evaluating the trailing code point
		itself, or pointing to a code point that is a sentinel value (part of a broken pair) will return
		the value of just that code point (not a valid Unicode value, but useful as a sentinel value). */
		unicode_char getChar( size_type loc ) const
		{
			const code_point* ptr = c_str();
			unicode_char uc;
			size_t len = _utf16_char_length( ptr[loc] );
			code_point cp[2] = { /* blame the code beautifier */ 0, 0 };
			cp[0] = ptr[loc];

			if ( len == 2 && ( loc + 1 ) < mData.length() )
			{
				cp[1] = ptr[loc+1];
			}
			_utf16_to_utf32( cp, uc );
			return uc;
		}
		//! sets the value of the character at \a loc to the Unicode value \a ch (UTF-32)
		/*! Providing sentinel values (values between U+D800-U+DFFF) are accepted, but you should be aware
		that you can also unwittingly create a valid surrogate pair if you don't pay attention to what you
		are doing. \note This operation may also lengthen the string if a surrogate pair is needed to
		represent the value given, but one is not available to replace; or alternatively shorten the string
		if an existing surrogate pair is replaced with a character that is representable without a surrogate
		pair. The return value will signify any lengthening or shortening performed, returning 0 if no change
		was made, -1 if the string was shortened, or 1 if the string was lengthened. Any single call can
		only change the string length by + or - 1. */
		int setChar( size_type loc, unicode_char ch )
		{
			code_point cp[2] = { /* blame the code beautifier */ 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			unicode_char existingChar = getChar( loc );
			size_t existingSize = _utf16_char_length( existingChar );
			size_t newSize = _utf16_char_length( ch );

			if ( newSize > existingSize )
			{
				at( loc ) = cp[0];
				insert( loc + 1, 1, cp[1] );
				return 1;
			}
			if ( newSize < existingSize )
			{
				erase( loc, 1 );
				at( loc ) = cp[0];
				return -1;
			}

			// newSize == existingSize
			at( loc ) = cp[0];
			if ( lc == 2 ) at( loc + 1 ) = cp[1];
			return 0;
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name iterator acquisition
		//@{
		//! returns an iterator to the first element of the string
		iterator begin()
		{
			iterator i;
			i.mIter = mData.begin();
			i.mString = this;
			return i;
		}
		//! returns an iterator to the first element of the string
		const_iterator begin() const
		{
			const_iterator i;
			i.mIter = const_cast<UString*>( this )->mData.begin();
			i.mString = const_cast<UString*>( this );
			return i;
		}
		//! returns an iterator just past the end of the string
		iterator end()
		{
			iterator i;
			i.mIter = mData.end();
			i.mString = this;
			return i;
		}
		//! returns an iterator just past the end of the string
		const_iterator end() const
		{
			const_iterator i;
			i.mIter = const_cast<UString*>( this )->mData.end();
			i.mString = const_cast<UString*>( this );
			return i;
		}
		//! returns a reverse iterator to the last element of the string
		reverse_iterator rbegin()
		{
			reverse_iterator i;
			i.mIter = mData.end();
			i.mString = this;
			return i;
		}
		//! returns a reverse iterator to the last element of the string
		const_reverse_iterator rbegin() const
		{
			const_reverse_iterator i;
			i.mIter = const_cast<UString*>( this )->mData.end();
			i.mString = const_cast<UString*>( this );
			return i;
		}
		//! returns a reverse iterator just past the beginning of the string
		reverse_iterator rend()
		{
			reverse_iterator i;
			i.mIter = mData.begin();
			i.mString = this;
			return i;
		}
		//! returns a reverse iterator just past the beginning of the string
		const_reverse_iterator rend() const
		{
			const_reverse_iterator i;
			i.mIter = const_cast<UString*>( this )->mData.begin();
			i.mString = const_cast<UString*>( this );
			return i;
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name assign
		//@{
		//! gives the current string the values from \a start to \a end
		UString& assign( iterator start, iterator end )
		{
			mData.assign( start.mIter, end.mIter );
			return *this;
		}
		//! assign \a str to the current string
		UString& assign( const UString& str )
		{
			mData.assign( str.mData );
			return *this;
		}
		//! assign the nul-terminated \a str to the current string
		UString& assign( const code_point* str )
		{
			mData.assign( str );
			return *this;
		}
		//! assign the first \a num characters of \a str to the current string
		UString& assign( const code_point* str, size_type num )
		{
			mData.assign( str, num );
			return *this;
		}
		//! assign \a len entries from \a str to the current string, starting at \a index
		UString& assign( const UString& str, size_type index, size_type len )
		{
			mData.assign( str.mData, index, len );
			return *this;
		}
		//! assign \a num copies of \a ch to the current string
		UString& assign( size_type num, const code_point& ch )
		{
			mData.assign( num, ch );
			return *this;
		}
		//! assign \a wstr to the current string (\a wstr is treated as a UTF-16 stream)
		UString& assign( const std::wstring& wstr )
		{
			mData.clear();
			mData.reserve( wstr.length() ); // best guess bulk allocate
#ifdef WCHAR_UTF16 // if we're already working in UTF-16, this is easy
			code_point tmp;
			std::wstring::const_iterator i, ie = wstr.end();
			for ( i = wstr.begin(); i != ie; ++i )
			{
				tmp = static_cast<code_point>( *i );
				mData.push_back( tmp );
			}
#else // otherwise we do it the safe way (which is still 100% safe to pass UTF-16 through, just slower)
			code_point cp[3] = { 0, 0, 0 };
			unicode_char tmp;
			std::wstring::const_iterator i, ie = wstr.end();
			for ( i = wstr.begin(); i != ie; i++ )
			{
				tmp = static_cast<unicode_char>( *i );
				size_t lc = _utf32_to_utf16( tmp, cp );
				if ( lc > 0 ) mData.push_back( cp[0] );
				if ( lc > 1 ) mData.push_back( cp[1] );
			}
#endif
			return *this;
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! assign \a w_str to the current string
		UString& assign( const wchar_t* w_str )
		{
			std::wstring tmp;
			tmp.assign( w_str );
			return assign( tmp );
		}
		//! assign the first \a num characters of \a w_str to the current string
		UString& assign( const wchar_t* w_str, size_type num )
		{
			std::wstring tmp;
			tmp.assign( w_str, num );
			return assign( tmp );
		}
#endif
		//! assign \a str to the current string (\a str is treated as a UTF-8 stream)
		UString& assign( const std::string& str )
		{
			size_type len = _verifyUTF8( str );
			clear(); // empty our contents, if there are any
			reserve( len ); // best guess bulk capacity growth

			// This is a 3 step process, converting each byte in the UTF-8 stream to UTF-32,
			// then converting it to UTF-16, then finally appending the data buffer

			unicode_char uc;          // temporary Unicode character buffer
			unsigned char utf8buf[7]; // temporary UTF-8 buffer
			utf8buf[6] = 0;
			size_t utf8len;           // UTF-8 length
			code_point utf16buff[3];  // temporary UTF-16 buffer
			utf16buff[2] = 0;
			size_t utf16len;          // UTF-16 length

			std::string::const_iterator i, ie = str.end();
			for ( i = str.begin(); i != ie; ++i )
			{
				utf8len = _utf8_char_length( static_cast<unsigned char>( *i ) ); // estimate bytes to load
				for ( size_t j = 0; j < utf8len; j++ )
				{ // load the needed UTF-8 bytes
					utf8buf[j] = ( static_cast<unsigned char>( *( i + j ) ) ); // we don't increment 'i' here just in case the estimate is wrong (shouldn't happen, but we're being careful)
				}
				utf8buf[utf8len] = 0; // nul terminate so we throw an exception before running off the end of the buffer
				utf8len = _utf8_to_utf32( utf8buf, uc ); // do the UTF-8 -> UTF-32 conversion
				i += utf8len - 1; // we subtract 1 for the increment of the 'for' loop

				utf16len = _utf32_to_utf16( uc, utf16buff ); // UTF-32 -> UTF-16 conversion
				append( utf16buff, utf16len ); // append the characters to the string
			}
			return *this;
		}
		//! assign \a c_str to the current string (\a c_str is treated as a UTF-8 stream)
		UString& assign( const char* c_str )
		{
			std::string tmp( c_str );
			return assign( tmp );
		}
		//! assign the first \a num characters of \a c_str to the current string (\a c_str is treated as a UTF-8 stream)
		UString& assign( const char* c_str, size_type num )
		{
			std::string tmp;
			tmp.assign( c_str, num );
			return assign( tmp );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name append
		//@{
		//! appends \a str on to the end of the current string
		UString& append( const UString& str )
		{
			mData.append( str.mData );
			return *this;
		}
		//! appends \a str on to the end of the current string
		UString& append( const code_point* str )
		{
			mData.append( str );
			return *this;
		}
		//! appends a substring of \a str starting at \a index that is \a len characters long on to the end of the current string
		UString& append( const UString& str, size_type index, size_type len )
		{
			mData.append( str.mData, index, len );
			return *this;
		}
		//! appends \a num characters of \a str on to the end of the current string
		UString& append( const code_point* str, size_type num )
		{
			mData.append( str, num );
			return *this;
		}
		//! appends \a num repetitions of \a ch on to the end of the current string
		UString& append( size_type num, code_point ch )
		{
			mData.append( num, ch );
			return *this;
		}
		//! appends the sequence denoted by \a start and \a end on to the end of the current string
		UString& append( iterator start, iterator end )
		{
			mData.append( start.mIter, end.mIter );
			return *this;
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! appends \a num characters of \a str on to the end of the current string
		UString& append( const wchar_t* w_str, size_type num )
		{
			std::wstring tmp( w_str, num );
			return append( tmp );
		}
		//! appends \a num repetitions of \a ch on to the end of the current string
		UString& append( size_type num, wchar_t ch )
		{
			return append( num, static_cast<unicode_char>( ch ) );
		}
#endif
		//! appends \a num characters of \a str on to the end of the current string  (UTF-8 encoding)
		UString& append( const char* c_str, size_type num )
		{
			UString tmp( c_str, num );
			append( tmp );
			return *this;
		}
		//! appends \a num repetitions of \a ch on to the end of the current string (Unicode values less than 128)
		UString& append( size_type num, char ch )
		{
			append( num, static_cast<code_point>( ch ) );
			return *this;
		}
		//! appends \a num repetitions of \a ch on to the end of the current string (Full Unicode spectrum)
		UString& append( size_type num, unicode_char ch )
		{
			code_point cp[2] = { 0, 0 };
			if ( _utf32_to_utf16( ch, cp ) == 2 )
			{
				for ( size_type i = 0; i < num; i++ )
				{
					append( 1, cp[0] );
					append( 1, cp[1] );
				}
			}
			else
			{
				for ( size_type i = 0; i < num; i++ )
				{
					append( 1, cp[0] );
				}
			}
			return *this;
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name insert
		//@{
		//! inserts \a ch before the code point denoted by \a i
		iterator insert( iterator i, const code_point& ch )
		{
			iterator ret;
			ret.mIter = mData.insert( i.mIter, ch );
			ret.mString = this;
			return ret;
		}
		//! inserts \a str into the current string, at location \a index
		UString& insert( size_type index, const UString& str )
		{
			mData.insert( index, str.mData );
			return *this;
		}
		//! inserts \a str into the current string, at location \a index
		UString& insert( size_type index, const code_point* str )
		{
			mData.insert( index, str );
			return *this;
		}
		//! inserts a substring of \a str (starting at \a index2 and \a num code points long) into the current string, at location \a index1
		UString& insert( size_type index1, const UString& str, size_type index2, size_type num )
		{
			mData.insert( index1, str.mData, index2, num );
			return *this;
		}
		//! inserts the code points denoted by \a start and \a end into the current string, before the code point specified by \a i
		void insert( iterator i, iterator start, iterator end )
		{
			mData.insert( i.mIter, start.mIter, end.mIter );
		}
		//! inserts \a num code points of \a str into the current string, at location \a index
		UString& insert( size_type index, const code_point* str, size_type num )
		{
			mData.insert( index, str, num );
			return *this;
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! inserts \a num code points of \a str into the current string, at location \a index
		UString& insert( size_type index, const wchar_t* w_str, size_type num )
		{
			UString tmp( w_str, num );
			insert( index, tmp );
			return *this;
		}
#endif
		//! inserts \a num code points of \a str into the current string, at location \a index
		UString& insert( size_type index, const char* c_str, size_type num )
		{
			UString tmp( c_str, num );
			insert( index, tmp );
			return *this;
		}
		//! inserts \a num copies of \a ch into the current string, at location \a index
		UString& insert( size_type index, size_type num, code_point ch )
		{
			mData.insert( index, num, ch );
			return *this;
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! inserts \a num copies of \a ch into the current string, at location \a index
		UString& insert( size_type index, size_type num, wchar_t ch )
		{
			insert( index, num, static_cast<unicode_char>( ch ) );
			return *this;
		}
#endif
		//! inserts \a num copies of \a ch into the current string, at location \a index
		UString& insert( size_type index, size_type num, char ch )
		{
			insert( index, num, static_cast<code_point>( ch ) );
			return *this;
		}
		//! inserts \a num copies of \a ch into the current string, at location \a index
		UString& insert( size_type index, size_type num, unicode_char ch )
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			if ( lc == 1 )
			{
				return insert( index, num, cp[0] );
			}
			for ( size_type c = 0; c < num; c++ )
			{
				// insert in reverse order to preserve ordering after insert
				insert( index, 1, cp[1] );
				insert( index, 1, cp[0] );
			}
			return *this;
		}
		//! inserts \a num copies of \a ch into the current string, before the code point denoted by \a i
		void insert( iterator i, size_type num, const code_point& ch )
		{
			mData.insert( i.mIter, num, ch );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! inserts \a num copies of \a ch into the current string, before the code point denoted by \a i
		void insert( iterator i, size_type num, const wchar_t& ch )
		{
			insert( i, num, static_cast<unicode_char>( ch ) );
		}
#endif
		//! inserts \a num copies of \a ch into the current string, before the code point denoted by \a i
		void insert( iterator i, size_type num, const char& ch )
		{
			insert( i, num, static_cast<code_point>( ch ) );
		}
		//! inserts \a num copies of \a ch into the current string, before the code point denoted by \a i
		void insert( iterator i, size_type num, const unicode_char& ch )
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			if ( lc == 1 )
			{
				insert( i, num, cp[0] );
			}
			else
			{
				for ( size_type c = 0; c < num; c++ )
				{
					// insert in reverse order to preserve ordering after insert
					insert( i, 1, cp[1] );
					insert( i, 1, cp[0] );
				}
			}
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name erase
		//@{
		//! removes the code point pointed to by \a loc, returning an iterator to the next character
		iterator erase( iterator loc )
		{
			iterator ret;
			ret.mIter = mData.erase( loc.mIter );
			ret.mString = this;
			return ret;
		}
		//! removes the code points between \a start and \a end (including the one at \a start but not the one at \a end), returning an iterator to the code point after the last code point removed
		iterator erase( iterator start, iterator end )
		{
			iterator ret;
			ret.mIter = mData.erase( start.mIter, end.mIter );
			ret.mString = this;
			return ret;
		}
		//! removes \a num code points from the current string, starting at \a index
		UString& erase( size_type index = 0, size_type num = npos )
		{
			if ( num == npos )
				mData.erase( index );
			else
				mData.erase( index, num );
			return *this;
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name replace
		//@{
		//! replaces up to \a num1 code points of the current string (starting at \a index1) with \a str
		UString& replace( size_type index1, size_type num1, const UString& str )
		{
			mData.replace( index1, num1, str.mData, 0, npos );
			return *this;
		}
		//! replaces up to \a num1 code points of the current string (starting at \a index1) with up to \a num2 code points from \a str
		UString& replace( size_type index1, size_type num1, const UString& str, size_type num2 )
		{
			mData.replace( index1, num1, str.mData, 0, num2 );
			return *this;
		}
		//! replaces up to \a num1 code points of the current string (starting at \a index1) with up to \a num2 code points from \a str beginning at \a index2
		UString& replace( size_type index1, size_type num1, const UString& str, size_type index2, size_type num2 )
		{
			mData.replace( index1, num1, str.mData, index2, num2 );
			return *this;
		}
		//! replaces code points in the current string from \a start to \a end with \a num code points from \a str
		UString& replace( iterator start, iterator end, const UString& str, size_type num = npos )
		{
			_const_fwd_iterator st(start); //Work around for gcc, allow it to find correct overload

			size_type index1 = begin() - st;
			size_type num1 = end - st;
			return replace( index1, num1, str, 0, num );
		}
		//! replaces up to \a num1 code points in the current string (beginning at \a index) with \c num2 copies of \c ch
		UString& replace( size_type index, size_type num1, size_type num2, code_point ch )
		{
			mData.replace( index, num1, num2, ch );
			return *this;
		}
		//! replaces the code points in the current string from \a start to \a end with \a num copies of \a ch
		UString& replace( iterator start, iterator end, size_type num, code_point ch )
		{
			_const_fwd_iterator st(start); //Work around for gcc, allow it to find correct overload

			size_type index1 = begin() - st;
			size_type num1 = end - st;
			return replace( index1, num1, num, ch );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name compare
		//@{
		//! compare \a str to the current string
		int compare( const UString& str ) const
		{
			return mData.compare( str.mData );
		}
		//! compare \a str to the current string
		int compare( const code_point* str ) const
		{
			return mData.compare( str );
		}
		//! compare \a str to a substring of the current string, starting at \a index for \a length characters
		int compare( size_type index, size_type length, const UString& str ) const
		{
			return mData.compare( index, length, str.mData );
		}
		//! compare a substring of \a str to a substring of the current string, where \a index2 and \a length2 refer to \a str and \a index and \a length refer to the current string
		int compare( size_type index, size_type length, const UString& str, size_type index2, size_type length2 ) const
		{
			return mData.compare( index, length, str.mData, index2, length2 );
		}
		//! compare a substring of \a str to a substring of the current string, where the substring of \a str begins at zero and is \a length2 characters long, and the substring of the current string begins at \a index and is \a length  characters long
		int compare( size_type index, size_type length, const code_point* str, size_type length2 ) const
		{
			return mData.compare( index, length, str, length2 );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! compare a substring of \a str to a substring of the current string, where the substring of \a str begins at zero and is \a length2 elements long, and the substring of the current string begins at \a index and is \a length characters long
		int compare( size_type index, size_type length, const wchar_t* w_str, size_type length2 ) const
		{
			UString tmp( w_str, length2 );
			return compare( index, length, tmp );
		}
#endif
		//! compare a substring of \a str to a substring of the current string, where the substring of \a str begins at zero and is \a length2 <b>UTF-8 code points</b> long, and the substring of the current string begins at \a index and is \a length characters long
		int compare( size_type index, size_type length, const char* c_str, size_type length2 ) const
		{
			UString tmp( c_str, length2 );
			return compare( index, length, tmp );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name find & rfind
		//@{
		//! returns the index of the first occurrence of \a str within the current string, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a str is a UTF-16 encoded string, but through implicit casting can also be a UTF-8 encoded string (const char* or std::string) */
		size_type find( const UString& str, size_type index = 0 ) const
		{
			return mData.find( str.c_str(), index );
		}
		//! returns the index of the first occurrence of \a str within the current string and within \a length code points, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a cp_str is a UTF-16 encoded string */
		size_type find( const code_point* cp_str, size_type index, size_type length ) const
		{
			UString tmp( cp_str );
			return mData.find( tmp.c_str(), index, length );
		}
		//! returns the index of the first occurrence of \a str within the current string and within \a length code points, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a cp_str is a UTF-8 encoded string */
		size_type find( const char* c_str, size_type index, size_type length ) const
		{
			UString tmp( c_str );
			return mData.find( tmp.c_str(), index, length );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the index of the first occurrence of \a str within the current string and within \a length code points, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a cp_str is a UTF-16 encoded string */
		size_type find( const wchar_t* w_str, size_type index, size_type length ) const
		{
			UString tmp( w_str );
			return mData.find( tmp.c_str(), index, length );
		}
#endif
		//! returns the index of the first occurrence \a ch within the current string, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a ch is only capable of representing Unicode values up to U+007F (127) */
		size_type find( char ch, size_type index = 0 ) const
		{
			return find( static_cast<code_point>( ch ), index );
		}
		//! returns the index of the first occurrence \a ch within the current string, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a ch is only capable of representing Unicode values up to U+FFFF (65535) */
		size_type find( code_point ch, size_type index = 0 ) const
		{
			return mData.find( ch, index );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the index of the first occurrence \a ch within the current string, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a ch is only capable of representing Unicode values up to U+FFFF (65535) */
		size_type find( wchar_t ch, size_type index = 0 ) const
		{
			return find( static_cast<unicode_char>( ch ), index );
		}
#endif
		//! returns the index of the first occurrence \a ch within the current string, starting at \a index; returns \c UString::npos if nothing is found
		/*! \a ch can fully represent any Unicode character */
		size_type find( unicode_char ch, size_type index = 0 ) const
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			return find( UString( cp, lc ), index );
		}

		//! returns the location of the first occurrence of \a str in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type rfind( const UString& str, size_type index = 0 ) const
		{
			return mData.rfind( str.c_str(), index );
		}
		//! returns the location of the first occurrence of \a str in the current string, doing a reverse search from \a index, searching at most \a num characters; returns \c UString::npos if nothing is found
		size_type rfind( const code_point* cp_str, size_type index, size_type num ) const
		{
			UString tmp( cp_str );
			return mData.rfind( tmp.c_str(), index, num );
		}
		//! returns the location of the first occurrence of \a str in the current string, doing a reverse search from \a index, searching at most \a num characters; returns \c UString::npos if nothing is found
		size_type rfind( const char* c_str, size_type index, size_type num ) const
		{
			UString tmp( c_str );
			return mData.rfind( tmp.c_str(), index, num );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the location of the first occurrence of \a str in the current string, doing a reverse search from \a index, searching at most \a num characters; returns \c UString::npos if nothing is found
		size_type rfind( const wchar_t* w_str, size_type index, size_type num ) const
		{
			UString tmp( w_str );
			return mData.rfind( tmp.c_str(), index, num );
		}
#endif
		//! returns the location of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type rfind( char ch, size_type index = 0 ) const
		{
			return rfind( static_cast<code_point>( ch ), index );
		}
		//! returns the location of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type rfind( code_point ch, size_type index ) const
		{
			return mData.rfind( ch, index );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the location of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type rfind( wchar_t ch, size_type index = 0 ) const
		{
			return rfind( static_cast<unicode_char>( ch ), index );
		}
#endif
		//! returns the location of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type rfind( unicode_char ch, size_type index = 0 ) const
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			return rfind( UString( cp, lc ), index );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name find_first/last_(not)_of
		//@{
		//! Returns the index of the first character within the current string that matches \b any character in \a str, beginning the search at \a index and searching at most \a num characters; returns \c UString::npos if nothing is found
		size_type find_first_of( const UString &str, size_type index = 0, size_type num = npos ) const
		{
			size_type i = 0;
			const size_type len = length();
			while ( i < num && ( index + i ) < len )
			{
				unicode_char ch = getChar( index + i );
				if ( str.inString( ch ) )
					return index + i;
				i += _utf16_char_length( ch ); // increment by the Unicode character length
			}
			return npos;
		}
		//! returns the index of the first occurrence of \a ch in the current string, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_of( code_point ch, size_type index = 0 ) const
		{
			UString tmp;
			tmp.assign( 1, ch );
			return find_first_of( tmp, index );
		}
		//! returns the index of the first occurrence of \a ch in the current string, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_of( char ch, size_type index = 0 ) const
		{
			return find_first_of( static_cast<code_point>( ch ), index );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the index of the first occurrence of \a ch in the current string, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_of( wchar_t ch, size_type index = 0 ) const
		{
			return find_first_of( static_cast<unicode_char>( ch ), index );
		}
#endif
		//! returns the index of the first occurrence of \a ch in the current string, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_of( unicode_char ch, size_type index = 0 ) const
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			return find_first_of( UString( cp, lc ), index );
		}

		//! returns the index of the first character within the current string that does not match any character in \a str, beginning the search at \a index and searching at most \a num characters; returns \c UString::npos if nothing is found
		size_type find_first_not_of( const UString& str, size_type index = 0, size_type num = npos ) const
		{
			size_type i = 0;
			const size_type len = length();
			while ( i < num && ( index + i ) < len )
			{
				unicode_char ch = getChar( index + i );
				if ( !str.inString( ch ) )
					return index + i;
				i += _utf16_char_length( ch ); // increment by the Unicode character length
			}
			return npos;
		}
		//! returns the index of the first character within the current string that does not match \a ch, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_not_of( code_point ch, size_type index = 0 ) const
		{
			UString tmp;
			tmp.assign( 1, ch );
			return find_first_not_of( tmp, index );
		}
		//! returns the index of the first character within the current string that does not match \a ch, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_not_of( char ch, size_type index = 0 ) const
		{
			return find_first_not_of( static_cast<code_point>( ch ), index );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the index of the first character within the current string that does not match \a ch, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_not_of( wchar_t ch, size_type index = 0 ) const
		{
			return find_first_not_of( static_cast<unicode_char>( ch ), index );
		}
#endif
		//! returns the index of the first character within the current string that does not match \a ch, starting the search at \a index; returns \c UString::npos if nothing is found
		size_type find_first_not_of( unicode_char ch, size_type index = 0 ) const
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			return find_first_not_of( UString( cp, lc ), index );
		}

		//! returns the index of the first character within the current string that matches any character in \a str, doing a reverse search from \a index and searching at most \a num characters; returns \c UString::npos if nothing is found
		size_type find_last_of( const UString& str, size_type index = npos, size_type num = npos ) const
		{
			size_type i = 0;
			const size_type len = length();
			if ( index > len ) index = len - 1;

			while ( i < num && ( index - i ) != npos )
			{
				size_type j = index - i;
				// careful to step full Unicode characters
				if ( j != 0 && _utf16_surrogate_follow( at( j ) ) && _utf16_surrogate_lead( at( j - 1 ) ) )
				{
					j = index - ++i;
				}
				// and back to the usual dull test
				unicode_char ch = getChar( j );
				if ( str.inString( ch ) )
					return j;
				i++;
			}
			return npos;
		}
		//! returns the index of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_of( code_point ch, size_type index = npos ) const
		{
			UString tmp;
			tmp.assign( 1, ch );
			return find_last_of( tmp, index );
		}
		//! returns the index of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_of( char ch, size_type index = npos ) const
		{
			return find_last_of( static_cast<code_point>( ch ), index );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the index of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_of( wchar_t ch, size_type index = npos ) const
		{
			return find_last_of( static_cast<unicode_char>( ch ), index );
		}
#endif
		//! returns the index of the first occurrence of \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_of( unicode_char ch, size_type index = npos ) const
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			return find_last_of( UString( cp, lc ), index );
		}

		//! returns the index of the last character within the current string that does not match any character in \a str, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_not_of( const UString& str, size_type index = npos, size_type num = npos ) const
		{
			size_type i = 0;
			const size_type len = length();
			if ( index > len ) index = len - 1;

			while ( i < num && ( index - i ) != npos )
			{
				size_type j = index - i;
				// careful to step full Unicode characters
				if ( j != 0 && _utf16_surrogate_follow( at( j ) ) && _utf16_surrogate_lead( at( j - 1 ) ) )
				{
					j = index - ++i;
				}
				// and back to the usual dull test
				unicode_char ch = getChar( j );
				if ( !str.inString( ch ) )
					return j;
				i++;
			}
			return npos;
		}
		//! returns the index of the last occurrence of a character that does not match \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_not_of( code_point ch, size_type index = npos ) const
		{
			UString tmp;
			tmp.assign( 1, ch );
			return find_last_not_of( tmp, index );
		}
		//! returns the index of the last occurrence of a character that does not match \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_not_of( char ch, size_type index = npos ) const
		{
			return find_last_not_of( static_cast<code_point>( ch ), index );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! returns the index of the last occurrence of a character that does not match \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_not_of( wchar_t ch, size_type index = npos ) const
		{
			return find_last_not_of( static_cast<unicode_char>( ch ), index );
		}
#endif
		//! returns the index of the last occurrence of a character that does not match \a ch in the current string, doing a reverse search from \a index; returns \c UString::npos if nothing is found
		size_type find_last_not_of( unicode_char ch, size_type index = npos ) const
		{
			code_point cp[3] = { 0, 0, 0 };
			size_t lc = _utf32_to_utf16( ch, cp );
			return find_last_not_of( UString( cp, lc ), index );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Operators
		//@{
		//! less than operator
		bool operator<( const UString& right ) const
		{
			return compare( right ) < 0;
		}
		//! less than or equal operator
		bool operator<=( const UString& right ) const
		{
			return compare( right ) <= 0;
		}
		//! greater than operator
		bool operator>( const UString& right ) const
		{
			return compare( right ) > 0;
		}
		//! greater than or equal operator
		bool operator>=( const UString& right ) const
		{
			return compare( right ) >= 0;
		}
		//! equality operator
		bool operator==( const UString& right ) const
		{
			return compare( right ) == 0;
		}
		//! inequality operator
		bool operator!=( const UString& right ) const
		{
			return !operator==( right );
		}
		//! assignment operator, implicitly casts all compatible types
		UString& operator=( const UString& s )
		{
			return assign( s );
		}
		//! assignment operator
		UString& operator=( code_point ch )
		{
			clear();
			return append( 1, ch );
		}
		//! assignment operator
		UString& operator=( char ch )
		{
			clear();
			return append( 1, ch );
		}
#if MYGUI_IS_NATIVE_WCHAR_T
		//! assignment operator
		UString& operator=( wchar_t ch )
		{
			clear();
			return append( 1, ch );
		}
#endif
		//! assignment operator
		UString& operator=( unicode_char ch )
		{
			clear();
			return append( 1, ch );
		}
		//! code point dereference operator
		code_point& operator[]( size_type index )
		{
			return at( index );
		}
		//! code point dereference operator
		const code_point& operator[]( size_type index ) const
		{
			return at( index );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Implicit Cast Operators
		//@{
		//! implicit cast to std::string
		operator std::string() const
		{
			return std::string( asUTF8() );
		}
		//! implicit cast to std::wstring
		operator std::wstring() const
		{
			return std::wstring( asWStr() );
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name UTF-16 character encoding/decoding
		//@{
		//! returns \c true if \a cp does not match the signature for the lead of follow code point of a surrogate pair in a UTF-16 sequence
		static bool _utf16_independent_char( code_point cp )
		{
			if ( 0xD800 <= cp && cp <= 0xDFFF ) // tests if the cp is within the surrogate pair range
				return false; // it matches a surrogate pair signature
			return true; // everything else is a standalone code point
		}
		//! returns \c true if \a cp matches the signature of a surrogate pair lead character
		static bool _utf16_surrogate_lead( code_point cp )
		{
			if ( 0xD800 <= cp && cp <= 0xDBFF ) // tests if the cp is within the 2nd word of a surrogate pair
				return true; // it is a 1st word
			return false; // it isn't
		}
		//! returns \c true if \a cp matches the signature of a surrogate pair following character
		static bool _utf16_surrogate_follow( code_point cp )
		{
			if ( 0xDC00 <= cp && cp <= 0xDFFF ) // tests if the cp is within the 2nd word of a surrogate pair
				return true; // it is a 2nd word
			return false; // everything else isn't
		}
		//! estimates the number of UTF-16 code points in the sequence starting with \a cp
		static size_t _utf16_char_length( code_point cp )
		{
			if ( 0xD800 <= cp && cp <= 0xDBFF ) // test if cp is the beginning of a surrogate pair
				return 2; // if it is, then we are 2 words long
			return 1; // otherwise we are only 1 word long
		}
		//! returns the number of UTF-16 code points needed to represent the given UTF-32 character \a cp
		static size_t _utf16_char_length( unicode_char uc )
		{
			if ( uc > 0xFFFF ) // test if uc is greater than the single word maximum
				return 2; // if so, we need a surrogate pair
			return 1; // otherwise we can stuff it into a single word
		}
		//! converts the given UTF-16 character buffer \a in_cp to a single UTF-32 Unicode character \a out_uc, returns the number of code points used to create the output character (2 for surrogate pairs, otherwise 1)
		/*! This function does it's best to prevent error conditions, verifying complete
		surrogate pairs before applying the algorithm. In the event that half of a pair
		is found it will happily generate a value in the 0xD800 - 0xDFFF range, which is
		normally an invalid Unicode value but we preserve them for use as sentinel values. */
		static size_t _utf16_to_utf32( const code_point in_cp[2], unicode_char& out_uc )
		{
			const code_point& cp1 = in_cp[0];
			const code_point& cp2 = in_cp[1];
			bool wordPair = false;

			// does it look like a surrogate pair?
			if ( 0xD800 <= cp1 && cp1 <= 0xDBFF )
			{
				// looks like one, but does the other half match the algorithm as well?
				if ( 0xDC00 <= cp2 && cp2 <= 0xDFFF )
					wordPair = true; // yep!
			}

			if ( !wordPair )
			{ // if we aren't a 100% authentic surrogate pair, then just copy the value
				out_uc = cp1;
				return 1;
			}

			unsigned short cU = cp1, cL = cp2; // copy upper and lower words of surrogate pair to writable buffers
			cU -= 0xD800; // remove the encoding markers
			cL -= 0xDC00;

			out_uc = ( cU & 0x03FF ) << 10; // grab the 10 upper bits and set them in their proper location
			out_uc |= ( cL & 0x03FF ); // combine in the lower 10 bits
			out_uc += 0x10000; // add back in the value offset

			return 2; // this whole operation takes to words, so that's what we'll return
		}
		//! writes the given UTF-32 \a uc_in to the buffer location \a out_cp using UTF-16 encoding, returns the number of code points used to encode the input (always 1 or 2)
		/*! This function, like its counterpart, will happily create invalid UTF-16 surrogate pairs. These
		invalid entries will be created for any value of \c in_uc that falls in the range U+D800 - U+DFFF.
		These are generally useful as sentinel values to represent various program specific conditions.
		\note This function will also pass through any single UTF-16 code point without modification,
		making it a safe method of ensuring a stream that is unknown UTF-32 or UTF-16 is truly UTF-16.*/
		static size_t _utf32_to_utf16( const unicode_char& in_uc, code_point out_cp[2] )
		{
			if ( in_uc <= 0xFFFF )
			{ // we blindly preserve sentinel values because our decoder understands them
				out_cp[0] = in_uc;
				return 1;
			}
			unicode_char uc = in_uc; // copy to writable buffer
			unsigned short tmp; // single code point buffer
			uc -= 0x10000; // subtract value offset

			//process upper word
			tmp = ( uc >> 10 ) & 0x03FF; // grab the upper 10 bits
			tmp += 0xD800; // add encoding offset
			out_cp[0] = tmp; // write

			// process lower word
			tmp = uc & 0x03FF; // grab the lower 10 bits
			tmp += 0xDC00; // add encoding offset
			out_cp[1] = tmp; // write

			return 2; // return used word count (2 for surrogate pairs)
		}
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name UTF-8 character encoding/decoding
		//@{
		//! returns \c true if \a cp is the beginning of a UTF-8 sequence
		static bool _utf8_start_char( unsigned char cp )
		{
			return ( cp & ~_cont_mask ) != _cont;
		}
		//! estimates the number of UTF-8 code points in the sequence starting with \a cp
		static size_t _utf8_char_length( unsigned char cp )
		{
			if ( !( cp & 0x80 ) ) return 1;
			if (( cp & ~_lead1_mask ) == _lead1 ) return 2;
			if (( cp & ~_lead2_mask ) == _lead2 ) return 3;
			if (( cp & ~_lead3_mask ) == _lead3 ) return 4;
			if (( cp & ~_lead4_mask ) == _lead4 ) return 5;
			if (( cp & ~_lead5_mask ) == _lead5 ) return 6;
			throw invalid_data( "invalid UTF-8 sequence header value" );
		}
		//! returns the number of UTF-8 code points needed to represent the given UTF-32 character \a cp
		static size_t _utf8_char_length( unicode_char uc )
		{
			/*
			7 bit:  U-00000000 - U-0000007F: 0xxxxxxx
			11 bit: U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
			16 bit: U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
			21 bit: U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
			26 bit: U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			31 bit: U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
			*/
			if ( !( uc & ~0x0000007F ) ) return 1;
			if ( !( uc & ~0x000007FF ) ) return 2;
			if ( !( uc & ~0x0000FFFF ) ) return 3;
			if ( !( uc & ~0x001FFFFF ) ) return 4;
			if ( !( uc & ~0x03FFFFFF ) ) return 5;
			if ( !( uc & ~0x7FFFFFFF ) ) return 6;
			throw invalid_data( "invalid UTF-32 value" );
		}

		//! converts the given UTF-8 character buffer to a single UTF-32 Unicode character, returns the number of bytes used to create the output character (maximum of 6)
		static size_t _utf8_to_utf32( const unsigned char in_cp[6], unicode_char& out_uc )
		{
			size_t len = _utf8_char_length( in_cp[0] );
			if ( len == 1 )
			{ // if we are only 1 byte long, then just grab it and exit
				out_uc = in_cp[0];
				return 1;
			}

			unicode_char c = 0; // temporary buffer
			size_t i = 0;
			switch ( len )
			{ // load header byte
			case 6:
				c = in_cp[i] & _lead5_mask;
				break;
			case 5:
				c = in_cp[i] & _lead4_mask;
				break;
			case 4:
				c = in_cp[i] & _lead3_mask;
				break;
			case 3:
				c = in_cp[i] & _lead2_mask;
				break;
			case 2:
				c = in_cp[i] & _lead1_mask;
				break;
			}

			for ( ++i; i < len; i++ )
			{ // load each continuation byte
				if (( in_cp[i] & ~_cont_mask ) != _cont )
					throw invalid_data( "bad UTF-8 continuation byte" );
				c <<= 6;
				c |= ( in_cp[i] & _cont_mask );
			}

			out_uc = c; // write the final value and return the used byte length
			return len;
		}
		//! writes the given UTF-32 \a uc_in to the buffer location \a out_cp using UTF-8 encoding, returns the number of bytes used to encode the input
		static size_t _utf32_to_utf8( const unicode_char& in_uc, unsigned char out_cp[6] )
		{
			size_t len = _utf8_char_length( in_uc ); // predict byte length of sequence
			unicode_char c = in_uc; // copy to temp buffer

			//stuff all of the lower bits
			for ( size_t i = len - 1; i > 0; i-- )
			{
				out_cp[i] = (( c ) & _cont_mask ) | _cont;
				c >>= 6;
			}

			//now write the header byte
			switch ( len )
			{
			case 6:
				out_cp[0] = (( c ) & _lead5_mask ) | _lead5;
				break;
			case 5:
				out_cp[0] = (( c ) & _lead4_mask ) | _lead4;
				break;
			case 4:
				out_cp[0] = (( c ) & _lead3_mask ) | _lead3;
				break;
			case 3:
				out_cp[0] = (( c ) & _lead2_mask ) | _lead2;
				break;
			case 2:
				out_cp[0] = (( c ) & _lead1_mask ) | _lead1;
				break;
			case 1:
			default:
				out_cp[0] = ( c ) & 0x7F;
				break;
			}

			// return the byte length of the sequence
			return len;
		}

		//! verifies a UTF-8 stream, returning the total number of Unicode characters found
		static size_type _verifyUTF8( const unsigned char* c_str )
		{
			std::string tmp( reinterpret_cast<const char*>( c_str ) );
			return _verifyUTF8( tmp );
		}
		//! verifies a UTF-8 stream, returning the total number of Unicode characters found
		static size_type _verifyUTF8( const std::string& str )
		{
			std::string::const_iterator i, ie = str.end();
			i = str.begin();
			size_type length = 0;

			while ( i != ie )
			{
				// characters pass until we find an extended sequence
				if (( *i ) & 0x80 )
				{
					unsigned char c = ( *i );
					size_t contBytes = 0;

					// get continuation byte count and test for overlong sequences
					if (( c & ~_lead1_mask ) == _lead1 )
					{ // 1 additional byte
						if ( c == _lead1 ) throw invalid_data( "overlong UTF-8 sequence" );
						contBytes = 1;

					}
					else if (( c & ~_lead2_mask ) == _lead2 )
					{ // 2 additional bytes
						contBytes = 2;
						if ( c == _lead2 )
						{ // possible overlong UTF-8 sequence
							c = ( *( i + 1 ) ); // look ahead to next byte in sequence
							if (( c & _lead2 ) == _cont ) throw invalid_data( "overlong UTF-8 sequence" );
						}

					}
					else if (( c & ~_lead3_mask ) == _lead3 )
					{ // 3 additional bytes
						contBytes = 3;
						if ( c == _lead3 )
						{ // possible overlong UTF-8 sequence
							c = ( *( i + 1 ) ); // look ahead to next byte in sequence
							if (( c & _lead3 ) == _cont ) throw invalid_data( "overlong UTF-8 sequence" );
						}

					}
					else if (( c & ~_lead4_mask ) == _lead4 )
					{ // 4 additional bytes
						contBytes = 4;
						if ( c == _lead4 )
						{ // possible overlong UTF-8 sequence
							c = ( *( i + 1 ) ); // look ahead to next byte in sequence
							if (( c & _lead4 ) == _cont ) throw invalid_data( "overlong UTF-8 sequence" );
						}

					}
					else if (( c & ~_lead5_mask ) == _lead5 )
					{ // 5 additional bytes
						contBytes = 5;
						if ( c == _lead5 )
						{ // possible overlong UTF-8 sequence
							c = ( *( i + 1 ) ); // look ahead to next byte in sequence
							if (( c & _lead5 ) == _cont ) throw invalid_data( "overlong UTF-8 sequence" );
						}
					}

					// check remaining continuation bytes for
					while ( contBytes-- )
					{
						c = ( *( ++i ) ); // get next byte in sequence
						if (( c & ~_cont_mask ) != _cont )
							throw invalid_data( "bad UTF-8 continuation byte" );
					}
				}
				length++;
				i++;
			}
			return length;
		}
		//@}

	private:
		//template<class ITER_TYPE> friend class _iterator;
		dstring mData;

		//! buffer data type identifier
		enum BufferType
		{
			bt_none,
			bt_string,
			bt_wstring,
			bt_utf32string
		};

		//! common constructor operations
		void _init()
		{
			m_buffer.mVoidBuffer = 0;
			m_bufferType = bt_none;
			m_bufferSize = 0;
		}

		///////////////////////////////////////////////////////////////////////
		// Scratch buffer
		//! auto cleans the scratch buffer using the proper delete for the stored type
		void _cleanBuffer() const
		{
			if ( m_buffer.mVoidBuffer != 0 )
			{
				switch ( m_bufferType )
				{
				case bt_string:
					delete m_buffer.mStrBuffer;
					break;
				case bt_wstring:
					delete m_buffer.mWStrBuffer;
					break;
				case bt_utf32string:
					delete m_buffer.mUTF32StrBuffer;
					break;
				case bt_none: // under the worse of circumstances, this is all we can do, and hope it works out
				default:
					//delete m_buffer.mVoidBuffer;
					// delete void* is undefined, don't do that
					MYGUI_ASSERT(false, "This should never happen - mVoidBuffer should never contain something if we "
						"don't know the type");
					break;
				}
				m_buffer.mVoidBuffer = 0;
				m_bufferSize = 0;
			}
		}

		//! create a std::string in the scratch buffer area
		void _getBufferStr() const
		{
			if ( m_bufferType != bt_string )
			{
				_cleanBuffer();
				m_buffer.mStrBuffer = new std::string();
				m_bufferType = bt_string;
			}
			m_buffer.mStrBuffer->clear();
		}
		//! create a std::wstring in the scratch buffer area
		void _getBufferWStr() const
		{
			if ( m_bufferType != bt_wstring )
			{
				_cleanBuffer();
				m_buffer.mWStrBuffer = new std::wstring();
				m_bufferType = bt_wstring;
			}
			m_buffer.mWStrBuffer->clear();
		}
		//! create a utf32string in the scratch buffer area
		void _getBufferUTF32Str() const
		{
			if ( m_bufferType != bt_utf32string )
			{
				_cleanBuffer();
				m_buffer.mUTF32StrBuffer = new utf32string();
				m_bufferType = bt_utf32string;
			}
			m_buffer.mUTF32StrBuffer->clear();
		}

		void _load_buffer_UTF8() const
		{
			_getBufferStr();
			std::string& buffer = ( *m_buffer.mStrBuffer );
			buffer.reserve( length() );

			unsigned char utf8buf[6];
			char* charbuf = ( char* )utf8buf;
			unicode_char c;
			size_t len;

			const_iterator i, ie = end();
			for ( i = begin(); i != ie; i.moveNext() )
			{
				c = i.getCharacter();
				len = _utf32_to_utf8( c, utf8buf );
				size_t j = 0;
				while ( j < len )
					buffer.push_back( charbuf[j++] );
			}
		}
		void _load_buffer_WStr() const
		{
			_getBufferWStr();
			std::wstring& buffer = ( *m_buffer.mWStrBuffer );
			buffer.reserve( length() ); // may over reserve, but should be close enough
#ifdef WCHAR_UTF16 // wchar_t matches UTF-16
			const_iterator i, ie = end();
			for ( i = begin(); i != ie; ++i )
			{
				buffer.push_back(( wchar_t )( *i ) );
			}
#else // wchar_t fits UTF-32
			unicode_char c;
			const_iterator i, ie = end();
			for ( i = begin(); i != ie; i.moveNext() )
			{
				c = i.getCharacter();
				buffer.push_back(( wchar_t )c );
			}
#endif
		}
		void _load_buffer_UTF32() const
		{
			_getBufferUTF32Str();
			utf32string& buffer = ( *m_buffer.mUTF32StrBuffer );
			buffer.reserve( length() ); // may over reserve, but should be close enough

			unicode_char c;

			const_iterator i, ie = end();
			for ( i = begin(); i != ie; i.moveNext() )
			{
				c = i.getCharacter();
				buffer.push_back( c );
			}
		}

		mutable BufferType m_bufferType; // identifies the data type held in m_buffer
		mutable size_t m_bufferSize; // size of the CString buffer

		// multi-purpose buffer used everywhere we need a throw-away buffer
		union Buffer
		{
			mutable void* mVoidBuffer;
			mutable std::string* mStrBuffer;
			mutable std::wstring* mWStrBuffer;
			mutable utf32string* mUTF32StrBuffer;
		}
		m_buffer;
	};

	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, const UString& s2 )
	{
		return UString( s1 ).append( s2 );
	}
	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, UString::code_point c )
	{
		return UString( s1 ).append( 1, c );
	}
	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, UString::unicode_char c )
	{
		return UString( s1 ).append( 1, c );
	}
	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, char c )
	{
		return UString( s1 ).append( 1, c );
	}
#if MYGUI_IS_NATIVE_WCHAR_T
	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, wchar_t c )
	{
		return UString( s1 ).append( 1, c );
	}
#endif
	//! string addition operator \relates UString
	inline UString operator+( UString::code_point c, const UString& s2 )
	{
		return UString().append( 1, c ).append( s2 );
	}
	//! string addition operator \relates UString
	inline UString operator+( UString::unicode_char c, const UString& s2 )
	{
		return UString().append( 1, c ).append( s2 );
	}
	//! string addition operator \relates UString
	inline UString operator+( char c, const UString& s2 )
	{
		return UString().append( 1, c ).append( s2 );
	}
#if MYGUI_IS_NATIVE_WCHAR_T
	//! string addition operator \relates UString
	inline UString operator+( wchar_t c, const UString& s2 )
	{
		return UString().append( 1, c ).append( s2 );
	}
#endif

	// (const) forward iterator common operators
	inline UString::size_type operator-( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return ( left.mIter - right.mIter );
	}
	inline bool operator==( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return left.mIter == right.mIter;
	}
	inline bool operator!=( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return left.mIter != right.mIter;
	}
	inline bool operator<( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return left.mIter < right.mIter;
	}
	inline bool operator<=( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return left.mIter <= right.mIter;
	}
	inline bool operator>( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return left.mIter > right.mIter;
	}
	inline bool operator>=( const UString::_const_fwd_iterator& left, const UString::_const_fwd_iterator& right )
	{
		return left.mIter >= right.mIter;
	}

	// (const) reverse iterator common operators
	// NB: many of these operations are evaluated in reverse because this is a reverse iterator wrapping a forward iterator
	inline UString::size_type operator-( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return ( right.mIter - left.mIter );
	}
	inline bool operator==( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return left.mIter == right.mIter;
	}
	inline bool operator!=( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return left.mIter != right.mIter;
	}
	inline bool operator<( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return right.mIter < left.mIter;
	}
	inline bool operator<=( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return right.mIter <= left.mIter;
	}
	inline bool operator>( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return right.mIter > left.mIter;
	}
	inline bool operator>=( const UString::_const_rev_iterator& left, const UString::_const_rev_iterator& right )
	{
		return right.mIter >= left.mIter;
	}

	//! std::ostream write operator \relates UString
	inline std::ostream& operator << ( std::ostream& os, const UString& s )
	{
		return os << s.asUTF8();
	}

	//! std::wostream write operator \relates UString
	//inline std::wostream& operator << ( std::wostream& os, const UString& s )
	//{
	//	return os << s.asWStr();
	//}



}

#endif // __MYGUI_U_STRING_H__
