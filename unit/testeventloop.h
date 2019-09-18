/**
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "rapidjson/error/en.h"
#include "rapidjson/pointer.h"

#include "gtest/gtest.h"

#include "apl/action/action.h"
#include "apl/component/component.h"
#include "apl/command/command.h"
#include "apl/command/commandfactory.h"
#include "apl/command/corecommand.h"
#include "apl/content/content.h"
#include "apl/engine/context.h"
#include "apl/engine/rootcontext.h"
#include "apl/graphic/graphic.h"
#include "apl/graphic/graphicelement.h"
#include "apl/time/coretimemanager.h"
#include "apl/content/metrics.h"
#include "apl/engine/builder.h"
#include "apl/content/rootconfig.h"
#include "apl/graphic/graphic.h"
#include "apl/utils/streamer.h"


namespace apl {

template<typename T>
::testing::AssertionResult MemoryMatch(const typename Counter<T>::Pair& atStart,
                                       const typename Counter<T>::Pair& atEnd)
{
    typename Counter<T>::Pair delta = atEnd - atStart;
    if (delta.created == delta.destroyed)
        return ::testing::AssertionSuccess();

    return ::testing::AssertionFailure() << " created(" << delta.created
        << ") is not equal to destroyed(" << delta.destroyed << ")"
        << " for " << typeid(T).name();
}

/************************************************************************
 * Test wrapper.  Track the number of things created and destroyed.
 * Also track console messages
 ************************************************************************/

class MixinCounter {
public:
    int getCount() const { return mCount; }
    void clear() { mCount = 0; }

    bool checkAndClear() {
        auto result = mCount != 0;
        mCount = 0;
        return result;
    }

    std::string getLast() const { return mLast; }

protected:
    int mCount = 0;
    std::string mLast;
};

class TestSession : public Session, public MixinCounter {
public:
    void write(const char *filename, const char *func, const char *value) override {
        mLast = std::string(value);
        mCount++;

        printf("SESSION %s\n", value);
    }
};

class TestLogBridge : public LogBridge, public MixinCounter {
public:
    void transport(LogLevel level, const std::string& log) override {
        mLast = log;
        mLastLevel = level;
        mCount++;

        printf("%s %s\n", LEVEL_MAPPING.at(level).c_str(), log.c_str());
    }

    LogLevel getLastLevel() const { return mLastLevel; }

private:
    const std::map<LogLevel, std::string> LEVEL_MAPPING = {
        {LogLevel::TRACE, "T"},
        {LogLevel::DEBUG, "D"},
        {LogLevel::INFO, "I"},
        {LogLevel::WARN, "W"},
        {LogLevel::ERROR, "E"},
        {LogLevel::CRITICAL, "C"}
    };

    LogLevel mLastLevel;
};

class MemoryWrapper : public ::testing::Test {
public:
    MemoryWrapper()
        : Test(),
#ifdef DEBUG_MEMORY_USE
          mActions(Action::itemsDelta()),
          mComponents(Component::itemsDelta()),
          mCommands(Command::itemsDelta()),
          mDependants(Dependant::itemsDelta()),
          mGraphics(Graphic::itemsDelta()),
          mGraphicElements(GraphicElement::itemsDelta()),
#endif
          session(std::make_shared<TestSession>()),
          logBridge(std::make_shared<TestLogBridge>())
    {
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
        auto actionTest = MemoryMatch<Action>(mActions, Action::itemsDelta());
        ASSERT_TRUE(actionTest);

        auto componentTest = MemoryMatch<Component>(mComponents, Component::itemsDelta());
        ASSERT_TRUE(componentTest);

        auto commandTest = MemoryMatch<Command>(mCommands, Command::itemsDelta());
        ASSERT_TRUE(commandTest);

        auto dependantTest = MemoryMatch<Dependant>(mDependants, Dependant::itemsDelta());
        ASSERT_TRUE(dependantTest);

        auto graphicTest = MemoryMatch<Graphic>(mGraphics, Graphic::itemsDelta());
        ASSERT_TRUE(graphicTest);

        auto graphicElementTest = MemoryMatch<GraphicElement>(mGraphicElements, GraphicElement::itemsDelta());
        ASSERT_TRUE(graphicElementTest);
#endif

        ASSERT_FALSE(session->getCount()) << "Extra console message left behind: " << session->getLast();
    }

