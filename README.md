# Transport catalogue

[Русский язык](README.ru.md)

Keeps transport routes with stops and provide processing queries to find fasters route, calulate traveling time, distance, and routes map visualisation. Transport catalogue state could be saved and recovered with serialization and deserialization.

1. [Functionality](#functionality)
2. [Input data format](#input_data_format)
3. [Requests for building a catalogue](#make_requests)
     1. [Router settings](#router_settings)
     2. [Requests to add a stop](#add_stops)
     3. [Request to add a bus](#add_buses)
4. [Requests to the transport catalogue and the format of responses to them](#stat_requests)
     1. [Getting route information](#route_info)
     2. [Visualization of route map](#route_visualization)
     3. [Request to build a route from stop to stop](#routing)
5. [Building the program and requirements](#make)
6. [Running a program and redirecting I/O](#run)
     1. [I/O redirection](#std_redirection)
     2. [Creating a transport catalogue](#base_init)
     3. [Using a saved transport catalogue to process requests](#stat_requests_run)

<a id="functionality"></a>
## Functionality
The program accepts requests and issues responses via the standard input/output stream in the format of JSON objects.
The program operates in a two-stage mode, selected when starting the program.
- Stage `make_base` - Mode of building a transport catalogue. Requests are accepted for entering information into the transport catalogue, as well as settings necessary for serializing the state of the catalogue, setting up the router, and setting up image rendering.
- Stage `process_requests` - Mode of responses to requests to the transport catalogue.
Loads the saved transport catalogue state from a file. Provides information about stops, routes, visualizes transport routes, builds the shortest route between stops with travel time calculations.

JSON loading/unloading is implemented based on our own library.
The visualization is provided in SVG format. Implemented based on our own library.
Serialization/deserialization is implemented using the Google Protobuf library.

<a id="input_data_format"></a>
## Input format
The data comes from the standard stream in JSON object format. Requests can be divided into two main stages:
- requests to create a transport catalogue ("make requests").
- requests for information from the transport catalogue ("stat requests").
The top-level JSON structure looks like this:

Input `make_base`:
```json
{
"serialization_settings": { ... },
"routing_settings": { ... },
"render_settings": { ... },
   "base_requests": [ ... ],
   "stat_requests": [ ... ]
}
```

- `serialization_settings` - settings for uploading the catalogue state to a file
- `routing_settings` - route builder settings
- `render_settings` - image rendering settings
- `base_requests` - requests to build a database. Arrays with descriptions of routes and stops are provided as input.

Input `process_requests`:
```json
{
       "serialization_settings": { ... },
       "stat_requests": [ ... ]
}
```

- `serialization_settings` - settings for loading catalogue state from a file
- `stat_requests` - requests to the transport catalogue

<a id="make_requests"></a>
## Requests for building a catalogue
<a id="router_settings"></a>
### Router Settings

The _routing_settings_ section specifies the route builder settings.

<details>
   <summary>Example router_settings:</summary>

```json
"routing_settings": {
       "bus_wait_time": 6,
       "bus_velocity": 40
}
```
</details>

- `bus_wait_time` — waiting time for the bus at the stop, in minutes. Consider that whenever a person comes to a stop and whatever that stop is, he will wait for any bus exactly the specified number of minutes. Value is an integer from 1 to 1000.
- `bus_velocity` — bus speed, in km/h. It is believed that the speed of any bus is constant and exactly equal to the indicated number. The time spent at stops is not taken into account, nor is the time of acceleration and braking. Value is a real number from 1 to 1000.

<a id="add_stops"></a>
### Requests to add a stop

<details>
   <summary>Example of stop description:</summary>

```json
{
   "type": "Stop",
   "name": "Electrical networks",
   "latitude": 43.598701,
   "longitude": 39.730623,
   "road_distances": {
     "Dokuchaeva Street": 3000,
     "Liza Chaikina Street": 4300
   }
}
```
</details>

Stop description - dictionary with keys:
- `type` — a string equal to `"Stop"`. Means that the dictionary describes a stop;
- `name` — name of the stop;
- `latitude` and `longitude` - latitude and longitude of the stop - floating point numbers;
- `road_distances` - a dictionary that specifies the road distance from this stop to neighboring ones. Each key in this dictionary is the name of a neighboring stop, the value is an integer distance in meters.

#### Distance between stops
List of distances from this stop to its neighboring stops. Distances are specified in meters.
The distance between stops A and B can be either unequal or equal to the distance between stops B and A. In the first case, the distance between stops is indicated twice: in the forward and reverse directions. When the distances are the same, it is enough to set the distance from A to B, or the distance from B to A. You can also set the distance from the stop to itself - this happens if the bus turns around and arrives at the same stop.

<a id="add_buses"></a>
### Request to add a bus

<details>
   <summary>Example of bus route description:</summary>

```json
{
   "type": "Bus",
   "name": "14",
   "stops": [
     "Liza Chaikina Street"
     "Electric networks",
     "Dokuchaev Street"
     "Liza Chaikina Street"
   ],
   "is_roundtrip": true
}
```
</details>

Bus route description - dictionary with keys:
`type` — string _"Bus"_. Means that the dictionary describes a bus route;
`name` — route name;
`stops` - an array with the names of the stops through which the route passes. On a circular route, the name of the last stop duplicates the name of the first. For example: [_"stop1"_, _"stop2"_, _"stop3"_, _"stop1"_];
`is_roundtrip` is a value of type _bool_. _true_ if the route is circular.

<a id="stat_requests"></a>
## Queries to the transport catalogue and the format of responses to them

Requests are sent in the stat_requests array.
Each request is a dictionary with the required keys `id` and `type`.

Responses are sent to a JSON array of responses:
```json
[
   {response to first request},
   {response to second request},
   ...
   {response to last request}
]
```

In the output JSON array, each `stat_requests` request must have a response in the form of a dictionary with the required `request_id` key. The key value must be equal to the `id` of the corresponding request.

<a id="route_info"></a>
### Getting route information

<details>
   <summary>Request for route information:</summary>

```json
{
   "id": 12345678,
   "type": "Bus",
   "name": "14"
}
```
</details>

- `type` has the value _“Bus”_. It can be used to determine that this is a request for route information.
- `name` specifies the name of the route for which the application should display statistical information.

<details>
   <summary>Response to request for route information:</summary>

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

- `curvature` — the tortuosity of the route. It is equal to the ratio of the length of the road distance of the route to the length of the geographical distance. Number of type double;
- `request_id` - must be equal to the id of the corresponding Bus request. Integer;
- `route_length` — length of the road distance of the route in meters, integer;
- `stop_count` — number of stops on the route;
- `unique_stop_count` — the number of unique stops on the route.

<a id="route_visualization"></a>
### Visualization of the route map

The request to visualize the route map is transmitted in the _stat_request_ array:
```json
{
   "type": "Map",
   "id": 11111
}
```

Visualization is carried out in SVG format.
The response to this request is given in the form of a dictionary with the keys _request_id_ and _map_:
```json
{
   "map": "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<svg xmlns=\"http://www.w3.org/2000/svg\" version =\"1.1\"> ... </svg>",
   "request_id": 11111
}
```
The contents of the _map_ key can be viewed as SVG format.

<a id="routing"></a>
### Request to build a route from stop to stop
<details>
   <summary>Query example:</summary>

```json
{
       "type": "Route",
       "from": "Biryulyovo Zapadnoye",
       "to": "Universam",
       "id": 4
}
```
</details>

- `from` — stop where you want to start the route.
- `to` — stop where you want to end the route.

<details>
   <summary>Answer example:</summary>

```json
{
     "request_id": <request id>,
     "total_time": <total time>,
     "items": [
         <route elements>
     ]
}
```
</details>

- `total_time` — the total time in minutes that is required to complete the route, displayed as a real number.
- `items` — a list of route elements, each of which describes the passenger’s continuous activity that requires time. Route elements are of two types: `Wait` - wait the required number of minutes, `Bus` - drive `span_count` stops.

<a id="make"></a>
# Program assembly and requirements

The program is written using C++ standard 17.
The Google Protobuf library version 3.21.12 was used (file format _"proto 3"_).

The project is built using CMake.

1. Download and build Google Protobuf for your compiler version.
2. Create a folder to build the program
3. Open the console in this folder and enter in the console:
```
cmake <path to the CMakeLists.txt file> -DCMAKE_PREFIX_PATH= <path to the built Protobuf library> -DTESTING = OFF
```
4. Enter the command:
```
cmake --build .
```

After assembly, the executable file transport_catalogue (unix, mac os) or transport_catalogue.exe (windows) will appear in the assembly folder.

<a id="run"></a>
# Run the program and redirect I/O

The program runs in one of two modes:
- `make_base` - Mode for building a transport catalogue.
- `process_requests` - Mode for processing requests to the transport catalogue.

Start the program in the desired mode using the parameter:
```
transport_catalogue [make_base|process_requests]
```
_without redirecting I/O streams, I/O is carried out to/from the standard stream_

<a id="std_redirection"></a>
## I/O redirection:
To input and output to a file instead of the console, standard stream I/O redirection is used:

Loading the database:
```
./transport_catalogue make_base <make_base.json
```

Query Processing:
```
./transport_catalogue process_requests <process_requests.json >output.json
```

<a id="base_init"></a>
## Create a transport catalogue
To create a transport catalogue, you need to prepare a file with data on routes, stops and settings in the format described above.

Run the assembled program with the _make_base_ key:
```
./transport_catalogue make_base <make_base.json
```
The program will read the make_base.json file, create a transport catalogue and save its state in binary form in the program folder in the transport_catalogue.db file (the file name corresponds to the setting in "serialization_settings").
In the `process_requests` request processing mode, this file will be loaded by the transport catalogue without the cost of building the catalogue again.

<details>
   <summary>Example of a correct make_base.json file:</summary>

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
## Using a saved transport catalogue to process requests
Run the assembled program with the _process_requests_ key:
```
./transport_catalogue process_requests
```
for console I/O or redirect I/O to files:
```
./transport_catalogue process_requests <process_requests.json >output.json
```
The catalogue state will be restored from the binary file whose name is specified in the "serialization_settings" settings.
Requests from the console (or process_requests.json file) will be processed and sent to the console (or output.json file).

<details>
   <summary>Example of a correct process_requests.json file:</summary>

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
               "name": "Electrical networks"
           },
           {
               "id": 1964680131,
               "type": "Route",
               "from": "Marine Station",
               "to": "Parallel street"
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
   <summary>Example result.json output:</summary>

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