/*eslint no-console: ["error", { allow: ["warn", "error", "info", "log"] }] */
require('dotenv').config();
var express = require('express');
var request = require('request-promise');
var xml2js = require('xml2js');
var Helper= require('./helper.js');
var cache = require('memory-cache');

var app = express();
var helper = new Helper();

// set the port of our application
// process.env.PORT lets the port be set by Heroku
var port = process.env.PORT || 8080;

app.get('/luas/:stopId/:limit*?', (req,res) => {
  const stopId = req.params.stopId;
  const limit = req.params.limit || 3;
  const url =`http://luasforecasts.rpa.ie/xml/get.ashx?action=forecast&stop=${stopId}&encrypt=false`;

  request(url)
    .then(body => {
      const tramsRaw = helper.parseTramData(xml2js, body);
      const stripedTramsData = helper.stripTramsData(tramsRaw,limit);
      cache.put(stopId, stripedTramsData);
      res.json(stripedTramsData);
   })
   .catch((error) => {
     console.error(error);
     console.error('Could not get data from luas service');
     const stripedTramsData = cache.get(stopId) || [{}];
     res.json(stripedTramsData);
   });
});


app.get('/bus/:stopNo/:filter*?/:limit*?', (req, res) => {
  const stopNo = req.params.stopNo;
  const limit = req.params.limit || 3;
  //max 10 filter criterias
  const defaultFilterCriteria = ['54A', '27', '65'];
  const busNumberFilterCriteria = (req.params.filter == undefined) ? defaultFilterCriteria : req.params.filter.split(',',"10");
  console.log(busNumberFilterCriteria);

  const url=`https://data.dublinked.ie/cgi-bin/rtpi/realtimebusinformation?stopid=${stopNo}&format=json`;

  request(url)
    .then((body) => {
      const parsedData = JSON.parse(body);
      const dublinBusErrorCode = parsedData['errorcode'];
        if (dublinBusErrorCode!=1) {
          const filteredBusData = helper.filterBusData(parsedData['results'], busNumberFilterCriteria, limit)
          const stripedBusData = helper.stripBusData(filteredBusData);
          cache.put(stopNo, stripedBusData);

          res.json(stripedBusData);
        }
        else {
          console.error("Something wrong in Dublin bus end point");
          const stripedBusData = cache.get(stopNo) || [{}];
          res.json(stripedBusData);
        }
    })
    .catch((error) => {
      console.error(error);
      console.error("Can't get answer from Dublin Bus API");
      const stripedBusData = cache.get(stopNo) || [{}];
      res.json(stripedBusData);
    });
});

app.get('/weather/:city,:country', (req,res) => {
  const APIKEY=process.env.WEATHERAPI;
  const oneHour = 3600000;
  const fourHours = (oneHour * 8)
  const cityName = req.params.city;
  const countyCode = req.params.country;
  const url = `http://api.openweathermap.org/data/2.5/weather?q=${cityName},${countyCode}&units=metric&appid=${APIKEY}`;
  const cachedData = cache.get(cityName);

  if (cachedData) {
    res.json(cachedData);
  }
  else {
    request(url)
      .then((body) => {
        const parsedData = JSON.parse(body);
        const stripedWeatherData = helper.stripWeatherData(parsedData);
        cache.put(cityName, stripedWeatherData, fourHours);
        res.json(stripedWeatherData);
    })
    .catch((error) => {
      console.error(error);
      console.error("Could not get weather data");
      res.json(cachedData);
    });
  }
});

app.listen(port, () => {
  console.info(`Server started at ${port}`);
});
