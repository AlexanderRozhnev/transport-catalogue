#include "stat_reader.h"

#include <iomanip>
#include <cmath>

namespace transport_catalogue {

using namespace std;

StatQuery StatReader::ParseStatQuery(string_view line) {

    using detail::SplitByFirst;

    auto [type, name] = SplitByFirst(line, ' ');
    /* detect query type */
    if (type.compare("Bus"sv) == 0) {
        return {StatQueryType::Bus, move(string{name})};
    } else if (type.compare("Stop"sv) == 0) {
        return {StatQueryType::Stop, move(string{name})};
    }
    return {};
}

/* Bus X
    Вывести информацию об автобусном маршруте X в следующем формате:
    Bus X: R stops on route, U unique stops, L route length, C curvature
    Здесь:
    X — название маршрута. Оно совпадает с названием, переданным в запрос Bus.
    R — количество остановок в маршруте автобуса от stop1 до stop1 включительно.
    U — количество уникальных остановок, на которых останавливается автобус. 
        Одинаковыми считаются остановки, имеющие одинаковые названия.
    L — длина маршрута в метрах, вычисляется с использованием дорожного расстояния, а не географических координат.
    С — извилистость */
void StatReader::AnswerBus(string_view name) {
    const Bus* bus = catalogue_.FindBus(name);
    if (!bus) {
        output_ << "Bus " << name << ": not found" << endl;
        return;
    }
    const BusInfo info = catalogue_.GetBusInfo(name);
    output_ << "Bus "
            << info.name << ": "
            << info.stops_count << " stops on route, "
            << info.unique_stops_count << " unique stops, "
            << info.route_length << " route length, "
            << std::setprecision(6)
            << info.curvature << " curvature" << endl;
}

/* Stop X
    Вывести информацию об остановке X в следующем формате:
    Stop X: buses bus1 bus2 ... busN
    
    bus1 bus2 ... busN — список автобусов, проходящих через остановку. Дубли не допускаются, названия должны быть отсортированы в алфавитном порядке.
    Если остановка X не найдена, выведите Stop X: not found.
    Если остановка X существует в базе, но через неё не проходят автобусы, выведите Stop X: no buses. */
void StatReader::AnswerStop(string_view name) {
    const Stop* stop = catalogue_.FindStop(name);
    if (!stop) {
        output_ << "Stop " << name << ": not found" << endl;
        return;
    }
    
    const StopInfo info = catalogue_.GetStopInfo(name);
    if (info.empty()) {
        output_ << "Stop " << name << ": no buses" << endl;
        return;
    }

    output_ << "Stop " << name << ": buses";
    for (const auto& bus : info) {
        output_ << " " << bus;
    }
    output_ << endl;
}

StatReader::StatReader(istream& input, ostream& output, TransportCatalogue catalogue) : 
                                                    input_{input},
                                                    output_{output},
                                                    catalogue_{catalogue} {
    /* Get count of queries */
    int queries_count;
    input_ >> queries_count;
    string s;
    getline(input_, s);

    /* Get stat queries and respond */
    for (int i = 0; i < queries_count; ++i) {
        getline(input_, s);
        StatQuery q = ParseStatQuery(s);
        if (q.type == StatQueryType::Bus) {
            AnswerBus(q.name);
        } else if (q.type == StatQueryType::Stop) {
            AnswerStop(q.name);
        }
    }
}

} // namespace transport_catalogue