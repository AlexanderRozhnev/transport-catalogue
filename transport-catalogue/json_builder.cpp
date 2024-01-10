#include "json_builder.h"

#include <utility>

using namespace std;

namespace json {

Builder::AfterStartDictContext Builder::StartDict() {
    StartContainer(json::Dict{}, "Unexpected StartDict."s);
    state_ = State::DICT_KEY;
    return {*this};
}

Builder& Builder::EndDict() {
    if (state_ != State::DICT_KEY) {
        throw logic_error("Unexpected EndDict."s);
    }

    PreviousState();
    return *this;
}

Builder::AfterDictKeyContext Builder::Key(std::string key) {
    if (state_ != State::DICT_KEY) {
        throw logic_error("Unexpected Key before StartDict."s);
    }

    new_key_ = move(key);
    state_ = State::DICT_VALUE;

    return {*this};
}

Builder::AfterStartArrayContext Builder::StartArray() {
    StartContainer(json::Array{}, "Unexpected StartArray."s);
    state_ = State::ARRAY;
    return {*this};
}

Builder& Builder::EndArray() {
    if (state_ != State::ARRAY) {
        throw logic_error("Unexpected EndArray."s);
    }

    PreviousState();
    return *this;
}

Builder& Builder::Value(Node::Value value) {
    json::Node new_value = std::visit([](const auto&& v){ return json::Node{v}; }, std::move(value));

    switch (state_) {
        case State::EMPTY :
            root_ = move(new_value);
            nodes_stack_.emplace_back(&root_);
            state_ = State::COMPLETE;
            break;
        
        case State::ARRAY :
            const_cast<json::Array&>(nodes_stack_.back()->AsArray()).emplace_back(new_value);
            break;

        case State::DICT_VALUE :
            const_cast<json::Dict&>(nodes_stack_.back()->AsDict()).insert({new_key_, new_value});
            state_ = State::DICT_KEY;
            break;

        default :
            throw logic_error("Unexpected Value."s);
    }
    return *this;
}

json::Node Builder::Build() {
    if (state_ != State::COMPLETE) {
        throw logic_error("Unexpected Build."s);
    }

    return root_;
}

void Builder::StartContainer(json::Node&& new_node, const string& error_message) {
    switch (state_) {
        case State::EMPTY :
            root_ = move(new_node);
            nodes_stack_.emplace_back(&root_);
            break;

        case State::ARRAY :
            const_cast<json::Array&>(nodes_stack_.back()->AsArray()).emplace_back(move(new_node));
            nodes_stack_.push_back(&(nodes_stack_.back()->AsArray().back()));
            break;

        case State::DICT_VALUE :
            {
            const auto [it, success] = const_cast<json::Dict&>(nodes_stack_.back()->AsDict()).insert({new_key_, move(new_node)});
            nodes_stack_.push_back(&(it->second));
            }
            break;
        
        default :
            throw logic_error(error_message);
            break;
    }
}

void Builder::PreviousState()
{
    nodes_stack_.pop_back();

    if (nodes_stack_.empty()) {
        state_ = State::COMPLETE;
    } else if (nodes_stack_.back()->IsArray()) {
        state_ = State::ARRAY;
    } else if (nodes_stack_.back()->IsDict()) {
        state_ = State::DICT_KEY;
    } else {
        throw logic_error("Broken Node structure."s);
    }
}

Builder::AfterStartDictContext Builder::BaseContext::StartDict() {
    return Builder::AfterStartDictContext{builder_.StartDict()};
}

Builder::AfterDictKeyContext Builder::BaseContext::Key(std::string key) {
    return Builder::AfterDictKeyContext{builder_.Key(key)};
}

Builder& Builder::BaseContext::EndDict() {
    return builder_.EndDict();
}

Builder::AfterStartArrayContext Builder::BaseContext::StartArray() {
    return builder_.StartArray();
}

Builder& Builder::BaseContext::EndArray() {
    return builder_.EndArray();
}

Builder::AfterDictValueContext Builder::AfterDictKeyContext::Value(Node::Value value) {
    builder_.Value(move(value));
    return Builder::AfterDictValueContext{builder_};
}

Builder::AfterArrayValueContext Builder::AfterStartArrayContext::Value(Node::Value value) {
    builder_.Value(move(value));
    return Builder::AfterArrayValueContext{builder_};
}

} // namespace json