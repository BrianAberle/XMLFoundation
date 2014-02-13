// note: "short int" and "unsigned short int" are used natively now so assert that they are 16 bit wide when this code is compiled.
// note: Everywhere XMLFoundation builds, a short is 32 bit
// note: "long long" and "__int64" are equivalent. I don't like either notation.  I would have used int64 - realistically that had likely alreay been used in many places as a variable name by then, so I might have even used int777 if that was what it took.
//		  I dont like the underscores or the exception to C++ that one data type is two words.
//		  XMLFoundation standardizes on one ugly word, __int64, in all builds rather than define a 3'rd equivalent word.
//		  like the "Sparse Hash Project" source does for a 32 bit type called:  u_int16_t
// 
// note: The C++ struct is not supported in the "Sparsh Hash Project" build on Windows.  Is is supported in this build.  I am still researching about the C++ enum. 
// note: This build also added support for wchar_t in Windows - by replacing this code kindly commented with with an outdated technical note.
// 					#if defined(_MSC_VER)
//				 with
//					#if !defined(_MSC_VER) && _MSC_VER >= 1200   ||   !defined(_MSC_VER)
//
// note: this build has a 64 bit target for Windows - the "Sparsh Hash Project" does not.
//
// note: This build is technically more correct if the sole intent is to use only the sparse hash algorithm 
//		 because many #include<notused.h> that the sole algorithm of interest is not dependent on are removed.
//
//  these globals no longer exist
//	static const unsigned short int DEFAULT_SPARSEGROUP_SIZE = 48;
//	static const unsigned short DEFAULT_GROUP_SIZE = 48;
//
// The smaller this is, the faster lookup is (because the group bitmap is smaller) and the faster insert is, 
// because there's less to move. On the other hand, there are more groups.  Since group::size_type is a short, 
// this number should be of the form 32*x + 16 to avoid waste.  To change it search and replace "48" in this file.
// This was causing build problems because of the original arrangement of the files - and also allows the compiler
// to optimize more naturally.
//
// also note - i am aware this code compiles on Win32 and Win64 and not with G++.  I will look into that.  
// I got this far and decided I should let some folks see what has been done so far.
// This file is still a work in progress and will be until it is "Portable" - like every other file in the XMLFoundation
//
// there are a few more notes here and there in the source.  
//
// How about templatizing GHash and GBTree.  

#ifndef _SPARSEHASH_TABLE_
#define _SPARSEHASH_TABLE_

#include <assert.h>             // for bounds checking
#include <vector>               // a sparsetable is a vector of groups


// This source code makes an assumption about the application that is not true - 
// Its sort of like a compiler warning about doing pointer arithmetic with a type cast - the warning is wrong if the code is intentional and safe.
// but its actually much worse than than an erroneous warning because it fires off a debug interrupt and hangs the application.
// so the solution is to "remove" the debug asserts - but leave them in the source because they are meaningful in understanding the code
#undef  assert
#define assert(_Expression)     ((void)0)




namespace base 
{
	// The probing method
	// Linear probing
	// #define JUMP_(key, num_probes)    ( 1 )
	// Quadratic probing
	#define JUMP_(key, num_probes)    ( num_probes )

	// integral_constant, defined in tr1, is a wrapper for an integer
	// value. We don't really need this generality; we could get away
	// with hardcoding the integer type to bool. We use the fully
	// general integer_constant for compatibility with tr1.

	template<class T, T v> struct integral_constant 
	{  
		static const T value = v;
		typedef T value_type;
		typedef integral_constant<T, v> type;
	};
	template <class T, T v> const T integral_constant<T, v>::value;

	// Abbreviations: true_type and false_type are structs that represent boolean
	// true and false values. Also define the boost::mpl versions of those names
	// true_ and false_.
	typedef integral_constant<bool, true>  true_type;
	typedef integral_constant<bool, false> false_type;
	typedef true_type  true_;
	typedef false_type false_;

	// is_integral is false except for the built-in integer types. A
	// cv-qualified type is integral if and only if the underlying type is.
	template <class T> struct is_integral : false_type { };
	template<> struct is_integral<bool> : true_type { };
	template<> struct is_integral<char> : true_type { };
	template<> struct is_integral<unsigned char> : true_type { };
	template<> struct is_integral<signed char> : true_type { };
	
	// note: this says #if "anything other than the VC6 build"
	#if !defined(_MSC_VER) && _MSC_VER >= 1200     ||     !defined(_MSC_VER)
	   // In old Microsoft compilers wchar_t is not a native type. It is #defined as "unsigned short"
	    template<> struct is_integral<__wchar_t> : true_type { };
	#else
	   template<> struct is_integral<wchar_t> : true_type { };
	#endif

	template<> struct is_integral<short> : true_type { };
	template<> struct is_integral<unsigned short> : true_type { };
	template<> struct is_integral<int> : true_type { };
	template<> struct is_integral<unsigned int> : true_type { };
	template<> struct is_integral<long> : true_type { };
	template<> struct is_integral<unsigned long> : true_type { };
	template<> struct is_integral<__int64> : true_type { };
	template<> struct is_integral<unsigned __int64> : true_type { };

	// note: template class has already been defined as a non-template class -	template <class T> struct is_integral<const T> : is_integral<T> { };
	template <class T> struct is_integral<volatile T> : is_integral<T> { };
	template <class T> struct is_integral<const volatile T> : is_integral<T> { };

	// is_floating_point is false except for the built-in floating-point types.
	// A cv-qualified type is integral if and only if the underlying type is.
	template <class T> struct is_floating_point				: false_type { };
	template<> struct is_floating_point<float>				: true_type { };
	template<> struct is_floating_point<double>				: true_type { };
	template<> struct is_floating_point<long double>		: true_type { };

	template <class T> struct is_floating_point<const T>	: is_floating_point<T> { };
	template <class T> struct is_floating_point<volatile T>	: is_floating_point<T> { };
	template <class T> struct is_floating_point<const volatile T> : is_floating_point<T> { };

	// is_pointer is false except for pointer types. A cv-qualified type (e.g.
	// "int* const", as opposed to "int const*") is cv-qualified if and only if
	// the underlying type is.
	template <class T> struct is_pointer : false_type { };
	template <class T> struct is_pointer<T*> : true_type { };
	template <class T> struct is_pointer<const T> : is_pointer<T> { };
	template <class T> struct is_pointer<volatile T> : is_pointer<T> { };
	template <class T> struct is_pointer<const volatile T> : is_pointer<T> { };

	// is_reference is false except for reference types.
	template<typename T> struct is_reference : false_type {};
	template<typename T> struct is_reference<T&> : true_type {};

	// We can't get is_pod right without compiler help, so fail conservatively.
	// We will assume it's false except for arithmetic types, enumerations,
	// pointers and cv-qualified versions thereof. Note that std::pair<T,U>
	// is not a POD even if T and U are PODs.

#if defined(_MSC_VER) && _MSC_VER <= 1200
// Old microsoft compilers
template <class T> struct is_pod : integral_constant<bool, (is_integral<T>::value ||                       is_floating_point<T>::value						)> { };
#else
	#if !(defined(__GNUC__) && __GNUC__ <= 3)
		#ifdef _MSC_VER
// New Microsoft compilers
template <class T> struct is_pod : integral_constant<bool, (is_integral<T>::value ||                       is_floating_point<T>::value || is_pointer<T>::value)> { };
		#else
// all other builds
template <class T> struct is_pod : integral_constant<bool, (is_integral<T>::value ||  is_enum<T>::value || is_floating_point<T>::value || is_pointer<T>::value)> { };
		#endif
	#else
// GCC <  version 3
template <class T> struct is_pod : integral_constant<bool, (is_integral<T>::value ||                       is_floating_point<T>::value || is_pointer<T>::value)> { };
	#endif
#endif


	template <class T> struct is_pod<const T>			: is_pod<T> { };
	template <class T> struct is_pod<volatile T>		: is_pod<T> { };
	template <class T> struct is_pod<const volatile T>	: is_pod<T> { };


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// This applies to the following 4 blocks of code
	// We can't get has_trivial_constructor right without compiler help, so fail conservatively. 
	// We will assume it's false except for: (1) types for which is_pod is true. (2) std::pair of types with trivial constructors. 
	// (3) array of a type with a trivial constructor. (4) const versions thereof.
	template <class T> struct has_trivial_constructor							  : is_pod<T> { };
	template <class T, class U> struct has_trivial_constructor<std::pair<T, U> >  : integral_constant<bool, (has_trivial_constructor<T>::value && has_trivial_constructor<U>::value)> { };
	template <class A, int N> struct has_trivial_constructor<A[N]>				  : has_trivial_constructor<A> { };
	template <class T> struct has_trivial_constructor<const T>					  : has_trivial_constructor<T> { };
	//
	template <class T> struct has_trivial_copy									: is_pod<T> { };
	template <class T, class U> struct has_trivial_copy<std::pair<T, U> >		: integral_constant<bool, (has_trivial_copy<T>::value && has_trivial_copy<U>::value)> { };
	template <class A, int N> struct has_trivial_copy<A[N]>						: has_trivial_copy<A> { };
	template <class T> struct has_trivial_copy<const T>							: has_trivial_copy<T> { };
	//
	// note : this block has no 'const' - case (4)
	template <class T> struct has_trivial_assign								: is_pod<T> { };
	template <class T, class U> struct has_trivial_assign<std::pair<T, U> >		: integral_constant<bool, (has_trivial_assign<T>::value &&   has_trivial_assign<U>::value)> { };
	template <class A, int N> struct has_trivial_assign<A[N]>					: has_trivial_assign<A> { };
	//
	template <class T> struct has_trivial_destructor							: is_pod<T> { };
	template <class T, class U> struct has_trivial_destructor<std::pair<T, U> >	 : integral_constant<bool,  (has_trivial_destructor<T>::value &&  has_trivial_destructor<U>::value)> { };
	template <class A, int N> struct has_trivial_destructor<A[N]>				 : has_trivial_destructor<A> { };
	template <class T> struct has_trivial_destructor<const T>					 : has_trivial_destructor<T> { };
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template<class T>	 struct remove_const;
	template<typename T> struct remove_const { typedef T type; };
	template<typename T> struct remove_const<T const> { typedef T type; };
	template<typename T> struct remove_volatile { typedef T type; };
	template<typename T> struct remove_volatile<T volatile> { typedef T type; };
	template<typename T> struct remove_cv {  typedef typename remove_const<typename remove_volatile<T>::type>::type type; };

	// Specified by TR1 [4.6] Relationships between types
	template<typename T, typename U> struct is_same : public false_type { };
	template<typename T> struct is_same<T, T> : public true_type { };


	// Specified by TR1 [4.5.1] primary type categories.

	// Implementation note:
	//
	// Each type is either void, integral, floating point, array, pointer,
	// reference, member object pointer, member function pointer, enum,
	// union or class. Out of these, only integral, floating point, reference,
	// class and enum types are potentially convertible to int. Therefore,
	// if a type is not a reference, integral, floating point or class and
	// is convertible to int, it's a enum. Adding cv-qualification to a type
	// does not change whether it's an enum.
	//
	// Is-convertible-to-int check is done only if all other checks pass,
	// because it can't be used with some types (e.g. void or classes with
	// inaccessible conversion operators).
#ifndef _WIN32
	template <class T> struct is_enum  internal::is_enum_impl< is_same<T, void>::value || is_integral<T>::value || is_floating_point<T>::value || is_reference<T>::value || internal::is_class_or_union<T>::value, T> { };
	template <class T> struct is_enum<const T>			: is_enum<T> { };
	template <class T> struct is_enum<volatile T>		: is_enum<T> { };
	template <class T> struct is_enum<const volatile T> : is_enum<T> { };
#endif

}

