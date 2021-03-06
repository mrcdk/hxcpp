#ifndef HX_GC_H
#define HX_GC_H


// Under the current scheme (as defined by HX_HCSTRING/HX_CSTRING in hxcpp.h)
//  each constant string data is prepended with a 4-byte header that says the string
//  is constant (ie, not part of GC) and whether there is(not) a pre-computed hash at
//  the end of the data.

#define HX_GC_CONST_ALLOC_BIT  0x80000000
#define HX_GC_CONST_ALLOC_MARK_BIT  0x80
#define HX_GC_NO_STRING_HASH   0x40000000
#define HX_GC_NO_HASH_MASK     (HX_GC_CONST_ALLOC_BIT | HX_GC_NO_STRING_HASH)




// Tell compiler the extra functions are supported
#define HXCPP_GC_FUNCTIONS_1

// Function called by the haxe code...


// Helpers for debugging code
void  __hxcpp_reachable(hx::Object *inKeep);
void  __hxcpp_enable(bool inEnable);
void  __hxcpp_collect(bool inMajor=true);
void   __hxcpp_gc_compact();
int   __hxcpp_gc_trace(hx::Class inClass, bool inPrint);
int   __hxcpp_gc_used_bytes();
int   __hxcpp_gc_mem_info(int inWhat);
void  __hxcpp_enter_gc_free_zone();
void  __hxcpp_exit_gc_free_zone();
void  __hxcpp_gc_safe_point();
void  __hxcpp_spam_collects(int inEveryNCalls);

// Finalizers from haxe code...
void  __hxcpp_gc_do_not_kill(Dynamic inObj);
void  __hxcpp_set_finalizer(Dynamic inObj, void *inFunction);
hx::Object *__hxcpp_get_next_zombie();

#ifdef HXCPP_TELEMETRY
void __hxcpp_set_hxt_finalizer(void* inObj, void *inFunc);
#endif

hx::Object *__hxcpp_weak_ref_create(Dynamic inObject);
hx::Object *__hxcpp_weak_ref_get(Dynamic inRef);


unsigned int __hxcpp_obj_hash(Dynamic inObj);
int __hxcpp_obj_id(Dynamic inObj);
hx::Object *__hxcpp_id_obj(int);






namespace hx
{
// These functions are optimised for haxe-generated objects
//  inSize is known to be "small", a multiple of 4 bytes and big enough to hold a pointer
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewHaxeObject(size_t inSize);
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewHaxeContainer(size_t inSize);
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewHaxeConstObject(size_t inSize);

// Generic allocation routine.
// If inSize is small (<4k) it will be allocated from the immix pool.
// Larger, and it will be allocated from a separate memory pool
// inIsObject specifies whether "__Mark"  should be called on the resulting object
void *InternalNew(int inSize,bool inIsObject);

// Used internall - realloc array data
void *InternalRealloc(void *inData,int inSize);

// Called after collection by an unspecified thread
typedef void (*finalizer)(hx::Object *v);

// Used internally by the runtime.
// The constructor will add this object to the internal list of finalizers.
// You must call "Mark" on this object within every collection loop to keep it alive,
//  otherwise the Gc system will call the finalizer and delete this object.
// This would typically be done in response to a "__Mark" call on another object
struct InternalFinalizer
{
   InternalFinalizer(hx::Object *inObj, finalizer inFinalizer=0);

   void Mark() { mUsed=true; }
   #ifdef HXCPP_VISIT_ALLOCS
   void Visit(VisitContext *__inCtx);
   #endif
   void Detach();

