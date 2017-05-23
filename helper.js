class Helper {

parseTramData(xml2js, tramResponseBody) {
   var xmlParser = xml2js.parseString;
   var trams = {};
   xmlParser(tramResponseBody, (error, result) => {
     trams = {
       trams: result.stopInfo.direction[0].tram,
       message: result.stopInfo.message
     };
   });
   return trams;
}

stripTramsData(trams, limit) {
  var luasData = [];
    trams.forEach((tram) => {
      var temp = {
        destination: this.filterLuasDestionationField(tram.$.destination),
        dueMins: tram.$.dueMins
      }
      luasData.push(temp);
    });
  // return first 3 option in order to fit into lcd screen
  return luasData.slice(0,limit);
}

//at the moment there one destination
//creating this method in case more stop comes
filterLuasDestionationField(luasDestinationName) {
  const stopWords = ['the'];
  let filteredDestinationName = '';
  luasDestinationName = luasDestinationName.toLowerCase();

  stopWords.forEach((word) => {
    filteredDestinationName = luasDestinationName.replace(word, "").trim();
  });

  return filteredDestinationName;
}

stripWeatherData(weatherData) {
  const speedKph = this.convertMphKph(weatherData.wind.speed);
  const temperature = Math.ceil(weatherData.main.temp) || '';
  const description = this.summarizeWeatherDescriptionField(weatherData.weather[0].description);

  return {
    description: description || '',
    temperature: temperature,
    windSpeed: speedKph || '',
  }
}

summarizeWeatherDescriptionField(weatherDescription) {
  let keywords = ['cloud', 'drizzle', 'thunderstrom', 'rain', 'snow', 'breeze'];

  for (let item of keywords) {
    if (weatherDescription.includes(item)) {
      return item;
    }
  }
  //lcd can display max 10 char among with other information
  return weatherDescription.substring(0,10);
}

convertMphKph(mphString) {
  const mph = parseFloat(mphString);
  const kph = Math.ceil((mph * 1.6093440));

  return kph;
}

filterBusData(rawData,busNumberFilterCriteria, limit) {
  // return first 3 option in order to fit into lcd screen
  let filteredBusData = rawData.filter((item) => {
    return busNumberFilterCriteria.includes(item.route);
  }).slice(0,limit);

  return filteredBusData;
}

stripBusData(busDatas) {
  var busObjectList = [];
  busDatas.forEach((busData) => {
    var temp = {
      route: busData['route'],
      duetime: busData['duetime']
    }
      busObjectList.push(temp);
   });

    return busObjectList;
  }
}

module.exports = Helper;
