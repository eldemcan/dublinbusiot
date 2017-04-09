class Helper {

parseTramData(xml2js, tramResponseBody) {
   var xmlParser = xml2js.parseString;
   var trams = {};
   xmlParser(tramResponseBody, (error, result) => {
     trams = result.stopInfo.direction[0].tram;
   });
   return trams;
}

stripTramsData(trams) {
   var luasData = [];
   trams.forEach((tram) => {
      var temp = {
       destination: tram.$.destination,
       dueMins: tram.$.dueMins
      }
      luasData.push(temp);
   });
   return luasData;
}

stripWeatherData(weatherData) {
  const speedKph = Helper.convertMphKph(weatherData.wind.speed);

  return {
    description: speedKph || '',
    temperature: weatherData.main.temp || '',
    windSpeed: weatherData.wind.speed || '',
  }
}

convertMphKph(mphString) {
  const mph = parseFloat(mphString);

  return (mph * 1.6093440).toFixed(2);
}

//TODO maybe return first 3 option in order to fit into lcd screen
filterBusData(rawData) {
  const busNumberFilterCriteria = ['54A', '27', '65'];
  const filteredBusData = rawData.filter((item) => {
    return busNumberFilterCriteria.includes(item.route);
  });
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
