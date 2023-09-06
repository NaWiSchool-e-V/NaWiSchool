#include <phyphoxBle.h>
#include <Wire.h>
#include <MPU6050.h>
#include <Math.h>


const float offsetX = -0.4;  //input your offset here
const float offsetY = 0.28;  //input your offset here
const float offsetZ = -0.75;  //input your offset here

float frequency = 50;
int period = (int)1000000 / frequency;

void receivedData();

// Create a BME280 object
MPU6050 mpu;
int16_t ax, ay, az;
float fx, fy, fz, fi;

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
  PhyphoxBleExperiment::InfoField myInfo;  //Creates an info-box.
  myInfo.setInfo("Hier kann die Frequenz der Messung angepasst werden.");
  //myInfo.setColor("404040");  //Sets font color. Uses a 6 digit hexadecimal value in "quotation marks".
  myInfo.setXMLAttribute("size=\"1.2\"");

  //Separator
  PhyphoxBleExperiment::Separator mySeparator;  //Creates a line to separate elements.
  mySeparator.setHeight(0.3);                   //Sets height of the separator.
  mySeparator.setColor("404040");               //Sets color of the separator. Uses a 6 digit hexadecimal value in "quotation marks".

  /*//Value
  PhyphoxBleExperiment::Value myValue;  //Creates a value-box.
  myValue.setLabel("Number");           //Sets the label
  myValue.setPrecision(2);              //The amount of digits shown after the decimal point.
  myValue.setUnit("u");                 //The physical unit associated with the displayed value.
  myValue.setColor("FFFFFF");           //Sets font color. Uses a 6 digit hexadecimal value in "quotation marks".
  myValue.setChannel(3);
  myValue.setXMLAttribute("size=\"2\"");
  */

  //Edit
  PhyphoxBleExperiment::Edit myEdit;
  myEdit.setLabel("Frequen:");
  myEdit.setUnit("Hz");
  myEdit.setSigned(false);
  myEdit.setDecimal(true);
  myEdit.setChannel(1);
  myEdit.setXMLAttribute("min=\"0.01\"");
  myEdit.setXMLAttribute("max=\"100\"");

  //Export
  PhyphoxBleExperiment::ExportSet mySet;  //Provides exporting the data to excel etc.
  mySet.setLabel("mySet");

  PhyphoxBleExperiment::ExportData myData1;
  myData1.setLabel("acceleration x");
  myData1.setDatachannel(1);

  PhyphoxBleExperiment::ExportData myData2;
  myData2.setLabel("acceleration y");
  myData2.setDatachannel(2);

  PhyphoxBleExperiment::ExportData myData3;
  myData3.setLabel("acceleration z");
  myData3.setDatachannel(3);

  //attach to experiment
  firstView.addElement(firstGraph);    //attach graph to view
  firstView.addElement(secondGraph);   //attach second graph to view
  firstView.addElement(thirdGraph);    //attach third graph to view
  secondView.addElement(myInfo);       //attach info to view
  secondView.addElement(mySeparator);  //attach separator to view
  //secondView.addElement(myValue);       //attach value to view
  secondView.addElement(myEdit);         //attach editfield to view (Linked to value)
  mpu6050Experiment.addView(firstView);  //attach view to experiment
  mpu6050Experiment.addView(secondView);
  mySet.addElement(myData1);                     //attach data to exportSet
  mySet.addElement(myData2);                     //attach data to exportSet
  mySet.addElement(myData3);                     //attach data to exportSet
  mpu6050Experiment.addExportSet(mySet);         //attach exportSet to experiment
  PhyphoxBLE::addExperiment(mpu6050Experiment);  //attach experiment to server

  Wire.begin();
  mpu.initialize();
}


void loop() {
  unsigned long start_time = micros();  // record start time,

  mpu.getAcceleration(&ax, &ay, &az);
  fx = (ax * 9.81 / 16384) + offsetX;
  fy = (ay * 9.81 / 16384) + offsetY;
  fz = -((az * 9.81 / 16384) - offsetZ);
  //Serial.println(fx + fy + fz);

  PhyphoxBLE::write(fx, fy, fz);


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
