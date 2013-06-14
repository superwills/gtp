#ifndef CALLBACK_H
#define CALLBACK_H

//#include "boost/function.hpp"  // phased out boost in favor of using std::function
#include <functional>
using namespace std ;

//The idea is for the Callback object to
// hide the complexity of the function call it invokes
// So anybody can contain a callback, and the
// 1) type, 2) number and 3) actual data of the
// arguments is hidden by the Callback base class.
// BUT this does mean that these Callbacks cannot have
// a return value, which is just as well because
// you can't rely on WHEN the callback job will complete..
// what you DO want is a function to execute WHEN the
// callback is complete, but we leave that to the caller,
// he can embed his own function invokation AT THE END
// of his Callback routine.
struct Callback
{
  virtual void exec() = 0 ;
} ;

// Callback function that takes 
// 0 arguments.
struct Callback0 : public Callback
{
  function<void ()> func ;

  Callback0(){}

  //Callback0( void (*theFunc)() )
  Callback0( function<void ()> theFunc ) // pass me a c++ function object
  {
    func = theFunc ;
  }

  void exec()
  {
    func() ;
  }
} ;

// 1 Argument, type of argument is also an argument.
template <typename typeArg1>
struct Callback1 : public Callback
{
  function<void (typeArg1)> func ;
  typeArg1 argument1 ;

  Callback1( void (*theFunc)( typeArg1 theArg1 ), typeArg1 arg1 )
  {
    func = theFunc ;
    argument1 = arg1 ;
  }

  void exec()
  {
    func( argument1 ) ;
  }
} ;

template <typename typeArg1, typename typeArg2>
struct Callback2 : public Callback
{
  function<void (typeArg1, typeArg2)> func ;
  typeArg1 argument1 ;
  typeArg2 argument2 ;
  
  Callback2( void (*theFunc)(
              typeArg1 theArg1,
              typeArg2 theArg2
            ),
            typeArg1 arg1,
            typeArg2 arg2 )
  {
    func = theFunc ;
    argument1 = arg1 ;
    argument2 = arg2 ;
  }

  void exec()
  {
    func( argument1, argument2 ) ;
  }
} ;

template <typename typeArg1,
          typename typeArg2,
          typename typeArg3>
struct Callback3 : public Callback
{
  function<void (typeArg1, typeArg2, typeArg3)> func ;
  typeArg1 argument1 ;
  typeArg2 argument2 ;
  typeArg3 argument3 ;

  Callback3( void (*theFunc)(
              typeArg1 theArg1,
              typeArg2 theArg2,
              typeArg3 theArg3
            ),
            typeArg1 arg1,
            typeArg2 arg2,
            typeArg3 arg3 )
  {
    func = theFunc ;
    argument1 = arg1 ;
    argument2 = arg2 ;
    argument3 = arg3 ;
  }

  void exec()
  {
    func( argument1, argument2, argument3 ) ;
  }
} ;

template <typename typeArg1,
          typename typeArg2,
          typename typeArg3,
          typename typeArg4>
struct Callback4 : public Callback
{
  function<void (typeArg1, typeArg2, typeArg3, typeArg4)> func ;
  typeArg1 argument1 ;
  typeArg2 argument2 ;
  typeArg3 argument3 ;
  typeArg4 argument4 ;

  Callback4( void (*theFunc)(
               typeArg1 theArg1,
               typeArg2 theArg2,
               typeArg3 theArg3,
               typeArg4 theArg4
             ),
             typeArg1 arg1,
             typeArg2 arg2,
             typeArg3 arg3,
             typeArg4 arg4 )
  {
    func = theFunc ;
    argument1 = arg1 ;
    argument2 = arg2 ;
    argument3 = arg3 ;
    argument4 = arg4 ;
  }

  void exec()
  {
    func( argument1, argument2, argument3, argument4 ) ;
  }
} ;


// Now, objects 
template <typename T, typename F>
inline void invoke(T& obj, F func)
{
  (obj.*func)();
}

template <typename T, typename F>
inline void invoke(T* obj, F func)
{
  (obj->*func)();
}