//namespace Stroustrup
//{
	using base::true_type;
	using base::false_type;
	using base::integral_constant;
	using base::has_trivial_destructor;
	using base::is_same;

	namespace sparsehash_internal 
	{
		template<typename Ignored>
		inline bool read_data_internal(Ignored*, FILE* fp,
									   void* data, size_t length) {
		  return fread(data, length, 1, fp) == 1;
		}

		template<typename Ignored>
		inline bool write_data_internal(Ignored*, FILE* fp,
										const void* data, size_t length) {
		  return fwrite(data, length, 1, fp) == 1;
		}

		// ----- low-level I/O for iostream ----

		// We want the caller to be responsible for #including <iostream>, not
		// us, because iostream is a big header!  According to the standard,
		// it's only legal to delay the instantiation the way we want to if
		// the istream/ostream is a template type.  So we jump through hoops.
		template<typename ISTREAM>
		inline bool read_data_internal_for_istream(ISTREAM* fp,
												   void* data, size_t length) {
		  return fp->read(reinterpret_cast<char*>(data), length).good();
		}
		template<typename Ignored>
		inline bool read_data_internal(Ignored*, std::istream* fp,
									   void* data, size_t length) {
		  return read_data_internal_for_istream(fp, data, length);
		}

		template<typename OSTREAM>
		inline bool write_data_internal_for_ostream(OSTREAM* fp,
													const void* data, size_t length) {
		  return fp->write(reinterpret_cast<const char*>(data), length).good();
		}
		template<typename Ignored>
		inline bool write_data_internal(Ignored*, std::ostream* fp,
										const void* data, size_t length) {
		  return write_data_internal_for_ostream(fp, data, length);
		}

		// ----- low-level I/O for custom streams ----

		// The INPUT type needs to support a Read() method that takes a
		// buffer and a length and returns the number of bytes read.
		template <typename INPUT>
		inline bool read_data_internal(INPUT* fp, void*,
									   void* data, size_t length) {
		  return static_cast<size_t>(fp->Read(data, length)) == length;
		}

		// The OUTPUT type needs to support a Write() operation that takes
		// a buffer and a length and returns the number of bytes written.
		template <typename OUTPUT>
		inline bool write_data_internal(OUTPUT* fp, void*,
										const void* data, size_t length) {
		  return static_cast<size_t>(fp->Write(data, length)) == length;
		}

		// ----- low-level I/O: the public API ----

		template <typename INPUT>
		inline bool read_data(INPUT* fp, void* data, size_t length) {
		  return read_data_internal(fp, fp, data, length);
		}

		template <typename OUTPUT>
		inline bool write_data(OUTPUT* fp, const void* data, size_t length) {
		  return write_data_internal(fp, fp, data, length);
		}

		// Uses read_data() and write_data() to read/write an integer.
		// length is the number of bytes to read/write (which may differ
		// from sizeof(IntType), allowing us to save on a 32-bit system
		// and load on a 64-bit system).  Excess bytes are taken to be 0.
		// INPUT and OUTPUT must match legal inputs to read/write_data (above).
		template <typename INPUT, typename IntType>
		bool read_bigendian_number(INPUT* fp, IntType* value, size_t length) {
		  *value = 0;
		  unsigned char byte;
		  // We require IntType to be unsigned or else the shifting gets all screwy.
		  SPARSEHASH_COMPILE_ASSERT(static_cast<IntType>(-1) > static_cast<IntType>(0),
									serializing_int_requires_an_unsigned_type);
		  for (size_t i = 0; i < length; ++i) {
			if (!read_data(fp, &byte, sizeof(byte))) return false;
			*value |= static_cast<IntType>(byte) << ((length - 1 - i) * 8);
		  }
		  return true;
		}

		template <typename OUTPUT, typename IntType>
		bool write_bigendian_number(OUTPUT* fp, IntType value, size_t length) {
		  unsigned char byte;
		  // We require IntType to be unsigned or else the shifting gets all screwy.
		  SPARSEHASH_COMPILE_ASSERT(static_cast<IntType>(-1) > static_cast<IntType>(0),
									serializing_int_requires_an_unsigned_type);
		  for (size_t i = 0; i < length; ++i) {
			byte = (sizeof(value) <= length-1 - i)
				? 0 : static_cast<unsigned char>((value >> ((length-1 - i) * 8)) & 255);
			if (!write_data(fp, &byte, sizeof(byte))) return false;
		  }
		  return true;
		}

		// If your keys and values are simple enough, you can pass this
		// serializer to serialize()/unserialize().  "Simple enough" means
		// value_type is a POD type that contains no pointers.  Note,
		// however, we don't try to normalize endianness.
		// This is the type used for NopointerSerializer.
		template <typename value_type> struct pod_serializer {
		  template <typename INPUT>
		  bool operator()(INPUT* fp, value_type* value) const {
			return read_data(fp, value, sizeof(*value));
		  }

		  template <typename OUTPUT>
		  bool operator()(OUTPUT* fp, const value_type& value) const {
			return write_data(fp, &value, sizeof(value));
		  }
		};


		// Settings contains parameters for growing and shrinking the table.
		// It also packages zero-size functor (ie. hasher).
		//
		// It does some munging of the hash value in cases where we think
		// (fear) the original hash function might not be very good.  In
		// particular, the default hash of pointers is the identity hash,
		// so probably all the low bits are 0.  We identify when we think
		// we're hashing a pointer, and chop off the low bits.  Note this
		// isn't perfect: even when the key is a pointer, we can't tell
		// for sure that the hash is the identity hash.  If it's not, this
		// is needless work (and possibly, though not likely, harmful).

		template<typename Key, typename HashFunc,	 typename SizeType, int HT_MIN_BUCKETS>	class sh_hashtable_settings : public HashFunc 
		{
		 public:
		  typedef Key key_type;
		  typedef HashFunc hasher;
		  typedef SizeType size_type;

		 public:
		  sh_hashtable_settings(const hasher& hf,
								const float ht_occupancy_flt,
								const float ht_empty_flt)
			  : hasher(hf),
				enlarge_threshold_(0),
				shrink_threshold_(0),
				consider_shrink_(false),
				use_empty_(false),
				use_deleted_(false),
				num_ht_copies_(0) {
			set_enlarge_factor(ht_occupancy_flt);
			set_shrink_factor(ht_empty_flt);
		  }

		  size_type hash(const key_type& v) const {
			// We munge the hash value when we don't trust hasher::operator().
			return hash_munger<Key>::MungedHash(hasher::operator()(v));
		  }

		  float enlarge_factor() const {
			return enlarge_factor_;
		  }
		  void set_enlarge_factor(float f) {
			enlarge_factor_ = f;
		  }
		  float shrink_factor() const {
			return shrink_factor_;
		  }
		  void set_shrink_factor(float f) {
			shrink_factor_ = f;
		  }

		  size_type enlarge_threshold() const {
			return enlarge_threshold_;
		  }
		  void set_enlarge_threshold(size_type t) {
			enlarge_threshold_ = t;
		  }
		  size_type shrink_threshold() const {
			return shrink_threshold_;
		  }
		  void set_shrink_threshold(size_type t) {
			shrink_threshold_ = t;
		  }

		  size_type enlarge_size(size_type x) const {
			return static_cast<size_type>(x * enlarge_factor_);
		  }
		  size_type shrink_size(size_type x) const {
			return static_cast<size_type>(x * shrink_factor_);
		  }

		  bool consider_shrink() const {
			return consider_shrink_;
		  }
		  void set_consider_shrink(bool t) {
			consider_shrink_ = t;
		  }

		  bool use_empty() const {
			return use_empty_;
		  }
		  void set_use_empty(bool t) {
			use_empty_ = t;
		  }

		  bool use_deleted() const {
			return use_deleted_;
		  }
		  void set_use_deleted(bool t) {
			use_deleted_ = t;
		  }

		  size_type num_ht_copies() const {
			return static_cast<size_type>(num_ht_copies_);
		  }
		  void inc_num_ht_copies() {
			++num_ht_copies_;
		  }

		  // Reset the enlarge and shrink thresholds
		  void reset_thresholds(size_type num_buckets) {
			set_enlarge_threshold(enlarge_size(num_buckets));
			set_shrink_threshold(shrink_size(num_buckets));
			// whatever caused us to reset already considered
			set_consider_shrink(false);
		  }

		  // Caller is resposible for calling reset_threshold right after
		  // set_resizing_parameters.
		  void set_resizing_parameters(float shrink, float grow) {
			assert(shrink >= 0.0);
			assert(grow <= 1.0);
			if (shrink > grow/2.0f)
			  shrink = grow / 2.0f;     // otherwise we thrash hashtable size
			set_shrink_factor(shrink);
			set_enlarge_factor(grow);
		  }

		  // This is the smallest size a hashtable can be without being too crowded
		  // If you like, you can give a min #buckets as well as a min #elts
		  size_type min_buckets(size_type num_elts, size_type min_buckets_wanted) {
			float enlarge = enlarge_factor();
			size_type sz = HT_MIN_BUCKETS;             // min buckets allowed
			while ( sz < min_buckets_wanted ||
					num_elts >= static_cast<size_type>(sz * enlarge) ) {
			  // This just prevents overflowing size_type, since sz can exceed
			  // max_size() here.
			  if (static_cast<size_type>(sz * 2) < sz) {
				throw std::length_error("resize overflow");  // protect against overflow
			  }
			  sz *= 2;
			}
			return sz;
		  }

		 private:
		  template<class HashKey> class hash_munger {
		   public:
			static size_t MungedHash(size_t hash) {
			  return hash;
			}
		  };
		  // This matches when the hashtable key is a pointer.
		  template<class HashKey> class hash_munger<HashKey*> {
		   public:
			static size_t MungedHash(size_t hash) {
			  // TODO(csilvers): consider rotating instead:
			  //    static const int shift = (sizeof(void *) == 4) ? 2 : 3;
			  //    return (hash << (sizeof(hash) * 8) - shift)) | (hash >> shift);
			  // This matters if we ever change sparse/dense_hash_* to compare
			  // hashes before comparing actual values.  It's speedy on x86.
			  return hash / sizeof(void*);   // get rid of known-0 bits
			}
		  };

		  size_type enlarge_threshold_;  // table.size() * enlarge_factor
		  size_type shrink_threshold_;   // table.size() * shrink_factor
		  float enlarge_factor_;         // how full before resize
		  float shrink_factor_;          // how empty before resize
		  // consider_shrink=true if we should try to shrink before next insert
		  bool consider_shrink_;
		  bool use_empty_;    // used only by densehashtable, not sparsehashtable
		  bool use_deleted_;  // false until delkey has been set
		  // num_ht_copies is a counter incremented every Copy/Move
		  unsigned int num_ht_copies_;
		};
		
	} // END: namespace sparsehash_internal



	template<class T>
	class libc_allocator_with_realloc {
	 public:
	  typedef T value_type;
	  typedef size_t size_type;
	  typedef ptrdiff_t difference_type;

	  typedef T* pointer;
	  typedef const T* const_pointer;
	  typedef T& reference;
	  typedef const T& const_reference;

	  libc_allocator_with_realloc() {}
	  libc_allocator_with_realloc(const libc_allocator_with_realloc&) {}
	  ~libc_allocator_with_realloc() {}

	  pointer address(reference r) const  { return &r; }
	  const_pointer address(const_reference r) const  { return &r; }

	  pointer allocate(size_type n, const_pointer = 0) {
		return static_cast<pointer>(malloc(n * sizeof(value_type)));
	  }
	  void deallocate(pointer p, size_type) {
		free(p);
	  }
	  pointer reallocate(pointer p, size_type n) {
		return static_cast<pointer>(realloc(p, n * sizeof(value_type)));
	  }

	  size_type max_size() const  {
		return static_cast<size_type>(-1) / sizeof(value_type);
	  }

	  void construct(pointer p, const value_type& val) {
		new(p) value_type(val);
	  }
	  void destroy(pointer p) { p->~value_type(); }

	  template <class U> libc_allocator_with_realloc(const libc_allocator_with_realloc<U>&) {}

	  template<class U>
	  struct rebind {
		typedef libc_allocator_with_realloc<U> other;
	  };
	};

	// libc_allocator_with_realloc<void> specialization.
	template<>
	class libc_allocator_with_realloc<void> {
	 public:
	  typedef void value_type;
	  typedef size_t size_type;
	  typedef ptrdiff_t difference_type;
	  typedef void* pointer;
	  typedef const void* const_pointer;

	  template<class U>
	  struct rebind {
		typedef libc_allocator_with_realloc<U> other;
	  };
	};

	template<class T>
	inline bool operator==(const libc_allocator_with_realloc<T>&,
						   const libc_allocator_with_realloc<T>&) {
	  return true;
	}

	template<class T>
	inline bool operator!=(const libc_allocator_with_realloc<T>&,
						   const libc_allocator_with_realloc<T>&) {
	  return false;
	}





// Hashtable class, used to implement the hashed associative containers
// hash_set and hash_map.
//
// Value: what is stored in the table (each bucket is a Value).
// Key: something in a 1-to-1 correspondence to a Value, that can be used
//      to search for a Value in the table (find() takes a Key).
// HashFcn: Takes a Key and returns an integer, the more unique the better.
// ExtractKey: given a Value, returns the unique Key associated with it.
//             Must inherit from unary_function, or at least have a
//             result_type enum indicating the return type of operator().
// SetKey: given a Value* and a Key, modifies the value such that
//         ExtractKey(value) == key.  We guarantee this is only called
//         with key == deleted_key.
// EqualKey: Given two Keys, says whether they are the same (that is,
//           if they are both associated with the same Value).
// Alloc: STL allocator to use to allocate memory.

template <class T, unsigned short int GROUP_SIZE = 48, class Alloc = libc_allocator_with_realloc<T>>			class sparsetable;
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A>								struct sparse_hashtable_destructive_iterator; 
template <class Value, class Key, class HashFcn, class ExtractKey, class SetKey, class EqualKey, class Alloc>	class sparse_hashtable;

// And once again, but this time freeing up memory as we iterate
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> struct sparse_hashtable_destructive_iterator 
{
 private:
  typedef typename A::template rebind<V>::other value_alloc_type;

 public:
  typedef sparse_hashtable_destructive_iterator<V,K,HF,ExK,SetK,EqK,A>	iterator;
  typedef typename sparsetable<V,48,A>::destructive_iterator			st_iterator;

  typedef std::forward_iterator_tag iterator_category;  // very little defined!
  typedef V value_type;
  typedef typename value_alloc_type::difference_type difference_type;
  typedef typename value_alloc_type::size_type size_type;
  typedef typename value_alloc_type::reference reference;
  typedef typename value_alloc_type::pointer pointer;

  // "Real" constructor and default constructor
  sparse_hashtable_destructive_iterator(const  sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> *h, st_iterator it, st_iterator it_end)
    : ht(h), pos(it), end(it_end)   { advance_past_deleted(); }
  sparse_hashtable_destructive_iterator() { }          // never used internally
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const { return *pos; }
  pointer operator->() const { return &(operator*()); }

  // Arithmetic.  The only hard part is making sure that
  // we're not on a marked-deleted array element
  void advance_past_deleted() {
    while ( pos != end && ht->test_deleted(*this) )
      ++pos;
  }
  iterator& operator++()   {
    assert(pos != end); ++pos; advance_past_deleted(); return *this;
  }
  iterator operator++(int) { iterator tmp(*this); ++*this; return tmp; }

  // Comparison.
  bool operator==(const iterator& it) const { return pos == it.pos; }
  bool operator!=(const iterator& it) const { return pos != it.pos; }


  // The actual data
  const sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> *ht;
  st_iterator pos, end;
};


template <class V, class K, class HF, class ExK, class SetK, class EqK, class A>	struct sparse_hashtable_const_iterator; 
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A>	struct sparse_hashtable_iterator; 

template <class V, class K, class HF, class ExK, class SetK, class EqK, class A>	struct sparse_hashtable_const_iterator 
{
 private:
  typedef typename A::template rebind<V>::other value_alloc_type;

 public:
  typedef sparse_hashtable_iterator<V,K,HF,ExK,SetK,EqK,A>       iterator;
  typedef sparse_hashtable_const_iterator<V,K,HF,ExK,SetK,EqK,A>	const_iterator;
  typedef typename sparsetable<V,48,A>::const_nonempty_iterator    st_iterator;

  typedef std::forward_iterator_tag iterator_category;  // very little defined!
  typedef V value_type;
  typedef typename value_alloc_type::difference_type difference_type;
  typedef typename value_alloc_type::size_type size_type;
  typedef typename value_alloc_type::const_reference reference;
  typedef typename value_alloc_type::const_pointer pointer;

  // "Real" constructor and default constructor
  sparse_hashtable_const_iterator(const sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> *h,  st_iterator it, st_iterator it_end)  : ht(h), pos(it), end(it_end)   
  { 
	  advance_past_deleted(); 
  }
  // This lets us convert regular iterators to const iterators
  sparse_hashtable_const_iterator() { }      // never used internally
  sparse_hashtable_const_iterator(const iterator &it)
    : ht(it.ht), pos(it.pos), end(it.end) { }
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const { return *pos; }
  pointer operator->() const { return &(operator*()); }

  // Arithmetic.  The only hard part is making sure that
  // we're not on a marked-deleted array element
  void advance_past_deleted() {
    while ( pos != end && ht->test_deleted(*this) )
      ++pos;
  }
  const_iterator& operator++() {
    assert(pos != end); ++pos; advance_past_deleted(); return *this;
  }
  const_iterator operator++(int) { const_iterator tmp(*this); ++*this; return tmp; }

  // Comparison.
  bool operator==(const const_iterator& it) const { return pos == it.pos; }
  bool operator!=(const const_iterator& it) const { return pos != it.pos; }


  // The actual data
  const sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> *ht;
  st_iterator pos, end;
};


template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> struct sparse_hashtable_iterator; 
 
 // As far as iterating, we're basically just a sparsetable that skips over deleted elements.
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> struct sparse_hashtable_iterator 
{
 private:
  typedef typename A::template rebind<V>::other value_alloc_type;

 public:
  typedef sparse_hashtable_iterator<V,K,HF,ExK,SetK,EqK,A>			iterator;
  typedef sparse_hashtable_const_iterator<V,K,HF,ExK,SetK,EqK,A> const_iterator;
  typedef typename sparsetable<V,48,A>::nonempty_iterator		    st_iterator;


  typedef std::forward_iterator_tag iterator_category;  // very little defined!
  typedef V value_type;
  typedef typename value_alloc_type::difference_type difference_type;
  typedef typename value_alloc_type::size_type size_type;
  typedef typename value_alloc_type::reference reference;
  typedef typename value_alloc_type::pointer pointer;

  // "Real" constructor and default constructor.
  sparse_hashtable_iterator(const sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> *h, st_iterator it, st_iterator it_end)  
	  : ht(h), pos(it), end(it_end)   
  { 
	  advance_past_deleted(); 
  }
  sparse_hashtable_iterator() { }      // not ever used internally
  // The default destructor is fine; we don't define one
  // The default operator= is fine; we don't define one

  // Happy dereferencer
  reference operator*() const { return *pos; }
  pointer operator->() const { return &(operator*()); }

  // Arithmetic.  The only hard part is making sure that
  // we're not on a marked-deleted array element
  void advance_past_deleted() {
    while ( pos != end && ht->test_deleted(*this) )
      ++pos;
  }
  iterator& operator++()   {
    assert(pos != end); ++pos; advance_past_deleted(); return *this;
  }
  iterator operator++(int) { iterator tmp(*this); ++*this; return tmp; }

  // Comparison.
  bool operator==(const iterator& it) const { return pos == it.pos; }
  bool operator!=(const iterator& it) const { return pos != it.pos; }


  // The actual data
  const sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> *ht;
  st_iterator pos, end;
};



	// Our iterator as simple as iterators can be: basically it's just
	// the index into our table.  Dereference, the only complicated
	// thing, we punt to the table class.  This just goes to show how
	// much machinery STL requires to do even the most trivial tasks.
	//
	// By templatizing over tabletype, we have one iterator type which
	// we can use for both sparsetables and sparsebins.  In fact it
	// works on any class that allows size() and operator[] (eg vector),
	// as long as it does the standard STL typedefs too (eg value_type).

	template <class tabletype> class table_element_adaptor;

	template <class tabletype>	class table_iterator 
	{
	 public:
	  typedef table_iterator iterator;

	  typedef std::random_access_iterator_tag iterator_category;
	  typedef typename tabletype::value_type value_type;
	  typedef typename tabletype::difference_type difference_type;
	  typedef typename tabletype::size_type size_type;
	  typedef table_element_adaptor<tabletype> reference;
	  typedef table_element_adaptor<tabletype>* pointer;

	  // The "real" constructor
	  table_iterator(tabletype *tbl, size_type p)
		: table(tbl), pos(p) { }
	  // The default constructor, used when I define vars of type table::iterator
	  table_iterator() : table(NULL), pos(0) { }
	  // The copy constructor, for when I say table::iterator foo = tbl.begin()
	  // The default destructor is fine; we don't define one
	  // The default operator= is fine; we don't define one

	  // The main thing our iterator does is dereference.  If the table entry
	  // we point to is empty, we return the default value type.
	  // This is the big different function from the const iterator.
	  reference operator*()              {
		return table_element_adaptor<tabletype>(table, pos);
	  }
	  pointer operator->()               { return &(operator*()); }

	  // Helper function to assert things are ok; eg pos is still in range
	  void check() const {
		assert(table);
		assert(pos <= table->size());
	  }

	  // Arithmetic: we just do arithmetic on pos.  We don't even need to
	  // do bounds checking, since STL doesn't consider that its job.  :-)
	  iterator& operator+=(size_type t) { pos += t; check(); return *this; }
	  iterator& operator-=(size_type t) { pos -= t; check(); return *this; }
	  iterator& operator++()            { ++pos; check(); return *this; }
	  iterator& operator--()            { --pos; check(); return *this; }
	  iterator operator++(int)          { iterator tmp(*this);     // for x++
										  ++pos; check(); return tmp; }
	  iterator operator--(int)          { iterator tmp(*this);     // for x--
										  --pos; check(); return tmp; }
	  iterator operator+(difference_type i) const  { iterator tmp(*this);
													 tmp += i; return tmp; }
	  iterator operator-(difference_type i) const  { iterator tmp(*this);
													 tmp -= i; return tmp; }
	  difference_type operator-(iterator it) const {      // for "x = it2 - it"
		assert(table == it.table);
		return pos - it.pos;
	  }
	  reference operator[](difference_type n) const {
		return *(*this + n);            // simple though not totally efficient
	  }

	  // Comparisons.
	  bool operator==(const iterator& it) const {
		return table == it.table && pos == it.pos;
	  }
	  bool operator<(const iterator& it) const {
		assert(table == it.table);              // life is bad bad bad otherwise
		return pos < it.pos;
	  }
	  bool operator!=(const iterator& it) const { return !(*this == it); }
	  bool operator<=(const iterator& it) const { return !(it < *this); }
	  bool operator>(const iterator& it) const { return it < *this; }
	  bool operator>=(const iterator& it) const { return !(*this < it); }

	  // Here's the info we actually need to be an iterator
	  tabletype *table;              // so we can dereference and bounds-check
	  size_type pos;                 // index into the table
	};


