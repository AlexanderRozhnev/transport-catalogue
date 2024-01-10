#include "json.h"

using namespace std;

namespace json {

namespace {

Node LoadNode(istream& input);

/* loading value, that could be: double, int */
Node LoadNumber(istream& input) {

    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
           throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                int value_int = std::stoi(parsed_num);
                return Node{value_int};
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        double value_double = std::stod(parsed_num);
        return Node{value_double};
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

/* loading litheral value, that could be: bool (true, false), null */
Node LoadLitheral(istream& input) {
    string word;
    auto it = istreambuf_iterator<char>(input);
    auto end = istreambuf_iterator<char>();
    for (int i = 0; (i < 4) && (it != end); ++i, ++it) {
        word += *it;
    }

    if (word == "true") {
        return Node{true};
    } else if (word == "null") {
        return Node{nullptr};
    } else if (word == "fals" && (it != end)) {
        word += *it;
        ++it;
        if (word == "false") {
            return Node{false};
        }
    }
    
    if (it == end) {
        throw ParsingError("Failed to read number from stream"s);
    }
    throw ParsingError("Failed to convert "s + word + " to litheral"s);
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
Node LoadString(istream& input) {
    using namespace std::literals;
    
    auto it = istreambuf_iterator<char>(input);
    auto end = istreambuf_iterator<char>();
    string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        } else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        } else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }

    return Node(move(s));
}

/* Loads an array formed by "[ ]" */
Node LoadArray(istream& input) {
    Array result;
    char c;
    while (input >> c && c != ']') {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
    }
    if (c != ']') throw ParsingError("Array parsing error");

    return Node(move(result));
}

/* Loads a dictionary formed by "{ }" */
Node LoadDict(istream& input) {
    Dict result;
    char c;
    while (input >> c && c != '}') {
        if (c == ',') {
            input >> c;
        }

        string key = LoadString(input).AsString();
        input >> c;
        result.insert({move(key), LoadNode(input)});
    }
    if (c != '}') throw ParsingError("Dictionary parsing error");

    return Node(move(result));
}

/* Loads any type of node */
Node LoadNode(istream& input) {
    char c;
    /* Skip leading spaces */    
    while (input.get(c)) {
        if (!(c == ' ' || c == '\n' || c == '\r' || c =='\t' || c == '\f' || c == '\v' )) {
            break;
        }
    }
    if (input.eof()) {
        throw ParsingError("Unexpected end of file."s);
    }

    /* Predict node type */
    switch (c) {
        case '[' :
            return LoadArray(input);
            break;

        case '{' :
            return LoadDict(input);
            break;

        case '"' :
            return LoadString(input);
            break;
        
        case 't' :
        case 'f' :
        case 'n' :
            input.putback(c);
            return LoadLitheral(input);
            break;          

        default :
            input.putback(c);
            return LoadNumber(input);
    }
}

}  // namespace

bool Node::IsInt() const { return holds_alternative<int>(*this); }

bool Node::IsDouble() const { return holds_alternative<double>(*this) 
                                     || holds_alternative<int>(*this); }

bool Node::IsPureDouble() const { return holds_alternative<double>(*this); }

bool Node::IsBool() const { return holds_alternative<bool>(*this); }

bool Node::IsString() const { return holds_alternative<string>(*this); }

bool Node::IsNull() const { return holds_alternative<nullptr_t>(*this); }

bool Node::IsArray() const { return holds_alternative<Array>(*this); }

bool Node::IsDict() const { return holds_alternative<Dict>(*this); }

const Node::Value& Node::GetValue() const { return static_cast<const Value&>(*this); }

const Array& Node::AsArray() const {
    if (const auto* result = get_if<Array>(this)) {
        return *result;
    }
    throw std::logic_error("wrong type");
}

const Dict& Node::AsDict() const {
    if (const auto* result = get_if<Dict>(this)) {
        return *result;
    }
    throw std::logic_error("wrong type");
}

int Node::AsInt() const {
    if (const auto* result = get_if<int>(this)) {
        return *result;
    }
    throw std::logic_error("wrong type");
}

double Node::AsDouble() const {
    if (const auto* result_double = get_if<double>(this)) {
        return *result_double;
    } else if (const auto* result_int = get_if<int>(this)) {
        return static_cast<double>(*result_int);
    }
    throw std::logic_error("wrong type");
}

bool Node::AsBool() const {
    if (const auto* result = get_if<bool>(this)) {
        return *result;
    }
    throw std::logic_error("wrong type");
}

const string& Node::AsString() const {
    if (const auto* result = get_if<string>(this)) {
        return *result;
    }
    throw std::logic_error("wrong type");
}

bool operator==(const Node &lhs, const Node &rhs) {
    return static_cast<Node::Value>(lhs) == static_cast<Node::Value>(rhs);
}

bool operator!=(const Node &lhs, const Node &rhs) {
    return static_cast<Node::Value>(lhs) != static_cast<Node::Value>(rhs);
}

Document::Document(Node root) : root_(move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator== (const Document &lhs, const Document &rhs) {
    return lhs.GetRoot() == rhs.GetRoot();
}

bool operator!= (const Document &lhs, const Document &rhs) {
    return lhs.GetRoot() != rhs.GetRoot();
}

Document Load(istream &input)
{
    return Document{LoadNode(input)};
}

void PrintValue(std::nullptr_t, const PrintContext& ctx) {
    ctx.out << "null"sv;
}

void PrintValue(const bool value, const PrintContext& ctx) {
    ctx.out << ((value) ? "true"s : "false"s);
}

void PrintValue(const string& value, const PrintContext& ctx) {
    string esc_string;

    for (auto c : value) {
        if (c == '\"') esc_string += "\\\"";
        else if (c == '\r') esc_string += "\\r";
        else if (c == '\n') esc_string += "\\n";
        else if (c == '\\') esc_string += "\\\\";
        else esc_string += c;
    }
    ctx.out << '\"' << esc_string << '\"';
}

/* Print Array */
void PrintValue(const Array& values, const PrintContext& ctx) {
    ctx.out << '[';
    if (!values.empty()) {
        ctx.out << endl;
        PrintContext ctx_indended = ctx.Indented();
        ctx_indended.PrintIndent();
        auto it = values.begin();
        PrintNode(*it, ctx_indended);
        ++it;
        for (; it != values.end(); ++it) {
            ctx.out << ", ";
            PrintNode(*it, ctx_indended);
        }
        ctx.out << endl;
        ctx.PrintIndent();
    }
    ctx.out << ']';
}

/* Print Dictionary */
void PrintValue(const Dict& values, const PrintContext& ctx) {
    ctx.out << '{';
    if (!values.empty()) {
        ctx.out << endl;
        PrintContext ctx_indended = ctx.Indented();
        ctx_indended.PrintIndent();
        auto it = values.begin();
        PrintValue(it->first, ctx_indended);
        ctx.out << ": ";
        PrintNode(it->second, ctx_indended);
        ++it;
        for (; it != values.end(); ++it) {
            ctx.out << "," << endl;
            ctx_indended.PrintIndent();
            PrintValue(it->first, ctx_indended);
            ctx.out << ": ";
            PrintNode(it->second, ctx_indended);
        }
        ctx.out << endl;
        ctx.PrintIndent();
    }
    ctx.out << '}';
}

/* Print Node of any type */
void PrintNode(const Node& node, const PrintContext& ctx) {
    std::visit([&ctx](const auto& value){ PrintValue(value, ctx); }, node.GetValue());
}

/* Print Document */
void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), PrintContext{output, 2, 0});
}

}  // namespace json