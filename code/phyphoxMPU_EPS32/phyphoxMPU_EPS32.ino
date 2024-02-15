/* ============================================

    Sketch for NaWiSchool
    Output to phyphox app 
    [This code outputs 3 graphs from the MPU6050 to the phyphox app]
    Author: T. Schumann
    Date: 2024-01-20

    Dependencies:
    phyphox Arduino BLE - https://github.com/phyphox/phyphox-arduino
    Adafruit_Sensor - https://github.com/adafruit/Adafruit_Sensor
    MPU6050 - https://github.com/electroniccats/mpu6050

    All rights reserved. Copyright Tim Schumann 2024
    
  ===============================================
*/


#include <phyphoxBle.h>
#include <Wire.h>
#include <MPU6050.h>
#include <Math.h>


const float offsetX = 0.0;  //input your offset here
const float offsetY = 0.0;  //input your offset here
const float offsetZ = 0.0;  //input your offset here

float frequency = 50;
int period = (int)1000000 / frequency;
unsigned long recent;

void receivedData();

// Create a BME280 object
MPU6050 mpu;
int16_t ax, ay, az;
float fx, fy, fz, cpuTime;

void setup() {
  Serial.begin(115200);
  PhyphoxBLE::begin(&Serial);
  PhyphoxBLE::start("ESP32_MPU6050");
  PhyphoxBLE::configHandler = &receivedData;  // used to receive data from PhyPhox.

  //Experiment
  PhyphoxBleExperiment mpu6050Experiment;  //generate experiment on Arduino which plot random values


  mpu6050Experiment.setTitle("MPU6050_ReadOut");
  mpu6050Experiment.setCategory("Arduino Experiments");
  mpu6050Experiment.setDescription("Outputs values from MPU6050");

  //View
  PhyphoxBleExperiment::View firstView;
  firstView.setLabel("Graphen");  //Create a "view"
  PhyphoxBleExperiment::View secondView;
  secondView.setLabel("Einstellungen");  //Create a "view"


  //Graph
  PhyphoxBleExperiment::Graph firstGraph;  //Create graph which will plot random numbers over time
  firstGraph.setLabel("Beschleunigung X");
  firstGraph.setUnitX("s");
  //firstGraph.setUnitY("mm/s2");
  firstGraph.setLabelX("time");
  firstGraph.setLabelY("acceleration ");
  firstGraph.setXPrecision(1);  //The amount of digits shown after the decimal point
  firstGraph.setYPrecision(1);
  firstGraph.setColor("a11b06");  //Sets Color of line

  /* Assign Channels, so which data is plotted on x or y axis
     first parameter represents x-axis, second y-axis
     Channel 0 means a timestamp is created after the BLE package arrives in phyphox
     Channel 1 to N corresponding to the N-parameter which is written in server.write()
  */
  firstGraph.setChannel(0, 1);


  //Second Graph
  PhyphoxBleExperiment::Graph secondGraph;  //Create graph which will plot random numbers over time
  secondGraph.setLabel("Beschleunigung Y");
  secondGraph.setUnitX("s");
  //secondGraph.setUnitY("%");
  secondGraph.setLabelX("time");
  secondGraph.setLabelY("acceleration");
  //secondGraph.setStyle("dots");
  secondGraph.setColor("2E728E");  //Sets Color of line

  /* Assign Channels, so which data is plotted on x or y axis
     first parameter represents x-axis, second y-axis
     Channel 0 means a timestamp is created after the BLE package arrives in phyphox
     Channel 1 to N corresponding to the N-parameter which is written in server.write()
  */
  secondGraph.setChannel(0, 2);


  //Third Graph
  PhyphoxBleExperiment::Graph thirdGraph;  //Create graph which will plot random numbers over time
  thirdGraph.setLabel("Beschleunigung Z");
  thirdGraph.setUnitX("s");
  //thirdGraph.setUnitY("hPa");
  thirdGraph.setLabelX("time");
  thirdGraph.setLabelY("acceleration");
  //secondGraph.setStyle("dots");
  thirdGraph.setColor("52b038");  //Sets Color of line

  /* Assign Channels, so which data is plotted on x or y axis
     first parameter represents x-axis, second y-axis
     Channel 0 means a timestamp is created after the BLE package arrives in phyphox
     Channel 1 to N corresponding to the N-parameter which is written in server.write()
  */
  thirdGraph.setChannel(0, 3);


  //Info
  PhyphoxBleExperiment::InfoField info;  //Creates an info-box.
  info.setInfo("Zeit und Frequenz der Messung.");
  //info.setColor("404040");  //Sets font color. Uses a 6 digit hexadecimal value in "quotation marks".
  info.setXMLAttribute("size=\"1.2\"");

  //Separator
  PhyphoxBleExperiment::Separator separator;  //Creates a line to separate elements.
  separator.setHeight(0.5);                   //Sets height of the separator.
  separator.setColor("404040");               //Sets color of the separator. Uses a 6 digit hexadecimal value in "quotation marks".

  //Value
  PhyphoxBleExperiment::Value dispTime;  //Creates a value-box.
  dispTime.setLabel("Time:");            //Sets the label
  dispTime.setPrecision(4);              //The amount of digits shown after the decimal point.
  dispTime.setUnit("s");                 //The physical unit associated with the displayed value.
  dispTime.setColor("FFFFFF");           //Sets font color. Uses a 6 digit hexadecimal value in "quotation marks".
  dispTime.setChannel(0);
  dispTime.setXMLAttribute("size=\"2\"");


  //Edit
  PhyphoxBleExperiment::Edit freq;
  freq.setLabel("Frequency:");
  freq.setUnit("Hz");
  freq.setSigned(false);
  freq.setDecimal(true);
  freq.setChannel(4);
  freq.setXMLAttribute("min=\"0.01\"");
  freq.setXMLAttribute("max=\"50\"");

  //Export
  PhyphoxBleExperiment::ExportSet exp;  //Provides exporting the data to excel etc.
  exp.setLabel("export MPU6050");

  PhyphoxBleExperiment::ExportData data0;
  data0.setLabel("time");
  data0.setDatachannel(0);

  PhyphoxBleExperiment::ExportData data1;
  data1.setLabel("acceleration x");
  data1.setDatachannel(1);

  PhyphoxBleExperiment::ExportData data2;
  data2.setLabel("acceleration y");
  data2.setDatachannel(2);

  PhyphoxBleExperiment::ExportData data3;
  data3.setLabel("acceleration z");
  data3.setDatachannel(3);

  PhyphoxBleExperiment::ExportData data4;
  data4.setLabel("cpu time millis");
  data4.setDatachannel(4);

  //attach to experiment
  firstView.addElement(firstGraph);              //attach graph to view
  firstView.addElement(secondGraph);             //attach second graph to view
  firstView.addElement(thirdGraph);              //attach third graph to view
  secondView.addElement(info);                   //attach info to view
  secondView.addElement(separator);              //attach separator to view
  secondView.addElement(dispTime);               //attach value to view
  secondView.addElement(freq);                   //attach editfield to view (Linked to value)
  mpu6050Experiment.addView(firstView);          //attach view to experiment
  mpu6050Experiment.addView(secondView);         //attach view to experiment
  exp.addElement(data0);                         //attach data to exportSet
  exp.addElement(data1);                         //attach data to exportSet
  exp.addElement(data2);                         //attach data to exportSet
  exp.addElement(data3);                         //attach data to exportSet
  exp.addElement(data4);                         //attach data to exportSet
  mpu6050Experiment.addExportSet(exp);           //attach exportSet to experiment
  PhyphoxBLE::addExperiment(mpu6050Experiment);  //attach experiment to server

  Wire.begin(26, 27);
  mpu.initialize();
}


void loop() {

  if (micros() - recent > period) {

    mpu.getAcceleration(&ax, &ay, &az);
    fx = (ax * 9.81 / 16384) + offsetX;
    fy = (ay * 9.81 / 16384) + offsetY;
    fz = (az * 9.81 / 16384) + offsetZ;
    //Serial.println(fx + fy + fz);
    cpuTime = millis();

    PhyphoxBLE::write(fx, fy, fz, cpuTime);
    PhyphoxBLE::poll();  //Only required for the Arduino Nano 33 IoT, but it does no harm for other boards.

    recent += period;
  }
}

void receivedData() {  // get data from PhyPhox appfloat receivedInterval;
  float receivedInterval;
  PhyphoxBLE::read(receivedInterval);
  if (receivedInterval > 0) {
    frequency = receivedInterval;
    period = 1000000 / frequency;
  }
  //Serial.println(period);
}



//-------------------------------------