template <class Value, class Key, class HashFcn, class ExtractKey, class SetKey, class EqualKey, class Alloc> class sparse_hashtable 
{
 private:
  typedef typename Alloc::template rebind<Value>::other value_alloc_type;

 public:
  typedef Key key_type;
  typedef Value value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;
  typedef Alloc allocator_type;

  typedef typename value_alloc_type::size_type size_type;
  typedef typename value_alloc_type::difference_type difference_type;
  typedef typename value_alloc_type::reference reference;
  typedef typename value_alloc_type::const_reference const_reference;
  typedef typename value_alloc_type::pointer pointer;
  typedef typename value_alloc_type::const_pointer const_pointer;
  typedef sparse_hashtable_iterator<Value, Key, HashFcn, ExtractKey, SetKey, EqualKey, Alloc>				iterator;
  typedef sparse_hashtable_const_iterator<Value, Key, HashFcn, ExtractKey, SetKey, EqualKey, Alloc>			const_iterator;
  typedef sparse_hashtable_destructive_iterator<Value, Key, HashFcn, ExtractKey,SetKey, EqualKey, Alloc>	destructive_iterator;

  // These come from tr1.  For us they're the same as regular iterators.
  typedef iterator local_iterator;
  typedef const_iterator const_local_iterator;

  // How full we let the table get before we resize, by default.
  // Knuth says .8 is good -- higher causes us to probe too much,
  // though it saves memory.
  static const int HT_OCCUPANCY_PCT; // = 80 (out of 100);

  // How empty we let the table get before we resize lower, by default.
  // (0.0 means never resize lower.)
  // It should be less than OCCUPANCY_PCT / 2 or we thrash resizing
  static const int HT_EMPTY_PCT; // = 0.4 * HT_OCCUPANCY_PCT;

  // Minimum size we're willing to let hashtables be.
  // Must be a power of two, and at least 4.
  // Note, however, that for a given hashtable, the initial size is a
  // function of the first constructor arg, and may be >HT_MIN_BUCKETS.
  static const size_type HT_MIN_BUCKETS = 4;

  // By default, if you don't specify a hashtable size at
  // construction-time, we use this size.  Must be a power of two, and
  // at least HT_MIN_BUCKETS.
  static const size_type HT_DEFAULT_STARTING_BUCKETS = 32;

  // ITERATOR FUNCTIONS
  iterator begin()             { return iterator(this, table.nonempty_begin(),
                                                 table.nonempty_end()); }
  iterator end()               { return iterator(this, table.nonempty_end(),
                                                 table.nonempty_end()); }
  const_iterator begin() const { return const_iterator(this,
                                                       table.nonempty_begin(),
                                                       table.nonempty_end()); }
  const_iterator end() const   { return const_iterator(this,
                                                       table.nonempty_end(),
                                                       table.nonempty_end()); }

  // These come from tr1 unordered_map.  They iterate over 'bucket' n.
  // For sparsehashtable, we could consider each 'group' to be a bucket,
  // I guess, but I don't really see the point.  We'll just consider
  // bucket n to be the n-th element of the sparsetable, if it's occupied,
  // or some empty element, otherwise.
  local_iterator begin(size_type i) {
    if (table.test(i))
      return local_iterator(this, table.get_iter(i), table.nonempty_end());
    else
      return local_iterator(this, table.nonempty_end(), table.nonempty_end());
  }
  local_iterator end(size_type i) {
    local_iterator it = begin(i);
    if (table.test(i) && !test_deleted(i))
      ++it;
    return it;
  }
  const_local_iterator begin(size_type i) const {
    if (table.test(i))
      return const_local_iterator(this, table.get_iter(i),
                                  table.nonempty_end());
    else
      return const_local_iterator(this, table.nonempty_end(),
                                  table.nonempty_end());
  }
  const_local_iterator end(size_type i) const {
    const_local_iterator it = begin(i);
    if (table.test(i) && !test_deleted(i))
      ++it;
    return it;
  }

  // This is used when resizing
  destructive_iterator destructive_begin() {
    return destructive_iterator(this, table.destructive_begin(),
                                table.destructive_end());
  }
  destructive_iterator destructive_end() {
    return destructive_iterator(this, table.destructive_end(),
                                table.destructive_end());
  }


  // ACCESSOR FUNCTIONS for the things we templatize on, basically
  hasher hash_funct() const               { return settings; }
  key_equal key_eq() const                { return key_info; }
  allocator_type get_allocator() const    { return table.get_allocator(); }

  // Accessor function for statistics gathering.
  int num_table_copies() const { return settings.num_ht_copies(); }

 private:
  // We need to copy values when we set the special marker for deleted
  // elements, but, annoyingly, we can't just use the copy assignment
  // operator because value_type might not be assignable (it's often
  // pair<const X, Y>).  We use explicit destructor invocation and
  // placement new to get around this.  Arg.
  void set_value(pointer dst, const_reference src) {
    dst->~value_type();   // delete the old value, if any
    new(dst) value_type(src);
  }

  // This is used as a tag for the copy constructor, saying to destroy its
  // arg We have two ways of destructively copying: with potentially growing
  // the hashtable as we copy, and without.  To make sure the outside world
  // can't do a destructive copy, we make the typename private.
  enum MoveDontCopyT {MoveDontCopy, MoveDontGrow};

  // DELETE HELPER FUNCTIONS
  // This lets the user describe a key that will indicate deleted
  // table entries.  This key should be an "impossible" entry --
  // if you try to insert it for real, you won't be able to retrieve it!
  // (NB: while you pass in an entire value, only the key part is looked
  // at.  This is just because I don't know how to assign just a key.)
 private:
  void squash_deleted() {           // gets rid of any deleted entries we have
    if ( num_deleted ) {            // get rid of deleted before writing
      sparse_hashtable tmp(MoveDontGrow, *this);
      swap(tmp);                    // now we are tmp
    }
    assert(num_deleted == 0);
  }

  // Test if the given key is the deleted indicator.  Requires
  // num_deleted > 0, for correctness of read(), and because that
  // guarantees that key_info.delkey is valid.
  bool test_deleted_key(const key_type& key) const {
    assert(num_deleted > 0);
    return equals(key_info.delkey, key);
  }

 public:
  void set_deleted_key(const key_type &key) {
    // It's only safe to change what "deleted" means if we purge deleted guys
    squash_deleted();
    settings.set_use_deleted(true);
    key_info.delkey = key;
  }
  void clear_deleted_key() {
    squash_deleted();
    settings.set_use_deleted(false);
  }
  key_type deleted_key() const {
    assert(settings.use_deleted()
           && "Must set deleted key before calling deleted_key");
    return key_info.delkey;
  }

  // These are public so the iterators can use them
  // True if the item at position bucknum is "deleted" marker
  bool test_deleted(size_type bucknum) const {
    // Invariant: !use_deleted() implies num_deleted is 0.
    assert(settings.use_deleted() || num_deleted == 0);
    return num_deleted > 0 && table.test(bucknum) &&
        test_deleted_key(get_key(table.unsafe_get(bucknum)));
  }
  bool test_deleted(const iterator &it) const {
    // Invariant: !use_deleted() implies num_deleted is 0.
    assert(settings.use_deleted() || num_deleted == 0);
    return num_deleted > 0 && test_deleted_key(get_key(*it));
  }
  bool test_deleted(const const_iterator &it) const {
    // Invariant: !use_deleted() implies num_deleted is 0.
    assert(settings.use_deleted() || num_deleted == 0);
    return num_deleted > 0 && test_deleted_key(get_key(*it));
  }
  bool test_deleted(const destructive_iterator &it) const {
    // Invariant: !use_deleted() implies num_deleted is 0.
    assert(settings.use_deleted() || num_deleted == 0);
    return num_deleted > 0 && test_deleted_key(get_key(*it));
  }

 private:
  void check_use_deleted(const char* caller) {
    (void)caller;    // could log it if the assert failed
    assert(settings.use_deleted());
  }

  // Set it so test_deleted is true.  true if object didn't used to be deleted.
  // TODO(csilvers): make these private (also in densehashtable.h)
  bool set_deleted(iterator &it) {
    check_use_deleted("set_deleted()");
    bool retval = !test_deleted(it);
    // &* converts from iterator to value-type.
    set_key(&(*it), key_info.delkey);
    return retval;
  }
  // Set it so test_deleted is false.  true if object used to be deleted.
  bool clear_deleted(iterator &it) {
    check_use_deleted("clear_deleted()");
    // Happens automatically when we assign something else in its place.
    return test_deleted(it);
  }

  // We also allow to set/clear the deleted bit on a const iterator.
  // We allow a const_iterator for the same reason you can delete a
  // const pointer: it's convenient, and semantically you can't use
  // 'it' after it's been deleted anyway, so its const-ness doesn't
  // really matter.
  bool set_deleted(const_iterator &it) {
    check_use_deleted("set_deleted()");
    bool retval = !test_deleted(it);
    set_key(const_cast<pointer>(&(*it)), key_info.delkey);
    return retval;
  }
  // Set it so test_deleted is false.  true if object used to be deleted.
  bool clear_deleted(const_iterator &it) {
    check_use_deleted("clear_deleted()");
    return test_deleted(it);
  }

  // FUNCTIONS CONCERNING SIZE
 public:
  size_type size() const      { return table.num_nonempty() - num_deleted; }
  size_type max_size() const          { return table.max_size(); }
  bool empty() const                  { return size() == 0; }
  size_type bucket_count() const      { return table.size(); }
  size_type max_bucket_count() const  { return max_size(); }
  // These are tr1 methods.  Their idea of 'bucket' doesn't map well to
  // what we do.  We just say every bucket has 0 or 1 items in it.
  size_type bucket_size(size_type i) const {
    return begin(i) == end(i) ? 0 : 1;
  }

 private:
  // Because of the above, size_type(-1) is never legal; use it for errors
  static const size_type ILLEGAL_BUCKET = size_type(-1);

  // Used after a string of deletes.  Returns true if we actually shrunk.
  // TODO(csilvers): take a delta so we can take into account inserts
  // done after shrinking.  Maybe make part of the Settings class?
  bool maybe_shrink() {
    assert(table.num_nonempty() >= num_deleted);
    assert((bucket_count() & (bucket_count()-1)) == 0); // is a power of two
    assert(bucket_count() >= HT_MIN_BUCKETS);
    bool retval = false;

    // If you construct a hashtable with < HT_DEFAULT_STARTING_BUCKETS,
    // we'll never shrink until you get relatively big, and we'll never
    // shrink below HT_DEFAULT_STARTING_BUCKETS.  Otherwise, something
    // like "dense_hash_set<int> x; x.insert(4); x.erase(4);" will
    // shrink us down to HT_MIN_BUCKETS buckets, which is too small.
    const size_type num_remain = table.num_nonempty() - num_deleted;
    const size_type shrink_threshold = settings.shrink_threshold();
    if (shrink_threshold > 0 && num_remain < shrink_threshold &&
        bucket_count() > HT_DEFAULT_STARTING_BUCKETS) {
      const float shrink_factor = settings.shrink_factor();
      size_type sz = bucket_count() / 2;    // find how much we should shrink
      while (sz > HT_DEFAULT_STARTING_BUCKETS &&
             num_remain < static_cast<size_type>(sz * shrink_factor)) {
        sz /= 2;                            // stay a power of 2
      }
      sparse_hashtable tmp(MoveDontCopy, *this, sz);
      swap(tmp);                            // now we are tmp
      retval = true;
    }
    settings.set_consider_shrink(false);   // because we just considered it
    return retval;
  }

  // We'll let you resize a hashtable -- though this makes us copy all!
  // When you resize, you say, "make it big enough for this many more elements"
  // Returns true if we actually resized, false if size was already ok.
  bool resize_delta(size_type delta) {
    bool did_resize = false;
    if ( settings.consider_shrink() ) {  // see if lots of deletes happened
      if ( maybe_shrink() )
        did_resize = true;
    }
    if (table.num_nonempty() >=
        (std::numeric_limits<size_type>::max)() - delta) {
      throw std::length_error("resize overflow");
    }
    if ( bucket_count() >= HT_MIN_BUCKETS &&
         (table.num_nonempty() + delta) <= settings.enlarge_threshold() )
      return did_resize;                       // we're ok as we are

    // Sometimes, we need to resize just to get rid of all the
    // "deleted" buckets that are clogging up the hashtable.  So when
    // deciding whether to resize, count the deleted buckets (which
    // are currently taking up room).  But later, when we decide what
    // size to resize to, *don't* count deleted buckets, since they
    // get discarded during the resize.
    const size_type needed_size =
        settings.min_buckets(table.num_nonempty() + delta, 0);
    if ( needed_size <= bucket_count() )      // we have enough buckets
      return did_resize;

    size_type resize_to =
        settings.min_buckets(table.num_nonempty() - num_deleted + delta,
                             bucket_count());
    if (resize_to < needed_size &&    // may double resize_to
        resize_to < (std::numeric_limits<size_type>::max)() / 2) {
      // This situation means that we have enough deleted elements,
      // that once we purge them, we won't actually have needed to
      // grow.  But we may want to grow anyway: if we just purge one
      // element, say, we'll have to grow anyway next time we
      // insert.  Might as well grow now, since we're already going
      // through the trouble of copying (in order to purge the
      // deleted elements).
      const size_type target =
          static_cast<size_type>(settings.shrink_size(resize_to*2));
      if (table.num_nonempty() - num_deleted + delta >= target) {
        // Good, we won't be below the shrink threshhold even if we double.
        resize_to *= 2;
      }
    }

    sparse_hashtable tmp(MoveDontCopy, *this, resize_to);
    swap(tmp);                             // now we are tmp
    return true;
  }

  // Used to actually do the rehashing when we grow/shrink a hashtable
  void copy_from(const sparse_hashtable &ht, size_type min_buckets_wanted) {
    clear();            // clear table, set num_deleted to 0

    // If we need to change the size of our table, do it now
    const size_type resize_to =
        settings.min_buckets(ht.size(), min_buckets_wanted);
    if ( resize_to > bucket_count() ) {      // we don't have enough buckets
      table.resize(resize_to);               // sets the number of buckets
      settings.reset_thresholds(bucket_count());
    }

    // We use a normal iterator to get non-deleted bcks from ht
    // We could use insert() here, but since we know there are
    // no duplicates and no deleted items, we can be more efficient
    assert((bucket_count() & (bucket_count()-1)) == 0);      // a power of two
    for ( const_iterator it = ht.begin(); it != ht.end(); ++it ) {
      size_type num_probes = 0;              // how many times we've probed
      size_type bucknum;
      const size_type bucket_count_minus_one = bucket_count() - 1;
      for (bucknum = hash(get_key(*it)) & bucket_count_minus_one;
           table.test(bucknum);                          // not empty
           bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one) {
        ++num_probes;
        assert(num_probes < bucket_count()
               && "Hashtable is full: an error in key_equal<> or hash<>");
      }
      table.set(bucknum, *it);               // copies the value to here
    }
    settings.inc_num_ht_copies();
  }

  // Implementation is like copy_from, but it destroys the table of the
  // "from" guy by freeing sparsetable memory as we iterate.  This is
  // useful in resizing, since we're throwing away the "from" guy anyway.
  void move_from(MoveDontCopyT mover, sparse_hashtable &ht,
                 size_type min_buckets_wanted) {
    clear();            // clear table, set num_deleted to 0

    // If we need to change the size of our table, do it now
    size_type resize_to;
    if ( mover == MoveDontGrow )
      resize_to = ht.bucket_count();         // keep same size as old ht
    else                                     // MoveDontCopy
      resize_to = settings.min_buckets(ht.size(), min_buckets_wanted);
    if ( resize_to > bucket_count() ) {      // we don't have enough buckets
      table.resize(resize_to);               // sets the number of buckets
      settings.reset_thresholds(bucket_count());
    }

    // We use a normal iterator to get non-deleted bcks from ht
    // We could use insert() here, but since we know there are
    // no duplicates and no deleted items, we can be more efficient
    assert( (bucket_count() & (bucket_count()-1)) == 0);      // a power of two
    // THIS IS THE MAJOR LINE THAT DIFFERS FROM COPY_FROM():
    for ( destructive_iterator it = ht.destructive_begin();
          it != ht.destructive_end(); ++it ) {
      size_type num_probes = 0;              // how many times we've probed
      size_type bucknum;
      for ( bucknum = hash(get_key(*it)) & (bucket_count()-1);  // h % buck_cnt
            table.test(bucknum);                          // not empty
            bucknum = (bucknum + JUMP_(key, num_probes)) & (bucket_count()-1) ) {
        ++num_probes;
        assert(num_probes < bucket_count()
               && "Hashtable is full: an error in key_equal<> or hash<>");
      }
      table.set(bucknum, *it);               // copies the value to here
    }
    settings.inc_num_ht_copies();
  }


  // Required by the spec for hashed associative container
 public:
  // Though the docs say this should be num_buckets, I think it's much
  // more useful as num_elements.  As a special feature, calling with
  // req_elements==0 will cause us to shrink if we can, saving space.
  void resize(size_type req_elements) {       // resize to this or larger
    if ( settings.consider_shrink() || req_elements == 0 )
      maybe_shrink();
    if ( req_elements > table.num_nonempty() )    // we only grow
      resize_delta(req_elements - table.num_nonempty());
  }

  // Get and change the value of shrink_factor and enlarge_factor.  The
  // description at the beginning of this file explains how to choose
  // the values.  Setting the shrink parameter to 0.0 ensures that the
  // table never shrinks.
  void get_resizing_parameters(float* shrink, float* grow) const {
    *shrink = settings.shrink_factor();
    *grow = settings.enlarge_factor();
  }
  void set_resizing_parameters(float shrink, float grow) {
    settings.set_resizing_parameters(shrink, grow);
    settings.reset_thresholds(bucket_count());
  }

  // CONSTRUCTORS -- as required by the specs, we take a size,
  // but also let you specify a hashfunction, key comparator,
  // and key extractor.  We also define a copy constructor and =.
  // DESTRUCTOR -- the default is fine, surprisingly.
  explicit sparse_hashtable(size_type expected_max_items_in_table = 0,
                            const HashFcn& hf = HashFcn(),
                            const EqualKey& eql = EqualKey(),
                            const ExtractKey& ext = ExtractKey(),
                            const SetKey& set = SetKey(),
                            const Alloc& alloc = Alloc())
      : settings(hf),
        key_info(ext, set, eql),
        num_deleted(0),
        table((expected_max_items_in_table == 0
               ? HT_DEFAULT_STARTING_BUCKETS
               : settings.min_buckets(expected_max_items_in_table, 0)),
              alloc) {
    settings.reset_thresholds(bucket_count());
  }

  // As a convenience for resize(), we allow an optional second argument
  // which lets you make this new hashtable a different size than ht.
  // We also provide a mechanism of saying you want to "move" the ht argument
  // into us instead of copying.
  sparse_hashtable(const sparse_hashtable& ht,
                   size_type min_buckets_wanted = HT_DEFAULT_STARTING_BUCKETS)
      : settings(ht.settings),
        key_info(ht.key_info),
        num_deleted(0),
        table(0, ht.get_allocator()) {
    settings.reset_thresholds(bucket_count());
    copy_from(ht, min_buckets_wanted);   // copy_from() ignores deleted entries
  }
  sparse_hashtable(MoveDontCopyT mover, sparse_hashtable& ht,
                   size_type min_buckets_wanted = HT_DEFAULT_STARTING_BUCKETS)
      : settings(ht.settings),
        key_info(ht.key_info),
        num_deleted(0),
        table(0, ht.get_allocator()) {
    settings.reset_thresholds(bucket_count());
    move_from(mover, ht, min_buckets_wanted);  // ignores deleted entries
  }

  sparse_hashtable& operator= (const sparse_hashtable& ht) {
    if (&ht == this)  return *this;        // don't copy onto ourselves
    settings = ht.settings;
    key_info = ht.key_info;
    num_deleted = ht.num_deleted;
    // copy_from() calls clear and sets num_deleted to 0 too
    copy_from(ht, HT_MIN_BUCKETS);
    // we purposefully don't copy the allocator, which may not be copyable
    return *this;
  }

  // Many STL algorithms use swap instead of copy constructors
  void swap(sparse_hashtable& ht) {
    std::swap(settings, ht.settings);
    std::swap(key_info, ht.key_info);
    std::swap(num_deleted, ht.num_deleted);
    table.swap(ht.table);
    settings.reset_thresholds(bucket_count());  // also resets consider_shrink
    ht.settings.reset_thresholds(ht.bucket_count());
    // we purposefully don't swap the allocator, which may not be swap-able
  }

  // It's always nice to be able to clear a table without deallocating it
  void clear() {
    if (!empty() || (num_deleted != 0)) {
      table.clear();
    }
    settings.reset_thresholds(bucket_count());
    num_deleted = 0;
  }

  // LOOKUP ROUTINES
 private:
  // Returns a pair of positions: 1st where the object is, 2nd where
  // it would go if you wanted to insert it.  1st is ILLEGAL_BUCKET
  // if object is not found; 2nd is ILLEGAL_BUCKET if it is.
  // Note: because of deletions where-to-insert is not trivial: it's the
  // first deleted bucket we see, as long as we don't find the key later
  std::pair<size_type, size_type> find_position(const key_type &key) const {
    size_type num_probes = 0;              // how many times we've probed
    const size_type bucket_count_minus_one = bucket_count() - 1;
    size_type bucknum = hash(key) & bucket_count_minus_one;
    size_type insert_pos = ILLEGAL_BUCKET; // where we would insert
    while ( 1 ) {                          // probe until something happens
      if ( !table.test(bucknum) ) {        // bucket is empty
        if ( insert_pos == ILLEGAL_BUCKET )  // found no prior place to insert
          return std::pair<size_type,size_type>(ILLEGAL_BUCKET, bucknum);
        else
          return std::pair<size_type,size_type>(ILLEGAL_BUCKET, insert_pos);

      } else if ( test_deleted(bucknum) ) {// keep searching, but mark to insert
        if ( insert_pos == ILLEGAL_BUCKET )
          insert_pos = bucknum;

      } else if ( equals(key, get_key(table.unsafe_get(bucknum))) ) {
        return std::pair<size_type,size_type>(bucknum, ILLEGAL_BUCKET);
      }
      ++num_probes;                        // we're doing another probe
      bucknum = (bucknum + JUMP_(key, num_probes)) & bucket_count_minus_one;
      assert(num_probes < bucket_count()
             && "Hashtable is full: an error in key_equal<> or hash<>");
    }
  }

 public:

  iterator find(const key_type& key) {
    if ( size() == 0 ) return end();
    std::pair<size_type, size_type> pos = find_position(key);
    if ( pos.first == ILLEGAL_BUCKET )     // alas, not there
      return end();
    else
      return iterator(this, table.get_iter(pos.first), table.nonempty_end());
  }

  const_iterator find(const key_type& key) const {
    if ( size() == 0 ) return end();
    std::pair<size_type, size_type> pos = find_position(key);
    if ( pos.first == ILLEGAL_BUCKET )     // alas, not there
      return end();
    else
      return const_iterator(this,
                            table.get_iter(pos.first), table.nonempty_end());
  }

  // This is a tr1 method: the bucket a given key is in, or what bucket
  // it would be put in, if it were to be inserted.  Shrug.
  size_type bucket(const key_type& key) const {
    std::pair<size_type, size_type> pos = find_position(key);
    return pos.first == ILLEGAL_BUCKET ? pos.second : pos.first;
  }

  // Counts how many elements have key key.  For maps, it's either 0 or 1.
  size_type count(const key_type &key) const {
    std::pair<size_type, size_type> pos = find_position(key);
    return pos.first == ILLEGAL_BUCKET ? 0 : 1;
  }

  // Likewise, equal_range doesn't really make sense for us.  Oh well.
  std::pair<iterator,iterator> equal_range(const key_type& key) {
    iterator pos = find(key);      // either an iterator or end
    if (pos == end()) {
      return std::pair<iterator,iterator>(pos, pos);
    } else {
      const iterator startpos = pos++;
      return std::pair<iterator,iterator>(startpos, pos);
    }
  }
  std::pair<const_iterator,const_iterator> equal_range(const key_type& key)
      const {
    const_iterator pos = find(key);      // either an iterator or end
    if (pos == end()) {
      return std::pair<const_iterator,const_iterator>(pos, pos);
    } else {
      const const_iterator startpos = pos++;
      return std::pair<const_iterator,const_iterator>(startpos, pos);
    }
  }


  // INSERTION ROUTINES
 private:
  // Private method used by insert_noresize and find_or_insert.
  iterator insert_at(const_reference obj, size_type pos) {
    if (size() >= max_size()) {
      throw std::length_error("insert overflow");
    }
    if ( test_deleted(pos) ) {      // just replace if it's been deleted
      // The set() below will undelete this object.  We just worry about stats
      assert(num_deleted > 0);
      --num_deleted;                // used to be, now it isn't
    }
    table.set(pos, obj);
    return iterator(this, table.get_iter(pos), table.nonempty_end());
  }

  // If you know *this is big enough to hold obj, use this routine
  std::pair<iterator, bool> insert_noresize(const_reference obj) {
    // First, double-check we're not inserting delkey
    assert((!settings.use_deleted() || !equals(get_key(obj), key_info.delkey))
           && "Inserting the deleted key");
    const std::pair<size_type,size_type> pos = find_position(get_key(obj));
    if ( pos.first != ILLEGAL_BUCKET) {      // object was already there
      return std::pair<iterator,bool>(iterator(this, table.get_iter(pos.first),
                                               table.nonempty_end()),
                                      false);     // false: we didn't insert
    } else {                                 // pos.second says where to put it
      return std::pair<iterator,bool>(insert_at(obj, pos.second), true);
    }
  }

  // Specializations of insert(it, it) depending on the power of the iterator:
  // (1) Iterator supports operator-, resize before inserting
  template <class ForwardIterator>  void insert(ForwardIterator f, ForwardIterator l, std::forward_iterator_tag) 
  {
    size_t dist = std::distance(f, l);
    if (dist >= (std::numeric_limits<size_type>::max)()) {
      throw std::length_error("insert-range overflow");
    }
    resize_delta(static_cast<size_type>(dist));
    for ( ; dist > 0; --dist, ++f) {
      insert_noresize(*f);
    }
  }

  // (2) Arbitrary iterator, can't tell how much to resize
  template <class InputIterator>  void insert(InputIterator f, InputIterator l, std::input_iterator_tag) 
  {
    for ( ; f != l; ++f)
      insert(*f);
  }

 public:
  // This is the normal insert routine, used by the outside world
  std::pair<iterator, bool> insert(const_reference obj) {
    resize_delta(1);                      // adding an object, grow if need be
    return insert_noresize(obj);
  }

  // When inserting a lot at a time, we specialize on the type of iterator
  template <class InputIterator>  void insert(InputIterator f, InputIterator l) 
  {
    // specializes on iterator type
    insert(f, l,
           typename std::iterator_traits<InputIterator>::iterator_category());
  }

  // DefaultValue is a functor that takes a key and returns a value_type
  // representing the default value to be inserted if none is found.
  template <class DefaultValue>  value_type& find_or_insert(const key_type& key) 
  {
    // First, double-check we're not inserting delkey
    assert((!settings.use_deleted() || !equals(key, key_info.delkey))
           && "Inserting the deleted key");
    const std::pair<size_type,size_type> pos = find_position(key);
    DefaultValue default_value;
    if ( pos.first != ILLEGAL_BUCKET) {  // object was already there
      return *table.get_iter(pos.first);
    } else if (resize_delta(1)) {        // needed to rehash to make room
      // Since we resized, we can't use pos, so recalculate where to insert.
      return *insert_noresize(default_value(key)).first;
    } else {                             // no need to rehash, insert right here
      return *insert_at(default_value(key), pos.second);
    }
  }

  // DELETION ROUTINES
  size_type erase(const key_type& key) {
    // First, double-check we're not erasing delkey.
    assert((!settings.use_deleted() || !equals(key, key_info.delkey))
           && "Erasing the deleted key");
    assert(!settings.use_deleted() || !equals(key, key_info.delkey));
    const_iterator pos = find(key);   // shrug: shouldn't need to be const
    if ( pos != end() ) {
      assert(!test_deleted(pos));  // or find() shouldn't have returned it
      set_deleted(pos);
      ++num_deleted;
      // will think about shrink after next insert
      settings.set_consider_shrink(true);
      return 1;                    // because we deleted one thing
    } else {
      return 0;                    // because we deleted nothing
    }
  }

  // We return the iterator past the deleted item.
  void erase(iterator pos) {
    if ( pos == end() ) return;    // sanity check
    if ( set_deleted(pos) ) {      // true if object has been newly deleted
      ++num_deleted;
      // will think about shrink after next insert
      settings.set_consider_shrink(true);
    }
  }

  void erase(iterator f, iterator l) {
    for ( ; f != l; ++f) {
      if ( set_deleted(f)  )       // should always be true
        ++num_deleted;
    }
    // will think about shrink after next insert
    settings.set_consider_shrink(true);
  }

  // We allow you to erase a const_iterator just like we allow you to
  // erase an iterator.  This is in parallel to 'delete': you can delete
  // a const pointer just like a non-const pointer.  The logic is that
  // you can't use the object after it's erased anyway, so it doesn't matter
  // if it's const or not.
  void erase(const_iterator pos) {
    if ( pos == end() ) return;    // sanity check
    if ( set_deleted(pos) ) {      // true if object has been newly deleted
      ++num_deleted;
      // will think about shrink after next insert
      settings.set_consider_shrink(true);
    }
  }
  void erase(const_iterator f, const_iterator l) {
    for ( ; f != l; ++f) {
      if ( set_deleted(f)  )       // should always be true
        ++num_deleted;
    }
    // will think about shrink after next insert
    settings.set_consider_shrink(true);
  }


  // COMPARISON
  bool operator==(const sparse_hashtable& ht) const {
    if (size() != ht.size()) {
      return false;
    } else if (this == &ht) {
      return true;
    } else {
      // Iterate through the elements in "this" and see if the
      // corresponding element is in ht
      for ( const_iterator it = begin(); it != end(); ++it ) {
        const_iterator it2 = ht.find(get_key(*it));
        if ((it2 == ht.end()) || (*it != *it2)) {
          return false;
        }
      }
      return true;
    }
  }
  bool operator!=(const sparse_hashtable& ht) const {
    return !(*this == ht);
  }


  // I/O
  // We support reading and writing hashtables to disk.  NOTE that
  // this only stores the hashtable metadata, not the stuff you've
  // actually put in the hashtable!  Alas, since I don't know how to
  // write a hasher or key_equal, you have to make sure everything
  // but the table is the same.  We compact before writing.
  //
  // The OUTPUT type needs to support a Write() operation. File and
  // OutputBuffer are appropriate types to pass in.
  //
  // The INPUT type needs to support a Read() operation. File and
  // InputBuffer are appropriate types to pass in.
  template <typename OUTPUT>
  bool write_metadata(OUTPUT *fp) {
    squash_deleted();           // so we don't have to worry about delkey
    return table.write_metadata(fp);
  }

  template <typename INPUT>
  bool read_metadata(INPUT *fp) {
    num_deleted = 0;            // since we got rid before writing
    const bool result = table.read_metadata(fp);
    settings.reset_thresholds(bucket_count());
    return result;
  }

  // Only meaningful if value_type is a POD.
  template <typename OUTPUT>
  bool write_nopointer_data(OUTPUT *fp) {
    return table.write_nopointer_data(fp);
  }

  // Only meaningful if value_type is a POD.
  template <typename INPUT>
  bool read_nopointer_data(INPUT *fp) {
    return table.read_nopointer_data(fp);
  }

  // INPUT and OUTPUT must be either a FILE, *or* a C++ stream
  //    (istream, ostream, etc) *or* a class providing
  //    Read(void*, size_t) and Write(const void*, size_t)
  //    (respectively), which writes a buffer into a stream
  //    (which the INPUT/OUTPUT instance presumably owns).

  typedef sparsehash_internal::pod_serializer<value_type> NopointerSerializer;

  // ValueSerializer: a functor.  operator()(OUTPUT*, const value_type&)
  template <typename ValueSerializer, typename OUTPUT>
  bool serialize(ValueSerializer serializer, OUTPUT *fp) {
    squash_deleted();           // so we don't have to worry about delkey
    return table.serialize(serializer, fp);
  }

  // ValueSerializer: a functor.  operator()(INPUT*, value_type*)
  template <typename ValueSerializer, typename INPUT>
  bool unserialize(ValueSerializer serializer, INPUT *fp) {
    num_deleted = 0;            // since we got rid before writing
    const bool result = table.unserialize(serializer, fp);
    settings.reset_thresholds(bucket_count());
    return result;
  }

 private:
  // Table is the main storage class.
  typedef sparsetable<value_type, 48, value_alloc_type> Table;

  // Package templated functors with the other types to eliminate memory
  // needed for storing these zero-size operators.  Since ExtractKey and
  // hasher's operator() might have the same function signature, they
  // must be packaged in different classes.
  struct Settings :  sparsehash_internal::sh_hashtable_settings<key_type, hasher, size_type, HT_MIN_BUCKETS> 
  {
    explicit Settings(const hasher& hf) : sparsehash_internal::sh_hashtable_settings<key_type, hasher, size_type, HT_MIN_BUCKETS>(  hf, HT_OCCUPANCY_PCT / 100.0f, HT_EMPTY_PCT / 100.0f) 	{	}
  };

  // KeyInfo stores delete key and packages zero-size functors:
  // ExtractKey and SetKey.
  class KeyInfo : public ExtractKey, public SetKey, public EqualKey {
   public:
    KeyInfo(const ExtractKey& ek, const SetKey& sk, const EqualKey& eq)
        : ExtractKey(ek),
          SetKey(sk),
          EqualKey(eq) {
    }
    // We want to return the exact same type as ExtractKey: Key or const Key&
    typename ExtractKey::result_type get_key(const_reference v) const {
      return ExtractKey::operator()(v);
    }
    void set_key(pointer v, const key_type& k) const {
      SetKey::operator()(v, k);
    }
    bool equals(const key_type& a, const key_type& b) const {
      return EqualKey::operator()(a, b);
    }

    // Which key marks deleted entries.
    // TODO(csilvers): make a pointer, and get rid of use_deleted (benchmark!)
    typename base::remove_const<key_type>::type delkey;
  };

  // Utility functions to access the templated operators
  size_type hash(const key_type& v) const {
    return settings.hash(v);
  }
  bool equals(const key_type& a, const key_type& b) const {
    return key_info.equals(a, b);
  }
  typename ExtractKey::result_type get_key(const_reference v) const {
    return key_info.get_key(v);
  }
  void set_key(pointer v, const key_type& k) const {
    key_info.set_key(v, k);
  }

 private:
  // Actual data
  Settings settings;
  KeyInfo key_info;
  size_type num_deleted;   // how many occupied buckets are marked deleted
  Table table;     // holds num_buckets and num_elements too
};


