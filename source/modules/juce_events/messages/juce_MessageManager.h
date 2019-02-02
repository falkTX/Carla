/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

class MessageManagerLock;
class ThreadPoolJob;
class ActionListener;
class ActionBroadcaster;

//==============================================================================
#if JUCE_MODULE_AVAILABLE_juce_opengl
class OpenGLContext;
#endif

//==============================================================================
/** See MessageManager::callFunctionOnMessageThread() for use of this function type. */
typedef void* (MessageCallbackFunction) (void* userData);


//==============================================================================
/**
    This class is in charge of the application's event-dispatch loop.

    @see Message, CallbackMessage, MessageManagerLock, JUCEApplication, JUCEApplicationBase
*/
class JUCE_API  MessageManager
{
public:
    //==============================================================================
    /** Returns the global instance of the MessageManager. */
    static MessageManager* getInstance();

    /** Returns the global instance of the MessageManager, or nullptr if it doesn't exist. */
    static MessageManager* getInstanceWithoutCreating() noexcept;

    /** Deletes the global MessageManager instance.
        Does nothing if no instance had been created.
    */
    static void deleteInstance();

    //==============================================================================
    /** Runs the event dispatch loop until a stop message is posted.

        This method is only intended to be run by the application's startup routine,
        as it blocks, and will only return after the stopDispatchLoop() method has been used.

        @see stopDispatchLoop
    */
    void runDispatchLoop();

    /** Sends a signal that the dispatch loop should terminate.

        After this is called, the runDispatchLoop() or runDispatchLoopUntil() methods
        will be interrupted and will return.

        @see runDispatchLoop
    */
    void stopDispatchLoop();

    /** Returns true if the stopDispatchLoop() method has been called.
    */
    bool hasStopMessageBeenSent() const noexcept        { return quitMessagePosted; }

   #if JUCE_MODAL_LOOPS_PERMITTED || DOXYGEN
    /** Synchronously dispatches messages until a given time has elapsed.

        Returns false if a quit message has been posted by a call to stopDispatchLoop(),
        otherwise returns true.
    */
    bool runDispatchLoopUntil (int millisecondsToRunFor);
   #endif

    //==============================================================================
    /** Asynchronously invokes a function or C++11 lambda on the message thread. */
    template <typename FunctionType>
    static void callAsync (FunctionType functionToCall)
    {
        new AsyncCallInvoker<FunctionType> (functionToCall);
    }

    /** Calls a function using the message-thread.

        This can be used by any thread to cause this function to be called-back
        by the message thread. If it's the message-thread that's calling this method,
        then the function will just be called; if another thread is calling, a message
        will be posted to the queue, and this method will block until that message
        is delivered, the function is called, and the result is returned.

        Be careful not to cause any deadlocks with this! It's easy to do - e.g. if the caller
        thread has a critical section locked, which an unrelated message callback then tries to lock
        before the message thread gets round to processing this callback.

        @param callback     the function to call - its signature must be @code
                            void* myCallbackFunction (void*) @endcode
        @param userData     a user-defined pointer that will be passed to the function that gets called
        @returns            the value that the callback function returns.
        @see MessageManagerLock
    */
    void* callFunctionOnMessageThread (MessageCallbackFunction* callback, void* userData);

    /** Returns true if the caller-thread is the message thread. */
    bool isThisTheMessageThread() const noexcept;

    /** Called to tell the manager that the current thread is the one that's running the dispatch loop.

        (Best to ignore this method unless you really know what you're doing..)
        @see getCurrentMessageThread
    */
    void setCurrentThreadAsMessageThread();

    /** Returns the ID of the current message thread, as set by setCurrentThreadAsMessageThread().

        (Best to ignore this method unless you really know what you're doing..)
        @see setCurrentThreadAsMessageThread
    */
    Thread::ThreadID getCurrentMessageThread() const noexcept            { return messageThreadId; }

    /** Returns true if the caller thread has currently got the message manager locked.

        see the MessageManagerLock class for more info about this.

        This will be true if the caller is the message thread, because that automatically
        gains a lock while a message is being dispatched.
    */
    bool currentThreadHasLockedMessageManager() const noexcept;

    //==============================================================================
    /** Sends a message to all other JUCE applications that are running.

        @param messageText      the string that will be passed to the actionListenerCallback()
                                method of the broadcast listeners in the other app.
        @see registerBroadcastListener, ActionListener
    */
    static void broadcastMessage (const String& messageText);

    /** Registers a listener to get told about broadcast messages.

        The actionListenerCallback() callback's string parameter
        is the message passed into broadcastMessage().

        @see broadcastMessage
    */
    void registerBroadcastListener (ActionListener* listener);

    /** Deregisters a broadcast listener. */
    void deregisterBroadcastListener (ActionListener* listener);

    //==============================================================================
    /** Internal class used as the base class for all message objects.
        You shouldn't need to use this directly - see the CallbackMessage or Message
        classes instead.
    */
    class JUCE_API  MessageBase  : public ReferenceCountedObject
    {
    public:
        MessageBase() noexcept {}
        virtual ~MessageBase() {}

        virtual void messageCallback() = 0;
        bool post();

        typedef ReferenceCountedObjectPtr<MessageBase> Ptr;

        JUCE_DECLARE_NON_COPYABLE (MessageBase)
    };

    //==============================================================================
   #ifndef DOXYGEN
    // Internal methods - do not use!
    void deliverBroadcastMessage (const String&);
    ~MessageManager() noexcept;
    static bool dispatchNextMessageOnSystemQueue (bool returnIfNoPendingMessages);
   #endif

private:
    //==============================================================================
    MessageManager() noexcept;

