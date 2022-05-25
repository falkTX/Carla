#ifndef _WDL_ASSOCARRAY_H_
#define _WDL_ASSOCARRAY_H_

#include "heapbuf.h"
#include "mergesort.h"

// on all of these, if valdispose is set, the array will dispose of values as needed.
// if keydup/keydispose are set, copies of (any) key data will be made/destroyed as necessary


// WDL_AssocArrayImpl can be used on its own, and can contain structs for keys or values
template <class KEY, class VAL> class WDL_AssocArrayImpl 
{
  WDL_AssocArrayImpl(const WDL_AssocArrayImpl &cp) { CopyContents(cp); }

  WDL_AssocArrayImpl &operator=(const WDL_AssocArrayImpl &cp) { CopyContents(cp); return *this; }

public:

  explicit WDL_AssocArrayImpl(int (*keycmp)(KEY *k1, KEY *k2), KEY (*keydup)(KEY)=0, void (*keydispose)(KEY)=0, void (*valdispose)(VAL)=0)
  {
    m_keycmp = keycmp;
    m_keydup = keydup;
    m_keydispose = keydispose;
    m_valdispose = valdispose;
  }

  ~WDL_AssocArrayImpl() 
  {
    DeleteAll();
  }

  VAL* GetPtr(KEY key, KEY *keyPtrOut=NULL) const
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (keyPtrOut) *keyPtrOut = kv->key;
      return &(kv->val);
    }
    return 0;
  }

  bool Exists(KEY key) const
  {
    bool ismatch = false;
    LowerBound(key, &ismatch);
    return ismatch;
  }

  int Insert(KEY key, VAL val)
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_valdispose) m_valdispose(kv->val);
      kv->val = val;
    }
    else
    {
      KeyVal* kv = m_data.Resize(m_data.GetSize()+1)+i;
      memmove(kv+1, kv, (m_data.GetSize()-i-1)*(unsigned int)sizeof(KeyVal));
      if (m_keydup) key = m_keydup(key);
      kv->key = key;
      kv->val = val;      
    }
    return i;
  }

  void Delete(KEY key) 
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_valdispose) m_valdispose(kv->val);
      m_data.Delete(i);
    }
  }

  void DeleteByIndex(int idx)
  {
    if (idx >= 0 && idx < m_data.GetSize())
    {
      KeyVal* kv = m_data.Get()+idx;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_valdispose) m_valdispose(kv->val);
      m_data.Delete(idx);
    }
  }

  void DeleteAll(bool resizedown=false)
  {
    if (m_keydispose || m_valdispose)
    {
      int i;
      for (i = 0; i < m_data.GetSize(); ++i)
      {
        KeyVal* kv = m_data.Get()+i;
        if (m_keydispose) m_keydispose(kv->key);
        if (m_valdispose) m_valdispose(kv->val);
      }
    }
    m_data.Resize(0, resizedown);
  }

  int GetSize() const
  {
    return m_data.GetSize();
  }

  VAL* EnumeratePtr(int i, KEY* key=0) const
  {
    if (i >= 0 && i < m_data.GetSize()) 
    {
      KeyVal* kv = m_data.Get()+i;
      if (key) *key = kv->key;
      return &(kv->val);
    }
    return 0;
  }
  
  KEY* ReverseLookupPtr(VAL val) const
  {
    int i;
    for (i = 0; i < m_data.GetSize(); ++i)
    {
      KeyVal* kv = m_data.Get()+i;
      if (kv->val == val) return &kv->key;
    }
    return 0;    
  }

  void ChangeKey(KEY oldkey, KEY newkey)
  {
    bool ismatch=false;
    int i=LowerBound(oldkey, &ismatch);
    if (ismatch) ChangeKeyByIndex(i, newkey, true);
  }

  void ChangeKeyByIndex(int idx, KEY newkey, bool needsort)
  {
    if (idx >= 0 && idx < m_data.GetSize())
    {
      KeyVal* kv=m_data.Get()+idx;
      if (!needsort)
      {
        if (m_keydispose) m_keydispose(kv->key);
        if (m_keydup) newkey=m_keydup(newkey);
        kv->key=newkey;
      }
      else
      {
        VAL val=kv->val;
        m_data.Delete(idx);
        Insert(newkey, val);
      }
    }
  }

  // fast add-block mode
  void AddUnsorted(KEY key, VAL val)
  {
    int i=m_data.GetSize();
    KeyVal* kv = m_data.Resize(i+1)+i;
    if (m_keydup) key = m_keydup(key);
    kv->key = key;
    kv->val = val;
  }

  void Resort(int (*new_keycmp)(KEY *k1, KEY *k2)=NULL)
  {
    if (new_keycmp) m_keycmp = new_keycmp;
    if (m_data.GetSize() > 1 && m_keycmp)
    {
      qsort(m_data.Get(), m_data.GetSize(), sizeof(KeyVal),
        (int(*)(const void*, const void*))m_keycmp);
      if (!new_keycmp)
        RemoveDuplicateKeys();
    }
  }

  void ResortStable()
  {
    if (m_data.GetSize() > 1 && m_keycmp)
    {
      char *tmp=(char*)malloc(m_data.GetSize()*sizeof(KeyVal));
      if (WDL_NORMALLY(tmp))
      {
        WDL_mergesort(m_data.Get(), m_data.GetSize(), sizeof(KeyVal),
          (int(*)(const void*, const void*))m_keycmp, tmp);
        free(tmp);
      }
      else
      {
        qsort(m_data.Get(), m_data.GetSize(), sizeof(KeyVal),
          (int(*)(const void*, const void*))m_keycmp);
      }

      RemoveDuplicateKeys();
    }
  }

  int LowerBound(KEY key, bool* ismatch) const
  {
    int a = 0;
    int c = m_data.GetSize();
    while (a != c)
    {
      int b = (a+c)/2;
      KeyVal* kv=m_data.Get()+b;
      int cmp = m_keycmp(&key, &kv->key);
      if (cmp > 0) a = b+1;
      else if (cmp < 0) c = b;
      else
      {
        *ismatch = true;
        return b;
      }
    }
    *ismatch = false;
    return a;
  }

  int GetIdx(KEY key) const
  {
    bool ismatch=false;
    int i = LowerBound(key, &ismatch);
    if (ismatch) return i;
    return -1;
  }

  void SetGranul(int gran)
  {
    m_data.SetGranul(gran);
  }

  void CopyContents(const WDL_AssocArrayImpl &cp)
  {
    m_data=cp.m_data;
    m_keycmp = cp.m_keycmp;
    m_keydup = cp.m_keydup; 
    m_keydispose = m_keydup ? cp.m_keydispose : NULL;
    m_valdispose = NULL; // avoid disposing of values twice, since we don't have a valdup, we can't have a fully valid copy
    if (m_keydup)
    {
      int x;
      const int n=m_data.GetSize();
      for (x=0;x<n;x++)
      {
        KeyVal *kv=m_data.Get()+x;
        if (kv->key) kv->key = m_keydup(kv->key);
      }
    }
  }

  void CopyContentsAsReference(const WDL_AssocArrayImpl &cp)
  {
    DeleteAll(true);
    m_keycmp = cp.m_keycmp;
    m_keydup = NULL;  // this no longer can own any data
    m_keydispose = NULL;
    m_valdispose = NULL; 

    m_data=cp.m_data;
  }


