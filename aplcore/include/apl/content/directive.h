/*
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
 *
 * Store an APL document and data
 */

#ifndef _APL_DIRECTIVE_H
#define _APL_DIRECTIVE_H

#include "apl/engine/rootcontext.h"
#include "apl/content/jsondata.h"
#include "apl/content/content.h"
#include "apl/component/component.h"
#include "apl/utils/noncopyable.h"

namespace apl {

/**
 * Parse an entire Alexa Directive.  The directive is assumed to be of the form:
 *
 *     {
 *       "name": "RenderDocument",
 *       "namespace": "Alexa.Presentation.APL",
 *       "keys": ....,
 *       "eventId": OPAQUE
 *       "payload": {
 *         "presentationToken": OPAQUE,
 *         "document": {
 *           "type": "APL",
 *           "version": VERSION,
 *           "mainTemplate": {
 *             "parameters": [ "payload": ],
 *             "item": {
 *             }
 *           },
 *           // ...other fields as per APL Specification
 *         },
 *         "datasources": {
 *           // ...arbitrary fields
 *         },
 *         "packages": null,   // Or a list of packages
 *         "supportedViewports": [ ARRAY ]
 *       }
 *     }
 *
 * In this structure, the "datasources" object is assigned to the "payload" of
 * the mainTemplate.
 *
 * TODO: We are not currently parsing the "packages" field of the directive
 */
class Directive : public NonCopyable {
public:
    /**
     * Parse and return a complete directive.  If the directive
     * cannot be parsed correctly, return nullptr.  Please note that this
     * method does not attempt to inflate the APL document, so an invalid
     * directive may be returned here and yet fail to inflate correctly.
     *
     * @param directive The directive to parse.
     * @return A pointer to the directive or nullptr if invalid
     */
    static std::shared_ptr<Directive> create(JsonData&& directive);

    /**
     * Parse and return a complete directive.  If the directive
     * cannot be parsed correctly, return nullptr.  Please note that this
     * method does not attempt to inflate the APL document, so an invalid
     * directive may be returned here and yet fail to inflate correctly.
     *
     * @param directive The directive to parse.
     * @param session The session to log to
     * @return A pointer to the directive or nullptr if invalid
     */
    static std::shared_ptr<Directive> create(JsonData&& directive, const SessionPtr& session);

    /**
     * Basic constructor.  Use the create(JsonData&&) function instead.
     * @param directive
     */
    Directive(const SessionPtr& session, JsonData&& directive);

    /**
     * Construct a top-level root context
     * @param metrics Display metrics
     * @return A pointer to the root context
     */
    RootContextPtr build(const Metrics& metrics) const;

    /**
     * Construct a top-level root context.
     * @param metrics Display metrics
     * @param config Configuration information
     * @return A pointer to the root context
     */
    RootContextPtr build(const Metrics& metrics, const RootConfig& config) const;

    /**
     * @return The content object created from the directive.
     */
    ContentPtr content() const { return mContent; }

private:
    SessionPtr mSession;
    JsonData mDirective;
    ContentPtr mContent;
};


} // namespace apl

#endif // _APL_DIRECTIVE_H