// We need a global swap as well
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> inline void swap(sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> &x, sparse_hashtable<V,K,HF,ExK,SetK,EqK,A> &y) 
{
  x.swap(y);
}

#undef JUMP_

template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> const typename sparse_hashtable<V,K,HF,ExK,SetK,EqK,A>::size_type  sparse_hashtable<V,K,HF,ExK,SetK,EqK,A>::ILLEGAL_BUCKET;

// How full we let the table get before we resize.  Knuth says .8 is good -- higher causes us to probe too much, though saves memory
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> const int sparse_hashtable<V,K,HF,ExK,SetK,EqK,A>::HT_OCCUPANCY_PCT = 80;

// How empty we let the table get before we resize lower.  It should be less than OCCUPANCY_PCT / 2 or we thrash resizing.
template <class V, class K, class HF, class ExK, class SetK, class EqK, class A> const int sparse_hashtable<V,K,HF,ExK,SetK,EqK,A>::HT_EMPTY_PCT  = static_cast<int>(0.4 *                     sparse_hashtable<V,K,HF,ExK,SetK,EqK,A>::HT_OCCUPANCY_PCT);


// 1D1D1D1D1D1D1D1D1D1D1D1D1D1D BEGIN

	/*
	// This is a 2-D iterator.  You specify a begin and end over a list
	// of *containers*.  We iterate over each container by iterating over
	// it.  It's actually simple:
	// VECTOR.begin() VECTOR[0].begin()  --------> VECTOR[0].end() ---,
	//     |          ________________________________________________/
	//     |          \_> VECTOR[1].begin()  -------->  VECTOR[1].end() -,
	//     |          ___________________________________________________/
	//     v          \_> ......
	// VECTOR.end()
	//
	// It's impossible to do random access on one of these things in constant
	// time, so it's just a bidirectional iterator.
	//
	// Unfortunately, because we need to use this for a non-empty iterator,
	// we use nonempty_begin() and nonempty_end() instead of begin() and end()
	// (though only going across, not down).
	*/

	#define TWOD_BEGIN_      nonempty_begin
	#define TWOD_END_        nonempty_end
	#define TWOD_ITER_       nonempty_iterator
	#define TWOD_CONST_ITER_ const_nonempty_iterator

	template <class tabletype> class table_element_adaptor;	
	
	template <class containertype>	class two_d_iterator 
	{
	 public:
	  typedef two_d_iterator iterator;

	  typedef std::bidirectional_iterator_tag iterator_category;
	  // apparently some versions of VC++ have trouble with two ::'s in a typename
	  typedef typename containertype::value_type _tmp_vt;
	  typedef typename _tmp_vt::value_type value_type;
	  typedef typename _tmp_vt::difference_type difference_type;
	  typedef typename _tmp_vt::reference reference;
	  typedef typename _tmp_vt::pointer pointer;

	  // The "real" constructor.  begin and end specify how many rows we have
	  // (in the diagram above); we always iterate over each row completely.
	  two_d_iterator(typename containertype::iterator begin,
					 typename containertype::iterator end,
					 typename containertype::iterator curr)
		: row_begin(begin), row_end(end), row_current(curr), col_current() {
		if ( row_current != row_end ) {
		  col_current = row_current->TWOD_BEGIN_();
		  advance_past_end();                 // in case cur->begin() == cur->end()
		}
	  }
	  // If you want to start at an arbitrary place, you can, I guess
	  two_d_iterator(typename containertype::iterator begin,
					 typename containertype::iterator end,
					 typename containertype::iterator curr,
					 typename containertype::value_type::TWOD_ITER_ col)
		: row_begin(begin), row_end(end), row_current(curr), col_current(col) {
		advance_past_end();                 // in case cur->begin() == cur->end()
	  }
	  // The default constructor, used when I define vars of type table::iterator
	  two_d_iterator() : row_begin(), row_end(), row_current(), col_current() { }
	  // The default destructor is fine; we don't define one
	  // The default operator= is fine; we don't define one

	  // Happy dereferencer
	  reference operator*() const    { return *col_current; }
	  pointer operator->() const     { return &(operator*()); }

	  // Arithmetic: we just do arithmetic on pos.  We don't even need to
	  // do bounds checking, since STL doesn't consider that its job.  :-)
	  // NOTE: this is not amortized constant time!  What do we do about it?
	  void advance_past_end() {          // used when col_current points to end()
		while ( col_current == row_current->TWOD_END_() ) {  // end of current row
		  ++row_current;                                // go to beginning of next
		  if ( row_current != row_end )                 // col is irrelevant at end
			col_current = row_current->TWOD_BEGIN_();
		  else
			break;                                      // don't go past row_end
		}
	  }

	  iterator& operator++() {
		assert(row_current != row_end);                 // how to ++ from there?
		++col_current;
		advance_past_end();                 // in case col_current is at end()
		return *this;
	  }
	  iterator& operator--() {
		while ( row_current == row_end ||
				col_current == row_current->TWOD_BEGIN_() ) {
		  assert(row_current != row_begin);
		  --row_current;
		  col_current = row_current->TWOD_END_();             // this is 1 too far
		}
		--col_current;
		return *this;
	  }
	  iterator operator++(int)       { iterator tmp(*this); ++*this; return tmp; }
	  iterator operator--(int)       { iterator tmp(*this); --*this; return tmp; }


	  // Comparisons.
	  bool operator==(const iterator& it) const {
		return ( row_begin == it.row_begin &&
				 row_end == it.row_end &&
				 row_current == it.row_current &&
				 (row_current == row_end || col_current == it.col_current) );
	  }
	  bool operator!=(const iterator& it) const { return !(*this == it); }


	  // Here's the info we actually need to be an iterator
	  // These need to be public so we convert from iterator to const_iterator
	  typename containertype::iterator row_begin, row_end, row_current;
	  typename containertype::value_type::TWOD_ITER_ col_current;
	};

	// The same thing again, but this time const.  :-(
	template <class containertype>	class const_two_d_iterator 
	{
	 public:
	  typedef const_two_d_iterator iterator;

	  typedef std::bidirectional_iterator_tag iterator_category;
	  // apparently some versions of VC++ have trouble with two ::'s in a typename
	  typedef typename containertype::value_type _tmp_vt;
	  typedef typename _tmp_vt::value_type value_type;
	  typedef typename _tmp_vt::difference_type difference_type;
	  typedef typename _tmp_vt::const_reference reference;
	  typedef typename _tmp_vt::const_pointer pointer;

	  const_two_d_iterator(typename containertype::const_iterator begin,
						   typename containertype::const_iterator end,
						   typename containertype::const_iterator curr)
		: row_begin(begin), row_end(end), row_current(curr), col_current() {
		if ( curr != end ) {
		  col_current = curr->TWOD_BEGIN_();
		  advance_past_end();                 // in case cur->begin() == cur->end()
		}
	  }
	  const_two_d_iterator(typename containertype::const_iterator begin,
						   typename containertype::const_iterator end,
						   typename containertype::const_iterator curr,
						   typename containertype::value_type::TWOD_CONST_ITER_ col)
		: row_begin(begin), row_end(end), row_current(curr), col_current(col) {
		advance_past_end();                 // in case cur->begin() == cur->end()
	  }
	  const_two_d_iterator()
		: row_begin(), row_end(), row_current(), col_current() {
	  }
	  // Need this explicitly so we can convert normal iterators to const iterators
	  const_two_d_iterator(const two_d_iterator<containertype>& it) :
		row_begin(it.row_begin), row_end(it.row_end), row_current(it.row_current),
		col_current(it.col_current) { }

	  typename containertype::const_iterator row_begin, row_end, row_current;
	  typename containertype::value_type::TWOD_CONST_ITER_ col_current;


	  // EVERYTHING FROM HERE DOWN IS THE SAME AS THE NON-CONST ITERATOR
	  reference operator*() const    { return *col_current; }
	  pointer operator->() const     { return &(operator*()); }

	  void advance_past_end() {          // used when col_current points to end()
		while ( col_current == row_current->TWOD_END_() ) {  // end of current row
		  ++row_current;                                // go to beginning of next
		  if ( row_current != row_end )                 // col is irrelevant at end
			col_current = row_current->TWOD_BEGIN_();
		  else
			break;                                      // don't go past row_end
		}
	  }
	  iterator& operator++() {
		assert(row_current != row_end);                 // how to ++ from there?
		++col_current;
		advance_past_end();                 // in case col_current is at end()
		return *this;
	  }
	  iterator& operator--() {
		while ( row_current == row_end ||
				col_current == row_current->TWOD_BEGIN_() ) {
		  assert(row_current != row_begin);
		  --row_current;
		  col_current = row_current->TWOD_END_();             // this is 1 too far
		}
		--col_current;
		return *this;
	  }
	  iterator operator++(int)       { iterator tmp(*this); ++*this; return tmp; }
	  iterator operator--(int)       { iterator tmp(*this); --*this; return tmp; }

	  bool operator==(const iterator& it) const {
		return ( row_begin == it.row_begin &&
				 row_end == it.row_end &&
				 row_current == it.row_current &&
				 (row_current == row_end || col_current == it.col_current) );
	  }
	  bool operator!=(const iterator& it) const { return !(*this == it); }
	};

	// We provide yet another version, to be as frugal with memory as
	// possible.  This one frees each block of memory as it finishes
	// iterating over it.  By the end, the entire table is freed.
	// For understandable reasons, you can only iterate over it once,
	// which is why it's an input iterator
	template <class containertype>	class destructive_two_d_iterator 
	{
	 public:
	  typedef destructive_two_d_iterator iterator;

	  typedef std::input_iterator_tag iterator_category;
	  // apparently some versions of VC++ have trouble with two ::'s in a typename
	  typedef typename containertype::value_type _tmp_vt;
	  typedef typename _tmp_vt::value_type value_type;
	  typedef typename _tmp_vt::difference_type difference_type;
	  typedef typename _tmp_vt::reference reference;
	  typedef typename _tmp_vt::pointer pointer;

	  destructive_two_d_iterator(typename containertype::iterator begin,
								 typename containertype::iterator end,
								 typename containertype::iterator curr)
		: row_begin(begin), row_end(end), row_current(curr), col_current() {
		if ( curr != end ) {
		  col_current = curr->TWOD_BEGIN_();
		  advance_past_end();                 // in case cur->begin() == cur->end()
		}
	  }
	  destructive_two_d_iterator(typename containertype::iterator begin,
								 typename containertype::iterator end,
								 typename containertype::iterator curr,
								 typename containertype::value_type::TWOD_ITER_ col)
		: row_begin(begin), row_end(end), row_current(curr), col_current(col) {
		advance_past_end();                 // in case cur->begin() == cur->end()
	  }
	  destructive_two_d_iterator()	: row_begin(), row_end(), row_current(), col_current() 	  
	  {
	  }

	  typename containertype::iterator row_begin, row_end, row_current;
	  typename containertype::value_type::TWOD_ITER_ col_current;

	  // This is the part that destroys
	  void advance_past_end() {          // used when col_current points to end()
		while ( col_current == row_current->TWOD_END_() ) {  // end of current row
		  row_current->clear();                         // the destructive part
		  // It would be nice if we could decrement sparsetable->num_buckets here
		  ++row_current;                                // go to beginning of next
		  if ( row_current != row_end )                 // col is irrelevant at end
			col_current = row_current->TWOD_BEGIN_();
		  else
			break;                                      // don't go past row_end
		}
	  }

	  // EVERYTHING FROM HERE DOWN IS THE SAME AS THE REGULAR ITERATOR
	  reference operator*() const    { return *col_current; }
	  pointer operator->() const     { return &(operator*()); }

	  iterator& operator++() {
		assert(row_current != row_end);                 // how to ++ from there?
		++col_current;
		advance_past_end();                 // in case col_current is at end()
		return *this;
	  }
	  iterator operator++(int)       { iterator tmp(*this); ++*this; return tmp; }

	  bool operator==(const iterator& it) const {
		return ( row_begin == it.row_begin &&
				 row_end == it.row_end &&
				 row_current == it.row_current &&
				 (row_current == row_end || col_current == it.col_current) );
	  }
	  bool operator!=(const iterator& it) const { return !(*this == it); }
	};

	#undef TWOD_BEGIN_
	#undef TWOD_END_
	#undef TWOD_ITER_
	#undef TWOD_CONST_ITER_



	// SPARSE-TABLE
	// ------------
	// The idea is that a table with (logically) t buckets is divided
	// into t/M *groups* of M buckets each.  (M is a constant set in
	// GROUP_SIZE for efficiency.)  Each group is stored sparsely.
	// Thus, inserting into the table causes some array to grow, which is
	// slow but still constant time.  Lookup involves doing a
	// logical-position-to-sparse-position lookup, which is also slow but
	// constant time.  The larger M is, the slower these operations are
	// but the less overhead (slightly).
	//
	// To store the sparse array, we store a bitmap B, where B[i] = 1 iff
	// bucket i is non-empty.  Then to look up bucket i we really look up
	// array[# of 1s before i in B].  This is constant time for fixed M.
	//
	// Terminology: the position of an item in the overall table (from
	// 1 .. t) is called its "location."  The logical position in a group
	// (from 1 .. M ) is called its "position."  The actual location in
	// the array (from 1 .. # of non-empty buckets in the group) is
	// called its "offset."
	template <class tabletype>	class table_iterator;
	template <class tabletype>	class const_table_iterator;

	template <class T, unsigned short int GROUP_SIZE, class Alloc>	class sparsegroup 
	{
	 private:
	  typedef typename Alloc::template rebind<T>::other value_alloc_type;

	 public:
	  // Basic types
	  typedef T value_type;
	  typedef Alloc allocator_type;
	  typedef typename value_alloc_type::reference reference;
	  typedef typename value_alloc_type::const_reference const_reference;
	  typedef typename value_alloc_type::pointer pointer;
	  typedef typename value_alloc_type::const_pointer const_pointer;

	  typedef table_iterator<sparsegroup<T, GROUP_SIZE, Alloc> >		iterator;
	  typedef const_table_iterator<sparsegroup<T, GROUP_SIZE, Alloc> >  const_iterator;
	  typedef table_element_adaptor<sparsegroup<T, GROUP_SIZE, Alloc> >  element_adaptor;
	  typedef unsigned short int size_type;                  // max # of buckets
	  typedef short int difference_type;
	  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	  typedef std::reverse_iterator<iterator> reverse_iterator;   // from iterator.h

	  // These are our special iterators, that go over non-empty buckets in a
	  // group.  These aren't const-only because you can change non-empty bcks.
	  typedef pointer nonempty_iterator;
	  typedef const_pointer const_nonempty_iterator;
	  typedef std::reverse_iterator<nonempty_iterator> reverse_nonempty_iterator;
	  typedef std::reverse_iterator<const_nonempty_iterator> const_reverse_nonempty_iterator;

	  // Iterator functions
	  iterator begin()                      { return iterator(this, 0); }
	  const_iterator begin() const          { return const_iterator(this, 0); }
	  iterator end()                        { return iterator(this, size()); }
	  const_iterator end() const            { return const_iterator(this, size()); }
	  reverse_iterator rbegin()             { return reverse_iterator(end()); }
	  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	  reverse_iterator rend()               { return reverse_iterator(begin()); }
	  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	  // We'll have versions for our special non-empty iterator too
	  nonempty_iterator nonempty_begin()             { return group; }
	  const_nonempty_iterator nonempty_begin() const { return group; }
	  nonempty_iterator nonempty_end() {
		return group + settings.num_buckets;
	  }
	  const_nonempty_iterator nonempty_end() const {
		return group + settings.num_buckets;
	  }
	  reverse_nonempty_iterator nonempty_rbegin() {
		return reverse_nonempty_iterator(nonempty_end());
	  }
	  const_reverse_nonempty_iterator nonempty_rbegin() const {
		return const_reverse_nonempty_iterator(nonempty_end());
	  }
	  reverse_nonempty_iterator nonempty_rend() {
		return reverse_nonempty_iterator(nonempty_begin());
	  }
	  const_reverse_nonempty_iterator nonempty_rend() const {
		return const_reverse_nonempty_iterator(nonempty_begin());
	  }


	  // This gives us the "default" value to return for an empty bucket.
	  // We just use the default constructor on T, the template type
	  const_reference default_value() const {
		static value_type defaultval = value_type();
		return defaultval;
	  }


	 private:
	  // We need to do all this bit manipulation, of course.  ick
	  static size_type charbit(size_type i)  { return i >> 3; }
	  static size_type modbit(size_type i)   { return 1 << (i&7); }
	  int bmtest(size_type i) const    { return bitmap[charbit(i)] & modbit(i); }
	  void bmset(size_type i)          { bitmap[charbit(i)] |= modbit(i); }
	  void bmclear(size_type i)        { bitmap[charbit(i)] &= ~modbit(i); }

	  pointer allocate_group(size_type n) {
		pointer retval = settings.allocate(n);
		if (retval == NULL) {
		  // We really should use PRIuS here, but I don't want to have to add
		  // a whole new configure option, with concomitant macro namespace
		  // pollution, just to print this (unlikely) error message.  So I cast.
		  fprintf(stderr, "sparsehash FATAL ERROR: failed to allocate %lu groups\n",
				  static_cast<unsigned long>(n));
		  exit(1);
		}
		return retval;
	  }

	  void free_group() {
		if (!group)  return;
		pointer end_it = group + settings.num_buckets;
		for (pointer p = group; p != end_it; ++p)
		  p->~value_type();
		settings.deallocate(group, settings.num_buckets);
		group = NULL;
	  }

	  static size_type bits_in_char(unsigned char c) {
		// We could make these ints.  The tradeoff is size (eg does it overwhelm
		// the cache?) vs efficiency in referencing sub-word-sized array elements.
		static const char bits_in[256] = {
		  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
		  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8,
		};
		return bits_in[c];
	  }

	 public:                         // get_iter() in sparsetable needs it
	  // We need a small function that tells us how many set bits there are
	  // in positions 0..i-1 of the bitmap.  It uses a big table.
	  // We make it static so templates don't allocate lots of these tables.
	  // There are lots of ways to do this calculation (called 'popcount').
	  // The 8-bit table lookup is one of the fastest, though this
	  // implementation suffers from not doing any loop unrolling.  See, eg,
	  //   http://www.dalkescientific.com/writings/diary/archive/2008/07/03/hakmem_and_other_popcounts.html
	  //   http://gurmeetsingh.wordpress.com/2008/08/05/fast-bit-counting-routines/

	  static size_type pos_to_offset(const unsigned char *bm, size_type pos) {
		size_type retval = 0;

		// [Note: condition pos > 8 is an optimization; convince yourself we
		// give exactly the same result as if we had pos >= 8 here instead.]
		for ( ; pos > 8; pos -= 8 )                   // bm[0..pos/8-1]
		  retval += bits_in_char(*bm++);              // chars we want *all* bits in
		return retval + bits_in_char(*bm & ((1 << pos)-1));    // char including pos
	  }

	  size_type pos_to_offset(size_type pos) const {  // not static but still const
		return pos_to_offset(bitmap, pos);
	  }

	  // Returns the (logical) position in the bm[] array, i, such that
	  // bm[i] is the offset-th set bit in the array.  It is the inverse
	  // of pos_to_offset.  get_pos() uses this function to find the index
	  // of an nonempty_iterator in the table.  Bit-twiddling from
	  // http://hackersdelight.org/basics.pdf
	  static size_type offset_to_pos(const unsigned char *bm, size_type offset) {
		size_type retval = 0;
		// This is sizeof(this->bitmap).
		const size_type group_size = (GROUP_SIZE-1) / 8 + 1;
		for (size_type i = 0; i < group_size; i++) {   // forward scan
		  const size_type pop_count = bits_in_char(*bm);
		  if (pop_count > offset) {
			unsigned char last_bm = *bm;
			for (; offset > 0; offset--) {
			  last_bm &= (last_bm-1);  // remove right-most set bit
			}
			// Clear all bits to the left of the rightmost bit (the &),
			// and then clear the rightmost bit but set all bits to the
			// right of it (the -1).
			last_bm = (last_bm & -last_bm) - 1;
			retval += bits_in_char(last_bm);
			return retval;
		  }
		  offset -= pop_count;
		  retval += 8;
		  bm++;
		}
		return retval;
	  }

	  size_type offset_to_pos(size_type offset) const {
		return offset_to_pos(bitmap, offset);
	  }


	 public:
	  // Constructors -- default and copy -- and destructor
	  explicit sparsegroup(allocator_type& a) :
		  group(0), settings(alloc_impl<value_alloc_type>(a)) {
		memset(bitmap, 0, sizeof(bitmap));
	  }
	  sparsegroup(const sparsegroup& x) : group(0), settings(x.settings) {
		if ( settings.num_buckets ) {
		  group = allocate_group(x.settings.num_buckets);
		  std::uninitialized_copy(x.group, x.group + x.settings.num_buckets, group);
		}
		memcpy(bitmap, x.bitmap, sizeof(bitmap));
	  }
	  ~sparsegroup() { free_group(); }

	  // Operator= is just like the copy constructor, I guess
	  // TODO(austern): Make this exception safe. Handle exceptions in value_type's
	  // copy constructor.
	  sparsegroup &operator=(const sparsegroup& x) {
		if ( &x == this ) return *this;                    // x = x
		if ( x.settings.num_buckets == 0 ) {
		  free_group();
		} else {
		  pointer p = allocate_group(x.settings.num_buckets);
		  std::uninitialized_copy(x.group, x.group + x.settings.num_buckets, p);
		  free_group();
		  group = p;
		}
		memcpy(bitmap, x.bitmap, sizeof(bitmap));
		settings.num_buckets = x.settings.num_buckets;
		return *this;
	  }

	  // Many STL algorithms use swap instead of copy constructors
	  void swap(sparsegroup& x) {
		std::swap(group, x.group);                // defined in <algorithm>
		for ( int i = 0; i < sizeof(bitmap) / sizeof(*bitmap); ++i )
		  std::swap(bitmap[i], x.bitmap[i]);      // swap not defined on arrays
		std::swap(settings.num_buckets, x.settings.num_buckets);
		// we purposefully don't swap the allocator, which may not be swap-able
	  }

	  // It's always nice to be able to clear a table without deallocating it
	  void clear() {
		free_group();
		memset(bitmap, 0, sizeof(bitmap));
		settings.num_buckets = 0;
	  }

	  // Functions that tell you about size.  Alas, these aren't so useful
	  // because our table is always fixed size.
	  size_type size() const           { return GROUP_SIZE; }
	  size_type max_size() const       { return GROUP_SIZE; }
	  bool empty() const               { return false; }
	  // We also may want to know how many *used* buckets there are
	  size_type num_nonempty() const   { return settings.num_buckets; }


	  // get()/set() are explicitly const/non-const.  You can use [] if
	  // you want something that can be either (potentially more expensive).
	  const_reference get(size_type i) const {
		if ( bmtest(i) )           // bucket i is occupied
		  return group[pos_to_offset(bitmap, i)];
		else
		  return default_value();  // return the default reference
	  }

	  // TODO(csilvers): make protected + friend
	  // This is used by sparse_hashtable to get an element from the table
	  // when we know it exists.
	  const_reference unsafe_get(size_type i) const {
		assert(bmtest(i));
		return group[pos_to_offset(bitmap, i)];
	  }

	  // TODO(csilvers): make protected + friend
	  reference mutating_get(size_type i) {    // fills bucket i before getting
		if ( !bmtest(i) )
		  set(i, default_value());
		return group[pos_to_offset(bitmap, i)];
	  }

	  // Syntactic sugar.  It's easy to return a const reference.  To
	  // return a non-const reference, we need to use the assigner adaptor.
	  const_reference operator[](size_type i) const {
		return get(i);
	  }

	  element_adaptor operator[](size_type i) {
		return element_adaptor(this, i);
	  }

	 private:
	  // Create space at group[offset], assuming value_type has trivial
	  // copy constructor and destructor, and the allocator_type is
	  // the default libc_allocator_with_alloc.  (Really, we want it to have
	  // "trivial move", because that's what realloc and memmove both do.
	  // But there's no way to capture that using type_traits, so we
	  // pretend that move(x, y) is equivalent to "x.~T(); new(x) T(y);"
	  // which is pretty much correct, if a bit conservative.)
	  void set_aux(size_type offset, true_type) {
		group = settings.realloc_or_die(group, settings.num_buckets+1);
		// This is equivalent to memmove(), but faster on my Intel P4,
		// at least with gcc4.1 -O2 / glibc 2.3.6.
		for (size_type i = settings.num_buckets; i > offset; --i)
		  memcpy(group + i, group + i-1, sizeof(*group));
	  }

	  // Create space at group[offset], without special assumptions about value_type
	  // and allocator_type.
	  void set_aux(size_type offset, false_type) {
		// This is valid because 0 <= offset <= num_buckets
		pointer p = allocate_group(settings.num_buckets + 1);
		std::uninitialized_copy(group, group + offset, p);
		std::uninitialized_copy(group + offset, group + settings.num_buckets,
								p + offset + 1);
		free_group();
		group = p;
	  }

	 public:
	  // This returns a reference to the inserted item (which is a copy of val).
	  // TODO(austern): Make this exception safe: handle exceptions from
	  // value_type's copy constructor.
	  reference set(size_type i, const_reference val) {
		size_type offset = pos_to_offset(bitmap, i);  // where we'll find (or insert)
		if ( bmtest(i) ) {
		  // Delete the old value, which we're replacing with the new one
		  group[offset].~value_type();
		} else {
		  typedef integral_constant<bool,
			  (base::has_trivial_copy<value_type>::value &&
			   base::has_trivial_destructor<value_type>::value &&
			   base::is_same<
				   allocator_type,
				   libc_allocator_with_realloc<value_type> >::value)>
			  realloc_and_memmove_ok; // we pretend mv(x,y) == "x.~T(); new(x) T(y)"
		  set_aux(offset, realloc_and_memmove_ok());
		  ++settings.num_buckets;
		  bmset(i);
		}
		// This does the actual inserting.  Since we made the array using
		// malloc, we use "placement new" to just call the constructor.
		new(&group[offset]) value_type(val);
		return group[offset];
	  }

	  // We let you see if a bucket is non-empty without retrieving it
	  bool test(size_type i) const {
		return bmtest(i) != 0;
	  }
	  bool test(iterator pos) const {
		return bmtest(pos.pos) != 0;
	  }

	 private:
	  // Shrink the array, assuming value_type has trivial copy
	  // constructor and destructor, and the allocator_type is the default
	  // libc_allocator_with_alloc.  (Really, we want it to have "trivial
	  // move", because that's what realloc and memmove both do.  But
	  // there's no way to capture that using type_traits, so we pretend
	  // that move(x, y) is equivalent to ""x.~T(); new(x) T(y);"
	  // which is pretty much correct, if a bit conservative.)
	  void erase_aux(size_type offset, true_type) {
		// This isn't technically necessary, since we know we have a
		// trivial destructor, but is a cheap way to get a bit more safety.
		group[offset].~value_type();
		// This is equivalent to memmove(), but faster on my Intel P4,
		// at lesat with gcc4.1 -O2 / glibc 2.3.6.
		assert(settings.num_buckets > 0);
		for (size_type i = offset; i < settings.num_buckets-1; ++i)
		  memcpy(group + i, group + i+1, sizeof(*group));  // hopefully inlined!
		group = settings.realloc_or_die(group, settings.num_buckets-1);
	  }

	  // Shrink the array, without any special assumptions about value_type and
	  // allocator_type.
	  void erase_aux(size_type offset, false_type) {
		// This is valid because 0 <= offset < num_buckets. Note the inequality.
		pointer p = allocate_group(settings.num_buckets - 1);
		std::uninitialized_copy(group, group + offset, p);
		std::uninitialized_copy(group + offset + 1, group + settings.num_buckets,
								p + offset);
		free_group();
		group = p;
	  }

	 public:
	  // This takes the specified elements out of the group.  This is
	  // "undefining", rather than "clearing".
	  // TODO(austern): Make this exception safe: handle exceptions from
	  // value_type's copy constructor.
	  void erase(size_type i) {
		if ( bmtest(i) ) {                         // trivial to erase empty bucket
		  size_type offset = pos_to_offset(bitmap,i); // where we'll find (or insert)
		  if ( settings.num_buckets == 1 ) {
			free_group();
			group = NULL;
		  } else {
			typedef base::integral_constant<bool,
				(base::has_trivial_copy<value_type>::value &&
				 base::has_trivial_destructor<value_type>::value &&
				 base::is_same<
					 allocator_type,
					 libc_allocator_with_realloc<value_type> >::value)>
				realloc_and_memmove_ok; // pretend mv(x,y) == "x.~T(); new(x) T(y)"
			erase_aux(offset, realloc_and_memmove_ok());
		  }
		  --settings.num_buckets;
		  bmclear(i);
		}
	  }

	  void erase(iterator pos) {
		erase(pos.pos);
	  }

	  void erase(iterator start_it, iterator end_it) {
		// This could be more efficient, but to do so we'd need to make
		// bmclear() clear a range of indices.  Doesn't seem worth it.
		for ( ; start_it != end_it; ++start_it )
		  erase(start_it);
	  }


	  // I/O
	  // We support reading and writing groups to disk.  We don't store
	  // the actual array contents (which we don't know how to store),
	  // just the bitmap and size.  Meant to be used with table I/O.

	  template <typename OUTPUT> bool write_metadata(OUTPUT *fp) const {
		// we explicitly set to unsigned short int
		assert(sizeof(settings.num_buckets) == 2);
		if ( !sparsehash_internal::write_bigendian_number(fp, settings.num_buckets,
														  2) )
		  return false;
		if ( !sparsehash_internal::write_data(fp, bitmap, sizeof(bitmap)) )
		  return false;
		return true;
	  }

	  // Reading destroys the old group contents!  Returns true if all was ok.
	  template <typename INPUT> bool read_metadata(INPUT *fp) {
		clear();
		if ( !sparsehash_internal::read_bigendian_number(fp, &settings.num_buckets,
														 2) )
		  return false;
		if ( !sparsehash_internal::read_data(fp, bitmap, sizeof(bitmap)) )
		  return false;
		// We'll allocate the space, but we won't fill it: it will be
		// left as uninitialized raw memory.
		group = allocate_group(settings.num_buckets);
		return true;
	  }

	  // Again, only meaningful if value_type is a POD.
	  template <typename INPUT> bool read_nopointer_data(INPUT *fp) {
		 for ( nonempty_iterator it = nonempty_begin();
			   it != nonempty_end(); ++it ) {
		   if ( !sparsehash_internal::read_data(fp, &(*it), sizeof(*it)) )
			 return false;
		 }
		 return true;
	  }

	  // If your keys and values are simple enough, we can write them
	  // to disk for you.  "simple enough" means POD and no pointers.
	  // However, we don't try to normalize endianness.
	  template <typename OUTPUT> bool write_nopointer_data(OUTPUT *fp) const {
		for ( const_nonempty_iterator it = nonempty_begin();
			  it != nonempty_end(); ++it ) {
		  if ( !sparsehash_internal::write_data(fp, &(*it), sizeof(*it)) )
			return false;
		}
		return true;
	  }


	  // Comparisons.  We only need to define == and < -- we get
	  // != > <= >= via relops.h (which we happily included above).
	  // Note the comparisons are pretty arbitrary: we compare
	  // values of the first index that isn't equal (using default
	  // value for empty buckets).
	  bool operator==(const sparsegroup& x) const {
		return ( settings.num_buckets == x.settings.num_buckets &&
				 memcmp(bitmap, x.bitmap, sizeof(bitmap)) == 0 &&
				 std::equal(begin(), end(), x.begin()) );    // from <algorithm>
	  }

	  bool operator<(const sparsegroup& x) const {      // also from <algorithm>
		return std::lexicographical_compare(begin(), end(), x.begin(), x.end());
	  }
	  bool operator!=(const sparsegroup& x) const { return !(*this == x); }
	  bool operator<=(const sparsegroup& x) const { return !(x < *this); }
	  bool operator>(const sparsegroup& x) const { return x < *this; }
	  bool operator>=(const sparsegroup& x) const { return !(*this < x); }

	 private:
	  template <class A> class alloc_impl : public A 
	  {
	   public:
		typedef typename A::pointer pointer;
		typedef typename A::size_type size_type;

		// Convert a normal allocator to one that has realloc_or_die()
		alloc_impl(const A& a) : A(a) { }

		// realloc_or_die should only be used when using the default
		// allocator (libc_allocator_with_realloc).
		pointer realloc_or_die(pointer /*ptr*/, size_type /*n*/) {
		  fprintf(stderr, "realloc_or_die is only supported for "
						  "libc_allocator_with_realloc\n");
		  exit(1);
		  return NULL;
		}
	  };

	  // A template specialization of alloc_impl for
	  // libc_allocator_with_realloc that can handle realloc_or_die.
	  template <class A> class alloc_impl<libc_allocator_with_realloc<A> > : public libc_allocator_with_realloc<A> 
	  {
	   public:
		typedef typename libc_allocator_with_realloc<A>::pointer pointer;
		typedef typename libc_allocator_with_realloc<A>::size_type size_type;

		alloc_impl(const libc_allocator_with_realloc<A>& a)
			: libc_allocator_with_realloc<A>(a) { }

		pointer realloc_or_die(pointer ptr, size_type n) {
		  pointer retval = this->reallocate(ptr, n);
		  if (retval == NULL) {
			fprintf(stderr, "sparsehash: FATAL ERROR: failed to reallocate "
					"%lu elements for ptr %p", static_cast<unsigned long>(n), ptr);
			exit(1);
		  }
		  return retval;
		}
	  };

	  // Package allocator with num_buckets to eliminate memory needed for the
	  // zero-size allocator.
	  // If new fields are added to this class, we should add them to
	  // operator= and swap.
	  class Settings : public alloc_impl<value_alloc_type> {
	   public:
		Settings(const alloc_impl<value_alloc_type>& a, unsigned short int n = 0)
			: alloc_impl<value_alloc_type>(a), num_buckets(n) { }
		Settings(const Settings& s)
			: alloc_impl<value_alloc_type>(s), num_buckets(s.num_buckets) { }

		unsigned short int num_buckets;                    // limits GROUP_SIZE to 64K
	  };

	  // The actual data
	  pointer group;                              // (small) array of T's
	  Settings settings;                          // allocator and num_buckets
	  unsigned char bitmap[(GROUP_SIZE-1)/8 + 1]; // fancy math is so we round up
	};

	// ---------------------------------------------------------------------------


	// We need a global swap as well
	template <class T, unsigned short int GROUP_SIZE, class Alloc>	inline void swap(sparsegroup<T,GROUP_SIZE,Alloc> &x,sparsegroup<T,GROUP_SIZE,Alloc> &y) 
	{
	  x.swap(y);
	}

	// ---------------------------------------------------------------------------


