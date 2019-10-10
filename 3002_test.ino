// Constants
const int VOLTAGE_PIN = A14; 
const int SENSOR_PIN = A15;  // Input pin for measuring Vout
const int RS = 10;          // Shunt resistor value (in ohms)
const int VOLTAGE_REF = 5;  // Reference voltage for analog read
const int BATTERY_VOLTAGE = 6;

// Global Variables
float sensorValue;   // Variable to store value from analog read
float current;       // Calculated current value
float voltagePort;       // to store value from analog read
float voltageValue;
unsigned long time_now = 0;
unsigned long energy;

void setup() {

  // Initialize serial monitor
  Serial.begin(9600);
  
}

void loop() {
  
  // Read a value from the INA169 board
  sensorValue = analogRead(SENSOR_PIN);

  // Remap the ADC value into a voltage number (5V reference)
  sensorValue = (sensorValue * VOLTAGE_REF) / 1023;

  // Follow the equation given by the INA169 datasheet to
  // determine the current flowing through RS. Assume RL = 10k
  // Is = (Vout x 1k) / (RS x RL)
  current = sensorValue / (10 * RS);

  // getting the voltage
  voltage = analogRead(VOLTAGE_PIN);
  voltageVal = 2 * (voltage * BATTERY_VOLTAGE) / 1023;

  time_now = millis();
  timeDiff = ((time_now - prevTime) * 10^(-3)) / 3600;
  energy = energy + (current * voltageVal * timeDiff);
  prevTime = time_now;
  // Output value (in amps) to the serial monitor to 3 decimal
  // places
  Serial.print(current, 3);
  Serial.println(" A");

  // Delay program for a few milliseconds
  delay(500);

}

void power_cal(){
  return energy / ((time_now * 10^(-3))/3600);
}
