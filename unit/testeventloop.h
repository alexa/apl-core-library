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

#ifndef _APL_TEST_EVENT_LOOP_H
#define _APL_TEST_EVENT_LOOP_H

#include <chrono>
#include <queue>
#include <vector>
#include <iostream>
#include <regex>
#include <algorithm>

#include "rapidjson/error/en.h"
#include "rapidjson/pointer.h"

#include "gtest/gtest.h"

#include "apl/apl.h"
#include "apl/command/commandfactory.h"
#include "apl/command/corecommand.h"
#include "apl/time/coretimemanager.h"

#include "apl/utils/searchvisitor.h"
#include "apl/utils/range.h"

namespace apl {

class SimpleTextMeasurement : public TextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode,
                       float height, MeasureMode heightMode) override;
    float baseline(Component *component, float width, float height) override;
};

class CountingTextMeasurement : public SimpleTextMeasurement {
public:
    LayoutSize measure(Component *component, float width, MeasureMode widthMode,
                       float height, MeasureMode heightMode) override {
        auto ls = SimpleTextMeasurement::measure(component, width, widthMode, height, heightMode);
        measures++;
        return ls;
    }

    float baseline(Component *component, float width, float height) override {
        auto bl = SimpleTextMeasurement::baseline(component, width, height);
        baselines++;
        return bl;
    }

    int measures = 0;
    int baselines = 0;
};

inline ::testing::AssertionResult
MemoryMatch(const CounterPair& atStart, const CounterPair& atEnd)
{
    auto delta = atEnd - atStart;
    if (delta.created == delta.destroyed)
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << " created(" << delta.created
        << ") is not equal to destroyed(" << delta.destroyed << ")";
}

/************************************************************************
 * Test wrapper.  Track the number of things created and destroyed.
 * Also track console messages
 ************************************************************************/

class MixinCounter {
public:
    int getCount() const { return mMessages.size(); }

    void clear() { mMessages.clear(); }

    bool check(const std::string& msg) {
        auto s = msg;
        s.erase(std::remove_if(s.begin(),
                               s.end(),
                               [](unsigned char x) { return std::isspace(x); }), s.end());

        for (const auto& m : mMessages) {
            auto s2 = m;
            s2.erase(std::remove_if(s2.begin(), s2.end(),
                                    [](unsigned char x) { return std::isspace(x); }), s2.end());

            if (s2.find(s) != std::string::npos)
                return true;
        }

        return false;
    }

    bool checkAndClear() {
        auto result = !mMessages.empty();
        if (!result)
            for (const auto& m : mMessages)
                fprintf(stderr, "SESSION %s\n", m.c_str());
        clear();
        return result;
    }

    // Check for the existing of message.  To simplify testing, we
    // strip all whitespace.
    bool checkAndClear(const std::string& msg) {
        auto result = check(msg);
        if (!result)
            for (const auto& m : mMessages)
                fprintf(stderr, "SESSION %s\n", m.c_str());
        clear();
        return result;
    }

    std::string getLast() const {
        if (mMessages.empty())
            return std::string();

        return mMessages.back();
    }

protected:
    std::vector<std::string> mMessages;
};

class TestSession : public Session, public MixinCounter {
public:
    void write(const char *filename, const char *func, const char *value) override {
        mMessages.emplace_back(std::string(value));
    }
};

class TestLogBridge : public LogBridge, public MixinCounter {
public:
    void transport(LogLevel level, const std::string& log) override {
        mMessages.push_back(log);
        mLastLevel = level;

        fprintf(stderr, "%s %s\n", LEVEL_MAPPING.at(level).c_str(), log.c_str());
        fflush(stderr);
    }

    LogLevel getLastLevel() const { return mLastLevel; }

private:
    const std::map<LogLevel, std::string> LEVEL_MAPPING = {
        {LogLevel::kTrace, "T"},
        {LogLevel::kDebug, "D"},
        {LogLevel::kInfo, "I"},
        {LogLevel::kWarn, "W"},
        {LogLevel::kError, "E"},
        {LogLevel::kCritical, "C"}
    };

    LogLevel mLastLevel = LogLevel::kCritical;
};

#ifdef DEBUG_MEMORY_USE
const std::map<std::string, std::function<CounterPair()>>& getMemoryCounterMap();
#endif

class MemoryWrapper : public ::testing::Test {
public:
    MemoryWrapper()
        : Test(),
          session(std::make_shared<TestSession>()),
          logBridge(std::make_shared<TestLogBridge>())
    {
#ifdef DEBUG_MEMORY_USE
        for (const auto& m : getMemoryCounterMap())
            mMemoryCounters.emplace(m.first, m.second());
#endif
        LoggerFactory::instance().initialize(logBridge);
    }

    void SetUp() override
    {
        session->clear();
        logBridge->clear();
    }

    void TearDown() override
    {
        Test::TearDown();

#ifdef DEBUG_MEMORY_USE
        for (const auto& m : getMemoryCounterMap())
            EXPECT_TRUE(MemoryMatch(mMemoryCounters.at(m.first), m.second())) << "for class " << m.first;
#endif

        ASSERT_FALSE(session->getCount()) << "Extra console message left behind: " << session->getLast();
    }