    static MessageManager* instance;

    friend class MessageBase;
    class QuitMessage;
    friend class QuitMessage;
    friend class MessageManagerLock;

    ScopedPointer<ActionBroadcaster> broadcaster;
    bool quitMessagePosted = false, quitMessageReceived = false;
    Thread::ThreadID messageThreadId;
    Thread::ThreadID volatile threadWithLock = {};
    CriticalSection lockingLock;

    static bool postMessageToSystemQueue (MessageBase*);
    static void* exitModalLoopCallback (void*);
    static void doPlatformSpecificInitialisation();
    static void doPlatformSpecificShutdown();

    template <typename FunctionType>
    struct AsyncCallInvoker  : public MessageBase
    {
        AsyncCallInvoker (FunctionType f) : callback (f)  { post(); }
        void messageCallback() override                   { callback(); }
        FunctionType callback;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AsyncCallInvoker)
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MessageManager)
};


//==============================================================================
/** Used to make sure that the calling thread has exclusive access to the message loop.

    Because it's not thread-safe to call any of the Component or other UI classes
    from threads other than the message thread, one of these objects can be used to
    lock the message loop and allow this to be done. The message thread will be
    suspended for the lifetime of the MessageManagerLock object, so create one on
    the stack like this: @code
    void MyThread::run()
    {
        someData = 1234;

        const MessageManagerLock mmLock;
        // the event loop will now be locked so it's safe to make a few calls..

        myComponent->setBounds (newBounds);
        myComponent->repaint();

        // ..the event loop will now be unlocked as the MessageManagerLock goes out of scope
    }
    @endcode

    Obviously be careful not to create one of these and leave it lying around, or
    your app will grind to a halt!

    MessageManagerLocks are re-entrant, so can be safely nested if the current thread
    already has the lock.

    Another caveat is that using this in conjunction with other CriticalSections
    can create lots of interesting ways of producing a deadlock! In particular, if
    your message thread calls stopThread() for a thread that uses these locks,
    you'll get an (occasional) deadlock..

    @see MessageManager, MessageManager::currentThreadHasLockedMessageManager
*/
class JUCE_API MessageManagerLock
{
public:
    //==============================================================================
    /** Tries to acquire a lock on the message manager.

        The constructor attempts to gain a lock on the message loop, and the lock will be
        kept for the lifetime of this object.

        Optionally, you can pass a thread object here, and while waiting to obtain the lock,
        this method will keep checking whether the thread has been given the
        Thread::signalThreadShouldExit() signal. If this happens, then it will return
        without gaining the lock. If you pass a thread, you must check whether the lock was
        successful by calling lockWasGained(). If this is false, your thread is being told to
        die, so you should take evasive action.

        If you pass nullptr for the thread object, it will wait indefinitely for the lock - be
        careful when doing this, because it's very easy to deadlock if your message thread
        attempts to call stopThread() on a thread just as that thread attempts to get the
        message lock.

        If the calling thread already has the lock, nothing will be done, so it's safe and
        quick to use these locks recursively.

        E.g.
        @code
        void run()
        {
            ...

            while (! threadShouldExit())
            {
                MessageManagerLock mml (Thread::getCurrentThread());

                if (! mml.lockWasGained())
                    return; // another thread is trying to kill us!

                ..do some locked stuff here..
            }

            ..and now the MM is now unlocked..
        }
        @endcode

    */
    MessageManagerLock (Thread* threadToCheckForExitSignal = nullptr);

    //==============================================================================
    /** This has the same behaviour as the other constructor, but takes a ThreadPoolJob
        instead of a thread.

        See the MessageManagerLock (Thread*) constructor for details on how this works.
    */
    MessageManagerLock (ThreadPoolJob* jobToCheckForExitSignal);

    //==============================================================================
    struct BailOutChecker
    {
        virtual ~BailOutChecker() {}

        /** Return true if acquiring the lock should be aborted. */
        virtual bool shouldAbortAcquiringLock() = 0;
    };

    /** This is an abstraction of the other constructors. You can pass this constructor
        a functor which is periodically checked if attempting the lock should be aborted.

        See the MessageManagerLock (Thread*) constructor for details on how this works.
     */
     MessageManagerLock (BailOutChecker&);


    //==============================================================================
    /** Releases the current thread's lock on the message manager.

        Make sure this object is created and deleted by the same thread,
        otherwise there are no guarantees what will happen!
   */
    ~MessageManagerLock() noexcept;

    //==============================================================================
    /** Returns true if the lock was successfully acquired.
        (See the constructor that takes a Thread for more info).
    */
    bool lockWasGained() const noexcept                     { return locked; }

private:
    class BlockingMessage;
    friend class ReferenceCountedObjectPtr<BlockingMessage>;
    ReferenceCountedObjectPtr<BlockingMessage> blockingMessage;

    struct ThreadChecker : BailOutChecker
    {
        ThreadChecker (Thread* const, ThreadPoolJob* const);

        // Required to supress VS2013 compiler warnings
        ThreadChecker& operator= (const ThreadChecker&) = delete;

        bool shouldAbortAcquiringLock() override;

        Thread* const threadToCheck;
        ThreadPoolJob* const job;
    };

    //==============================================================================
    ThreadChecker checker;
    bool locked;

    //==============================================================================
    bool attemptLock (BailOutChecker*);

    JUCE_DECLARE_NON_COPYABLE (MessageManagerLock)
};

} // namespace juce
