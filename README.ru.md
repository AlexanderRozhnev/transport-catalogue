# Транспортный каталог

Информационная система о транспортных маршрутах с остановками. Обрабатывает запросы на создание базы, построение кратчайшего маршрута, строит визуализацию маршрутов. Запросы и ответы посупают в формате JSON. Состояние транспортного справочника сохраняется и восстановливается с помощью сериализации и десериализации.

1. [Функционал](#functionality)
2. [Формат входных данных](#input_data_format)
3. [Запросы построения справочника](#make_requests)
    1. [Настройки построителя маршрутов](#router_settings)
    2. [Запросы на добавление остановки](#add_stops)
    3. [Запрос на добавление автобусного маршрута](#add_buses)
4. [Запросы к транспортному справочнику и формат ответов на них](#stat_requests)
    1. [Получение информации о маршруте](#route_info)
    2. [Визуализация карты маршрутов](#route_visualization)
    3. [Запрос на построение маршрута от остановки к остановке](#routing)
5. [Сборка программы и требования](#make)
6. [Запуск программы и перенаправление ввода-вывода](#run)
    1. [Перенаправление ввода-вывода](#std_redirection)
    2. [Создание транспортного каталога](#base_init)
    3. [Использование сохранённого транспортного каталога для обработки запросов](#stat_requests_run)

<a id="functionality"></a>
## Функционал
Программа принимает запросы и выдаёт ответы через из стандартный поток ввода-вывода в формате JSON-объектов.
Программа работает в двухстадийном режиме, выбираемом при запуске программы. 
- Стадия `make_base` - Режим построения транспортного справочника. Принимаются запросы на внесение информации в транспортный справочник, а также настройки необходимые для сериализации состояния справочника, настройки построителя маршрутов, настройки рендера изображений.
- Стадия `process_requests` - Режим ответов на хапросы к транспортному справочнику.
Загружает сохраненное состояние транспортного справочника из файла. Предоставлячет информацию об остановках, маршрутах, визуализирует транспортные маршруты, строит кратчайший маршрут между остановками с расчётом времени в пути.

Загрузка/выгрузка JSON реализованы на основе сосбственной библиотеки.
Визуализация предоставляется в формате SVG. Реализовано на основе сосбственной библиотеки.
Сериализация/десериализация реализована с помощью библиотеки Google Protobuf.

<a id="input_data_format"></a>
## Формат входных данных
Данные поступают из стандартного потока в формате JSON-объекта. Запросы можно разделить на две основные категории:
- запросы на создание транспортнго справочника ("make requests").
- запросы информации из транспортного справочника ("stat requests").
Верхнеуровневая структура JSON выглядит так:

Ввод `make_base`:
```json
{
	"serialization_settings": { ...	},
	"routing_settings": { ... },
	"render_settings": { ... },
  	"base_requests": [ ... ],
  	"stat_requests": [ ... ]
}
```

- `serialization_settings` - настройки для выгрузки состояния справочника в файл
- `routing_settings` - настройки построителя маршрутов
- `render_settings` - настройки рендера изображений
- `base_requests` - запросы на построение базы. На вход подаются массивы с описанием маршрутов и остановок.

Ввод `process_requests`:
```json
{
      "serialization_settings": { ... },
      "stat_requests": [ ... ]
}
```

- `serialization_settings` - настройки для загрузки состояния справочника из файла
- `stat_requests` - запросы к транспортному справочнику

<a id="make_requests"></a>
## Запросы построения справочника
<a id="router_settings"></a>
### Настройки построителя маршрутов

Секция _routing_settings_ задаёт настройки построителя маршрутов.

<details>
  <summary>Пример router_settings:</summary>

```json
"routing_settings": {
      "bus_wait_time": 6,
      "bus_velocity": 40
} 
```
</details>

- `bus_wait_time` — время ожидания автобуса на остановке, в минутах. Считайте, что когда бы человек ни пришёл на остановку и какой бы ни была эта остановка, он будет ждать любой автобус в точности указанное количество минут. Значение — целое число от 1 до 1000.
- `bus_velocity` — скорость автобуса, в км/ч. Считается, что скорость любого автобуса постоянна и в точности равна указанному числу. Время стоянки на остановках не учитывается, время разгона и торможения тоже. Значение — вещественное число от 1 до 1000.

<a id="add_stops"></a>
### Запросы на добавление остановки

<details>
  <summary>Пример описания остановки:</summary>

```json
{
  "type": "Stop",
  "name": "Электросети",
  "latitude": 43.598701,
  "longitude": 39.730623,
  "road_distances": {
    "Улица Докучаева": 3000,
    "Улица Лизы Чайкиной": 4300
  }
}
```
</details>

Описание остановки — словарь с ключами:
- `type` — строка, равная `"Stop"`. Означает, что словарь описывает остановку;
- `name` — название остановки;
- `latitude` и `longitude` — широта и долгота остановки — числа с плавающей запятой;
- `road_distances` — словарь, задающий дорожное расстояние от этой остановки до соседних. Каждый ключ в этом словаре — название соседней остановки, значение — целочисленное расстояние в метрах.

#### Расстояние между остановками
Список расстояний от этой остановки до соседних с ней остановок. Расстояния задаются в метрах.
Расстояние между остановками A и B может быть как не равно, так и равно расстоянию между остановками B и A. В первом случае расстояние между остановками указывается дважды: в прямом и в обратном направлении. Когда расстояния одинаковы достаточно задать расстояние от A до B, либо расстояние от B до A. Также разрешается задать расстояние от остановки до самой себя — так бывает, если автобус разворачивается и приезжает на ту же остановку.

<a id="add_buses"></a>
### Запрос на добавление автобусного маршрута

<details>
  <summary>Пример описания автобусного маршрута:</summary>

```json
{
  "type": "Bus",
  "name": "14",
  "stops": [
    "Улица Лизы Чайкиной",
    "Электросети",
    "Улица Докучаева",
    "Улица Лизы Чайкиной"
  ],
  "is_roundtrip": true
}
```
</details>

Описание автобусного маршрута — словарь с ключами:
`type` — строка _"Bus"_. Означает, что словарь описывает автобусный маршрут;
`name` — название маршрута;
`stops` — массив с названиями остановок, через которые проходит маршрут. У кольцевого маршрута название последней остановки дублирует название первой. Например: [_"stop1"_, _"stop2"_, _"stop3"_, _"stop1"_];
`is_roundtrip` — значение типа _bool_. _true_, если маршрут кольцевой.

<a id="stat_requests"></a>
## Запросы к транспортному справочнику и формат ответов на них

Запросы передаются в массиве stat_requests.
Каждый запрос — словарь с обязательными ключами `id` и `type`.

Ответы передаются в JSON-массив ответов:
```json
[
  { ответ на первый запрос },
  { ответ на второй запрос },
  ...
  { ответ на последний запрос }
]
```

В выходном JSON-массиве на каждый запрос `stat_requests` должен быть ответ в виде словаря с обязательным ключом `request_id`. Значение ключа должно быть равно `id` соответствующего запроса.

<a id="route_info"></a>
### Получение информации о маршруте

<details>
  <summary>Запрос на получение информации о маршруте:</summary>

```json
{
  "id": 12345678,
  "type": "Bus",
  "name": "14"
}
```
</details>

- `type` имеет значение _“Bus”_. По нему можно определить, что это запрос на получение информации о маршруте.
- `name` задаёт название маршрута, для которого приложение должно вывести статистическую информацию.

<details>
  <summary>Ответ на запрос об информации о маршруте:</summary>

```json
{
  "curvature": 2.18604,
  "request_id": 12345678,
  "route_length": 9300,
  "stop_count": 4,
  "unique_stop_count": 3
}
```
</details>

- `curvature` — извилистость маршрута. Она равна отношению длины дорожного расстояния маршрута к длине географического расстояния. Число типа double;
- `request_id` — должен быть равен id соответствующего запроса Bus. Целое число;
- `route_length` — длина дорожного расстояния маршрута в метрах, целое число;
- `stop_count` — количество остановок на маршруте;
- `unique_stop_count` — количество уникальных остановок на маршруте.

<a id="route_visualization"></a>
### Визуализация карты маршрутов

Запрос на визуализацию карты маршрутов передаётся в массиве _stat_request_:
```json
{
  "type": "Map",
  "id": 11111
}
```

Визуализация осуществляется в формате SVG.
Ответ на этот запрос отдаётся в виде словаря с ключами _request_id_ и _map_:
```json
{
  "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"> ... </svg>",
  "request_id": 11111
}
```
Содержимое по ключу _map_ может быть просмотрено, как формат SVG.

<a id="routing"></a>
### Запрос на построение маршрута от остановки к остановке
<details>
  <summary>Пример запроса:</summary>

```json
{
      "type": "Route",
      "from": "Biryulyovo Zapadnoye",
      "to": "Universam",
      "id": 4
}
```
</details>

- `from` — остановка, где нужно начать маршрут.
- `to` — остановка, где нужно закончить маршрут.

<details>
  <summary>Пример ответа:</summary>

```json
{
    "request_id": <id запроса>,
    "total_time": <суммарное время>,
    "items": [
        <элементы маршрута>
    ]
}
```
</details>

- `total_time` — суммарное время в минутах, которое требуется для прохождения маршрута, выведенное в виде вещественного числа.
- `items` — список элементов маршрута, каждый из которых описывает непрерывную активность пассажира, требующую временных затрат. Элементы маршрута бывают двух типов: `Wait` — подождать нужное количество минут, `Bus` — проехать `span_count` остановок.

<a id="make"></a>
# Сборка программы и требования

Сборка проекта осуществляеся с помощью CMake.

1. Скачайте и соберите Google Protobuf под вашу версию компилятора.
2. Создайте папку для сборки программы
3. Откройте консоль в данной папке и введите в консоли: 
```
cmake <путь к файлу CMakeLists.txt> -DCMAKE_PREFIX_PATH= <путь к собранной библиотеке Protobuf> -DTESTING = OFF
```
4. Введите команду : 
```
cmake --build .
```

После сборки в папке сборки появится исполняемый файл transport_catalogue (unix, mac os) или transport_catalogue.exe (windows).

<a id="run"></a>
# Запуск программы и перенаправление ввода-вывода

Программа запускается в одном из двух режимов: 
- `make_base` - Режим построения транспортного справочника.
- `process_requests` - Режим обработки запросов к транспортному справочнику.

Запуск программы в нужном режиме с помощью параметра:
```
transport_catalogue [make_base|process_requests]
```
_без перенаправления потоков ввода/вывода ввод-вывод осуществляется в/из стандартного потока_

<a id="std_redirection"></a>
## Перенаправление ввода-вывода:
Для ввода и вывода в файл вместо консоли используется перенаправление ввода/вывода стандартных потоков:

Загрузка базы:
```
./transport_catalogue make_base <make_base.json
```

Обработка запросов:
```
./transport_catalogue process_requests <process_requests.json >output.json
```

<a id="base_init"></a>
## Создание транспортного каталога
Для создания транспортного каталога необходимо подгодотовить файл с данными о маршрутах, остановках и настройками, в формате описанном выше.

Запустите собранную программу с ключом _make_base_:
```
./transport_catalogue make_base <make_base.json
```
Программа прочитает файл make_base.json, создаст транспортный каталог и сохранит его состояние в двоичном виде в папке с программой в файле transport_catalogue.db (название файла соответствует настройке в "serialization_settings").
В режиме обработки запросов `process_requests` этот файл будет загружен транспортным справочником без затрат на построение српвочника заново.

<details>
  <summary>Пример корректного файла make_base.json:</summary>

```json 
  {
      "serialization_settings": {
          "file": "transport_catalogue.db"
      },
      "routing_settings": {
          "bus_wait_time": 2,
          "bus_velocity": 30
      },
      "render_settings": {
          "width": 1200,
          "height": 500,
          "padding": 50,
          "stop_radius": 5,
          "line_width": 14,
          "bus_label_font_size": 20,
          "bus_label_offset": [
              7,
              15
          ],
          "stop_label_font_size": 18,
          "stop_label_offset": [
              7,
              -3
          ],
          "underlayer_color": [
              255,
              255,
              255,
              0.85
          ],
          "underlayer_width": 3,
          "color_palette": [
              "green",
              [
                  255,
                  160,
                  0
              ],
              "red"
          ]
      },
      "base_requests": [
          {
              "type": "Bus",
              "name": "14",
              "stops": [
                  "Улица Лизы Чайкиной",
                  "Электросети",
                  "Ривьерский мост",
                  "Гостиница Сочи",
                  "Кубанская улица",
                  "По требованию",
                  "Улица Докучаева",
                  "Улица Лизы Чайкиной"
              ],
              "is_roundtrip": true
          },
          {
              "type": "Bus",
              "name": "24",
              "stops": [
                  "Улица Докучаева",
                  "Параллельная улица",
                  "Электросети",
                  "Санаторий Родина"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Bus",
              "name": "114",
              "stops": [
                  "Морской вокзал",
                  "Ривьерский мост"
              ],
              "is_roundtrip": false
          },
          {
              "type": "Stop",
              "name": "Улица Лизы Чайкиной",
              "latitude": 43.590317,
              "longitude": 39.746833,
              "road_distances": {
                  "Электросети": 4300,
                  "Улица Докучаева": 2000
              }
          },
          {
              "type": "Stop",
              "name": "Морской вокзал",
              "latitude": 43.581969,
              "longitude": 39.719848,
              "road_distances": {
                  "Ривьерский мост": 850
              }
          },
          {
              "type": "Stop",
              "name": "Электросети",
              "latitude": 43.598701,
              "longitude": 39.730623,
              "road_distances": {
                  "Санаторий Родина": 4500,
                  "Параллельная улица": 1200,
                  "Ривьерский мост": 1900
              }
          },
          {
              "type": "Stop",
              "name": "Ривьерский мост",
              "latitude": 43.587795,
              "longitude": 39.716901,
              "road_distances": {
                  "Морской вокзал": 850,
                  "Гостиница Сочи": 1740
              }
          },
          {
              "type": "Stop",
              "name": "Гостиница Сочи",
              "latitude": 43.578079,
              "longitude": 39.728068,
              "road_distances": {
                  "Кубанская улица": 320
              }
          },
          {
              "type": "Stop",
              "name": "Кубанская улица",
              "latitude": 43.578509,
              "longitude": 39.730959,
              "road_distances": {
                  "По требованию": 370
              }
          },
          {
              "type": "Stop",
              "name": "По требованию",
              "latitude": 43.579285,
              "longitude": 39.733742,
              "road_distances": {
                  "Улица Докучаева": 600
              }
          },
          {
              "type": "Stop",
              "name": "Улица Докучаева",
              "latitude": 43.585586,
              "longitude": 39.733879,
              "road_distances": {
                  "Параллельная улица": 1100
              }
          },
          {
              "type": "Stop",
              "name": "Параллельная улица",
              "latitude": 43.590041,
              "longitude": 39.732886,
              "road_distances": {}
          },
          {
              "type": "Stop",
              "name": "Санаторий Родина",
              "latitude": 43.601202,
              "longitude": 39.715498,
              "road_distances": {}
          }
      ]
  }  
```
</details>

<a id="stat_requests_run"></a>
## Использование сохранённого транспортного каталога для обработки запросов
Запустите собранную программу с ключом _process_requests_:
```
./transport_catalogue process_requests
```
для ввода-вывода в консоль или перенаправьте ввод-вывод в файлы:
```
./transport_catalogue process_requests <process_requests.json >output.json
```
Состояние справочника будет восстановлено из двоичного файла, название которго уккзано в настройках "serialization_settings".
Запросы с консоли (или файла process_requests.json) будут обработаны и направлены в консоль (или файл output.json).

<details>
  <summary>Пример корректного файла process_requests.json:</summary>

```json 
  {
      "serialization_settings": {
          "file": "transport_catalogue.db"
      },
      "stat_requests": [
          {
              "id": 218563507,
              "type": "Bus",
              "name": "14"
          },
          {
              "id": 508658276,
              "type": "Stop",
              "name": "Электросети"
          },
          {
              "id": 1964680131,
              "type": "Route",
              "from": "Морской вокзал",
              "to": "Параллельная улица"
          },
          {
              "id": 1359372752,
              "type": "Map"
          }
      ]
  }
```
</details>

<details>
  <summary>Пример вывода result.json:</summary>

```json
[{
  "curvature": 1.60481,
  "request_id": 218563507,
  "route_length": 11230,
  "stop_count": 8,
  "unique_stop_count": 7
}, {
  "buses": ["14", "24"],
  "request_id": 508658276
}, {
  "items": [{
    "stop_name": "Морской вокзал",
    "time": 2,
    "type": "Wait"
  }, {
    "bus": "114",
    "span_count": 1,
    "time": 1.7,
    "type": "Bus"
  }, {
    "stop_name": "Ривьерский мост",
    "time": 2,
    "type": "Wait"
  }, {
    "bus": "14",
    "span_count": 4,
    "time": 6.06,
    "type": "Bus"
  }, {
    "stop_name": "Улица Докучаева",
    "time": 2,
    "type": "Wait"
  }, {
    "bus": "24",
    "span_count": 1,
    "time": 2.2,
    "type": "Bus"
  }],
  "request_id": 1964680131,
  "total_time": 15.96
}, {
  "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n  <polyline points=\"125.25,382.708 74.2702,281.925 125.25,382.708\" fill=\"none\" stroke=\"green\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"592.058,238.297 311.644,93.2643 74.2702,281.925 267.446,450 317.457,442.562 365.599,429.138 367.969,320.138 592.058,238.297\" fill=\"none\" stroke=\"rgb(255,160,0)\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <polyline points=\"367.969,320.138 350.791,243.072 311.644,93.2643 50,50 311.644,93.2643 350.791,243.072 367.969,320.138\" fill=\"none\" stroke=\"red\" stroke-width=\"14\" stroke-linecap=\"round\" stroke-linejoin=\"round\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"green\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"green\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">114</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">14</text>\n  <text fill=\"rgb(255,160,0)\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">14</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <text fill=\"red\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"50\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <text fill=\"red\" x=\"50\" y=\"50\" dx=\"7\" dy=\"15\" font-size=\"20\" font-family=\"Verdana\" font-weight=\"bold\">24</text>\n  <circle cx=\"267.446\" cy=\"450\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"317.457\" cy=\"442.562\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"125.25\" cy=\"382.708\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"350.791\" cy=\"243.072\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"365.599\" cy=\"429.138\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"74.2702\" cy=\"281.925\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"50\" cy=\"50\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"367.969\" cy=\"320.138\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"592.058\" cy=\"238.297\" r=\"5\" fill=\"white\"/>\n  <circle cx=\"311.644\" cy=\"93.2643\" r=\"5\" fill=\"white\"/>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"267.446\" y=\"450\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Гостиница Сочи</text>\n  <text fill=\"black\" x=\"267.446\" y=\"450\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Гостиница Сочи</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"317.457\" y=\"442.562\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Кубанская улица</text>\n  <text fill=\"black\" x=\"317.457\" y=\"442.562\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Кубанская улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"black\" x=\"125.25\" y=\"382.708\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Морской вокзал</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"350.791\" y=\"243.072\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Параллельная улица</text>\n  <text fill=\"black\" x=\"350.791\" y=\"243.072\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Параллельная улица</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"365.599\" y=\"429.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">По требованию</text>\n  <text fill=\"black\" x=\"365.599\" y=\"429.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">По требованию</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Ривьерский мост</text>\n  <text fill=\"black\" x=\"74.2702\" y=\"281.925\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Ривьерский мост</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"50\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Санаторий Родина</text>\n  <text fill=\"black\" x=\"50\" y=\"50\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Санаторий Родина</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Докучаева</text>\n  <text fill=\"black\" x=\"367.969\" y=\"320.138\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Докучаева</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Лизы Чайкиной</text>\n  <text fill=\"black\" x=\"592.058\" y=\"238.297\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Улица Лизы Чайкиной</text>\n  <text fill=\"rgba(255,255,255,0.85)\" stroke=\"rgba(255,255,255,0.85)\" stroke-width=\"3\" stroke-linecap=\"round\" stroke-linejoin=\"round\" x=\"311.644\" y=\"93.2643\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Электросети</text>\n  <text fill=\"black\" x=\"311.644\" y=\"93.2643\" dx=\"7\" dy=\"-3\" font-size=\"18\" font-family=\"Verdana\">Электросети</text>\n</svg>",
  "request_id": 1359372752
}]
```
</details>
