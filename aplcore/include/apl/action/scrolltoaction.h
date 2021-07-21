/**
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef _APL_SCROLL_TO_ACTION_H
#define _APL_SCROLL_TO_ACTION_H

#include "apl/action/animatedscrollaction.h"
#include "apl/command/commandproperties.h"

namespace apl {

class CoreCommand;
class Component;

/**
 * Scroll or page to bring a target component into view.
 *
 * This action results in either a kEventScrollTo or kEventPageTo event being fired.
 * The following properties are passed with the event:
 *
 *    kCommandPropertyDirection        Set to forward/back (only for PageTo)
 *    kCommandPropertyPosition         The scroll position or the page number.
 */
class ScrollToAction : public AnimatedScrollAction {
public:
    /**
     * Called from SpeakItem during block highlight mode.
     * @param timers Timer reference.
     * @param command Command that spawned this action.
     * @param target Component to scroll to.
     * @return
     */
    static std::shared_ptr<ScrollToAction> make(const TimersPtr& timers,
                                                const std::shared_ptr<CoreCommand>& command,
                                                const CoreComponentPtr& target = nullptr);
    /**
     * Called from SpeakItem during line highlight mode.
     * @param timers Timer reference.
     * @param command Command that spawned this action.
     * @param subBounds Bounds within the target to scroll to.
     * @param target Component to scroll to.
     * @return
     */
    static std::shared_ptr<ScrollToAction> make(const TimersPtr& timers,
                                                const std::shared_ptr<CoreCommand>& command,
                                                const Rect& subBounds,
                                                const CoreComponentPtr& target = nullptr);


    /**
     * Called from rootcontext by viewhost during line karaoke.
     * @param timers Timer reference.
     * @param align Scroll slignment.
     * @param subBounds Bounds within the target to scroll to.
     * @param context Target context.
     * @param target Component to scroll to.
     * @return
     */
    static std::shared_ptr<ScrollToAction> make(const TimersPtr& timers,
                                                const CommandScrollAlign& align,
                                                const Rect& subBounds,
                                                const ContextPtr& context,
                                                const CoreComponentPtr& target = nullptr);

    /**
     * Called in order to bring child into view, utilizing snap setting if exists.
     * @param timers Timer reference.
     * @param target Component to scroll to.
     * @param duration scrolling duration.
     * @return
     */
    static std::shared_ptr<ScrollToAction> makeUsingSnap(const TimersPtr& timers,
                                                         const CoreComponentPtr& target,
                                                         apl_duration_t duration);

    ScrollToAction(const TimersPtr& timers,
                   const CommandScrollAlign& align,
                   const Rect& subBounds,
                   const ContextPtr& context,
                   bool scrollToSubBounds,
                   const CoreComponentPtr& target,
                   const CoreComponentPtr& scrollableParent,
                   apl_duration_t duration);

private:
    static std::shared_ptr<ScrollToAction> make(const TimersPtr& timers,
                                                const CommandScrollAlign& align,
                                                const Rect& subBounds,
                                                const ContextPtr& context,
                                                bool scrollToSubBounds,
                                                const CoreComponentPtr& target = nullptr,
                                                apl_duration_t duration = 0,
                                                bool useSnap = false);

    void start();
    void pageTo();
    void scrollTo();

private:
    CommandScrollAlign mAlign;
    Rect mSubBounds;
    bool mScrollToSubBounds;
    CoreComponentPtr mTarget;
};


} // namespace apl

#endif //_APL_SCROLL_TO_ACTION_H