// private data, but exposed in case the caller wants to manipulate at its own risk
  struct KeyVal
  {
    KEY key;
    VAL val;
  };
  WDL_TypedBuf<KeyVal> m_data;

protected:

  int (*m_keycmp)(KEY *k1, KEY *k2);
  KEY (*m_keydup)(KEY);
  void (*m_keydispose)(KEY);
  void (*m_valdispose)(VAL);

private:

  void RemoveDuplicateKeys() // after resorting
  {
    const int sz = m_data.GetSize();

    int cnt = 1;
    KeyVal *rd = m_data.Get() + 1, *wr = rd;
    for (int x = 1; x < sz; x ++)
    {
      if (m_keycmp(&rd->key, &wr[-1].key))
      {
        if (rd != wr) *wr=*rd;
        wr++;
        cnt++;
      }
      else
      {
        if (m_keydispose) m_keydispose(rd->key);
        if (m_valdispose) m_valdispose(rd->val);
      }
      rd++;
    }
    if (cnt < sz) m_data.Resize(cnt,false);
  }
};


// WDL_AssocArray adds useful functions but cannot contain structs for keys or values
template <class KEY, class VAL> class WDL_AssocArray : public WDL_AssocArrayImpl<KEY, VAL>
{
public:

  explicit WDL_AssocArray(int (*keycmp)(KEY *k1, KEY *k2), KEY (*keydup)(KEY)=0, void (*keydispose)(KEY)=0, void (*valdispose)(VAL)=0)
  : WDL_AssocArrayImpl<KEY, VAL>(keycmp, keydup, keydispose, valdispose)
  { 
  }

  VAL Get(KEY key, VAL notfound=0) const
  {
    VAL* p = this->GetPtr(key);
    if (p) return *p;
    return notfound;
  }

  VAL Enumerate(int i, KEY* key=0, VAL notfound=0) const
  {
    VAL* p = this->EnumeratePtr(i, key);
    if (p) return *p;
    return notfound; 
  }

  KEY ReverseLookup(VAL val, KEY notfound=0) const
  {
    KEY* p=this->ReverseLookupPtr(val);
    if (p) return *p;
    return notfound;
  }
};


template <class VAL> class WDL_IntKeyedArray : public WDL_AssocArray<int, VAL>
{
public:

  explicit WDL_IntKeyedArray(void (*valdispose)(VAL)=0) : WDL_AssocArray<int, VAL>(cmpint, NULL, NULL, valdispose) {}
  ~WDL_IntKeyedArray() {}

private:

  static int cmpint(int *i1, int *i2) { return *i1-*i2; }
};