// note: fwd declared as     template <class T, unsigned short int GROUP_SIZE = 48, class Alloc = libc_allocator_with_realloc<T>>			class sparsetable;
template <class T, unsigned short int GROUP_SIZE, class Alloc >	class sparsetable 
{
	 private:
	  typedef typename Alloc::template rebind<T>::other value_alloc_type;
	  typedef typename Alloc::template rebind< sparsegroup<T, GROUP_SIZE, value_alloc_type>  >::other vector_alloc;

	 public:
	  // Basic types
	  typedef T value_type;                        // stolen from stl_vector.h
	  typedef Alloc allocator_type;
	  typedef typename value_alloc_type::size_type size_type;
	  typedef typename value_alloc_type::difference_type difference_type;
	  typedef typename value_alloc_type::reference reference;
	  typedef typename value_alloc_type::const_reference const_reference;
	  typedef typename value_alloc_type::pointer pointer;
	  typedef typename value_alloc_type::const_pointer const_pointer;
	  typedef table_iterator<sparsetable<T, GROUP_SIZE, Alloc> > iterator;
	  typedef const_table_iterator<sparsetable<T, GROUP_SIZE, Alloc> >  const_iterator;
	  typedef table_element_adaptor<sparsetable<T, GROUP_SIZE, Alloc> >
		  element_adaptor;
	  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
	  typedef std::reverse_iterator<iterator> reverse_iterator;   // from iterator.h

	  // These are our special iterators, that go over non-empty buckets in a
	  // table.  These aren't const only because you can change non-empty bcks.
	  typedef two_d_iterator< std::vector< sparsegroup<value_type, GROUP_SIZE,
													   value_alloc_type>,
									  vector_alloc> >
		 nonempty_iterator;
	  typedef const_two_d_iterator< std::vector< sparsegroup<value_type,
															 GROUP_SIZE,
															 value_alloc_type>,
											vector_alloc> >
		 const_nonempty_iterator;
	  typedef std::reverse_iterator<nonempty_iterator> reverse_nonempty_iterator;
	  typedef std::reverse_iterator<const_nonempty_iterator> const_reverse_nonempty_iterator;
	  // Another special iterator: it frees memory as it iterates (used to resize)
	  typedef destructive_two_d_iterator< std::vector< sparsegroup<value_type,
																   GROUP_SIZE,
																   value_alloc_type>,
												  vector_alloc> >
		 destructive_iterator;

	  // Iterator functions
	  iterator begin()                      { return iterator(this, 0); }
	  const_iterator begin() const          { return const_iterator(this, 0); }
	  iterator end()                        { return iterator(this, size()); }
	  const_iterator end() const            { return const_iterator(this, size()); }
	  reverse_iterator rbegin()             { return reverse_iterator(end()); }
	  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
	  reverse_iterator rend()               { return reverse_iterator(begin()); }
	  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }

	  // Versions for our special non-empty iterator
	  nonempty_iterator nonempty_begin()             {
		return nonempty_iterator(groups.begin(), groups.end(), groups.begin());
	  }
	  const_nonempty_iterator nonempty_begin() const {
		return const_nonempty_iterator(groups.begin(),groups.end(), groups.begin());
	  }
	  nonempty_iterator nonempty_end() {
		return nonempty_iterator(groups.begin(), groups.end(), groups.end());
	  }
	  const_nonempty_iterator nonempty_end() const {
		return const_nonempty_iterator(groups.begin(), groups.end(), groups.end());
	  }
	  reverse_nonempty_iterator nonempty_rbegin() {
		return reverse_nonempty_iterator(nonempty_end());
	  }
	  const_reverse_nonempty_iterator nonempty_rbegin() const {
		return const_reverse_nonempty_iterator(nonempty_end());
	  }
	  reverse_nonempty_iterator nonempty_rend() {
		return reverse_nonempty_iterator(nonempty_begin());
	  }
	  const_reverse_nonempty_iterator nonempty_rend() const {
		return const_reverse_nonempty_iterator(nonempty_begin());
	  }
	  destructive_iterator destructive_begin() {
		return destructive_iterator(groups.begin(), groups.end(), groups.begin());
	  }
	  destructive_iterator destructive_end() {
		return destructive_iterator(groups.begin(), groups.end(), groups.end());
	  }

	  typedef sparsegroup<value_type, GROUP_SIZE, allocator_type> group_type;
	  typedef std::vector<group_type, vector_alloc > group_vector_type;

	  typedef typename group_vector_type::reference GroupsReference;
	  typedef typename group_vector_type::const_reference GroupsConstReference;
	  typedef typename group_vector_type::iterator GroupsIterator;
	  typedef typename group_vector_type::const_iterator GroupsConstIterator;

	  // How to deal with the proper group
	  static size_type num_groups(size_type num) {   // how many to hold num buckets
		return num == 0 ? 0 : ((num-1) / GROUP_SIZE) + 1;
	  }

	  unsigned short int pos_in_group(size_type i) const {
		return static_cast<unsigned short int>(i % GROUP_SIZE);
	  }
	  size_type group_num(size_type i) const {
		return i / GROUP_SIZE;
	  }
	  GroupsReference which_group(size_type i) {
		return groups[group_num(i)];
	  }
	  GroupsConstReference which_group(size_type i) const {
		return groups[group_num(i)];
	  }

	 public:
	  // Constructors -- default, normal (when you specify size), and copy
	  explicit sparsetable(size_type sz = 0, Alloc alloc = Alloc())
		  : groups(vector_alloc(alloc)), settings(alloc, sz) {
		groups.resize(num_groups(sz), group_type(settings));
	  }
	  // We can get away with using the default copy constructor,
	  // and default destructor, and hence the default operator=.  Huzzah!

	  // Many STL algorithms use swap instead of copy constructors
	  void swap(sparsetable& x) {
		std::swap(groups, x.groups);              // defined in stl_algobase.h
		std::swap(settings.table_size, x.settings.table_size);
		std::swap(settings.num_buckets, x.settings.num_buckets);
	  }

	  // It's always nice to be able to clear a table without deallocating it
	  void clear() {
		GroupsIterator group;
		for ( group = groups.begin(); group != groups.end(); ++group ) {
		  group->clear();
		}
		settings.num_buckets = 0;
	  }

	  // ACCESSOR FUNCTIONS for the things we templatize on, basically
	  allocator_type get_allocator() const {
		return allocator_type(settings);
	  }


	  // Functions that tell you about size.
	  // NOTE: empty() is non-intuitive!  It does not tell you the number
	  // of not-empty buckets (use num_nonempty() for that).  Instead
	  // it says whether you've allocated any buckets or not.
	  size_type size() const           { return settings.table_size; }
	  size_type max_size() const       { return settings.max_size(); }
	  bool empty() const               { return settings.table_size == 0; }
	  // We also may want to know how many *used* buckets there are
	  size_type num_nonempty() const   { return settings.num_buckets; }

	  // OK, we'll let you resize one of these puppies
	  void resize(size_type new_size) {
		groups.resize(num_groups(new_size), group_type(settings));
		if ( new_size < settings.table_size) {
		  // lower num_buckets, clear last group
		  if ( pos_in_group(new_size) > 0 )     // need to clear inside last group
			groups.back().erase(groups.back().begin() + pos_in_group(new_size),
								groups.back().end());
		  settings.num_buckets = 0;                   // refigure # of used buckets
		  GroupsConstIterator group;
		  for ( group = groups.begin(); group != groups.end(); ++group )
			settings.num_buckets += group->num_nonempty();
		}
		settings.table_size = new_size;
	  }


	  // We let you see if a bucket is non-empty without retrieving it
	  bool test(size_type i) const {
		assert(i < settings.table_size);
		return which_group(i).test(pos_in_group(i));
	  }
	  bool test(iterator pos) const {
		return which_group(pos.pos).test(pos_in_group(pos.pos));
	  }
	  bool test(const_iterator pos) const {
		return which_group(pos.pos).test(pos_in_group(pos.pos));
	  }

	  // We only return const_references because it's really hard to
	  // return something settable for empty buckets.  Use set() instead.
	  const_reference get(size_type i) const {
		assert(i < settings.table_size);
		return which_group(i).get(pos_in_group(i));
	  }

	  // TODO(csilvers): make protected + friend
	  // This is used by sparse_hashtable to get an element from the table
	  // when we know it exists (because the caller has called test(i)).
	  const_reference unsafe_get(size_type i) const {
		assert(i < settings.table_size);
		assert(test(i));
		return which_group(i).unsafe_get(pos_in_group(i));
	  }

	  // TODO(csilvers): make protected + friend element_adaptor
	  reference mutating_get(size_type i) {    // fills bucket i before getting
		assert(i < settings.table_size);
		typename group_type::size_type old_numbuckets = which_group(i).num_nonempty();
		reference retval = which_group(i).mutating_get(pos_in_group(i));
		settings.num_buckets += which_group(i).num_nonempty() - old_numbuckets;
		return retval;
	  }

	  // Syntactic sugar.  As in sparsegroup, the non-const version is harder
	  const_reference operator[](size_type i) const {
		return get(i);
	  }

	  element_adaptor operator[](size_type i) {
		return element_adaptor(this, i);
	  }

	  // Needed for hashtables, gets as a nonempty_iterator.  Crashes for empty bcks
	  const_nonempty_iterator get_iter(size_type i) const {
		assert(test(i));    // how can a nonempty_iterator point to an empty bucket?  (a production user asks in 2012++.... )  I suspect that by replacing the global static const bucket size with the "equally as hardcoded" solution here, it allows the compiler to optimize as efficiently as possible.  The machine code from this build is likely the solution.  Test it.
		return const_nonempty_iterator(
		  groups.begin(), groups.end(),
		  groups.begin() + group_num(i),
		  (groups[group_num(i)].nonempty_begin() +
		   groups[group_num(i)].pos_to_offset(pos_in_group(i))));
	  }
	  // For nonempty we can return a non-const version
	  nonempty_iterator get_iter(size_type i) {
		assert(test(i));    // how can a nonempty_iterator point to an empty bucket?
		return nonempty_iterator(
		  groups.begin(), groups.end(),
		  groups.begin() + group_num(i),
		  (groups[group_num(i)].nonempty_begin() +
		   groups[group_num(i)].pos_to_offset(pos_in_group(i))));
	  }

	  // And the reverse transformation.
	  size_type get_pos(const const_nonempty_iterator it) const {
		difference_type current_row = it.row_current - it.row_begin;
		difference_type current_col = (it.col_current -
									   groups[current_row].nonempty_begin());
		return ((current_row * GROUP_SIZE) +
				groups[current_row].offset_to_pos(current_col));
	  }


	  // This returns a reference to the inserted item (which is a copy of val)
	  // The trick is to figure out whether we're replacing or inserting anew
	  reference set(size_type i, const_reference val) {
		assert(i < settings.table_size);
		typename group_type::size_type old_numbuckets = which_group(i).num_nonempty();
		reference retval = which_group(i).set(pos_in_group(i), val);
		settings.num_buckets += which_group(i).num_nonempty() - old_numbuckets;
		return retval;
	  }

	  // This takes the specified elements out of the table.  This is
	  // "undefining", rather than "clearing".
	  void erase(size_type i) {
		assert(i < settings.table_size);
		typename group_type::size_type old_numbuckets = which_group(i).num_nonempty();
		which_group(i).erase(pos_in_group(i));
		settings.num_buckets += which_group(i).num_nonempty() - old_numbuckets;
	  }

	  void erase(iterator pos) {
		erase(pos.pos);
	  }

	  void erase(iterator start_it, iterator end_it) {
		// This could be more efficient, but then we'd need to figure
		// out if we spanned groups or not.  Doesn't seem worth it.
		for ( ; start_it != end_it; ++start_it )
		  erase(start_it);
	  }


	  // We support reading and writing tables to disk.  We don't store
	  // the actual array contents (which we don't know how to store),
	  // just the groups and sizes.  Returns true if all went ok.

	 private:
	  // Every time the disk format changes, this should probably change too
	  typedef unsigned long MagicNumberType;
	  static const MagicNumberType MAGIC_NUMBER = 0x24687531;

	  // Old versions of this code write all data in 32 bits.  We need to
	  // support these files as well as having support for 64-bit systems.
	  // So we use the following encoding scheme: for values < 2^32-1, we
	  // store in 4 bytes in big-endian order.  For values > 2^32, we
	  // store 0xFFFFFFF followed by 8 bytes in big-endian order.  This
	  // causes us to mis-read old-version code that stores exactly
	  // 0xFFFFFFF, but I don't think that is likely to have happened for
	  // these particular values.
	  template <typename OUTPUT, typename IntType>
	  static bool write_32_or_64(OUTPUT* fp, IntType value) {
		if ( value < 0xFFFFFFFFULL ) {        // fits in 4 bytes
		  if ( !sparsehash_internal::write_bigendian_number(fp, value, 4) )
			return false;
		} else {
		  if ( !sparsehash_internal::write_bigendian_number(fp, 0xFFFFFFFFUL, 4) )
			return false;
		  if ( !sparsehash_internal::write_bigendian_number(fp, value, 8) )
			return false;
		}
		return true;
	  }

	  template <typename INPUT, typename IntType>
	  static bool read_32_or_64(INPUT* fp, IntType *value) {  // reads into value
		MagicNumberType first4 = 0;   // a convenient 32-bit unsigned type
		if ( !sparsehash_internal::read_bigendian_number(fp, &first4, 4) )
		  return false;
		if ( first4 < 0xFFFFFFFFULL ) {
		  *value = first4;
		} else {
		  if ( !sparsehash_internal::read_bigendian_number(fp, value, 8) )
			return false;
		}
		return true;
	  }

	 public:
	  // read/write_metadata() and read_write/nopointer_data() are DEPRECATED.
	  // Use serialize() and unserialize(), below, for new code.

	  template <typename OUTPUT> bool write_metadata(OUTPUT *fp) const {
		if ( !write_32_or_64(fp, MAGIC_NUMBER) )  return false;
		if ( !write_32_or_64(fp, settings.table_size) )  return false;
		if ( !write_32_or_64(fp, settings.num_buckets) )  return false;

		GroupsConstIterator group;
		for ( group = groups.begin(); group != groups.end(); ++group )
		  if ( group->write_metadata(fp) == false )  return false;
		return true;
	  }

	  // Reading destroys the old table contents!  Returns true if read ok.
	  template <typename INPUT> bool read_metadata(INPUT *fp) {
		size_type magic_read = 0;
		if ( !read_32_or_64(fp, &magic_read) )  return false;
		if ( magic_read != MAGIC_NUMBER ) {
		  clear();                        // just to be consistent
		  return false;
		}

		if ( !read_32_or_64(fp, &settings.table_size) )  return false;
		if ( !read_32_or_64(fp, &settings.num_buckets) )  return false;

		resize(settings.table_size);                    // so the vector's sized ok
		GroupsIterator group;
		for ( group = groups.begin(); group != groups.end(); ++group )
		  if ( group->read_metadata(fp) == false )  return false;
		return true;
	  }

	  // This code is identical to that for SparseGroup
	  // If your keys and values are simple enough, we can write them
	  // to disk for you.  "simple enough" means no pointers.
	  // However, we don't try to normalize endianness
	  bool write_nopointer_data(FILE *fp) const {
		for ( const_nonempty_iterator it = nonempty_begin();
			  it != nonempty_end(); ++it ) {
		  if ( !fwrite(&*it, sizeof(*it), 1, fp) )  return false;
		}
		return true;
	  }

	  // When reading, we have to override the potential const-ness of *it
	  bool read_nopointer_data(FILE *fp) {
		for ( nonempty_iterator it = nonempty_begin();
			  it != nonempty_end(); ++it ) {
		  if ( !fread(reinterpret_cast<void*>(&(*it)), sizeof(*it), 1, fp) )
			return false;
		}
		return true;
	  }

	  // INPUT and OUTPUT must be either a FILE, *or* a C++ stream
	  //    (istream, ostream, etc) *or* a class providing
	  //    Read(void*, size_t) and Write(const void*, size_t)
	  //    (respectively), which writes a buffer into a stream
	  //    (which the INPUT/OUTPUT instance presumably owns).

	  typedef sparsehash_internal::pod_serializer<value_type> NopointerSerializer;

	  // ValueSerializer: a functor.  operator()(OUTPUT*, const value_type&)
	  template <typename ValueSerializer, typename OUTPUT>
	  bool serialize(ValueSerializer serializer, OUTPUT *fp) {
		if ( !write_metadata(fp) )
		  return false;
		for ( const_nonempty_iterator it = nonempty_begin();
			  it != nonempty_end(); ++it ) {
		  if ( !serializer(fp, *it) )  return false;
		}
		return true;
	  }

	  // ValueSerializer: a functor.  operator()(INPUT*, value_type*)
	  template <typename ValueSerializer, typename INPUT>
	  bool unserialize(ValueSerializer serializer, INPUT *fp) {
		clear();
		if ( !read_metadata(fp) )
		  return false;
		for ( nonempty_iterator it = nonempty_begin();
			  it != nonempty_end(); ++it ) {
		  if ( !serializer(fp, &*it) )  return false;
		}
		return true;
	  }

	  // Comparisons.  Note the comparisons are pretty arbitrary: we
	  // compare values of the first index that isn't equal (using default
	  // value for empty buckets).
	  bool operator==(const sparsetable& x) const {
		return ( settings.table_size == x.settings.table_size &&
				 settings.num_buckets == x.settings.num_buckets &&
				 groups == x.groups );
	  }

	  bool operator<(const sparsetable& x) const {
		return std::lexicographical_compare(begin(), end(), x.begin(), x.end());
	  }
	  bool operator!=(const sparsetable& x) const { return !(*this == x); }
	  bool operator<=(const sparsetable& x) const { return !(x < *this); }
	  bool operator>(const sparsetable& x) const { return x < *this; }
	  bool operator>=(const sparsetable& x) const { return !(*this < x); }


	 private:
	  // Package allocator with table_size and num_buckets to eliminate memory
	  // needed for the zero-size allocator.
	  // If new fields are added to this class, we should add them to
	  // operator= and swap.
	  class Settings : public allocator_type {
	   public:
		typedef typename allocator_type::size_type size_type;

		Settings(const allocator_type& a, size_type sz = 0, size_type n = 0)
			: allocator_type(a), table_size(sz), num_buckets(n) { }

		Settings(const Settings& s)
			: allocator_type(s),
			  table_size(s.table_size), num_buckets(s.num_buckets) { }

		size_type table_size;          // how many buckets they want
		size_type num_buckets;         // number of non-empty buckets
	  };

	  // The actual data
	  group_vector_type groups;        // our list of groups
	  Settings settings;               // allocator, table size, buckets
};