    bool CheckNoActions()
    {
#ifdef DEBUG_MEMORY_USE
        return Counter<AccessibilityAction>::itemsDelta() == mMemoryCounters.at("Action");
#else
        return true;
#endif
    }

    bool ConsoleMessage()
    {
        return session->checkAndClear();
    }

    bool LogMessage()
    {
        return logBridge->checkAndClear();
    }

private:
#ifdef DEBUG_MEMORY_USE
    std::map<std::string, CounterPair> mMemoryCounters;
#endif

public:
    std::shared_ptr<TestSession> session;
    std::shared_ptr<TestLogBridge> logBridge;
};


/************************************************************************
 * Fake event loop.  Uses a heap to track registered timeouts.
 ************************************************************************/

class TestTimeManager : public CoreTimeManager {
public:
    TestTimeManager() : CoreTimeManager(0) {}

    void advanceToTime(apl_time_t time) {
        updateTime(time);
    }

    void advance() {
        if (size())
            advanceToTime(nextTimeout());
    }

    void advanceToEnd() {
        while (size())
            advanceToTime(nextTimeout());
    }

    void advanceBy(apl_duration_t delay) {
        advanceToTime(currentTime() + delay);
    }

    int animatorCount() const {
        return mAnimatorCount;
    }
};

/************************************************************************
 * Test wrapper.  Track the number of actions created and destroyed.
 *                Initialize the custom time manager.
 ************************************************************************/

class ActionWrapper : public MemoryWrapper {
public:
    ActionWrapper()
        : MemoryWrapper()
    {
        loop = std::make_shared<TestTimeManager>();
    }

    void TearDown() override
    {
        MemoryWrapper::TearDown();
        ASSERT_EQ(0, loop->size());
    }

    std::shared_ptr<TestTimeManager> loop;
};

/************************************************************************
 * Test wrapper.  Includes loading a document and tearing it down.
 ************************************************************************/

class DocumentWrapper : public ActionWrapper {
public:
    DocumentWrapper() : ActionWrapper()
    {
        config = RootConfig::create();
        metrics.size(1024,800).dpi(160).theme("dark");
        config->agent("Unit tests", "1.0").timeManager(loop).session(session);
        config->measure(std::make_shared<SimpleTextMeasurement>());
    }

    void loadDocument(const char *docName, const char *dataName = nullptr) {
        createContent(docName, dataName);
        inflate();
        ASSERT_TRUE(root);
    }

    void loadDocumentExpectFailure(const char *docName, const char *dataName = nullptr) {
        createContent(docName, dataName);
        inflate();
        ASSERT_FALSE(root);
    }

    void loadDocumentBadContent(const char *docName, const char *dataName = nullptr) {
        createContent(docName, dataName);
        ASSERT_TRUE(content->isError());
    }

    void loadDocumentWithPackage(const char *doc, const char *pkg) {
        content = Content::create(doc, session);
        ASSERT_TRUE(content->isWaiting());
        auto pkgs = content->getRequestedPackages();
        ASSERT_EQ(1, pkgs.size());
        auto it = pkgs.begin();
        content->addPackage(*it, pkg);
        ASSERT_TRUE(content->isReady());

        postCreateContent();

        inflate();
        ASSERT_TRUE(root);
        advanceTime(10);
    }

    template<class... Args>
    void loadDocumentWithMultiPackage(const char *doc, Args... args) {
        std::vector<std::string> pp = {args...};
        content = Content::create(doc, session);
        ASSERT_TRUE(content->isWaiting());
        auto pkgs = content->getRequestedPackages();

        auto it = pkgs.begin();
        for (int i=0; i < pp.size() && it != pkgs.end(); i++) {
            content->addPackage(*it, pp[i]);
            ASSERT_FALSE(content->isError());
            // request more packages if all outstanding request
            // have been satisfied on a waiting doc
            if (++it == pkgs.end() && content->isWaiting()) {
                pkgs = content->getRequestedPackages();
                it = pkgs.begin();
            }
        }

        ASSERT_TRUE(content->isReady());

        postCreateContent();

        inflate();
        ASSERT_TRUE(root);
        advanceTime(10);
    }

    /*
     * Release the component, context, and command.
     * This allows the ActionWrapper to verify that these objects will be correctly
     * destructed.
     */
    void TearDown() override {
        component = nullptr;

        clearDirty();

        if (root) {
            ASSERT_FALSE(root->hasEvent());
            ASSERT_EQ(false, root->screenLock());
        }

        context = nullptr;
        root = nullptr;
        content = nullptr;
        config = nullptr;

        // Call this last - it checks if all of the components, commands, and actions are released
        ActionWrapper::TearDown();
    }

    // Clear any dirty events.  Fail if there are any other type of event
    void clearDirty() {
        if (root) {
            ASSERT_TRUE(!root->hasEvent());
            if (root->isDirty())
                root->clearDirty();
            ASSERT_FALSE(root->isDirty());
        }
    }

