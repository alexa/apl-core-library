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

#ifndef _ALEXAEXT_H
#define _ALEXAEXT_H

/**
* Public facing API for Alexa Extensions
*
* Extensions are optional enhancements to a runtime that provide additional sources of data,
* commands, and event handlers. Extensions must be explicitly requested before they are made
* available.  The extension user must ask for each extension that it is interested in and then
* check to see if the requested extension is available on that runtime.
*
* Each extension version has a unique URI. The URI is defined by the extension author. Extension
* authors should follow RFC-3986 Uniform Resource Identifier (URI): Generic Syntax when
* specifying the URI of an extension.  A sample extension URI that defines version 1.0 of a
* feature for the Alexa platform, may look like:
*                     alexaext:myfeature:10
*/

#include "extension.h"
#include "extensionbase.h"
#include "extensionexception.h"
#include "executor.h"
#include "extensionmessage.h"
#include "extensionprovider.h"
#include "extensionproxy.h"
#include "extensionschema.h"
#include "extensionregistrar.h"
#include "localextensionproxy.h"
#include "APLAudioPlayerExtension/AplAudioPlayerExtension.h"

#endif //_ALEXAEXT_H
