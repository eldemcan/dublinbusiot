/*eslint no-console: ["error", { allow: ["warn", "error", "info", "log"] }] */
require('dotenv').config();
var express = require('express');
var request = require('request-promise');
var xml2js = require('xml2js');
var Helper= require('./helper.js');
var cache = require('memory-cache');
var datadogApi = require('dogapi');

var app = express();
var helper = new Helper();

const dogApiOptions = {
api_key: process.env.DATADOG_API_KEY,
app_key: process.env.DATADOG_APP_KEY,
}
// process.env.PORT lets the port be set by Heroku
var port = process.env.PORT || 8080;
datadogApi.initialize(dogApiOptions);
const features = process.env.FEATURES.split(",");

app.get('/luas/:stopId/:limit*?', (req,res) => {
  const stopId = req.params.stopId;
  const limit = req.params.limit || 3;
  const url =`http://luasforecasts.rpa.ie/xml/get.ashx?action=forecast&stop=${stopId}&encrypt=false`;
  const now = parseInt(new Date().getTime() / 1000);

  request(url)
    .then(body => {
      const parcedData = helper.parseTramData(xml2js, body);
      //luas does not function propery
      if (!parcedData.message[0].toLowerCase().includes('normally')) res.json({ destination: 'No Luas', dueMins: 0 });

      const stripedTramsData = helper.stripTramsData(parcedData.trams, limit);
      cache.put(stopId, stripedTramsData);
      res.json(stripedTramsData);
      if(features.includes('datadog_api')) datadogApi.metric.send('tramData.success',[now,1]);
   })
   .catch((error) => {
     console.error(error);
     console.error('Could not get data from luas service');
     const stripedTramsData = cache.get(stopId) || [{}];
     res.json(stripedTramsData);
     if(features.includes('datadog_api')) datadogApi.metric.send('tramData.failure',[now,1]);
   });
});


app.get('/bus/:stopNo/:filter?/:limit?', (req, res) => {
  const stopNo = req.params.stopNo;
  const limit = req.params.limit || 3;
  const now = parseInt(new Date().getTime() / 1000);

  //max 10 filter criterias
  const defaultFilterCriteria = ['54A', '27', '65'];
  const busNumberFilterCriteria = (req.params.filter == undefined) ? defaultFilterCriteria : req.params.filter.split(',',"10");

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
          if(features.includes('datadog_api')) datadogApi.metric.send('busData.success',[now,1]);
        }
        else {
          console.error("Something wrong in Dublin bus end point");
          const stripedBusData = cache.get(stopNo) || [{}];
          res.json(stripedBusData);
          if(features.includes('datadog_api')) datadogApi.metric.send('busData.service.failure',[now,1]);
        }
    })
    .catch((error) => {
      console.error(error);
      console.error("Can't get answer from Dublin Bus API");
      const stripedBusData = cache.get(stopNo) || [{}];
      res.json(stripedBusData);
      if(features.includes('datadog_api')) datadogApi.metric.send('busData.failure',[now,1]);
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
  const now = parseInt(new Date().getTime() / 1000);

  if (cachedData) {
    datadogApi.metric.send('weatherdata.cached', [now, 1]);
    res.json(cachedData);
  }
  else {
    request(url)
      .then((body) => {
        const parsedData = JSON.parse(body);
        const stripedWeatherData = helper.stripWeatherData(parsedData);
        cache.put(cityName, stripedWeatherData, fourHours);
        res.json(stripedWeatherData);
        if (features.includes('datadog_api')) datadogApi.metric.send('weatherdata.success', [now, 1]);
    })
    .catch((error) => {
      console.error(error);
      console.error("Could not get weather data");
      res.json(cachedData);
      if (features.includes('datadog_api'))  datadogApi.metric.send('weatherdata.failures',[now,1]);
    });
  }
});

app.listen(port, () => {
  console.info(`Server started at ${port}`);
});

