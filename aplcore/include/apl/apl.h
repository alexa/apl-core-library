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

#ifndef _APL_H
#define _APL_H

/**
 * This file contains all of the internal core library's public-facing
 * APIs.  You should be able to write binding and view host code by
 * adding #include <apl/apl.h> and nothing else.
 */

#include "rapidjson/document.h"

#include "apl/apl_config.h"
#include "apl/action/action.h"
#include "apl/buildTimeConstants.h"
#include "apl/common.h"
#include "apl/component/component.h"
#include "apl/component/textmeasurement.h"
#include "apl/content/configurationchange.h"
#include "apl/content/content.h"
#include "apl/content/importref.h"
#include "apl/content/importrequest.h"
#include "apl/content/jsondata.h"
#include "apl/content/metrics.h"
#include "apl/content/package.h"
#include "apl/content/rootconfig.h"
#include "apl/datasource/datasourceconnection.h"
#include "apl/datasource/datasourceprovider.h"
#include "apl/engine/event.h"
#include "apl/engine/rootcontext.h"
#include "apl/extension/extensionclient.h"
#include "apl/focus/focusdirection.h"
#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicfilter.h"
#include "apl/graphic/graphicpattern.h"
#include "apl/livedata/livearray.h"
#include "apl/livedata/livemap.h"
#include "apl/media/mediamanager.h"
#include "apl/media/mediaobject.h"
#include "apl/media/mediaplayer.h"
#include "apl/media/mediaplayerfactory.h"
#include "apl/primitives/accessibilityaction.h"
#include "apl/primitives/dimension.h"
#include "apl/primitives/filter.h"
#include "apl/primitives/gradient.h"
#include "apl/primitives/mediasource.h"
#include "apl/primitives/mediastate.h"
#include "apl/primitives/object.h"
#include "apl/primitives/styledtext.h"
#include "apl/scaling/metricstransform.h"
#include "apl/touch/pointerevent.h"
#include "apl/utils/localemethods.h"
#include "apl/utils/log.h"
#include "apl/utils/session.h"

#ifdef ALEXAEXTENSIONS
#include "apl/extension/extensionmediator.h"
#endif

#endif // _APL_H
