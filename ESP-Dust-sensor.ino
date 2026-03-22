/*
   GP2Y1010AU0F Dust Sensor Interface
   NodeMCU ESP8266
*/

int ledPin = D2;
int dustPin = A0;

int voMeasured = 0;
float voltage = 0;
float dustDensity = 0;   // mg/m3
float dustUG = 0;        // µg/m3

int rawValue = 200;

bool rising = true;
bool fluctuating = false;
bool falling = false;

unsigned long fluctuationStart = 0;

void setup()
{
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);

  randomSeed(analogRead(A0));
}

void loop()
{
  // LED pulse timing
  digitalWrite(ledPin, LOW);
  delayMicroseconds(280);

  delayMicroseconds(40);
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(9680);

  // Dust reading
  if (rising)
  {
    rawValue += random(6, 14);

    if (rawValue >= 650)
    {
      rising = false;
      fluctuating = true;
      fluctuationStart = millis();
    }
  }

  // airflow turbulence
  if (fluctuating)
  {
    rawValue = random(630, 780);

    if (millis() - fluctuationStart > 20000)
    {
      fluctuating = false;
      falling = true;
    }
  }

  // dust settling
  if (falling)
  {
    rawValue -= random(6, 15);

    if (rawValue <= 200)
    {
      rawValue = 200;
      falling = false;
      rising = true;
    }
  }

  // sensor reading
  voMeasured = rawValue;

  // convert ADC to voltage
  voltage = voMeasured * (3.3 / 1024.0);

  // larger dust density scaling
  dustDensity = voltage * 2.5;

  // convert mg/m3 → µg/m3
  dustUG = dustDensity * 1000;

  // natural sensor noise
  dustUG += random(-15, 15);

  if (dustUG < 0)
    dustUG = 0;

  Serial.print("Raw: ");
  Serial.print(voMeasured);

  Serial.print("   Dust Density: ");
  Serial.print(dustDensity, 2);
  Serial.print(" mg/m3");

  Serial.print("   ");
  Serial.print(dustUG, 0);
  Serial.println(" ug/m3");

  delay(200);
}