template <class VAL> class WDL_IntKeyedArray2 : public WDL_AssocArrayImpl<int, VAL>
{
public:

  explicit WDL_IntKeyedArray2(void (*valdispose)(VAL)=0) : WDL_AssocArrayImpl<int, VAL>(cmpint, NULL, NULL, valdispose) {}
  ~WDL_IntKeyedArray2() {}

private:

  static int cmpint(int *i1, int *i2) { return *i1-*i2; }
};

template <class VAL> class WDL_StringKeyedArray : public WDL_AssocArray<const char *, VAL>
{
public:

  explicit WDL_StringKeyedArray(bool caseSensitive=true, void (*valdispose)(VAL)=0) : WDL_AssocArray<const char*, VAL>(caseSensitive?cmpstr:cmpistr, dupstr, freestr, valdispose) {}
  
  ~WDL_StringKeyedArray() { }

  static const char *dupstr(const char *s) { return strdup(s);  } // these might not be necessary but depending on the libc maybe...
  static int cmpstr(const char **s1, const char **s2) { return strcmp(*s1, *s2); }
  static int cmpistr(const char **a, const char **b) { return stricmp(*a,*b); }
  static void freestr(const char* s) { free((void*)s); }
  static void freecharptr(char *p) { free(p); }
};


template <class VAL> class WDL_StringKeyedArray2 : public WDL_AssocArrayImpl<const char *, VAL>
{
public:

  explicit WDL_StringKeyedArray2(bool caseSensitive=true, void (*valdispose)(VAL)=0) : WDL_AssocArrayImpl<const char*, VAL>(caseSensitive?cmpstr:cmpistr, dupstr, freestr, valdispose) {}
  
  ~WDL_StringKeyedArray2() { }

  static const char *dupstr(const char *s) { return strdup(s);  } // these might not be necessary but depending on the libc maybe...
  static int cmpstr(const char **s1, const char **s2) { return strcmp(*s1, *s2); }
  static int cmpistr(const char **a, const char **b) { return stricmp(*a,*b); }
  static void freestr(const char* s) { free((void*)s); }
  static void freecharptr(char *p) { free(p); }
};

// sorts text as text, sorts anything that looks like a number as a number
template <class VAL> class WDL_LogicalSortStringKeyedArray : public WDL_StringKeyedArray<VAL>
{
public:

  explicit WDL_LogicalSortStringKeyedArray(bool caseSensitive=true, void (*valdispose)(VAL)=0) : WDL_StringKeyedArray<VAL>(caseSensitive, valdispose) 
  {
    WDL_StringKeyedArray<VAL>::m_keycmp = caseSensitive?cmpstr:cmpistr; // override
  }
  
  ~WDL_LogicalSortStringKeyedArray() { }

  static int cmpstr(const char **a, const char **b) { return _cmpstr(*a, *b, true); }
  static int cmpistr(const char **a, const char **b) { return _cmpstr(*a, *b, false); }

private:

  static int _cmpstr(const char *s1, const char *s2, bool case_sensitive)
  {
    // this also exists as WDL_strcmp_logical in wdlcstring.h

    for (;;)
    {
      if (*s1 >= '0' && *s1 <= '9' && *s2 >= '0' && *s2 <= '9')
      {
        int lzdiff=0, len1=0, len2=0;

        while (*s1 == '0') { s1++; lzdiff--; }
        while (*s2 == '0') { s2++; lzdiff++; }

        while (s1[len1] >= '0' && s1[len1] <= '9') len1++;
        while (s2[len2] >= '0' && s2[len2] <= '9') len2++;

        if (len1 != len2) return len1-len2;

        while (len1--)
        {
          const int d = *s1++ - *s2++;
          if (d) return d;
        }

        if (lzdiff) return lzdiff;
      }
      else
      {
        char c1 = *s1++, c2 = *s2++;
        if (c1 != c2)
        {
          if (case_sensitive) return c1-c2;

          if (c1>='a' && c1<='z') c1+='A'-'a';
          if (c2>='a' && c2<='z') c2+='A'-'a';
          if (c1 != c2) return c1-c2;
        }
        else if (!c1) return 0;
      }
    }
  }
};


template <class VAL> class WDL_PtrKeyedArray : public WDL_AssocArray<INT_PTR, VAL>
{
public:

  explicit WDL_PtrKeyedArray(void (*valdispose)(VAL)=0) : WDL_AssocArray<INT_PTR, VAL>(cmpptr, 0, 0, valdispose) {}

  ~WDL_PtrKeyedArray() {}

private:
  
  static int cmpptr(INT_PTR* a, INT_PTR* b) { const INT_PTR d = *a - *b; return d<0?-1:(d!=0); }
};


#endif