    ActionPtr executeCommand(const std::string& name, const std::map<std::string, Object>& values, bool fastMode) {
        rapidjson::Value cmd(rapidjson::kObjectType);
        auto& alloc = command.GetAllocator();
        cmd.AddMember("type", rapidjson::Value(name.c_str(), alloc).Move(), alloc);
        for (auto& m : values)
            cmd.AddMember(rapidjson::StringRef(m.first.c_str()), m.second.serialize(alloc), alloc);
        command.SetArray().PushBack(cmd, alloc);
        return root->executeCommands(command, fastMode);
    }

    /// Given a provenance path, return the JSON that created this
    const rapidjson::Value* followPath(const std::string& path) {
        size_t offset = path.find('/');
        if (offset == std::string::npos)
            return nullptr;

        PackagePtr pkg = content->getPackage(path.substr(0,offset));
        if (!pkg)
            return nullptr;

        return rapidjson::Pointer(path.substr(offset).c_str()).Get(pkg->json());
    }

    void performClick(int x, int y) {
        root->handlePointerEvent(PointerEvent(kPointerDown, Point(x, y), 0, kMousePointer));
        root->handlePointerEvent(PointerEvent(kPointerUp, Point(x, y), 0, kMousePointer));
    }

    void performTap(int x, int y) {
        root->handlePointerEvent(PointerEvent(kPointerDown, Point(x, y), 0, kTouchPointer));
        root->handlePointerEvent(PointerEvent(kPointerUp, Point(x, y), 0, kTouchPointer));
    }

    void advanceTime(apl_duration_t duration) {
        root->updateTime(root->currentTime() + duration);
        root->clearPending();
    }

protected:
    void createContent(const char *document, const char *data) {
        content = Content::create(document, session);

        postCreateContent();

        if (!data)
            return;

        // If the content calls for a single parameter named "payload", we assign the data directly
        if (content->getParameterCount() == 1 && content->getParameterAt(0) == "payload") {
            content->addData("payload", data);
        }
        else {  // Otherwise, we require "data" to be a JSON object where the keys match the requested parameters
            rawData = std::unique_ptr<JsonData>(new JsonData(data));
            ASSERT_TRUE(rawData->get().IsObject());
            for (auto it = rawData->get().MemberBegin() ;
                      it != rawData->get().MemberEnd() ;
                      it++) {
                content->addData(it->name.GetString(), it->value);
            }
        }
    }

    // Override this in a subclass to insert processing after the content is created
    virtual void postCreateContent() {}

    // Override this in a subclass to insert processing after the context is created
    virtual void postInflate() {}

    void inflate() {
        ASSERT_TRUE(content);
        ASSERT_TRUE(content->isReady());

        root = RootContext::create(metrics, content, *config, createCallback);

        if (root) {
            context = root->contextPtr();
            ASSERT_TRUE(context);
            ASSERT_FALSE(context->getReinflationFlag());
            component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
        }

        postInflate();
    }

    // Register a configuration change
    void configChange(const ConfigurationChange& change) {
        ASSERT_TRUE(content);
        ASSERT_TRUE(content->isReady());
        ASSERT_TRUE(root);

        root->configurationChange(change);
    }

    void processReinflate() {
        root->clearPending();
        ASSERT_TRUE(root->hasEvent());
        auto event = root->popEvent();
        ASSERT_EQ(kEventTypeReinflate, event.getType());
        root->reinflate();
        context = root->contextPtr();
        ASSERT_TRUE(context);
        ASSERT_TRUE(context->getReinflationFlag());
        component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
    }

    // Register a configuration change and expect a reinflate event
    void configChangeReinflate(const ConfigurationChange& change) {
        configChange(change);
        processReinflate();
    }

public:
    CoreComponentPtr component;
    RootContextPtr root;
    ContextPtr context;
    Metrics metrics;
    RootConfigPtr config;
    rapidjson::Document command;
    std::function<void(const RootContextPtr&)> createCallback;
    ContentPtr content;
    std::unique_ptr<JsonData> rawData;
};


/**********************************************************************
 * Create a command that directly wraps around another command and forwards requests.
 * We use this wrapper to count the number of times the command has been inflated
 * into an action that takes some time to execute.
 *
 * Note that most actions do not take time to execute.
 * @tparam T The command type
 **********************************************************************/
template<class T>
class CommandWrapper : public Command {
public:
    CommandWrapper(T wrapped, int& counter)
        : Command(),
          mWrapped(std::move(wrapped)),
          mCounter(counter)
    {
        assert(mWrapped);
    }

    unsigned long delay() const override { return mWrapped->delay(); }
    std::string name() const override { return mWrapped->name(); }

    ActionPtr execute(const TimersPtr& eventLoop, bool fastMode) override {
        mCounter++;
        return mWrapped->execute(eventLoop, fastMode);
    }
    std::string sequencer() const override { return mWrapped->sequencer(); }

private:
    T mWrapped;
    int& mCounter;
};


/***********************************************************************
 * Build a custom command.  This command is packaged in a custom event
 * and sent to the view host in all cases.  It takes an "arguments"
 * parameter for passing additional data.
 ***********************************************************************/

class TestEventCommand : public CoreCommand {
public:
    static const char *COMMAND;
    static const char *EVENT;

    static void initialize() {
        auto index = sCommandNameBimap.append(COMMAND);
        sEventTypeBimap.append(EVENT);
        sCommandCreatorMap.emplace(index, create);
    }

