//////////////////////////////////////////////////////////////////////////////////////////
// DESARROLLO Y MONTAJE DE DISTRIBUIDOR DE AGUAS MARINAS PARA DESALINIZACIÓN MEDIANTE CDI
//            Trabajo Fin de Grado - Escuela de Ingeniería de Guipúzcoa
//                               Maider Arano Saldias
//////////////////////////////////////////////////////////////////////////////////////////

//CÓDIGO PARA ARDUINO MASTER

#include <Wire.h>

#define FM 3                    //Flowmeter YF-S201
#define BC1 9                   //PWM Bomba de Caudal 1 3~12V
#define BC2 10                  //PWM Bomba de Caudal 2 3~12V

// Constantes del flowmeter
float flow;                     //Caudal medido [L/min]
volatile float pulse = 0;       //Pulsos medidos
float K = 318.0;                //Pulsos equivalentes a 1L de H2O (constante de calibración) Caudalímetro1

// Constantes de la bomba
float voltage=0;                //Voltaje de salida
float value=72;                 //Valor PWM para conseguir el voltaje de salida requerido
float offset=0.42;
float slope;
float max_slope=0.63;

// Constantes del controlador
double kp=0.80;
double ki=0.005;
double kd=0.0;

// Variables externas del controlador
float Input=0;
float Output=0;
float Setpoint=0;
 
// Variables internas del controlador
unsigned long currentTime, previousTime;
float elapsedTime;
float error=0;
float lastError=0;
float cumError=0;
float rateError=0;

//Variable internas comunicación I2C
const byte I2C_SLAVE = 1;
unsigned char slave_data[4];
unsigned char data[6];

void setup() {       
  //Inicializar Serial
  Serial.begin(9600);  
  
  //Unirse al bus I2C como master
  Wire.begin();      
    
  // Caudalímetro
  pinMode(FM, INPUT_PULLUP);
  
  // Bombas
  pinMode(BC1, OUTPUT);
  pinMode(BC2, OUTPUT);
  analogWrite(BC1, value);
  analogWrite(BC2, value);
  
  // Habilitar interrupción INT0 para cada cambio de valor de low a high
  attachInterrupt(digitalPinToInterrupt(FM), ISR_pulsecounter, RISING);
  interrupts();
  
  // Inicializar consigna [L/min]
  Setpoint = 2.0;  

  //Guardar start-time
  previousTime = millis();
}

void loop() {
  // Leer caudal
  elapsedTime=1000;
  pulse = 0;
  delay(elapsedTime);
  float flow = (pulse/K)*60;

  //Cálculo PID
  error = Setpoint - flow;                              // determinar el error entre la consigna y la medición
  cumError = cumError+(error * elapsedTime);            // calcular la integral del error
  rateError = (error - lastError) / elapsedTime;        // calcular la derivada del error
  Output = kp*error + ki*cumError + kd*rateError;       // calcular la salida del PID 
  lastError = error;                                    // almacenar error anterior

  //Cálculo voltaje de salida
  voltage =((Output - 0.7444)/0.1615)+1.5;

  if (voltage > 11.0){voltage = 11.0;}
  if (voltage < 3.0){voltage = 3.0;}
  if ((voltage > 7)and(voltage < 11)){max_slope = 0.81;}
  
  slope = (voltage - 3)*max_slope/9;  
  value = (voltage + offset - slope)*255/12;
  analogWrite(BC1, value);
  analogWrite(BC2, value);

  //Pedir datos al Slave
  Wire.requestFrom(1,4);
  while(Wire.available()){
      for (int i=0; i<4; i++){
        slave_data[i]=Wire.read();
      }
  }
  //Enviar datos al control de usuario
  data[0]=flow*100;
  data[1]=slave_data[0];
  data[2]=slave_data[1];
  data[3]=slave_data[2];
  data[4]=slave_data[3]*10;
  data[5]=Setpoint*100;
  Serial.print(data[0]);Serial.print(":");
  Serial.print(data[1]);Serial.print(":");
  Serial.print(data[2]);Serial.print(":");
  Serial.print(data[3]);Serial.print(":");
  Serial.print(data[4]);Serial.print(":");
  Serial.println(data[5]);
  
  delay(3000);
  interrupts(); 
}

// Interrupción del caudalímetro para medida de pulsos
void ISR_pulsecounter()
{
  pulse++;
}
