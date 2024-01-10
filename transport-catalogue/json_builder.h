#pragma once

#include <string>

#include "json.h"

namespace json {

class Builder {
    class BaseContext;
    class AfterStartDictContext;
    class AfterDictKeyContext;
    using AfterDictValueContext = AfterStartDictContext;
    class AfterStartArrayContext;
    using AfterArrayValueContext = AfterStartArrayContext;
    
    enum class State {
        EMPTY,
        COMPLETE,
        ARRAY,
        DICT_KEY,
        DICT_VALUE
    };

public:
    AfterStartDictContext StartDict();
    Builder& EndDict();
    AfterDictKeyContext Key(std::string key);
    AfterStartArrayContext StartArray();
    Builder& EndArray();
    Builder& Value(Node::Value value);
    json::Node Build();

private:
    Node root_;
    std::vector<const Node*> nodes_stack_;

    State state_ = State::EMPTY;
    std::string new_key_;

    void StartContainer(json::Node&& new_node, const std::string& error_message);
    void PreviousState();
};

class Builder::BaseContext {
public:
    BaseContext(Builder& builder) : builder_{builder} {}

protected:
    AfterStartDictContext StartDict();
    AfterDictKeyContext Key(std::string key);
    Builder& EndDict();
    AfterStartArrayContext StartArray();
    Builder& EndArray();

    Builder& builder_;
};


class Builder::AfterStartDictContext final : public Builder::BaseContext {
public:
    using BaseContext::Key;
    using BaseContext::EndDict;
};

class Builder::AfterDictKeyContext final : public Builder::BaseContext {
public:
    Builder::AfterDictValueContext Value(Node::Value value);
    using BaseContext::StartDict;
    using BaseContext::StartArray;
};

class Builder::AfterStartArrayContext final : public Builder::BaseContext {
public:
    Builder::AfterArrayValueContext Value(Node::Value value);
    using BaseContext::StartDict;
    using BaseContext::StartArray;
    using BaseContext::EndArray;
};

} // namespace json