template <typename T, typename F, typename A>
inline void invoke(T& obj, F func, const A& arg)
{
  (obj.*func)(arg);
}

template <typename T, typename F, typename A>
inline void invoke(T* obj, F func, const A& arg)
{
  (obj->*func)(arg);
}

template <typename T, typename F, typename A, typename A2>
inline void invoke(T& obj, F func, const A& arg, const A2& arg2)
{
  (obj.*func)(arg, arg2);
}

template <typename T, typename F, typename A, typename A2>
inline void invoke(T* obj, F func, const A& arg, const A2& arg2)
{
  (obj->*func)(arg, arg2);
}

template <typename T, typename F, typename A, typename A2, typename A3>
inline void invoke(T& obj, F func, const A& arg, const A2& arg2, const A3& arg3)
{
  (obj.*func)(arg, arg2, arg3);
}

template <typename T, typename F, typename A, typename A2, typename A3>
inline void invoke(T* obj, F func, const A& arg, const A2& arg2, const A3& arg3)
{
  (obj->*func)(arg, arg2, arg3);
}

template <typename T, typename F, typename A, typename A2, typename A3, typename A4>
inline void invoke(T& obj, F func, const A& arg, const A2& arg2, const A3& arg3, const A4& arg4)
{
  (obj.*func)(arg, arg2, arg3, arg4);
}

template <typename T, typename F, typename A, typename A2, typename A3, typename A4>
inline void invoke(T* obj, F func, const A& arg, const A2& arg2, const A3& arg3, const A4& arg4)
{
  (obj->*func)(arg, arg2, arg3, arg4);
}

template <typename T, typename F, typename A, typename A2, typename A3, typename A4, typename A5>
inline void invoke(T* obj, F func, const A& arg, const A2& arg2, const A3& arg3, const A4& arg4, const A5& arg5 )
{
  (obj->*func)(arg, arg2, arg3, arg4, arg5);
}

template <typename T, typename F, typename A, typename A2, typename A3, typename A4, typename A5, typename A6>
inline void invoke(T* obj, F func, const A& arg, const A2& arg2, const A3& arg3, const A4& arg4, const A5& arg5, const A6& arg6 )
{
  (obj->*func)(arg, arg2, arg3, arg4, arg5, arg6);
}


template <typename objectType,
          typename memberFunctionPtrType>
struct CallbackObject0 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;

  CallbackObject0(objectType iObj,
                  memberFunctionPtrType iFcnPtr )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
  }

  void exec()
  {
    // this resolves into either (obj.*func) or 
    // (obj->*func), depending on
    invoke(obj, fcnPtr);
  }
} ;


template <typename objectType,
          typename memberFunctionPtrType,
          typename memberFcnArg1Type>
struct CallbackObject1 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;
  memberFcnArg1Type arg1 ;

  CallbackObject1(objectType iObj,
                  memberFunctionPtrType iFcnPtr,
                  memberFcnArg1Type iArg1 )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
    arg1 = iArg1 ;
  }

  void exec()
  {
    invoke(obj, fcnPtr, arg1);
  }
} ;

template <typename objectType,
          typename memberFunctionPtrType,
          typename memberFcnArg1Type,
          typename memberFcnArg2Type>
struct CallbackObject2 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;
  memberFcnArg1Type arg1 ;
  memberFcnArg2Type arg2 ;

  CallbackObject2(objectType iObj,
                  memberFunctionPtrType iFcnPtr,
                  memberFcnArg1Type iArg1,
                  memberFcnArg2Type iArg2 )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
    arg1 = iArg1 ;
    arg2 = iArg2 ;
  }

  void exec()
  {
    invoke(obj, fcnPtr, arg1, arg2);
  }
} ;

template <typename objectType,
          typename memberFunctionPtrType,
          typename memberFcnArg1Type,
          typename memberFcnArg2Type,
          typename memberFcnArg3Type>