   bool      mUsed;
   bool      mValid;
   finalizer mFinalizer;
   hx::Object  *mObject;
};

// If another thread wants to do a collect, it will signal this variable.
// This automatically gets checked when you call "new", but if you are in long-running
//  loop with no new call, you might starve another thread if you to not check this.
//  0xffffffff = pause requested
extern int gPauseForCollect;
// Call in response to a gPauseForCollect. Normally, this is done for you in "new"
void PauseForCollect();


// Used by WeakHash to work out if it needs to dispose its keys
bool IsWeakRefValid(hx::Object *inPtr);

// Used by CFFI to scan a block of memory for GC Pointers. May picks up random crap
//  that points to real, active objects.
void MarkConservative(int *inBottom, int *inTop,hx::MarkContext *__inCtx);


// Create/Remove a root.
// All statics are explicitly registered - this saves adding the whole data segment
//  to the collection list.
// It takes a pointer-pointer so it can move the contents, and the caller can change the contents
void GCAddRoot(hx::Object **inRoot);
void GCRemoveRoot(hx::Object **inRoot);




// This is used internally in hxcpp
// It calls InternalNew, and takes care of null-terminating the result
HX_CHAR *NewString(int inLen);

// The concept of 'private' is from the old conservative Gc method.
// Now with explicit marking, these functions do the same thing, which is
//  to allocate some GC memory and optionally copy the 'inData' into those bytes
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewGCBytes(void *inData,int inSize);
HXCPP_EXTERN_CLASS_ATTRIBUTES void *NewGCPrivate(void *inData,int inSize);


// Defined in Class.cpp, these function is called from the Gc to start the marking/visiting
void MarkClassStatics(hx::MarkContext *__inCtx);
#ifdef HXCPP_VISIT_ALLOCS
void VisitClassStatics(hx::VisitContext *__inCtx);
#endif




void  GCSetFinalizer( hx::Object *, hx::finalizer f );


void GCCheckPointer(void *);
void InternalEnableGC(bool inEnable);
void *InternalCreateConstBuffer(const void *inData,int inSize,bool inAddStringHash=false);
void RegisterNewThread(void *inTopOfStack);
void SetTopOfStack(void *inTopOfStack,bool inForce=false);
int InternalCollect(bool inMajor,bool inCompact);
void GCChangeManagedMemory(int inDelta, const char *inWhy=0);

void EnterGCFreeZone();
void ExitGCFreeZone();

// Threading ...
void RegisterCurrentThread(void *inTopOfStack);
void UnregisterCurrentThread();
void EnterSafePoint();
void GCPrepareMultiThreaded();




HXCPP_EXTERN_CLASS_ATTRIBUTES extern unsigned int gPrevMarkIdMask;

HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkAllocUnchecked(void *inPtr ,hx::MarkContext *__inCtx);

inline void MarkAlloc(void *inPtr ,hx::MarkContext *__inCtx)
{
   if ( ((unsigned int *)inPtr)[-1] & hx::gPrevMarkIdMask )
      MarkAllocUnchecked(inPtr,__inCtx);
}

HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkObjectAllocUnchecked(hx::Object *inPtr ,hx::MarkContext *__inCtx);

inline void MarkObjectAlloc(hx::Object *inPtr ,hx::MarkContext *__inCtx)
{
   if ( ((unsigned int *)inPtr)[-1] & hx::gPrevMarkIdMask )
      MarkObjectAllocUnchecked(inPtr,__inCtx);
}

void MarkObjectArray(hx::Object **inPtr, int inLength, hx::MarkContext *__inCtx);
void MarkStringArray(String *inPtr, int inLength, hx::MarkContext *__inCtx);

#ifdef HXCPP_DEBUG
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkSetMember(const char *inName ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkPushClass(const char *inName ,hx::MarkContext *__inCtx);
HXCPP_EXTERN_CLASS_ATTRIBUTES
void MarkPopClass(hx::MarkContext *__inCtx);
#endif

// Make sure we can do a conversion to hx::Object **
inline void EnsureObjPtr(hx::Object *) { }

} // end namespace hx



// It was theoretically possible to redefine the MarkContext arg type (or skip it)
//  incase the particular GC scheme did not need it.  This may take a bit of extra
//  work to get going again

#define HX_MARK_ARG __inCtx
//#define HX_MARK_ADD_ARG ,__inCtx
#define HX_MARK_PARAMS hx::MarkContext *__inCtx
//#define HX_MARK_ADD_PARAMS ,hx::MarkContext *__inCtx

#ifdef HXCPP_VISIT_ALLOCS
#define HX_VISIT_ARG __inCtx
#define HX_VISIT_PARAMS hx::VisitContext *__inCtx
#else
#define HX_VISIT_ARG
#define HX_VISIT_PARAMS
#endif





// These macros add debug to the mark/visit calls if required
// They also perform some inline checking to avoid function calls if possible


#ifdef HXCPP_DEBUG

#define HX_MARK_MEMBER_NAME(x,name) { hx::MarkSetMember(name, __inCtx); hx::MarkMember(x, __inCtx ); }
#define HX_MARK_BEGIN_CLASS(x) hx::MarkPushClass(#x, __inCtx );
#define HX_MARK_END_CLASS() hx::MarkPopClass(__inCtx );
#define HX_MARK_MEMBER(x) { hx::MarkSetMember(0, __inCtx); hx::MarkMember(x, __inCtx ); }
#define HX_MARK_MEMBER_ARRAY(x,len) { hx::MarkSetMember(0, __inCtx); hx::MarkMemberArray(x, len, __inCtx ); }

#else

#define HX_MARK_MEMBER_NAME(x,name) hx::MarkMember(x, __inCtx )
#define HX_MARK_BEGIN_CLASS(x)
#define HX_MARK_END_CLASS()
#define HX_MARK_MEMBER(x) hx::MarkMember(x, __inCtx )
#define HX_MARK_MEMBER_ARRAY(x,len) hx::MarkMemberArray(x, len, __inCtx )

#endif

#define HX_MARK_OBJECT(ioPtr) if (ioPtr) hx::MarkObjectAlloc(ioPtr, __inCtx );




#define HX_MARK_STRING(ioPtr) \
   if (ioPtr) hx::MarkAlloc((void *)ioPtr, __inCtx );

#define HX_MARK_ARRAY(ioPtr) { if (ioPtr) hx::MarkAlloc((void *)ioPtr, __inCtx ); }




#define HX_VISIT_MEMBER_NAME(x,name) hx::VisitMember(x, __inCtx )
#define HX_VISIT_MEMBER(x) hx::VisitMember(x, __inCtx )

#define HX_VISIT_OBJECT(ioPtr) \
  { hx::EnsureObjPtr(ioPtr); if (ioPtr) __inCtx->visitObject( (hx::Object **)&ioPtr); }

#define HX_VISIT_STRING(ioPtr) \
   if (ioPtr && !(((unsigned int *)ioPtr)[-1] & HX_GC_CONST_ALLOC_BIT) ) __inCtx->visitAlloc((void **)&ioPtr);

#define HX_VISIT_ARRAY(ioPtr) { if (ioPtr) __inCtx->visitAlloc((void **)&ioPtr); }







#endif