//   2D BEGIN  - 2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D


	template <class T, unsigned short int GROUP_SIZE, class Alloc>	inline void swap(sparsetable<T,GROUP_SIZE,Alloc> &x, sparsetable<T,GROUP_SIZE,Alloc> &y) 
	{
	  x.swap(y);
	}



	// Our iterator as simple as iterators can be: basically it's just
	// the index into our table.  Dereference, the only complicated
	// thing, we punt to the table class.  This just goes to show how
	// much machinery STL requires to do even the most trivial tasks.
	//
	// A NOTE ON ASSIGNING:
	// A sparse table does not actually allocate memory for entries
	// that are not filled.  Because of this, it becomes complicated
	// to have a non-const iterator: we don't know, if the iterator points
	// to a not-filled bucket, whether you plan to fill it with something
	// or whether you plan to read its value (in which case you'll get
	// the default bucket value).  Therefore, while we can define const
	// operations in a pretty 'normal' way, for non-const operations, we
	// define something that returns a helper object with operator= and
	// operator& that allocate a bucket lazily.  We use this for table[]
	// and also for regular table iterators.

	template <class tabletype> class table_element_adaptor 
	{
	 public:
	  typedef typename tabletype::value_type value_type;
	  typedef typename tabletype::size_type size_type;
	  typedef typename tabletype::reference reference;
	  typedef typename tabletype::pointer pointer;

	  table_element_adaptor(tabletype *tbl, size_type p)
		: table(tbl), pos(p) { }
	  table_element_adaptor& operator= (const value_type &val) {
		table->set(pos, val);
		return *this;
	  }
	  operator value_type() { return table->get(pos); }   // we look like a value
	  pointer operator& () { return &table->mutating_get(pos); }

	 private:
	  tabletype* table;
	  size_type pos;
	};


	// support for "3 + iterator" has to be defined outside the class, alas
	template<class T>
	table_iterator<T> operator+(typename table_iterator<T>::difference_type i,
								table_iterator<T> it) {
	  return it + i;               // so people can say it2 = 3 + it
	}

	template <class tabletype>	class const_table_iterator 
	{
	 public:
	  typedef table_iterator<tabletype> iterator;
	  typedef const_table_iterator const_iterator;

	  typedef std::random_access_iterator_tag iterator_category;
	  typedef typename tabletype::value_type value_type;
	  typedef typename tabletype::difference_type difference_type;
	  typedef typename tabletype::size_type size_type;
	  typedef typename tabletype::const_reference reference;  // we're const-only
	  typedef typename tabletype::const_pointer pointer;

	  // The "real" constructor
	  const_table_iterator(const tabletype *tbl, size_type p)
		: table(tbl), pos(p) { }
	  // The default constructor, used when I define vars of type table::iterator
	  const_table_iterator() : table(NULL), pos(0) { }
	  // The copy constructor, for when I say table::iterator foo = tbl.begin()
	  // Also converts normal iterators to const iterators
	  const_table_iterator(const iterator &from)
		: table(from.table), pos(from.pos) { }
	  // The default destructor is fine; we don't define one
	  // The default operator= is fine; we don't define one

	  // The main thing our iterator does is dereference.  If the table entry
	  // we point to is empty, we return the default value type.
	  reference operator*() const       { return (*table)[pos]; }
	  pointer operator->() const        { return &(operator*()); }

	  // Helper function to assert things are ok; eg pos is still in range
	  void check() const {
		assert(table);
		assert(pos <= table->size());
	  }

	  // Arithmetic: we just do arithmetic on pos.  We don't even need to
	  // do bounds checking, since STL doesn't consider that its job.  :-)
	  const_iterator& operator+=(size_type t) { pos += t; check(); return *this; }
	  const_iterator& operator-=(size_type t) { pos -= t; check(); return *this; }
	  const_iterator& operator++()            { ++pos; check(); return *this; }
	  const_iterator& operator--()            { --pos; check(); return *this; }
	  const_iterator operator++(int)          { const_iterator tmp(*this); // for x++
												++pos; check(); return tmp; }
	  const_iterator operator--(int)          { const_iterator tmp(*this); // for x--
												--pos; check(); return tmp; }
	  const_iterator operator+(difference_type i) const  { const_iterator tmp(*this);
														   tmp += i; return tmp; }
	  const_iterator operator-(difference_type i) const  { const_iterator tmp(*this);
														   tmp -= i; return tmp; }
	  difference_type operator-(const_iterator it) const {   // for "x = it2 - it"
		assert(table == it.table);
		return pos - it.pos;
	  }
	  reference operator[](difference_type n) const {
		return *(*this + n);            // simple though not totally efficient
	  }

	  // Comparisons.
	  bool operator==(const const_iterator& it) const {
		return table == it.table && pos == it.pos;
	  }
	  bool operator<(const const_iterator& it) const {
		assert(table == it.table);              // life is bad bad bad otherwise
		return pos < it.pos;
	  }
	  bool operator!=(const const_iterator& it) const { return !(*this == it); }
	  bool operator<=(const const_iterator& it) const { return !(it < *this); }
	  bool operator>(const const_iterator& it) const { return it < *this; }
	  bool operator>=(const const_iterator& it) const { return !(*this < it); }

	  // Here's the info we actually need to be an iterator
	  const tabletype *table;        // so we can dereference and bounds-check
	  size_type pos;                 // index into the table
	};

	// support for "3 + iterator" has to be defined outside the class, alas
	template<class T>
	const_table_iterator<T> operator+(typename
									  const_table_iterator<T>::difference_type i,
									  const_table_iterator<T> it) {
	  return it + i;               // so people can say it2 = 3 + it
	}

