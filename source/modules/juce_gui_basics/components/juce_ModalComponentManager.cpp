/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2017 - ROLI Ltd.

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 5 End-User License
   Agreement and JUCE 5 Privacy Policy (both updated and effective as of the
   27th April 2017).

   End User License Agreement: www.juce.com/juce-5-licence
   Privacy Policy: www.juce.com/juce-5-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

class ModalComponentManager::ModalItem  : public ComponentMovementWatcher
{
public:
    ModalItem (Component* const comp, const bool autoDelete_)
        : ComponentMovementWatcher (comp),
          component (comp), returnValue (0),
          isActive (true), autoDelete (autoDelete_)
    {
        jassert (comp != nullptr);
    }

    void componentMovedOrResized (bool, bool) override {}

    void componentPeerChanged() override
    {
        if (! component->isShowing())
            cancel();
    }

    void componentVisibilityChanged() override
    {
        if (! component->isShowing())
            cancel();
    }

    void componentBeingDeleted (Component& comp) override
    {
        ComponentMovementWatcher::componentBeingDeleted (comp);

        if (component == &comp || comp.isParentOf (component))
        {
            autoDelete = false;
            cancel();
        }
    }

    void cancel()
    {
        if (isActive)
        {
            isActive = false;

            if (ModalComponentManager* mcm = ModalComponentManager::getInstanceWithoutCreating())
                mcm->triggerAsyncUpdate();
        }
    }

    Component* component;
    OwnedArray<Callback> callbacks;
    int returnValue;
    bool isActive, autoDelete;

private:
    JUCE_DECLARE_NON_COPYABLE (ModalItem)
};

//==============================================================================
ModalComponentManager::ModalComponentManager()
{
}

ModalComponentManager::~ModalComponentManager()
{
    stack.clear();
    clearSingletonInstance();
}

juce_ImplementSingleton_SingleThreaded (ModalComponentManager)


//==============================================================================
void ModalComponentManager::startModal (Component* component, bool autoDelete)
{
    if (component != nullptr)
        stack.add (new ModalItem (component, autoDelete));
}

void ModalComponentManager::attachCallback (Component* component, Callback* callback)
{
    if (callback != nullptr)
    {
        ScopedPointer<Callback> callbackDeleter (callback);

        for (int i = stack.size(); --i >= 0;)
        {
            ModalItem* const item = stack.getUnchecked(i);

            if (item->component == component)
            {
                item->callbacks.add (callback);
                callbackDeleter.release();
                break;
            }
        }
    }
}

void ModalComponentManager::endModal (Component* component)
{
    for (int i = stack.size(); --i >= 0;)
    {
        ModalItem* const item = stack.getUnchecked(i);

        if (item->component == component)
            item->cancel();
    }
}

void ModalComponentManager::endModal (Component* component, int returnValue)
{
    for (int i = stack.size(); --i >= 0;)
    {
        ModalItem* const item = stack.getUnchecked(i);

        if (item->component == component)
        {
            item->returnValue = returnValue;
            item->cancel();
        }
    }
}

int ModalComponentManager::getNumModalComponents() const
{
    int n = 0;
    for (int i = 0; i < stack.size(); ++i)
        if (stack.getUnchecked(i)->isActive)
            ++n;

    return n;
}

Component* ModalComponentManager::getModalComponent (const int index) const
{
    int n = 0;
    for (int i = stack.size(); --i >= 0;)
    {
        const ModalItem* const item = stack.getUnchecked(i);
        if (item->isActive)
            if (n++ == index)
                return item->component;
    }

    return nullptr;
}

bool ModalComponentManager::isModal (Component* const comp) const
{
    for (int i = stack.size(); --i >= 0;)
    {
        const ModalItem* const item = stack.getUnchecked(i);
        if (item->isActive && item->component == comp)
            return true;
    }

    return false;
}

bool ModalComponentManager::isFrontModalComponent (Component* const comp) const
{
    return comp == getModalComponent (0);
}

void ModalComponentManager::handleAsyncUpdate()
{
    for (int i = stack.size(); --i >= 0;)
    {
        const ModalItem* const item = stack.getUnchecked(i);

        if (! item->isActive)
        {
            ScopedPointer<ModalItem> deleter (stack.removeAndReturn (i));
            Component::SafePointer<Component> compToDelete (item->autoDelete ? item->component : nullptr);

            for (int j = item->callbacks.size(); --j >= 0;)
                item->callbacks.getUnchecked(j)->modalStateFinished (item->returnValue);

            compToDelete.deleteAndZero();
        }
    }
}

void ModalComponentManager::bringModalComponentsToFront (bool topOneShouldGrabFocus)
{
    ComponentPeer* lastOne = nullptr;

    for (int i = 0; i < getNumModalComponents(); ++i)
    {
        Component* const c = getModalComponent (i);

        if (c == nullptr)
            break;

        ComponentPeer* peer = c->getPeer();

        if (peer != nullptr && peer != lastOne)
        {
            if (lastOne == nullptr)
            {
                peer->toFront (topOneShouldGrabFocus);

                if (topOneShouldGrabFocus)
                    peer->grabFocus();
            }
            else
                peer->toBehind (lastOne);

            lastOne = peer;
        }
    }
}

bool ModalComponentManager::cancelAllModalComponents()
{
    const int numModal = getNumModalComponents();

    for (int i = numModal; --i >= 0;)
        if (Component* const c = getModalComponent(i))
            c->exitModalState (0);

    return numModal > 0;
}

//==============================================================================
#if JUCE_MODAL_LOOPS_PERMITTED
class ModalComponentManager::ReturnValueRetriever     : public ModalComponentManager::Callback
{
public:
    ReturnValueRetriever (int& v, bool& done) : value (v), finished (done) {}

    void modalStateFinished (int returnValue) override
    {
        finished = true;
        value = returnValue;
    }

private:
    int& value;
    bool& finished;

    JUCE_DECLARE_NON_COPYABLE (ReturnValueRetriever)
};

int ModalComponentManager::runEventLoopForCurrentComponent()
{
    // This can only be run from the message thread!
    jassert (MessageManager::getInstance()->isThisTheMessageThread());

    int returnValue = 0;

    if (Component* currentlyModal = getModalComponent (0))
    {
        FocusRestorer focusRestorer;

        bool finished = false;
        attachCallback (currentlyModal, new ReturnValueRetriever (returnValue, finished));

        JUCE_TRY
        {
            while (! finished)
            {
                if  (! MessageManager::getInstance()->runDispatchLoopUntil (20))
                    break;
            }
        }
        JUCE_CATCH_EXCEPTION
    }

    return returnValue;
}
#endif

//==============================================================================
struct LambdaCallback  : public ModalComponentManager::Callback
{
    LambdaCallback (std::function<void(int)> fn) noexcept : function (fn) {}
    void modalStateFinished (int result) override  { function (result); }

    std::function<void(int)> function;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LambdaCallback)
};

ModalComponentManager::Callback* ModalCallbackFunction::create (std::function<void(int)> f)
{
    return new LambdaCallback (f);
}

} // namespace juce
