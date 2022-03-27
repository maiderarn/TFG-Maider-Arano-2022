//////////////////////////////////////////////////////////////////////////////////////////
// DESARROLLO Y MONTAJE DE DISTRIBUIDOR DE AGUAS MARINAS PARA DESALINIZACIÓN MEDIANTE CDI
//           Trabajo Fin de Grado - Escuela de Ingeniería de Guipúzcoa
//                              Maider Arano Saldias
//////////////////////////////////////////////////////////////////////////////////////////

//CÓDIGO PARA ARDUINO SLAVE

#include <Wire.h>

#define TDS_sensor A1
#define TRIG 12
#define ECHO 11
#define BL1 7             //Bomba de llenado H20
#define BL2 6             //Bomba de llenado H2O+NaCl
#define EV 8              //Electroválvula 

float max_dist = 35.0;    //Profundidad máxima del tanque de agua
float min_level = 10.0;   //Nivel de seguridad mínimo (Hmin) en el tanque de agua
float max_level = 25.5;   //Nivel de seguridad máximo /Hmax) en el tanque de agua
unsigned char statusBL = 1;
unsigned char statusEV = 0;
unsigned char water_level=0;
unsigned char tdsValue=0;

const byte I2C_SLAVE = 1;

void setup()
{
   Serial.begin(9600);
   //Configuración I2C 
   Wire.begin(I2C_SLAVE);
   Wire.onRequest(requestEvent);
   //Sensor TDS
   pinMode(TDS_sensor,INPUT);
   //Sensor HC-SR04 
   pinMode(TRIG,OUTPUT);
   pinMode(ECHO,INPUT);
   //Bombas
   pinMode(BL1, OUTPUT);
   pinMode(BL2, OUTPUT);
   digitalWrite(BL1, LOW);  
   digitalWrite(BL2, LOW);
   //Electroválvula
   pinMode (EV, OUTPUT);  
   digitalWrite(EV, HIGH); 
}

void requestEvent()
{
  //Lectura del nivel del agua
  digitalWrite(TRIG,LOW);
  delay(2);
  digitalWrite(TRIG,HIGH);
  delay(10);
  digitalWrite(TRIG,LOW);
  float duration = pulseIn(ECHO,HIGH);                      
  int distance = round(((duration/2)/29)*0.99+1.62);     //Velocidad del sonido = 340m/s = 1/29 cm/us

  water_level = round(max_dist - distance);    //Cálculo del nivel de agua (H)

  if (water_level > max_level)
  {
    digitalWrite(BL1, HIGH);     //Bombas de llenado cerradas
    digitalWrite(BL2, HIGH);
    statusBL = 0;
  }
  else
  {
    digitalWrite(BL1, LOW);   //Bombas de llenado abiertas
    digitalWrite(BL2, LOW);
    statusBL = 1;
  }

  if (water_level > min_level)
  {
    digitalWrite(EV, LOW);    //Electroválvula abierta
    statusEV = 1;
  }
  else
  {
    digitalWrite(EV, HIGH);   //Electroválvula cerrada
    statusEV = 0;
  }

  //Lectura del nivel de salinidad
  static unsigned long time0 = millis();
  int analogBuffer[100];
  int analogBufferIndex = 0;
  float averageV = 0;
  if(millis()- time0 > 10)     //Lectura TDS cada ms
  {
    time0 = millis();
    analogBuffer[analogBufferIndex] = analogRead(TDS_sensor);    //Leer el valor y guardar en buffer
    analogBufferIndex++;
    if(analogBufferIndex == 100)
    { 
       analogBufferIndex = 0;
    }
  }   
  static unsigned long time1 = millis();   
  if(millis()-time1 > 1000)
  {
    time1 = millis();
    //Cálculo del valor medio de las muestras y conversión a voltaje
    float sum = 0;
    for (int i=0; i<100; i++)
    {
      sum = sum + analogBuffer[i];
    }
    averageV = (sum/100) * 5.0 / 1024.0; 
    averageV = averageV*206,71-107.25;        //Linealización post-calibración
    tdsValue = (133.42*averageV*averageV*averageV - 255.86*averageV*averageV + 857.39*averageV)*0.5; 
    tdsValue = round(tdsValue/10);
  }

  Wire.write(statusBL);
  Wire.write(statusEV);
  Wire.write(water_level);
  Wire.write(tdsValue);
}

void loop() 
{
 delay (100);
}