    static EventType eventType() {
        return static_cast<EventType>(sEventTypeBimap.at(EVENT));
    }

    static CommandType commandType() {
        return static_cast<CommandType>(sCommandNameBimap.at(COMMAND));
    }

    static CommandPtr create(const ContextPtr& context,
                             Properties&& properties,
                             const CoreComponentPtr& base,
                             const std::string& parentSequencer) {
        auto ptr = std::make_shared<TestEventCommand>(context, std::move(properties), base, parentSequencer);
        return ptr->validate() ? ptr : nullptr;
    }

    TestEventCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base,
                     const std::string& parentSequencer)
        : CoreCommand(context, std::move(properties), base, parentSequencer)
    {}

    const CommandPropDefSet& propDefSet() const override {
        static CommandPropDefSet sTestEventCommandProperties(CoreCommand::propDefSet(), {
            {kCommandPropertyArguments,  Object::EMPTY_ARRAY(), asArray},
            {kCommandPropertyComponents, Object::EMPTY_ARRAY(), asArray},
        });

        return sTestEventCommandProperties;
    };

    CommandType type() const override { return static_cast<CommandType>(sCommandNameBimap.at("Custom")); }

    ActionPtr execute(const TimersPtr& timers, bool fastMode) override {
        if (!calculateProperties())
            return nullptr;

        // Calculate the component map
        auto componentsMap = std::make_shared<ObjectMap>();
        for (auto& compId : mValues.at(kCommandPropertyComponents).getArray()) {
            auto comp = std::dynamic_pointer_cast<CoreComponent>(mContext->findComponentById(compId.getString()));
            if (comp) {
                componentsMap->emplace(compId.getString(), comp->getValue());
            }
        }

        EventBag bag;
        bag.emplace(kEventPropertySource, mContext->opt("event").get("source"));
        bag.emplace(kEventPropertyArguments, mValues.at(kCommandPropertyArguments));
        bag.emplace(kEventPropertyComponents, componentsMap);
        mContext->pushEvent(Event(eventType(), std::move(bag)));

        return nullptr;
    }
};


/***********************************************************************
 * Install custom creators for command types in the command factory.
 * When the command is instantiated, we:
 *   1. Update a counter (ordered by command type)
 *   2. Call the "real" create method to build the command.
 *   3. Add a wrapper around the command to count when it is inflated into an Action.
 *   4. Push the command on the back of a list of commands for later inspection.
 ***********************************************************************/

class CommandTest : public DocumentWrapper {
public:
    CommandTest()
        : DocumentWrapper(),
          mCommandCount(sCommandNameBimap.maxA() + 2,0),
          mActionCount(sCommandNameBimap.maxA() + 2, 0)
    {
        TestEventCommand::initialize();
        for (const auto& m : sCommandNameBimap)
            wrap(m.second.c_str(), m.first);
    }

    void wrap(const char *name, int index)
    {
        if (sCommandCreatorMap.find(index) == sCommandCreatorMap.end())
            return;

        mOld.emplace(name, CommandFactory::instance().get(name));
        CommandFactory::instance().set(name,
                                       [&, index](const ContextPtr& context,
                                                  Properties&& props,
                                                  const CoreComponentPtr& base,
                                                  const std::string& parentSequencer) {
                                           auto ptr = sCommandCreatorMap.at(index)(context, std::move(props), base, parentSequencer);
                                           if (!ptr)
                                               return ptr;

                                           mCommandCount[index]++;
                                           mIssuedCommands.push_back(ptr);
                                           auto p = std::make_shared<CommandWrapper<CommandPtr>>(ptr, mActionCount[index]);
                                           return std::static_pointer_cast<Command>(p);
                                       });
    }

    /*
     * Release the component, context, and command.
     * This allows the ActionWrapper to verify that these objects will be correctly
     * destructed.
     */
    void TearDown() override {
        mIssuedCommands.clear();

        // Restore the old commands
        auto& cf = CommandFactory::instance();
        for (const auto& m : mOld)
            cf.set(m.first.c_str(), m.second);

        // Call this last - it checks if all of the components, commands, and actions are released
        DocumentWrapper::TearDown();
    }

public:
    std::vector<CommandPtr> mIssuedCommands;
    std::vector<int> mCommandCount;  // Incremented when a command is parsed into a CommandPtr
    std::vector<int> mActionCount;   // Incremented when an action is generated from the CommandPtr
    std::map<std::string, CommandFunc> mOld;
};

/***********************************************************************
 * Convenience methods for checking to see if the right number and types
 * of properties are dirty.  These methods call clearDirty after executing.
 ***********************************************************************/

template<class K, Bimap<int, std::string> &bimap>
std::string join(std::set<K> values) {
    std::stringstream ss;
    bool first = true;
    for (auto key : values) {
        if (first) first = false;
        else       ss << ",";
        ss << bimap.at(key);
    }
    return ss.str();
}

