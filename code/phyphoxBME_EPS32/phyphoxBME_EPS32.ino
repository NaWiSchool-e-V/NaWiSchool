/* ============================================

    Sketch for NaWiSchool
    Output to phyphox app 
    [This code outputs 3 graphs from the BME280 to the phyphox app]
    Author: T. Schumann
    Date: 2023-03-12

    Dependencies:
    phyphox Arduino BLE - https://github.com/phyphox/phyphox-arduino
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    Adafruit_BME380 - https://github.com/adafruit/Adafruit_BME280_Library

    All rights reserved. Copyright Tim Schumann 2023
    
  ===============================================
*/


#include "phyphoxBle.h"
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Math.h>

float frequency = 50;
int period = (int)1000000 / frequency;

void receivedData();

// Create a BME280 object
Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  PhyphoxBLE::begin(&Serial);
  PhyphoxBLE::start("ESP32_BME280");
  PhyphoxBLE::configHandler = &receivedData;  // used to receive data from PhyPhox.

  //Experiment
  PhyphoxBleExperiment bme280Experiment;  //generate experiment on Arduino which plot random values


  bme280Experiment.setTitle("BME280_ReadOut");
  bme280Experiment.setCategory("Arduino Experiments");
  bme280Experiment.setDescription("Outputs values from BME280");

  //View
  PhyphoxBleExperiment::View firstView;
  firstView.setLabel("Graphen");  //Create a "view"
  PhyphoxBleExperiment::View secondView;
  secondView.setLabel("Einstellungen");  //Create a "view"


  //Graph
  PhyphoxBleExperiment::Graph firstGraph;  
  firstGraph.setLabel("Temperatur");
  firstGraph.setUnitX("s");
  firstGraph.setUnitY("°C");
  firstGraph.setLabelX("time");
  firstGraph.setLabelY("temperature ");
  firstGraph.setXPrecision(1); 
  firstGraph.setYPrecision(1);

  firstGraph.setChannel(0, 1);

  //Second Graph
  PhyphoxBleExperiment::Graph secondGraph;  
  secondGraph.setLabel("Luftfeuchtigkeit");
  secondGraph.setUnitX("s");
  secondGraph.setUnitY("%");
  secondGraph.setLabelX("time");
  secondGraph.setLabelY("humidity");
  //secondGraph.setStyle("dots");
  secondGraph.setColor("2E728E"); 

  secondGraph.setChannel(0, 2);


  //Third Graph
  PhyphoxBleExperiment::Graph thirdGraph; 
  thirdGraph.setLabel("Druck");
  thirdGraph.setUnitX("s");
  thirdGraph.setUnitY("hPa");
  thirdGraph.setLabelX("time");
  thirdGraph.setLabelY("pressure");
  //secondGraph.setStyle("dots");
  thirdGraph.setColor("52b038");  

  thirdGraph.setChannel(0, 3);

  //Info
  PhyphoxBleExperiment::InfoField myInfo;  //Creates an info-box.
  myInfo.setInfo("Hier kann die Frequenz der Messung angepasst werden.");
  myInfo.setXMLAttribute("size=\"1.2\"");

  //Separator
  PhyphoxBleExperiment::Separator mySeparator;  //Creates a line to separate elements.
  mySeparator.setHeight(0.3);                   //Sets height of the separator.
  mySeparator.setColor("404040");               //Sets color of the separator. Uses a 6 digit hexadecimal value in "quotation marks".

  //Edit
  PhyphoxBleExperiment::Edit myEdit;
  myEdit.setLabel("Frequen:");
  myEdit.setUnit("Hz");
  myEdit.setSigned(false);
  myEdit.setDecimal(true);
  myEdit.setChannel(1);
  myEdit.setXMLAttribute("min=\"0.1\"");

  //Export
  PhyphoxBleExperiment::ExportSet mySet;  
  mySet.setLabel("mySet");
  
  PhyphoxBleExperiment::ExportData myData1;
  myData1.setLabel("temperature");
  myData1.setDatachannel(1);

  PhyphoxBleExperiment::ExportData myData2;
  myData2.setLabel("humidity");
  myData2.setDatachannel(2);

  PhyphoxBleExperiment::ExportData myData3;
  myData3.setLabel("pressure");
  myData3.setDatachannel(3);

  //attach to experiment
  firstView.addElement(firstGraph);    //attach graph to view
  firstView.addElement(secondGraph);   //attach second graph to view
  firstView.addElement(thirdGraph);    //attach third graph to view
  secondView.addElement(myInfo);       //attach info to view
  secondView.addElement(mySeparator);  //attach separator to view
  secondView.addElement(myEdit);        //attach editfield to view (Linked to value)
  bme280Experiment.addView(firstView);  //attach view to experiment
  bme280Experiment.addView(secondView);
  mySet.addElement(myData1);                    //attach data to exportSet
  mySet.addElement(myData2);                    //attach data to exportSet
  mySet.addElement(myData3);                    //attach data to exportSet
  bme280Experiment.addExportSet(mySet);         //attach exportSet to experiment
  PhyphoxBLE::addExperiment(bme280Experiment);  //attach experiment to server

  bme.begin(0x76, &Wire);
}


void loop() {
  unsigned long start_time = micros();  // record start time
  float temp = bme.readTemperature();
  float hum = bme.readHumidity();
  float pres = bme.readPressure() / 100.0F;

  PhyphoxBLE::write(temp, hum, pres);


  unsigned long execution_time = micros() - start_time;  // calculate execution time
  unsigned long delay_time = period - execution_time;    // calculate delay time
  if (delay_time > 0) {
    delayMicroseconds(delay_time);  // delay for the remaining time
  }
  //Serial.println(delay_time);

  PhyphoxBLE::poll();  //Only required for the Arduino Nano 33 IoT, but it does no harm for other boards.
}

void receivedData() {  // get data from PhyPhox appfloat receivedInterval;
  float receivedInterval;
  PhyphoxBLE::read(receivedInterval);
  if (receivedInterval > 0) {
    frequency = receivedInterval;
    period = (int)1000000 / frequency;
  }
  //Serial.println(frequency);
}
