#ifndef HELPER_HPP
#define HELPER_HPP

#include <algorithm> //std::swap

#if __GNUC__ >= 3
  #define LIKELY(X) (__builtin_expect((X),true))
  #define UNLIKELY(X) (__builtin_expect((X),false))
  #define ABS(X) (__builtin_fabs((X)))
#else
  #define LIKELY(X) (X)
  #define UNLIKELY(X) (X)
  #define ABS(X) ((X<0.0?-X:X))
#endif

namespace Common{
  //TODO replace with gcc implementation when compiler available
  template <std::size_t = sizeof(std::size_t)>
  struct hash_fct{
    static std::size_t hash(const char* first, std::size_t length){
      std::size_t result=0;
      for(;length>0;--length) result=(result*131)+ *first++;
      return result;
    }
  };

  template <>
  struct hash_fct<4>{
    static std::size_t hash(const char* first, std::size_t length){
      std::size_t result=static_cast<std::size_t>(2166136261UL);
      for(;length>0;--length){
        result^=static_cast<std::size_t>(*first++);
        result*=static_cast<std::size_t>(16777619UL);
      }
      return result;
    }
  };

  template <>
  struct hash_fct<8>{
    static std::size_t hash(const char* first, std::size_t length){
      std::size_t result=static_cast<std::size_t>(14695981039346656037ULL);
      for(;length>0;--length){
        result^=static_cast<std::size_t>(*first++);
        result*=static_cast<std::size_t>(1099511628211ULL);
      }
      return result;
    }
  };

  template <typename T> struct hash;

  template <>
  struct hash<unsigned long>{
    std::size_t operator()(unsigned long lval) const {
      std::size_t result=0;
      result=hash_fct<>::hash(reinterpret_cast<const char*>(&lval),sizeof(lval));
      return result;
    }
  };

  template <>
  struct hash<double>{
    std::size_t operator()(double dval) const {
      std::size_t result=0;
      if(dval!=0.0) result=hash_fct<>::hash(reinterpret_cast<const char*>(&dval),sizeof(dval));
      return result;
    }
  };
  
  template <typename T>
  inline unsigned long hashFunction(T key){
    hash<unsigned long> h;
    return h((unsigned long) (key));
  }

  template <typename T>
  inline unsigned long hashFunction(T key1, T key2){
    hash<unsigned long> h;
    return hashFunction<T>(key1)^(h((unsigned long)(key2))*12582917);
  }

  template <typename T>
  inline unsigned long hashFunction(T key1, T key2, T key3){
    hash<unsigned long> h;
    return hashFunction<T>(key1,key2)^(h((unsigned long)(key3))*12582917);
  }

  template <typename T>
  inline unsigned long hashFunction(T key1, T key2, T key3, T key4){
    hash<unsigned long> h;
    return hashFunction<T>(key1,key2,key3)^(h((unsigned long)(key4))*12582917);
  }

  template <typename T>
  inline unsigned long hashFunction(T key1, T key2, T key3, T key4, T key5){
    hash<unsigned long> h;
    return hashFunction<T>(key1,key2,key3,key4)^(h((unsigned long)(key5))*12582917);
  }
  
  template <typename T, unsigned short N>
  struct ArrayHelper{
    static inline unsigned long hashFromArray(T* keys){
      return 0;
    }
  };

  template <typename T>
  struct ArrayHelper<T,2>{
    static inline unsigned long hashFromArray(T* keys){
      return hashFunction(keys[0],keys[1]);
    }
  };

  template <typename T>
  struct ArrayHelper<T,4>{
    static inline unsigned long hashFromArray(T* keys){
      return hashFunction(keys[0],keys[1],keys[2],keys[3]);
    }
  };

  static inline void atomicInc(unsigned long& v){
    return (void) (__sync_fetch_and_add(&v,1));
  }

  static inline void atomicDec(unsigned long& v){
    return (void) (__sync_fetch_and_sub(&v,1));
  }

  static inline int atomicIncAndTest(unsigned long& v){
    return (__sync_add_and_fetch(&v,1)==1);
  }

  static inline int atomicDecAndTest(unsigned long& v){
    return (__sync_sub_and_fetch(&v,1)==0);
  }

  template <typename T>
  inline bool atomicCompareAndSwap(T& v, T oldValue, T newValue){
    return __sync_bool_compare_and_swap(&v,oldValue,newValue);
  }

  template <typename T>
  struct PtrAndLock {
    PtrAndLock() : ptr(0), lock(false) {}
    T* ptr;
    bool lock;
  };

  template <typename T>
  struct COMMUTATIVE{
    static inline void cacheOpt(const T*& u, const T*& v){
      if(u<v) std::swap(u,v);
    }
  };

  template <typename T>
  struct NOT_COMMUTATIVE{
    static inline void cacheOpt(const T*&, const T*&){}
  };

  template <typename T>
  inline T* calcPtr(const T* node, unsigned short op){
    return reinterpret_cast<T*>(reinterpret_cast<unsigned long>(node)+op);
  }

  template <typename T>
  inline bool bitSet(const T* ptr){
    return reinterpret_cast<unsigned long>(ptr)&1;
  }

  template <typename T>
  inline T* getPtr(const T* ptr){
    return reinterpret_cast<T*>(reinterpret_cast<unsigned long>(ptr)&(-2));
  }

  template <typename T>
  inline T* setBit(const T* ptr){
    return reinterpret_cast<T*>(reinterpret_cast<unsigned long>(ptr)|1);
  }

  template <typename T>
  inline T* flipBit(const T* ptr){
    return reinterpret_cast<T*>(reinterpret_cast<unsigned long>(ptr)^1);
  }
  
  template <typename T>
  struct SmartAccess{
    const T* ptr;
    SmartAccess(const T* newPtr) : ptr(newPtr){}
    const T* operator->() const {
      return Common::getPtr(ptr);
    }
    const T* getPtr() const {
      return Common::getPtr(ptr);
    }
  };
}

#endif
