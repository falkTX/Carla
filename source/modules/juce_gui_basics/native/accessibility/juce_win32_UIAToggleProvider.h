/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

//==============================================================================
class UIAToggleProvider  : public UIAProviderBase,
                           public ComBaseClassHelper<ComTypes::IToggleProvider>
{
public:
    using UIAProviderBase::UIAProviderBase;

    //==============================================================================
    JUCE_COMRESULT Toggle() override
    {
        if (! isElementValid())
            return (HRESULT) UIA_E_ELEMENTNOTAVAILABLE;

        const auto& handler = getHandler();

        if (handler.getActions().invoke (AccessibilityActionType::toggle)
            || handler.getActions().invoke (AccessibilityActionType::press))
        {
            VARIANT newValue;
            VariantHelpers::setInt (getCurrentToggleState(), &newValue);

            sendAccessibilityPropertyChangedEvent (handler, UIA_ToggleToggleStatePropertyId, newValue);

            return S_OK;
        }

        return (HRESULT) UIA_E_NOTSUPPORTED;
    }

    JUCE_COMRESULT get_ToggleState (ComTypes::ToggleState* pRetVal) override
    {
        return withCheckedComArgs (pRetVal, *this, [&]
        {
            *pRetVal = getCurrentToggleState();
            return S_OK;
        });
    }

private:
    ComTypes::ToggleState getCurrentToggleState() const
    {
        return getHandler().getCurrentState().isChecked() ? ComTypes::ToggleState_On
                                                          : ComTypes::ToggleState_Off;
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UIAToggleProvider)
};

} // namespace juce