// 2D END   2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D2D





//  Now begin the class definition that wraps it at the low level





//  Note: ext::hash, __gnu_cxx::hash , std::tr1::hash, stdext::hash_compare, depending on the OS
#ifdef _WIN32
template <class Key, class T, class HashFcn = std::tr1::hash<Key>,  class EqualKey = std::equal_to<Key>, class Alloc = libc_allocator_with_realloc<std::pair<const Key, T> > >  class sparse_hash_map 
#else
template <class Key, class T, class HashFcn = __gnu_cxx::hash<Key>,  class EqualKey = std::equal_to<Key>, class Alloc = libc_allocator_with_realloc<std::pair<const Key, T> > >  class sparse_hash_map 
#endif
{
 protected: // changed from private to protected 12/19/2013.
  // Apparently select1st is not stl-standard, so we define our own
  struct SelectKey 
  {
    typedef const Key& result_type;
    const Key& operator()(const std::pair<const Key, T>& p) const 
	{
      return p.first;
    }
  };
  struct SetKey {
    void operator()(std::pair<const Key, T>* value, const Key& new_key) const 
	{
      *const_cast<Key*>(&value->first) = new_key;
      // It would be nice to clear the rest of value here as well, in
      // case it's taking up a lot of memory.  We do this by clearing
      // the value.  This assumes T has a zero-arg constructor!
      value->second = T();
    }
  };
  // For operator[].
  struct DefaultValue {
    std::pair<const Key, T> operator()(const Key& key) 
	{
      return std::make_pair(key, T());
    }
  };

  // The actual data
  typedef sparse_hashtable<std::pair<const Key, T>, Key, HashFcn, SelectKey, SetKey, EqualKey, Alloc> ht;
  ht rep;

 public:
  typedef typename ht::key_type key_type;
  typedef T data_type;
  typedef T mapped_type;
  typedef typename ht::value_type value_type;
  typedef typename ht::hasher hasher;

  typedef typename ht::key_equal key_equal;
  typedef Alloc allocator_type;

  typedef typename ht::size_type size_type;
  typedef typename ht::difference_type difference_type;
  typedef typename ht::pointer pointer;
  typedef typename ht::const_pointer const_pointer;
  typedef typename ht::reference reference;
  typedef typename ht::const_reference const_reference;

  typedef typename ht::iterator iterator;
  typedef typename ht::const_iterator const_iterator;
  typedef typename ht::local_iterator local_iterator;
  typedef typename ht::const_local_iterator const_local_iterator;

  // Iterator functions
  iterator begin()                               { return rep.begin(); }
  iterator end()                                 { return rep.end(); }
  const_iterator begin() const                   { return rep.begin(); }
  const_iterator end() const                     { return rep.end(); }

  // These come from tr1's unordered_map. For us, a bucket has 0 or 1 elements.
  local_iterator begin(size_type i)              { return rep.begin(i); }
  local_iterator end(size_type i)                { return rep.end(i); }
  const_local_iterator begin(size_type i) const  { return rep.begin(i); }
  const_local_iterator end(size_type i) const    { return rep.end(i); }

  // Accessor functions
  allocator_type get_allocator() const           { return rep.get_allocator(); }
  hasher hash_funct() const                      { return rep.hash_funct(); }
  hasher hash_function() const                   { return hash_funct(); }
  key_equal key_eq() const                       { return rep.key_eq(); }


  // Constructors
  explicit sparse_hash_map(size_type expected_max_items_in_table = 0,  const hasher& hf = hasher(), const key_equal& eql = key_equal(), const allocator_type& alloc = allocator_type())
    : rep(expected_max_items_in_table, hf, eql, SelectKey(), SetKey(), alloc) 
  {
  }

  template <class InputIterator>  sparse_hash_map(InputIterator f, InputIterator l,size_type expected_max_items_in_table = 0, const hasher& hf = hasher(),const key_equal& eql = key_equal(), const allocator_type& alloc = allocator_type())   : rep(expected_max_items_in_table, hf, eql, SelectKey(), SetKey(), alloc) 
  {
    rep.insert(f, l);
  }
  // We use the default copy constructor
  // We use the default operator=()
  // We use the default destructor

  void clear()                        { rep.clear(); }
  void swap(sparse_hash_map& hs)      { rep.swap(hs.rep); }


  // Functions concerning size
  size_type size() const              { return rep.size(); }
  size_type max_size() const          { return rep.max_size(); }
  bool empty() const                  { return rep.empty(); }
  size_type bucket_count() const      { return rep.bucket_count(); }
  size_type max_bucket_count() const  { return rep.max_bucket_count(); }

  // These are tr1 methods.  bucket() is the bucket the key is or would be in.
  size_type bucket_size(size_type i) const    { return rep.bucket_size(i); }
  size_type bucket(const key_type& key) const { return rep.bucket(key); }
  float load_factor() const 
  {
    return size() * 1.0f / bucket_count();
  }
  float max_load_factor() const 
  {
    float shrink, grow;
    rep.get_resizing_parameters(&shrink, &grow);
    return grow;
  }
  void max_load_factor(float new_grow) 
  {
    float shrink, grow;
    rep.get_resizing_parameters(&shrink, &grow);
    rep.set_resizing_parameters(shrink, new_grow);
  }
  // These aren't tr1 methods but perhaps ought to be.
  float min_load_factor() const 
  {
    float shrink, grow;
    rep.get_resizing_parameters(&shrink, &grow);
    return shrink;
  }
  void min_load_factor(float new_shrink) 
  {
    float shrink, grow;
    rep.get_resizing_parameters(&shrink, &grow);
    rep.set_resizing_parameters(new_shrink, grow);
  }
//  // Deprecated; use min_load_factor() or max_load_factor() instead.
 // void set_resizing_parameters(float shrink, float grow) {
 //   rep.set_resizing_parameters(shrink, grow);
//  }

  void resize(size_type hint)         { rep.resize(hint); }
  void rehash(size_type hint)         { resize(hint); }      // the tr1 name

  // Lookup routines
  iterator find(const key_type& key)                 { return rep.find(key); }
  const_iterator find(const key_type& key) const     { return rep.find(key); }

  data_type& operator[](const key_type& key) 
  {
    // If key is in the hashtable, returns find(key)->second,
    // otherwise returns insert(value_type(key, T()).first->second.
    // Note it does not create an empty T unless the find fails.
    return rep.template find_or_insert<DefaultValue>(key).second;
  }

  size_type count(const key_type& key) const         { return rep.count(key); }

  std::pair<iterator, iterator> equal_range(const key_type& key) {
    return rep.equal_range(key);
  }
  std::pair<const_iterator, const_iterator> equal_range(const key_type& key)
      const {
    return rep.equal_range(key);
  }

  // Insertion routines
  std::pair<iterator, bool> insert(const value_type& obj) {
    return rep.insert(obj);
  }
  template <class InputIterator> void insert(InputIterator f, InputIterator l) 
  {
    rep.insert(f, l);
  }
  void insert(const_iterator f, const_iterator l) {
    rep.insert(f, l);
  }
  // Required for std::insert_iterator; the passed-in iterator is ignored.
  iterator insert(iterator, const value_type& obj) {
    return insert(obj).first;
  }

  // Deletion routines
  // THESE ARE NON-STANDARD!  I make you specify an "impossible" key
  // value to identify deleted buckets.  You can change the key as
  // time goes on, or get rid of it entirely to be insert-only.
  void set_deleted_key(const key_type& key)   {
    rep.set_deleted_key(key);
  }
  void clear_deleted_key()                    { rep.clear_deleted_key(); }
  key_type deleted_key() const                { return rep.deleted_key(); }

  // These are standard
  size_type erase(const key_type& key)               { return rep.erase(key); }
  void erase(iterator it)                            { rep.erase(it); }
  void erase(iterator f, iterator l)                 { rep.erase(f, l); }


  // Comparison
  bool operator==(const sparse_hash_map& hs) const   { return rep == hs.rep; }
  bool operator!=(const sparse_hash_map& hs) const   { return rep != hs.rep; }


  // I/O -- this is an add-on for writing metainformation to disk
  //
  // For maximum flexibility, this does not assume a particular
  // file type (though it will probably be a FILE *).  We just pass
  // the fp through to rep.

  // If your keys and values are simple enough, you can pass this
  // serializer to serialize()/unserialize().  "Simple enough" means
  // value_type is a POD type that contains no pointers.  Note,
  // however, we don't try to normalize endianness.
  typedef typename ht::NopointerSerializer NopointerSerializer;

  // serializer: a class providing operator()(OUTPUT*, const value_type&)
  //    (writing value_type to OUTPUT).  You can specify a
  //    NopointerSerializer object if appropriate (see above).
  // fp: either a FILE*, OR an ostream subclass_of_ostream*, OR a
  //    pointer to a class providing size_t Write(const void*, size_t),
  //    which writes a buffer into a stream (which fp presumably
  //    owns) and returns the number of bytes successfully written.
  //    Note basic_ostream<not_char> is not currently supported.
  template <typename ValueSerializer, typename OUTPUT>
  bool serialize(ValueSerializer serializer, OUTPUT* fp) {
    return rep.serialize(serializer, fp);
  }

  // serializer: a functor providing operator()(INPUT*, value_type*)
  //    (reading from INPUT and into value_type).  You can specify a
  //    NopointerSerializer object if appropriate (see above).
  // fp: either a FILE*, OR an istreamsubclass_of_istream*, OR a
  //    pointer to a class providing size_t Read(void*, size_t),
  //    which reads into a buffer from a stream (which fp presumably
  //    owns) and returns the number of bytes successfully read.
  //    Note basic_istream<not_char> is not currently supported.
  // NOTE: Since value_type is std::pair<const Key, T>, ValueSerializer
  // may need to do a const cast in order to fill in the key.
  // NOTE: if Key or T are not POD types, the serializer MUST use
  // placement-new to initialize their values, rather than a normal
  // equals-assignment or similar.  (The value_type* passed into the
  // serializer points to garbage memory.)
  template <typename ValueSerializer, typename INPUT>
  bool unserialize(ValueSerializer serializer, INPUT* fp) {
    return rep.unserialize(serializer, fp);
  }

  // The four methods below are DEPRECATED.
  // Use serialize() and unserialize() for new code.
  template <typename OUTPUT>
  bool write_metadata(OUTPUT *fp)       { return rep.write_metadata(fp); }

  template <typename INPUT>
  bool read_metadata(INPUT *fp)         { return rep.read_metadata(fp); }

  template <typename OUTPUT>
  bool write_nopointer_data(OUTPUT *fp) { return rep.write_nopointer_data(fp); }

  template <typename INPUT>
  bool read_nopointer_data(INPUT *fp)   { return rep.read_nopointer_data(fp); }
};

