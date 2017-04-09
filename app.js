/*eslint no-console: ["error", { allow: ["warn", "error", "info", "log"] }] */
require('dotenv').config();
var express = require('express');
var request = require('request');
var xml2js = require('xml2js');
var Helper= require('./helper.js');

var app = express();
var helper = new Helper();

app.get('/luas/:stopId', (req,res) => {
  const stopId = req.params.stopId;
  const url =`http://luasforecasts.rpa.ie/xml/get.ashx?action=forecast&stop=${stopId}&encrypt=false`;
  request(url, (error, response, body) => {
    if (response.statusCode == 200) {
     const tramsRaw = helper.parseTramData(xml2js, body);
     const stripedTramsData = helper.stripTramsData(tramsRaw);
     res.json(stripedTramsData);
    }
    else {
      console.error('Could not get data from luas service');
    }
  })
});


app.get('/bus/:stopNo', (req, res) => {
  const stopNo = req.params.stopNo;
  const url=`https://data.dublinked.ie/cgi-bin/rtpi/realtimebusinformation?stopid=${stopNo}&format=json`;
  request(url, (error, response, body) => {
    if (response.statusCode == 200) {
      const parsedData = JSON.parse(body);
      const dublinBusErrorCode = parsedData['errorcode'];
        if (dublinBusErrorCode!=1) {
          const filteredBusData = helper.filterBusData(parsedData['results'])
          const stripedBusData = helper.stripBusData(filteredBusData);

          res.json(stripedBusData);
        }
        else {
          console.error("Something wrong in Dublin bus end point");
        }
    }
    else {
      console.error("Can't get answer from Dublin Bus API");
    }
  });
});

app.get('/weather/:city,:country', (req,res) => {
  const APIKEY=process.env.WEATHERAPI;
  const cityName = req.params.city;
  const countyCode = req.params.country;
  const requestUrl = `http://api.openweathermap.org/data/2.5/weather?q=${cityName},${countyCode}&units=metric&appid=${APIKEY}`;
  request(requestUrl, (error, resonse, body) => {
    const parsedData = JSON.parse(body);
    const stripedWeatherData = helper.stripWeatherData(parsedData);
    res.json(stripedWeatherData);
  });
});

app.listen(3000, () => {
  console.info('Server Started');
});
