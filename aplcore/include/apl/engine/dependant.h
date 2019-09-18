/**
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef _APL_DEPENDANT_H
#define _APL_DEPENDANT_H

#include <memory>

#include "apl/utils/counter.h"

namespace apl {

/**
 * A Dependant connects something that changes (like a data-binding) to something that needs to be informed
 * when a change occurs. The upstream object normally holds an array of dependants to be recalculated.  Each
 * dependant object is responsible for executing the appropriate recalculations on its downstream object.
 *
 * It's common for the target object to have its own list of dependants so that changes in one part of the
 * system will fan out and update many targets.  Loops in the dependant graph are not allowed.
 */
class Dependant : private Counter<Dependant>,  public std::enable_shared_from_this<Dependant> {
#ifdef DEBUG_MEMORY_USE
public:
    using Counter<Dependant>::itemsDelta;
#endif

public:
    virtual ~Dependant() = default;

    /**
     * Remove this dependant from the source or upstream
     */
    virtual void removeFromSource() = 0;

    /**
     * Recalculate the values in the target object.
     * @param useDirtyFlag If true, mark downstream changes as dirty
     */
    virtual void recalculate(bool useDirtyFlag) const = 0;
};

}  // namespace apl

#endif //_APL_DEPENDANT_H