// We need a global swap as well
template <class Key, class T, class HashFcn, class EqualKey, class Alloc> inline void swap(sparse_hash_map<Key, T, HashFcn, EqualKey, Alloc>& hm1,      sparse_hash_map<Key, T, HashFcn, EqualKey, Alloc>& hm2) 
{
  hm1.swap(hm2);
}

//}


// XMLFoundation extensions

class GSparseHash : public sparse_hash_map<__int64, void *>
{
	unsigned __int64 FVN1Hash(const char *key)
	{
		unsigned __int64 nHash = 14695981039346656037;
		while(*key)
		{
			nHash = (nHash * 1099511628211) ^ *key++;
		}
		return nHash;
	}
public:
	GSparseHash(){}
	virtual ~GSparseHash(){}
    void Insert(const char *key, XMLObject *value) 
	{
        (*this)[FVN1Hash(key)] = (void *)value;
	}
    void Insert(const __int64 &key, XMLObject *value) 
	{
        (*this)[key] = (void *)value;
    }
    void *Search(const char *key) 
	{
		sparse_hash_map<__int64,void *>::const_iterator i2 = find(FVN1Hash(key));
		if (i2 != end())
			return (XMLObject *)i2->second;
		return (void *)0;
	}
    void *Search(const __int64 key) const 
	{
		sparse_hash_map<__int64,void *>::const_iterator i2 = find(key);
		if (i2 != end())
			return (XMLObject *)i2->second;
		return 0;
    }	
    void remove(const int key) {  this->erase(key);   }
    void remove(const char *key)   {  this->erase( FVN1Hash(key) );    }
};

#include "ListAbstraction.h"

typedef void * SparseHashIterator;
class GSparseHashAbstraction : public KeyedDataStructureAbstraction
{
public:
	XMLObject *search(KeyedDataStructure kds, const char *pzKey)
	{
		XMLObject *pRet = 0;
		GSparseHash *pTyped = (GSparseHash *)kds;
		return (XMLObject *)pTyped->Search(pzKey);
	}
	void destroy(KeyedDataStructure kds){ };
	void create(KeyedDataStructure kds) { };
	void insert(KeyedDataStructure kds, XMLObject *pObj, const char *pzKey)
	{
		GSparseHash *pTyped = (GSparseHash *)kds;
		pTyped->Insert(pzKey,pObj);

	}
	XMLObject *getFirst(KeyedDataStructure kds, SparseHashIterator *pIterator)
	{
		XMLObject *pRet = 0;
		const GSparseHash *pTyped = (const GSparseHash *)kds;

		if ( pTyped->size() )
		{
			sparse_hash_map<__int64,void *>::const_iterator *pIter = new sparse_hash_map<__int64,void *>::const_iterator (pTyped->begin());
			*pIterator = pIter;

			if (*pIter != pTyped->end())
			{
				pRet = (XMLObject *)((*pIter)->second);
				(*pIter)++;
			}
		}
		return pRet;
	}
	XMLObject *getNext(KeyedDataStructure kds, SparseHashIterator Iter)
	{
		GSparseHash *pTyped = (GSparseHash *)kds;
		sparse_hash_map<__int64,void *>::const_iterator *pIter = (sparse_hash_map<__int64,void *>::const_iterator *)Iter;
		XMLObject *pRet = 0;
		if (*pIter != pTyped->end())
		{
			pRet = (XMLObject *)((*pIter)->second);
			(*pIter)++;
		}
		else
		{
			delete pIter;
			pIter = 0;
		}
		return pRet;
	}
	void remove(KeyedDataStructure kds, SparseHashIterator Iterator, const char *pzKey, XMLObject *pObj)
	{
		GSparseHash *pTyped = (GSparseHash *)kds;
		SparseHashIterator *pIter = (SparseHashIterator *)Iterator;
		pTyped->remove(pzKey);
	}
	__int64 itemCount(KeyedDataStructure kds)
	{
		GSparseHash *pTyped = (GSparseHash *)kds;
		return pTyped->size();
	}
	void releaseIterator(SparseHashIterator Iterator)
	{
		delete (SparseHashIterator *)Iterator;
		Iterator = 0;
	}
}

g_SparseHashAbstraction ;

#endif //_SPARSEHASH_TABLE_







