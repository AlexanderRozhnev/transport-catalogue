#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

class Node;

using Array = std::vector<Node>;
using Dict = std::map<std::string, Node>;

class ParsingError : public std::runtime_error {    // Ошибка при ошибках парсинга JSON
public:
    using runtime_error::runtime_error;
};

class Node final : 
        private std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict> {
public:    
    using variant::variant;
    using Value = variant;

    bool IsArray() const;       // Возвращает true, если в Node хранится Array
    bool IsDict() const;         // Возвращает true, если в Node хранится Dict
    bool IsInt() const;         // Возвращает true, если в Node хранится int
    bool IsDouble() const;      // Возвращает true, если в Node хранится int либо double.
    bool IsPureDouble() const;  // Возвращает true, если в Node хранится double.
    bool IsBool() const;        // Возвращает true, если в Node хранится bool
    bool IsString() const;      // Возвращает true, если в Node хранится string
    bool IsNull() const;        // Возвращает true, если в Node хранится nullptr_t

    const Value& GetValue() const;  // Возвращает значение Node, как variant

    const Array& AsArray() const;
    const Dict& AsDict() const;
    int AsInt() const;
    double AsDouble() const;    // Возвращает значение типа double, если внутри хранится double либо int.
    bool AsBool() const;
    const std::string& AsString() const;
    friend bool operator== (const Node& lhs, const Node& rhs);
    friend bool operator!= (const Node& lhs, const Node& rhs);
};

// Контекст вывода, хранит ссылку на поток вывода и текущий отсуп
struct PrintContext {
    std::ostream& out;
    int indent_step = 4;
    int indent = 0;

    void PrintIndent() const {
        for (int i = 0; i < indent; ++i) {
            out.put(' ');
        }
    }

    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented() const {
        return {out, indent_step, indent_step + indent};
    }
};

template <typename Numbers>
void PrintValue(const Numbers& value, const PrintContext& ctx) {  // для вывода double и int
    ctx.out << value;
}

void PrintValue(std::nullptr_t, const PrintContext& ctx);       // для вывода значений null
void PrintValue(const bool value, const PrintContext& ctx);     // для вывода значений bool
void PrintValue(const std::string& value, const PrintContext& ctx); // для вывода значений string
void PrintValue(const Array& value, const PrintContext& ctx);   // для вывода значений Array
void PrintValue(const Dict& value, const PrintContext& ctx);    // для вывода значений Dict

void PrintNode(const Node& node, const PrintContext& ctx);

class Document {
public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    friend bool operator== (const Document& lhs, const Document& rhs);
    friend bool operator!= (const Document &lhs, const Document &rhs);

private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

}  // namespace json