struct CallbackObject3 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;
  memberFcnArg1Type arg1 ;
  memberFcnArg2Type arg2 ;
  memberFcnArg3Type arg3 ;

  CallbackObject3(objectType iObj,
                  memberFunctionPtrType iFcnPtr,
                  memberFcnArg1Type iArg1,
                  memberFcnArg2Type iArg2,
                  memberFcnArg3Type iArg3 )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
    arg1 = iArg1 ;
    arg2 = iArg2 ;
    arg3 = iArg3 ;
  }

  void exec()
  {
    invoke(obj, fcnPtr, arg1, arg2, arg3);
  }
} ;

template <typename objectType,
          typename memberFunctionPtrType,
          typename memberFcnArg1Type,
          typename memberFcnArg2Type,
          typename memberFcnArg3Type,
          typename memberFcnArg4Type>
struct CallbackObject4 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;
  memberFcnArg1Type arg1 ;
  memberFcnArg2Type arg2 ;
  memberFcnArg3Type arg3 ;
  memberFcnArg4Type arg4 ;

  CallbackObject4(objectType iObj,
                  memberFunctionPtrType iFcnPtr,
                  memberFcnArg1Type iArg1,
                  memberFcnArg2Type iArg2,
                  memberFcnArg3Type iArg3,
                  memberFcnArg4Type iArg4 )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
    arg1 = iArg1 ;
    arg2 = iArg2 ;
    arg3 = iArg3 ;
    arg4 = iArg4 ;
  }

  void exec()
  {
    invoke(obj, fcnPtr, arg1, arg2, arg3, arg4);
  }
} ;



template <typename objectType,
          typename memberFunctionPtrType,
          typename memberFcnArg1Type,
          typename memberFcnArg2Type,
          typename memberFcnArg3Type,
          typename memberFcnArg4Type,
          typename memberFcnArg5Type>
struct CallbackObject5 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;
  memberFcnArg1Type arg1 ;
  memberFcnArg2Type arg2 ;
  memberFcnArg3Type arg3 ;
  memberFcnArg4Type arg4 ;
  memberFcnArg5Type arg5 ;

  CallbackObject5(objectType iObj,
                  memberFunctionPtrType iFcnPtr,
                  memberFcnArg1Type iArg1,
                  memberFcnArg2Type iArg2,
                  memberFcnArg3Type iArg3,
                  memberFcnArg4Type iArg4,
                  memberFcnArg5Type iArg5 )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
    arg1 = iArg1 ;
    arg2 = iArg2 ;
    arg3 = iArg3 ;
    arg4 = iArg4 ;
    arg5 = iArg5 ;
  }

  void exec()
  {
    invoke(obj, fcnPtr, arg1, arg2, arg3, arg4, arg5);
  }
} ;



template <typename objectType,
          typename memberFunctionPtrType,
          typename memberFcnArg1Type,
          typename memberFcnArg2Type,
          typename memberFcnArg3Type,
          typename memberFcnArg4Type,
          typename memberFcnArg5Type,
          typename memberFcnArg6Type>
struct CallbackObject6 : public Callback
{
  objectType obj ;
  memberFunctionPtrType fcnPtr ;
  memberFcnArg1Type arg1 ;
  memberFcnArg2Type arg2 ;
  memberFcnArg3Type arg3 ;
  memberFcnArg4Type arg4 ;
  memberFcnArg5Type arg5 ;
  memberFcnArg5Type arg6 ;

  CallbackObject6(objectType iObj,
                  memberFunctionPtrType iFcnPtr,
                  memberFcnArg1Type iArg1,
                  memberFcnArg2Type iArg2,
                  memberFcnArg3Type iArg3,
                  memberFcnArg4Type iArg4,
                  memberFcnArg5Type iArg5,
                  memberFcnArg5Type iArg6 )
  {
    obj = iObj ;
    fcnPtr = iFcnPtr ;
    arg1 = iArg1 ;
    arg2 = iArg2 ;
    arg3 = iArg3 ;
    arg4 = iArg4 ;
    arg5 = iArg5 ;
    arg6 = iArg6 ;
  }

  void exec()
  {
    invoke(obj, fcnPtr, arg1, arg2, arg3, arg4, arg5, arg6);
  }
} ;




#endif