template<class... Args>
::testing::AssertionResult CheckDirty(const ComponentPtr& component, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::size_t actual = component->getDirty().size();
    std::vector<PropertyKey> v = {args...};

    if (actual != value)
        return ::testing::AssertionFailure() << "expected number of dirty component properties=" << value
            << " actual=" << actual
            << " [" << join<PropertyKey, sComponentPropertyBimap>(component->getDirty()) << "]";

    for (auto key : v) {
        if (component->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing property key " << sComponentPropertyBimap.at(key);
    }

    component->clearDirty();
    return ::testing::AssertionSuccess();
}


template<class... Args>
::testing::AssertionResult CheckDirtyDoNotClear(const ComponentPtr& component, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::size_t actual = component->getDirty().size();
    std::vector<PropertyKey> v = {args...};

    if (actual != value)
        return ::testing::AssertionFailure() << "expected number of dirty component properties=" << value
                                             << " actual=" << actual
                                             << " [" << join<PropertyKey, sComponentPropertyBimap>(component->getDirty()) << "]";

    for (auto key : v) {
        if (component->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing property key " << sComponentPropertyBimap.at(key);
    }

    return ::testing::AssertionSuccess();
}

// Check that EXACTLY these components are dirty
template<class... Args>
::testing::AssertionResult CheckDirty(const RootContextPtr& root, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::vector<ComponentPtr> v = {args...};
    for (const auto& key : v) {
        if (root->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing component " << key << ":" << key->toDebugString();
    }

    for (const auto &key : root->getDirty()) {
        auto it = std::find(v.begin(), v.end(), key);
        if (it == v.end())
            return ::testing::AssertionFailure() << "extra component " << key << ":" << key->toDebugString();
    }

    std::size_t actual = root->getDirty().size();
    if (actual != value)
        return ::testing::AssertionFailure() << "wrong number of dirty components.  Expected " << value << " but got " << actual;

    root->clearDirty();
    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult CheckDirtyDoNotClear(const RootContextPtr& root, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::vector<ComponentPtr> v = {args...};
    for (const auto& key : v) {
        if (root->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing component " << key << ":" << key->toDebugString();
    }

    for (const auto &key : root->getDirty()) {
        auto it = std::find(v.begin(), v.end(), key);
        if (it == v.end())
            return ::testing::AssertionFailure() << "extra component " << key << ":" << key->toDebugString();
    }

    std::size_t actual = root->getDirty().size();
    if (actual != value)
        return ::testing::AssertionFailure() << "wrong number of dirty components.  Expected " << value << " but got " << actual;

    return ::testing::AssertionSuccess();
}

// Check that AT LEAST all of this components are dirty.  More may be dirty; that's fine.
template<class... Args>
::testing::AssertionResult CheckDirtyAtLeast(const RootContextPtr& root, Args... args)
{
    std::vector<ComponentPtr> v = {args...};
    for (const auto& key : v) {
        if (root->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing component " << key << ":" << key->toDebugString();
    }

    root->clearDirty();
    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult CheckDirty(const GraphicPtr& graphic, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::size_t actual = graphic->getDirty().size();
    if (actual != value)
        return ::testing::AssertionFailure() << "wrong number of dirty graphic elements.  Expected " << value << " but got " << actual;

    std::vector<GraphicElementPtr> v = {args...};
    for (const auto& key : v) {
        if (graphic->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing graphic element " << key;
    }

    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult CheckDirty(const GraphicElementPtr& element, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::size_t actual = element->getDirtyProperties().size();
    std::vector<GraphicPropertyKey> v = {args...};
    if (actual != value)
        return ::testing::AssertionFailure() << "expected number of dirty graphic properties=" << value
                << " actual=" << actual
                << " [" << join<GraphicPropertyKey, sGraphicPropertyBimap>(element->getDirtyProperties()) << "]";

    for (auto key : v) {
        if (element->getDirtyProperties().count(key) != 1)
            return ::testing::AssertionFailure() << "missing graphic key " << sGraphicPropertyBimap.at(key);
    }

    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult CheckState(const ComponentPtr& component, Args... args)
{
    // Construct an array of values for the expected state from the passed arguments
    std::vector<bool> values(kStatePropertyCount, false);
    std::vector<StateProperty> v = {args...};
    for (auto key : v)
        values[key] = true;

    auto& core = std::static_pointer_cast<CoreComponent>(component)->getState();
    for (int i = 0 ; i < kStatePropertyCount ; i++) {
        auto key = static_cast<StateProperty>(i);
        if (core.get(key) != values[i])
            return ::testing::AssertionFailure() << "incorrect state key " << sStateBimap.at(key) << " is " << core.get(key);
    }

    return ::testing::AssertionSuccess();
}


template<class... Args>
::testing::AssertionResult TransformComponent(const RootContextPtr& root, const std::string& id, Args... args) {
    auto component = root->topComponent()->findComponentById(id);
    if (!component)
        return ::testing::AssertionFailure() << "Unable to find component " << id;

    std::vector<Object> values = {args...};
    if (values.size() % 2 != 0)
        return ::testing::AssertionFailure() << "Must provide an even number of transformations";

    ObjectArray transforms;
    for (auto it = values.begin() ; it != values.end() ; ) {
        auto key = *it++;
        auto value = *it++;
        auto map = std::make_shared<ObjectMap>();
        map->emplace(key.asString(), value);
        transforms.emplace_back(std::move(map));
    }

    auto event = std::make_shared<ObjectMap>();
    event->emplace("type", "SetValue");
    event->emplace("componentId", id);
    event->emplace("property", "transform");
    event->emplace("value", std::move(transforms));

    root->executeCommands(ObjectArray{event}, true);
    root->clearPending();

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult IsEqual(const Transform2D& lhs, const Transform2D& rhs)
{
    for (int i = 0 ; i < 6 ; i++)
        if (std::abs(lhs.get()[i] - rhs.get()[i]) > 0.0001) {
            streamer s;
            s << "[" << lhs << "] != [" << rhs << "]";
            return ::testing::AssertionFailure() << s.str();
        }

    return ::testing::AssertionSuccess();
}

template<class T>
::testing::AssertionResult IsEqual(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size())
        return ::testing::AssertionFailure() << "Size mismatch a=" << a.size() << " b=" << b.size();

    for (int i = 0; i < a.size(); i++)
        if (a.at(i) != b.at(i))
            return ::testing::AssertionFailure()
                   << "Element mismatch index=" << i << " a=" << a.at(i) << " b=" << b.at(i);

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult IsEqual(const Object& lhs, const Object& rhs)
{
    if (lhs != rhs)
        return ::testing::AssertionFailure() << lhs.toDebugString() << " != " << rhs.toDebugString();

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult MatchEvent(const Event&        event,
                                      EventType           eventType,
                                      EventBag&&          bag,
                                      const ComponentPtr& component )
{
    if (event.matches(Event(eventType, std::move(bag), component)))
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << event.getType() << " doesn't match " << eventType;
}

inline void
CheckMediaState(const MediaState &state, const CalculatedPropertyMap &propertyMap) {
    ASSERT_EQ(state.getTrackIndex(), propertyMap.get(kPropertyTrackIndex).asInt());
    ASSERT_EQ(state.getTrackCount(), propertyMap.get(kPropertyTrackCount).asInt());
    ASSERT_EQ(state.getDuration(), propertyMap.get(kPropertyTrackDuration).asInt());
    ASSERT_EQ(state.getCurrentTime(), propertyMap.get(kPropertyTrackCurrentTime).asInt());
    ASSERT_EQ(state.isPaused(), propertyMap.get(kPropertyTrackPaused).asBoolean());
    ASSERT_EQ(state.isEnded(), propertyMap.get(kPropertyTrackEnded).asBoolean());
    ASSERT_EQ(state.getTrackState(), propertyMap.get(kPropertyTrackState).asInt());
}

inline
::testing::AssertionResult
CheckChildLaidOut(const ComponentPtr& component, int childIndex, bool laidOut) {
    bool actualLaidOut = component->getChildAt(childIndex)->getCalculated(kPropertyLaidOut).asBoolean();
    if (laidOut != actualLaidOut) {
        return ::testing::AssertionFailure()
                << "component " << childIndex << " layout state is wrong. Expected: " << laidOut << ","
                << "actual: " << actualLaidOut;
    }
    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
CheckChildrenLaidOut(const ComponentPtr& component, Range childRange, bool laidOut) {
    for (int i = childRange.lowerBound(); i <= childRange.upperBound(); i++) {
        auto result = CheckChildLaidOut(component, i, laidOut);
        if (!result) {
            return result;
        }
    }
    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
CheckChildrenLaidOut(const ComponentPtr& component, std::set<int>&& laidOutChildren)
{
    int matched = 0;
    for (size_t childIndex = 0; childIndex < component->getChildCount(); childIndex++) {
        bool expectedToBeLaidOut = laidOutChildren.find(childIndex) != laidOutChildren.end();
        auto result = CheckChildLaidOut(component, childIndex, expectedToBeLaidOut);
        if (!result)
            return result;
        if (expectedToBeLaidOut)
            matched++;
    }
    if (matched != laidOutChildren.size())
        return ::testing::AssertionFailure() << "expected " << laidOutChildren.size() << " children; actually found "
                                             << matched << " laid-out children";

    return ::testing::AssertionSuccess();
}

template<class... Args>
inline
::testing::AssertionResult
CheckChildLaidOutDirtyFlags(const ComponentPtr& component, int idx, Args... additionalFlags) {
    return CheckDirty(component->getChildAt(idx),
                      kPropertyBounds, kPropertyInnerBounds, kPropertyVisualHash, kPropertyLaidOut, additionalFlags...);
}

inline
::testing::AssertionResult
CheckChildLaidOutDirtyFlagsWithNotify(const ComponentPtr& component, int idx) {
    return CheckDirty(component->getChildAt(idx),
                      kPropertyBounds, kPropertyInnerBounds, kPropertyLaidOut, kPropertyNotifyChildrenChanged, kPropertyVisualHash);
}

template<class... Args>
inline
::testing::AssertionResult
CheckChildrenLaidOutDirtyFlags(const ComponentPtr& component, Range range, Args... additionalFlags) {
    for (int idx = range.lowerBound(); idx <= range.upperBound(); idx++) {
        ::testing::AssertionResult result = CheckChildLaidOutDirtyFlags(component, idx, additionalFlags...);
        if (!result) {
            return result << ", for component: " << idx;
        }
    }
    return ::testing::AssertionSuccess();
}

template<class... Args>
inline
::testing::AssertionResult
CheckChildrenLaidOutDirtyFlagsWithNotify(const ComponentPtr& component, Range range, Args... additionalFlags) {
    for (int idx = range.lowerBound(); idx <= range.upperBound(); idx++) {
        ::testing::AssertionResult result = CheckChildLaidOutDirtyFlagsWithNotify(component, idx, additionalFlags...);
        if (!result) {
            return result << ", for component: " << idx;
        }
    }
    return ::testing::AssertionSuccess();
}


template<class... Args>
::testing::AssertionResult CheckDirtyVisualContext(const RootContextPtr& root, Args... args)
{
    static const bool value = (sizeof...(Args) != 0);

    if (root->isVisualContextDirty() != value)
        return ::testing::AssertionFailure() << "expected root visual context dirty to be " << value;

    std::vector<ComponentPtr> v = {args...};
    for (const auto& target : v)
        if (!std::dynamic_pointer_cast<CoreComponent>(target)->isVisualContextDirty())
            return ::testing::AssertionFailure() << "invalid visual context isDirty" ;

    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult
CheckSendEvent(const RootContextPtr& root, Args... args) {
    if (!root->hasEvent())
        return ::testing::AssertionFailure() << "Has no events.";

    auto event = root->popEvent();
    if (event.getType() != kEventTypeSendEvent)
        return ::testing::AssertionFailure() << "Event has wrong type:"
                                             << " Expected: SendEvent"
                                             << ", actual: " << sEventTypeBimap.at(event.getType());

    auto arguments = event.getValue(kEventPropertyArguments);
    std::vector<Object> values = {args...};

    if (arguments.size() != values.size())
        return ::testing::AssertionFailure()
               << "Mismatch in argument list size actual=" << arguments.size()
               << " expected=" << values.size();

    // Allow generic matches for stuff like "NUMBER[]", "STRING[4]", "MAP[]"
    const std::regex PATTERN("([A-Z]+)\\[([0-9]*)\\]");

    for (int i = 0; i < values.size(); i++) {
        auto expected = values.at(i);
        auto actual = arguments.at(i);
        if (expected == actual)
            continue;

        std::smatch pieces;
        if (expected.isString()) {
            auto s = expected.asString();  // GCC forbids temporary strings in regex_match
            if (std::regex_match(s, pieces, PATTERN)) {
                if (pieces[1] == "NUMBER" && actual.isNumber())
                    continue;

                if ((pieces[1] == "STRING" && actual.isString()) ||
                    (pieces[1] == "MAP" && actual.isMap()) ||
                    (pieces[1] == "ARRAY" && actual.isArray())) {

                    // If there is a number specified, it must match the object size
                    if (pieces.str(2).empty() || std::stoi(pieces.str(2)) == actual.size())
                        continue;
                }
            }
        }

        if (expected.isNumber()) {
            auto x1 = expected.asNumber();
            auto x2 = actual.asNumber();
            if (std::isnan(x2) && std::isnan(x1))
                continue;
            else if (!std::isnan(x2) && std::abs(x1 - x2) < 0.001)
                continue;
        }

        return ::testing::AssertionFailure() << "Event has wrong argument[" << i << "]:"
                                             << " Expected: " << expected.toDebugString()
                                             << ", actual: " << actual.toDebugString();
    }

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseDown(const RootContextPtr& root, const CoreComponentPtr& comp, double x, double y) {
    auto point = Point(x,y);
    root->handlePointerEvent(PointerEvent(kPointerDown, point));

    auto visitor = TouchableAtPosition(point);
    std::dynamic_pointer_cast<CoreComponent>(root->topComponent())->raccept(visitor);
    auto target = std::dynamic_pointer_cast<CoreComponent>(visitor.getResult());

    if (target != comp)
        return ::testing::AssertionFailure() << "Down failed to hit target at " << x << "," << y;

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseUp(const RootContextPtr& root, const CoreComponentPtr& comp, double x, double y) {
    auto point = Point(x,y);
    root->handlePointerEvent(PointerEvent(kPointerUp, point));

    auto visitor = TouchableAtPosition(point);
    std::dynamic_pointer_cast<CoreComponent>(root->topComponent())->raccept(visitor);
    auto target = std::dynamic_pointer_cast<CoreComponent>(visitor.getResult());

    if (target != comp)
        return ::testing::AssertionFailure() << "Up failed to hit target at " << x << "," << y;

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseDown(const RootContextPtr& root, double x, double y) {
    auto point = Point(x,y);
    root->handlePointerEvent(PointerEvent(kPointerDown, point));

    auto visitor = TouchableAtPosition(point);
    std::dynamic_pointer_cast<CoreComponent>(root->topComponent())->raccept(visitor);
    auto target = std::dynamic_pointer_cast<CoreComponent>(visitor.getResult());

    if (!target)
        return ::testing::AssertionFailure() << "Down failed to hit target at " << x << "," << y;

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseUp(const RootContextPtr& root, double x, double y) {
    auto point = Point(x,y);
    root->handlePointerEvent(PointerEvent(kPointerUp, point));

    auto visitor = TouchableAtPosition(point);
    std::dynamic_pointer_cast<CoreComponent>(root->topComponent())->raccept(visitor);
    auto target = std::dynamic_pointer_cast<CoreComponent>(visitor.getResult());

    if (!target)
        return ::testing::AssertionFailure() << "Up failed to hit target at " << x << "," << y;

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseMove(const RootContextPtr& root, double x, double y) {
    auto point = Point(x,y);
    root->handlePointerEvent(PointerEvent(kPointerMove, point));

    auto visitor = TouchableAtPosition(point);
    std::dynamic_pointer_cast<CoreComponent>(root->topComponent())->raccept(visitor);
    auto target = std::dynamic_pointer_cast<CoreComponent>(visitor.getResult());

    if (!target)
        return ::testing::AssertionFailure() << "Up failed to hit target at " << x << "," << y;

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseClick(const RootContextPtr& root, double x, double y) {
    auto downResult = MouseDown(root, x, y);
    auto upResult = MouseUp(root, x, y);

    if (downResult != ::testing::AssertionSuccess())
        return downResult;
    if (upResult != ::testing::AssertionSuccess())
        return upResult;

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
MouseClick(const RootContextPtr& root, const CoreComponentPtr& comp, double x, double y) {
    auto downResult = MouseDown(root, comp, x, y);
    auto upResult = MouseUp(root, comp, x, y);

    if (downResult != ::testing::AssertionSuccess())
        return downResult;
    if (upResult != ::testing::AssertionSuccess())
        return upResult;

    return ::testing::AssertionSuccess();
}

template<class... Args>
::testing::AssertionResult
HandlePointerEvent(const RootContextPtr& root, PointerEventType type, const Point& point, bool consumed, Args... args) {
    if (root->handlePointerEvent(PointerEvent(type, point)) != consumed)
        return ::testing::AssertionFailure() << "Event consumption mismatch.";

    if (sizeof...(Args) > 0) {
        return CheckSendEvent(root, args...);
    }

    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
compareTransformApprox(const Transform2D& left, const Transform2D& right, float delta = 0.001F) {
    auto leftComponents = left.get();
    auto rightComponents = right.get();

    for (int i = 0; i < 6; i++) {
        auto diff = std::abs(leftComponents.at(i) - rightComponents.at(i));
        if (diff > delta) {
            return ::testing::AssertionFailure()
                    << "transorms is not equal: "
                    << " Expected: " << left.toDebugString()
                    << ", actual: " << right.toDebugString();
        }
    }
    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
CheckTransform(const Transform2D& expected, const ComponentPtr& component) {
    return compareTransformApprox(expected, component->getCalculated(kPropertyTransform).getTransform2D());
}

inline
::testing::AssertionResult
CheckTransformApprox(const Transform2D& expected, const ComponentPtr& component, float delta) {
    return compareTransformApprox(expected, component->getCalculated(kPropertyTransform).getTransform2D(), delta);
}

inline
::testing::AssertionResult
expectBounds(ComponentPtr comp, float top, float left, float bottom, float right) {
    auto bounds = comp->getCalculated(kPropertyBounds).getRect();
    if (bounds.getTop() != top)
        return ::testing::AssertionFailure() << "bounds.getTop() does not equal top: "
                                             << bounds.getTop() << " != " << top;
    if (bounds.getLeft() != left)
        return ::testing::AssertionFailure() << "bounds.getLeft() does not equal left: "
                                             << bounds.getLeft() << " != " << left;
    if (bounds.getBottom() != bottom)
        return ::testing::AssertionFailure() << "bounds.getBottom() does not equal bottom: "
                                             << bounds.getBottom() << " != " << bottom;
    if (bounds.getRight() != right)
        return ::testing::AssertionFailure() << "bounds.getRight() does not equal right: "
                                             << bounds.getRight() << " != " << right;
    return ::testing::AssertionSuccess();
}

inline
::testing::AssertionResult
expectInnerBounds(ComponentPtr comp, float top, float left, float bottom, float right) {
    auto bounds = comp->getCalculated(kPropertyInnerBounds).getRect();
    if (bounds.getTop() != top)
        return ::testing::AssertionFailure() << "bounds.getTop() does not equal top: "
                                             << bounds.getTop() << " != " << top;
    if (bounds.getLeft() != left)
        return ::testing::AssertionFailure() << "bounds.getLeft() does not equal left: "
                                             << bounds.getLeft() << " != " << left;
    if (bounds.getBottom() != bottom)
        return ::testing::AssertionFailure() << "bounds.getBottom() does not equal bottom: "
                                             << bounds.getBottom() << " != " << bottom;
    if (bounds.getRight() != right)
        return ::testing::AssertionFailure() << "bounds.getRight() does not equal right: "
                                             << bounds.getRight() << " != " << right;
    return ::testing::AssertionSuccess();
}

inline
Object
StringToMapObject(const std::string& payload) {
    rapidjson::Document doc;
    doc.Parse(payload.c_str());
    return Object(std::move(doc));
}

extern std::ostream& operator<<(std::ostream& os, const Point& point);
extern std::ostream& operator<<(std::ostream& os, const Transform2D& t);
extern std::ostream& operator<<(std::ostream& os, const Radii& r);
extern std::ostream& operator<<(std::ostream& os, const Object& object);

} // namespace apl

#endif //_APL_TEST_EVENT_LOOP_H
