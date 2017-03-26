require('dotenv').config()
var express = require('express');
var request = require('request');
var app = express();

app.get('/bus/:stopNo', (req, res) => {
  const stopNo = req.params.stopNo;
  var url=`https://data.dublinked.ie/cgi-bin/rtpi/realtimebusinformation?stopid=${stopNo}&format=json`
  request(url, (error, response, body) => {
    if (response.statusCode == 200) {
      const parsedData = JSON.parse(body);
      const dublinBusErrorCode = parsedData['errorcode'];
      filteredBusData = filterBusData(parsedData['results'])
      stripBusData(filteredBusData);
    }
    else {
      console.log("Can't get answer from Dublin Bus API");
    }
  });
})

app.get('/weather/:city,:country', (req,res) => {
  const APIKEY=process.env.WEATHERAPI;
  const cityName = req.params.city;
  const countyCode = req.params.country;
  var requestUrl = `http://api.openweathermap.org/data/2.5/weather?q=${cityName},${countyCode}&units=metric&appid=${APIKEY}`;
  request(requestUrl, (error, resonse, body) => {
    var parsedData = JSON.parse(body);
    printWeatherData(parsedData);
  });
});

app.listen(3000, ()=> {
  console.log('Server Started');
})

function stripWeatherData(weatherData) {
  const speedKph = convertMphKph(weatherData['wind']['speed']);

  return {
    description: speedKph || "",
    temperature: weatherData['main']['temp'] || "",
    windSpeed:weatherData['wind']['speed'] || ""
  }
}

function convertMphKph(mphString) {
  const mph = parseFloat(mphString);

  return (mph * 1.6093440).toFixed(2);
}

//TODO maybe return first 3 option in order to fit into lcd screen
function filterBusData(rawData) {
  const busNumberFilterCriteria = ["54A", "27", "65"];
  const filteredBusData = rawData.filter((item) => {
    return busNumberFilterCriteria.includes(item['route']);
  });
  return filteredBusData;
}

function stripBusData(busDatas) {
  var busObjectList = [];
  busDatas.forEach((busData) => {
    var temp = {
      route: busData['route'],
      duetime: busData['duetime']
    }
    console.log(temp);
    busObjectList.push(temp);
  });
}
