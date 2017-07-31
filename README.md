# Dublin Transportation IoT project
Small iot project to display Dublin Bus, Luas and weather information in LCD screen

![alt text](https://github.com/eldemcan/dublinbusiot/blob/master/images/IMG_20170610_212454.jpg  )

![alt text](https://github.com/eldemcan/dublinbusiot/blob/master/images/IMG_20170610_212432.jpg "Some sort of case")

![alt text](https://github.com/eldemcan/dublinbusiot/blob/master/images/IMG_20170610_184433.jpg "Wiring")

# General Components

![alt text](https://github.com/eldemcan/dublinbusiot/blob/master/images/Screen%20Shot%202017-07-31%20at%2022.25.20.png "Component Diagram")

- My node app is deployed to Heroku
- Client has a scheduler which wakes up between 07:00 - 09:00 and updates information every minute automatically. (weekdays) For other times if you like to fetch information you need to push yellow button

# Api Documentation
- You can find it in repo under [apiDoc](https://github.com/eldemcan/dublinbusiot/tree/master/apidoc) file. I used a tool called apidocjs.
