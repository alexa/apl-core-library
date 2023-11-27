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

#ifndef _APL_CONTENT_H
#define _APL_CONTENT_H

#include "apl/common.h"
#include "apl/content/extensionrequest.h"
#include "apl/content/metrics.h"
#include "apl/content/package.h"
#include "apl/content/settings.h"
#include "apl/engine/properties.h"
#include "apl/utils/counter.h"
#include "apl/utils/session.h"

namespace apl {

class JsonData;
class ImportRequest;
class ImportRef;
class RootConfig;

/**
 * Hold all of the documents and data necessary to inflate an APL component hierarchy.
 * The an approximate use of Content is (without error-checking):
 *
 *      // Initial creation of Content from an APL document
 *      auto content = Content::create( document );
 *      if (!content)
 *         return;  // Failed to create the document
 *      if (checkRequests(content))
 *         return READY_TO_GO;
 *
 *      // When a package comes in, call:
 *      content->addPackage(request, data);
 *      if (checkRequests(content))
 *          return READY_TO_GO
 *
 *      // Helper method to check for new packages that are needed
 *      bool checkRequests(ContentPtr& content) {
 *        for (ImportRequest request : content->getRequestedPackages())
 *          // Request package "request"
 *
 *        return content->isReady();
 *      }
 *
 * The other aspect of content is connecting the named APL document parameters
 * with actual data sets.  Use the addData() method to wire up parameter names
 * with JSON data.
 */
class Content : public Counter<Content>,
                public std::enable_shared_from_this<Content> {
public:
    /**
     * Construct the working Content object from a document.
     * @param document The JSON document.
     * @return A pointer to the content or nullptr if invalid.
     * @deprecated Use #create(JsonData&&, const SessionPtr&, const Metrics&, const RootConfig&)
     *             for root document and #create(JsonData&& document, const SessionPtr& session) for
     *             embedded document.
     */
    static APL_DEPRECATED ContentPtr create(JsonData&& document);

    /**
     * Construct the working Content object from a document, including a session for
     * reporting errors.
     * @param document The JSON document
     * @param session A logging session
     * @return A pointer to the content or nullptr if invalid
     * @note Should be used only for Embedded documents.
     */
    static ContentPtr create(JsonData&& document, const SessionPtr& session);

    /**
     * Construct the working Content object.
     * @param document The JSON document
     * @param session A logging session
     * @param metrics Viewport metrics.
     * @param config Document config.
     * @return A pointer to the content or nullptr if invalid
     */
    static ContentPtr create(JsonData&& document, const SessionPtr& session,
                             const Metrics& metrics, const RootConfig& config);

    /**
     * Refresh content with new (or finally known) parameters.
     * @param metrics Viewport metrics.
     * @param config Document config.
     */
    void refresh(const Metrics& metrics, const RootConfig& config);

    /**
     * Refresh content with embedded document request.
     * @param request request.
     * @param documentConfig DocumentConfig.
     */
    void refresh(const EmbedRequest& request, const DocumentConfigPtr& documentConfig);

    /**
     * @return The main document package
     */
    const PackagePtr& getDocument() const { return mMainPackage; }

    /**
     * Return a package by name
     * @param name The package name
     * @return The PackagePtr or nullptr if it does not exist.
     */
    PackagePtr getPackage(const std::string& name) const;

    /**
     * Retrieve a set of packages that have been requested.  This method only returns an
     * individual package a single time.  Once it has been called, the "requested" packages
     * are moved internally into a "pending" list of packages.
     *
     * @return The set of packages that should be loaded.
     */
    std::set<ImportRequest> getRequestedPackages();

    /**
     * @return true if this document is waiting for a number of packages to be loaded.
     */
    bool isWaiting() const { return mRequested.size() > 0 || mPending.size() > 0; }

    /**
     * @return True if this content is complete and ready to be inflated.
     */
    bool isReady() const { return mState == State::READY; }

    /**
     * @return True if this content is in an error state and can't be inflated.
     */
    bool isError() const { return mState == State::ERROR; }

    /**
     * Add a requested package to the document.
     * @param request The requested package import structure.
     * @param raw Parsed data for the package.
     */
    void addPackage(const ImportRequest& request, JsonData&& raw);

    /**
     * Add data
     * @param name The name of the data source
     * @param data The raw data source
     */
    void addData(const std::string& name, JsonData&& data);

    /**
     * Add data
     * @param name The name of the data source
     * @param data The raw data source
     */
    void addObjectData(const std::string& name, const Object& data);

    /**
     * @return The number of parameters
     */
    size_t getParameterCount() const { return mAllParameters.size(); }

    /**
     * Retrieve the name of a parameter
     * @param index Index of the parameter
     * @return Name
     */
    const std::string& getParameterAt(size_t index) const {
        return mAllParameters.at(index);
    }

    /**
     * @return Main document APL version.
     */
    std::string getAPLVersion() const { return mMainPackage->version(); }

    /**
     * @return The background object (color or gradient) for this document.  Returns
     *         the transparent color if no background is defined.
     * @deprecated Use #getBackground(). This method will create temporary evaluation context.
     */
    Object getBackground(const Metrics& metrics, const RootConfig& config) const;

    /**
     * @return The background object (color or gradient) for this document.  Returns
     *         the transparent color if no background is defined.
     * @note Usable only if full (#create(JsonData&&, const SessionPtr&, const Metrics&, const RootConfig&))
     *       constructor used as it requires stable evaluation context.
     */
    Object getBackground() const;

    /**
     * Returned object for the getEnvironment method.  Defined as a structure for
     * future expansion.
     */
    struct Environment {
        std::string language;
        LayoutDirection layoutDirection;
    };

    /**
     * Calculate environment properties
     * @param config Configuration settings
     * @return The environment object
     */
    Environment getEnvironment(const RootConfig& config) const;

    /**
     * @return document-wide properties.
     */
    const SettingsPtr getDocumentSettings() const;

    /**
     * @return The set of requested extensions (a list of URI values)
     */
    std::set<std::string> getExtensionRequests() const;

    /**
     * @return The ordered collection of extension requests
     */
    const std::vector<ExtensionRequest>& getExtensionRequestsV2() const;

    /**
     * Retrieve the settings associated with an extension request.
     * @param uri The uri of the extension.
     * @return Map of settings, Object::NULL_OBJECT() if no settings are specified in the document.
     */
    Object getExtensionSettings(const std::string& uri);

    /**
     * @return The active session
     */
    const SessionPtr& getSession() const { return mSession; }

    /**
     * @return An ordered list of the loaded packages, not including the main package
     */
    std::vector<std::string> getLoadedPackageNames() const;

    /**
     * @return The set of pending parameters.
     */
    std::set<std::string> getPendingParameters() const { return mPendingParameters; }

    /**
     * @return True if content can change due to evaluation support, false otherwise.
     */
    bool isMutable() const { return mEvaluationContext != nullptr; }

private:  // Non-public methods used by other classes
    friend class CoreDocumentContext;

    const std::vector<PackagePtr>& ordered() const {return mOrderedDependencies;};
    const rapidjson::Value& getMainTemplate() const { return mMainTemplate; }
    bool getMainProperties(Properties& out) const;

public:
    /**
     * Internal constructor. Do not call this directly.
     * @param session The APL session
     * @param mainPackagePtr The main package
     * @param mainTemplate The RapidJSON main template object
     */
    Content(const SessionPtr& session,
            const PackagePtr& mainPackagePtr,
            const rapidjson::Value& mainTemplate,
            const Metrics& metrics,
            const RootConfig& rootConfig);

private:  // Private internal methods
    void init(bool supportsEvaluation);
    void addImportList(Package& package);
    bool addImport(
        Package& package,
        const rapidjson::Value& value,
        const std::string& name = "",
        const std::string& version = "",
        const std::set<std::string>& loadAfter = {});
    void addExtensions(Package& package);
    void updateStatus();
    void loadExtensionSettings();
    bool orderDependencyList();
    bool addToDependencyList(std::vector<PackagePtr>& ordered, std::set<PackagePtr>& inProgress, const PackagePtr& package);
    bool allowAdd(const std::string& name);
    std::string extractTheme(const Metrics& metrics) const;
    static ContentPtr create(JsonData&& document, const SessionPtr& session, const Metrics& metrics,
                             const RootConfig& config, bool supportsEvaluation);
    Object extractBackground(const Context& evaluationContext) const;
    void loadPackage(const ImportRef& ref, const PackagePtr& package);

private:
    enum State {
        LOADING,
        READY,
        ERROR
    };

private:
    SessionPtr mSession;
    PackagePtr mMainPackage;

    std::vector<ExtensionRequest> mExtensionRequests;
    ObjectMapPtr mExtensionSettings; // Map Name -> <settingKey, settingValue> may be null

    State mState;
    const rapidjson::Value& mMainTemplate;
    Metrics mMetrics;
    RootConfig mConfig;
    ContextPtr mEvaluationContext;

    std::set<ImportRequest> mRequested;
    std::set<ImportRequest> mPending;
    std::map<ImportRef, PackagePtr> mLoaded;
    std::map<ImportRef, PackagePtr> mStashed;
    std::vector<PackagePtr> mOrderedDependencies;

    std::map<std::string, Object> mParameterValues;
    std::vector<std::string> mMainParameters;  // Requested by the main template
    std::vector<std::string> mEnvironmentParameters;  // Requested by the environment block
    std::set<std::string> mPendingParameters;  // Union of main and environment parameters
    std::vector<std::string> mAllParameters;   // Ordered of mPendingParameters.  First N elements match main
};


} // namespace apl

#endif //_APL_CONTENT_H
