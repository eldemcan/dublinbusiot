define({ "api": [  {    "type": "get",    "url": "/bus/:stopNo/:filter?/:limit?",    "title": "Get bus information",    "name": "GetDublinBus",    "group": "Public_Transport",    "parameter": {      "fields": {        "Parameter": [          {            "group": "Parameter",            "type": "Number",            "optional": false,            "field": "stopId",            "description": "<p>Bus stop id i.e  4456</p>"          },          {            "group": "Parameter",            "type": "Number",            "optional": true,            "field": "filter",            "defaultValue": "54A, 27, 65",            "description": "<p>Bus numbers that you would like to filter</p>"          },          {            "group": "Parameter",            "type": "Number",            "optional": true,            "field": "limit",            "defaultValue": "3",            "description": "<p>Number of stop information to return</p>"          }        ]      }    },    "success": {      "examples": [        {          "title": "Success-Response:",          "content": "HTTP/1.1 200 OK\n{\n  \"route\": \"27\",\n  \"dueTime\": \"21\"\n}",          "type": "json"        }      ]    },    "version": "0.0.0",    "filename": "./app.js",    "groupTitle": "Public_Transport"  },  {    "type": "get",    "url": "/luas/:stopId/:limit*?",    "title": "Get luas information",    "name": "GetLuas",    "group": "Public_Transport",    "parameter": {      "fields": {        "Parameter": [          {            "group": "Parameter",            "type": "String",            "optional": false,            "field": "stopId",            "description": "<p>Luas stop id i.e Tallaght</p>"          },          {            "group": "Parameter",            "type": "Number",            "optional": true,            "field": "limit",            "defaultValue": "3",            "description": "<p>Number of stop information to return</p>"          }        ]      }    },    "success": {      "examples": [        {          "title": "Success-Response:",          "content": "HTTP/1.1 200 OK\n{\n  \"destination\": \"point\",\n  \"dueMins\": \"8\"\n}",          "type": "json"        }      ]    },    "version": "0.0.0",    "filename": "./app.js",    "groupTitle": "Public_Transport"  },  {    "type": "get",    "url": "/weather/:city,:country'",    "title": "Get weather information",    "name": "GetWeather",    "group": "Weather",    "parameter": {      "fields": {        "Parameter": [          {            "group": "Parameter",            "type": "String",            "optional": false,            "field": "city",            "description": "<p>City name</p>"          },          {            "group": "Parameter",            "type": "String",            "optional": false,            "field": "country",            "description": "<p>Country code</p>"          }        ]      }    },    "success": {      "examples": [        {          "title": "Success-Response:",          "content": "HTTP/1.1 200 OK\n{\n \"description\": Sunny,\n \"temperature\": 18,\n  \"windSpeed\" : 20,\n}",          "type": "json"        }      ]    },    "version": "0.0.0",    "filename": "./app.js",    "groupTitle": "Weather"  }] });