    bool CheckNoActions()
    {
#ifdef DEBUG_MEMORY_USE
        return mActions.created == mActions.destroyed;
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

public:
    std::shared_ptr<TestSession> session;
    std::shared_ptr<TestLogBridge> logBridge;

private:
#ifdef DEBUG_MEMORY_USE
    Counter<Action>::Pair mActions;
    Counter<Component>::Pair mComponents;
    Counter<Command>::Pair mCommands;
    Counter<Dependant>::Pair mDependants;
    Counter<Graphic>::Pair mGraphics;
    Counter<GraphicElement>::Pair mGraphicElements;
#endif
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
        metrics.size(1024,800).dpi(160).theme("dark");
        config.agent("Unit tests", "1.0").timeManager(loop).session(session);
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

    /*
     * Release the component, context, and command.
     * This allows the ActionWrapper to verify that these objects will be correctly
     * destructed.
     */
    void TearDown() override {
        if (component) {
            component->release();
            component = nullptr;
        }

        clearDirty();

        if (root) {
            ASSERT_FALSE(root->hasEvent());
            ASSERT_EQ(false, root->screenLock());
        }

        context = nullptr;
        root = nullptr;

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

    ActionPtr executeCommand(const std::string& name, const std::map<std::string, Object> values, bool fastMode) {
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

protected:
    void createContent(const char *document, const char *data) {
        content = Content::create(document, session);

        if (data != nullptr)
            content->addData(content->getParameterAt(0), data);
    }

    void inflate() {
        ASSERT_TRUE(content);
        ASSERT_TRUE(content->isReady());

        root = RootContext::create(metrics, content, config, createCallback);

        if (root) {
            context = root->contextPtr();
            ASSERT_TRUE(context);

            component = std::dynamic_pointer_cast<CoreComponent>(root->topComponent());
        }
    }

public:
    CoreComponentPtr component;
    RootContextPtr root;
    ContextPtr context;
    Metrics metrics;
    RootConfig config;
    rapidjson::Document command;
    std::function<void(const RootContextPtr&)> createCallback;

    std::vector<CommandPtr> issuedCommands;
    ContentPtr content;
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
          mWrapped(wrapped),
          mCounter(counter)
    {
        assert(mWrapped);
    }

    virtual unsigned long delay() const override { return mWrapped->delay(); }
    virtual std::string name() const override { return mWrapped->name(); }

    virtual ActionPtr execute(const TimersPtr& eventLoop, bool fastMode) override {
        mCounter++;
        return mWrapped->execute(eventLoop, fastMode);
    }

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
                             const CoreComponentPtr& base) {
        auto ptr = std::make_shared<TestEventCommand>(context, std::move(properties), base);
        return ptr->validate() ? ptr : nullptr;
    }

    TestEventCommand(const ContextPtr& context, Properties&& properties, const CoreComponentPtr& base)
        : CoreCommand(context, std::move(properties), base)
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
        assert(sCommandCreatorMap.at(index) != nullptr);

        mOld.emplace(name, CommandFactory::instance().get(name));
        CommandFactory::instance().set(name,
                                       [&, index](const ContextPtr& context,
                                                  Properties&& props,
                                                  const CoreComponentPtr& base) {
                                           auto ptr = sCommandCreatorMap.at(index)(context, std::move(props), base);
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
    PropertyKey v[value] = {args...};

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
};

// Check that EXACTLY these components are dirty
template<class... Args>
::testing::AssertionResult CheckDirty(const RootContextPtr& root, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    ComponentPtr v[value] = {args...};
    for (auto key : v) {
        if (root->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing component " << key << ":" << key->toDebugString();
    }

    for (auto key : root->getDirty()) {
        auto it = std::find(v, v + value, key);
        if (it == v+value)
            return ::testing::AssertionFailure() << "extra component " << key << ":" << key->toDebugString();
    }

    std::size_t actual = root->getDirty().size();
    if (actual != value)
        return ::testing::AssertionFailure() << "wrong number of dirty components.  Expected " << value << " but got " << actual;

    root->clearDirty();
    return ::testing::AssertionSuccess();
};

// Check that AT LEAST all of this components are dirty.  More may be dirty; that's fine.
template<class... Args>
::testing::AssertionResult CheckDirtyAtLeast(const RootContextPtr& root, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    ComponentPtr v[value] = {args...};
    for (auto key : v) {
        if (root->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing component " << key << ":" << key->toDebugString();
    }

    root->clearDirty();
    return ::testing::AssertionSuccess();
};

template<class... Args>
::testing::AssertionResult CheckDirty(const GraphicPtr& graphic, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::size_t actual = graphic->getDirty().size();
    if (actual != value)
        return ::testing::AssertionFailure() << "wrong number of dirty graphic elements.  Expected " << value << " but got " << actual;

    GraphicElementPtr v[value] = {args...};
    for (auto key : v) {
        if (graphic->getDirty().count(key) != 1)
            return ::testing::AssertionFailure() << "missing graphic element " << key;
    }

    graphic->clearDirty();
    return ::testing::AssertionSuccess();
};

template<class... Args>
::testing::AssertionResult CheckDirty(const GraphicElementPtr& element, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    std::size_t actual = element->getDirtyProperties().size();
    GraphicPropertyKey v[value] = {args...};
    if (actual != value)
        return ::testing::AssertionFailure() << "expected number of dirty graphic properties=" << value
                << " actual=" << actual
                << " [" << join<GraphicPropertyKey, sGraphicPropertyBimap>(element->getDirtyProperties()) << "]";

    for (auto key : v) {
        if (element->getDirtyProperties().count(key) != 1)
            return ::testing::AssertionFailure() << "missing graphic key " << sGraphicPropertyBimap.at(key);
    }

    element->clearDirtyProperties();
    return ::testing::AssertionSuccess();
};

template<class... Args>
::testing::AssertionResult CheckState(const ComponentPtr& component, Args... args)
{
    static const std::size_t value = sizeof...(Args);

    // Construct an array of values for the expected state from the passed arguments
    std::vector<bool> values(kStatePropertyCount, false);
    StateProperty v[value] = {args...};
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

inline
::testing::AssertionResult IsEqual(Easing lhs, Easing rhs)
{
    for (int i = 0 ; i <= 10 ; i++) {
        if (lhs(i * 0.1) != rhs(i * 0.1))
            return ::testing::AssertionFailure();
    }

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

static std::map<std::string, std::function<std::string(const Component&)>> sPseudoProperties =
    {
        // Return the global bounding rectangle for this component.
        {"__global",  [](const Component& c) {
            return c.getGlobalBounds().toString();
        }},

        // Return the actual bounding rectangle for this component.  It has been clipped by all parents
        {"__actual",  [](const Component& c) {
            Rect r = c.getGlobalBounds();
            auto parent = c.getParent();
            while (parent) {
                r = r.intersect(
                    parent->getGlobalBounds());
                parent = parent->getParent();
            }
            return r.toString();
        }},

        // Return whether or not this component inherits state from the parent component
        {"__inherit", [](const Component& c) {
            return static_cast<const CoreComponent *>(&c)->getInheritParentState()
                   ? "true"
                   : "false";
        }},

        // Return the component scroll offset
        {"__scroll", [](const Component& c) {
            return static_cast<const CoreComponent *>(&c)->scrollPosition().toString();
        }},

        // Return "true" if the component has non-zero opacity and should be drawn.  It may
        // not actually be on the screen.
        {"__visible", [](const Component& c) {
            if (c.getCalculated(kPropertyOpacity).getDouble() <= 0.0 ||
                c.getCalculated(kPropertyDisplay).getInteger() != kDisplayNormal)
                return "false";

            for (auto parent = c.getParent() ; parent ; parent = parent->getParent()) {
                if (parent->getCalculated(kPropertyOpacity).getDouble() <= 0.0 ||
                    parent->getCalculated(kPropertyDisplay).getInteger() != kDisplayNormal)
                    return "false";
            }

            return "true";
        }},
    };

class HierarchyVisitor : public Visitor<Component> {
public:
    HierarchyVisitor(std::vector<std::string>&& args) : mIndent(0), mArgs(std::move(args)) {}

    void visit(const Component& component) override {
        std::string result = component.name() + component.getUniqueId();
        if (!component.getId().empty())
            result += "(" + component.getId() + ")";

        for (auto& m : mArgs) {
            if (sComponentPropertyBimap.has(m)) {
                PropertyKey key = static_cast<PropertyKey>(sComponentPropertyBimap.at(m));
                auto value = component.getCalculated(key);
                if (!value.empty())
                    result += std::string(" ") + m + ":" + component.getCalculated(key).toDebugString();
            } else {
                auto it = sPseudoProperties.find(m);
                if (it != sPseudoProperties.end())
                    result += std::string(" ") + m + ":" + it->second(component);
            }
        }
        LOG(LogLevel::DEBUG) << std::string(mIndent, ' ') << result;
    }

    void push() override { mIndent += 2; }

    void pop() override { mIndent -= 2; }

private:
    int mIndent;
    std::vector<std::string> mArgs;
};

/**
 * Convenience method for dumping the visual hierarchy.  Each component in the hierarchy will
 * always report type, unique id, and any user-assigned ID.  Pass additional string arguments
 * to specify properties you'd like to display.  In addition to the standard component properties,
 * you may also pass "pseudo" properties such as "__inherit".  Refer to the pseudo-property
 * list above for valid pseudo properties.
 *
 * For example, to check the actual drawing bounds of components clipped to their parents, try:
 *
 *    dumpHierarchy( component, "__actual", "_bounds", "__inherit" );
 *
 * @tparam Args
 * @param component
 * @param args
 */
inline void
dumpHierarchy(const ComponentPtr& component, std::initializer_list<std::string> args)
{
    HierarchyVisitor hv(args);
    component->accept(hv);
}

extern std::ostream& operator<<(std::ostream& os, const Point& point);
extern std::ostream& operator<<(std::ostream& os, const Transform2D& t);
extern std::ostream& operator<<(std::ostream& os, const Radii& r);
extern std::ostream& operator<<(std::ostream& os, const Object& object);

} // namespace apl

#endif //_APL_TEST_EVENT_LOOP_H
