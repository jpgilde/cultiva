
//__________________________________________________________________________________________________________________________________________________________________________________________________________________
//______________________________LIBRERIAS Y VARIABLES GLOBALES___________________________________________________________________________________________________________________________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________________________________________________________________________________________________________

#include <virtuabotixRTC.h>
#include <LiquidCrystal.h>
#include <DHT.h>
  
byte punto_selector[8] = {B00000,B01000,B01100,B01110,B01100,B01000,B00000}; //se crea el caracter de seleccion del menu
byte grados[8] = {B00000,B01100,B01100,B00000,B00000,B00000,B00000}; //se crea el caracter de seleccion del menu
int filamaxLCD=3; //dimension (alto) del LCD

//variables de repososLCD
int seg=0;
int seg_ant=0;
int seg_ant_ls=0;
int cont_reposoLCD=0;
int cont_lectura_sensores=0;


//variables globales generales
int resultado_boton=0;//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
boolean loop_menu=true;
int seleccionador=-1;
int cont_parpadeo=0;
float tiempo_Sprox; //variable para la lectura de sensores de proximidad


//___VARIABLES DE CONTROL AMBIENTAL____________________________________________________________________________
//Variables Riego
int hora_riego; //hora de regado
int minuto_riego; //minuto de regado
int dias_riego; //intervalo de dias sin recibir riego
int tiempo_riego=3000; //tiempo de regado para el calculo del volumen
int limite_humedad=165; //165: limite seco, 215 limite humedo
int desactivaRiego; //variable que determina que se riege 1 sola vez si la tierra esta seca.


//Variables de llenado del estanque
int desactiva_llenadoEstanque; //
int nivel_agua; //= (40cm - sensor_prox1)

//Variable Sensores
int sensor_T_amb; //Sensor de Humedad y temperatura amb. conectado en: D11
int sensor_H_amb; //Sensor de Humedad y temperatura amb. conectado en: D11
float sensor_T_caja; //Sensor de temperatura caja conectado en: A1
float sensor_prox1; //Sensor de proximidad conectado en: trig: D33 echo:D31 2
int sensor_hum_tierra;
//______________________________________________________________________________________________________________
//Pines de sensores
int pin_botones_analog=0;
int pin_sensor_HyT_amb=11;//digital
int pin_sensor_T_caja=1;//analogo
int pin_sensor_prox1_trig=33;
int pin_sensor_prox1_echo=31;
int pin_sensor_hum_tierra=7;

//PINES
int pin_encoder1 = 3;
int pin_encoder2 = 2;
int pin_brilloLCD=25;
int pin_rele_valvula=43;
int pin_rele_bomba=45;
int rere_x4_1=51;
int rere_x4_2=49;
int rere_x4_3=47;
int rere_x4_4=45;

//Variable encoder
volatile int lastEncoded = 1000000;
volatile long encoderValue = 1000000;
volatile float encoderNavegador= 1000000;
float last_encoderNavegador= 1000000;
long lastencoderValue = 1000000;
int lastMSB = 1000000;
int lastLSB = 1000000;
float cont_encoder=1000000;
int cont_encoder2=1000000;
int last_cont_encoder2=1000000;

virtuabotixRTC myRTC(8, 9, 10); //(CLK,DAT, RST)
LiquidCrystal lcd(29, 6, 5, 27, 4, 22);  //(12, 11, 5, 4, 3, 2);
DHT dht(pin_sensor_HyT_amb,DHT11);

//__________________________________________________________________________________________________________________________________________________________________________________________________________________
//______________________________SETUP Y LOOP_________________________________________________________________________________________________________________________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________________________________________________________________________________________________________

void setup() {
//Se inician las librerias
  Serial.begin(9600);  
  lcd.begin(20,4);
  dht.begin();
  pinMode(pin_botones_analog,INPUT);//no es necesario definirlo
  pinMode(pin_sensor_HyT_amb,INPUT);
  pinMode(pin_rele_valvula,OUTPUT);
  pinMode(pin_rele_bomba,OUTPUT);
  pinMode(rere_x4_1,OUTPUT);
  pinMode(rere_x4_2,OUTPUT);
  pinMode(rere_x4_3,OUTPUT);
  pinMode(rere_x4_4,OUTPUT);  
  digitalWrite(pin_rele_bomba,HIGH);
  digitalWrite(pin_rele_valvula,HIGH);
  digitalWrite(rere_x4_1,HIGH);
  digitalWrite(rere_x4_2,HIGH);
  digitalWrite(rere_x4_3,HIGH);
  digitalWrite(rere_x4_4,HIGH);

  //myRTC.setDS1302Time(myRTC.seconds, myRTC.minutes, myRTC.hours, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year);
  myRTC.setDS1302Time(00,00,00,1,7,1,2000);  
  lcd.createChar(0,punto_selector); //asocioa el char punto_selector a char(0)  
  lcd.createChar(1,grados); //asocioa el char punto_selector a char(1)    
//se apaga el brillo y diplay para encenderlo posterior mente al precionar un boton
  lcd.clear();
  analogWrite(pin_brilloLCD,0);
  lcd.noDisplay();

  pinMode(pin_encoder1, INPUT); 
  pinMode(pin_encoder2, INPUT);
  pinMode(pin_sensor_prox1_trig,OUTPUT);
  pinMode(pin_sensor_prox1_echo,INPUT);      
  
  digitalWrite(pin_encoder1, HIGH); //turn pullup resistor on
  digitalWrite(pin_encoder2, HIGH); //turn pullup resistor on
  attachInterrupt(0, actualizaEncoder, CHANGE); 
  attachInterrupt(1, actualizaEncoder, CHANGE);
}

void loop(){
  LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);
  resultado_boton=0;
  presionarBoton(cont_reposoLCD);
  if( resultado_boton!=0){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    lcd.display();
    analogWrite(pin_brilloLCD,255);
    lcd.clear();  
    lcd.setCursor(0, 0); lcd.print("Bienvenido");
    delay(1000);		
    menu_Principal();
  }
}//_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________



//__________________________________________________________________________________________________________________________________________________________________________________________________________________
//______________________________FUNCIONES DEL SISTEMA__________________________________________________________________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________________________________________________________________________________________________________

void LecturaSensores(){//int &sensor_HyT_amb_2,int &pin_sensor_HyT_amb_2, int sensor_T_amb_2,int pin_sensor_T_amb_2) //___________________________________________Lectura Sensores    
  myRTC.updateTime();
  seg=myRTC.seconds;
  if(seg != seg_ant_ls){
    seg_ant_ls=seg;
    cont_lectura_sensores++;
    if(cont_lectura_sensores >=5){
      cont_lectura_sensores=0;
      sensor_T_amb= dht.readTemperature();
      sensor_H_amb= dht.readHumidity();
      sensor_T_caja=analogRead(pin_sensor_T_caja); //cada 10 [mV] aumenta 1 [C]. Se escala el voltaje del sensor a mV y se divide en 10
      int hum_tierra=analogRead(pin_sensor_hum_tierra);
      sensor_hum_tierra= (215-hum_tierra)*(100/(215-165));//valores lite del sensor: 215 tierra muy mojada, 165 tierra muy seca
      Serial.println(sensor_hum_tierra);
    }
      //lectura sensor de proximidad
  digitalWrite(pin_sensor_prox1_trig,LOW);
  delayMicroseconds(2);
  digitalWrite(pin_sensor_prox1_trig,HIGH);
  delayMicroseconds(10);
  digitalWrite(pin_sensor_prox1_trig,LOW);
  Serial.println("INICIO");
  tiempo_Sprox= pulseIn(pin_sensor_prox1_echo,HIGH);
  sensor_prox1= (0.017*tiempo_Sprox); //ajuste segun celeridad para determinar la distancia
  Serial.println("FIN");
  }  
  Control_Ambiental();
}//_______________________________________________________________________________________________________________________________________________________________________________



void Control_Ambiental(){ //______________________________________________________________________________________________________________________________________Control Ambiental

  if(sensor_prox1>40){
    digitalWrite(pin_rele_bomba,LOW);
//    desactiva_llenadoEstanque=0; 
  }
//  if(sensor_prox1<5){desactiva_llenadoEstanque=1;}
  if(sensor_prox1<=5){  //if(sensor_prox1<5 && desactiva_llenadoEstanque==0){
    digitalWrite(pin_rele_bomba,HIGH);
//    desactiva_llenadoEstanque=1; 
  }
  
  if(sensor_hum_tierra < limite_humedad){
    desactivaRiego=0;
  }
  if(sensor_hum_tierra > limite_humedad && desactivaRiego==0){
    digitalWrite(pin_rele_valvula,LOW);
    delay(tiempo_riego);    
    digitalWrite(pin_rele_valvula,HIGH);
    //arreglar que no se active constantemente
    desactivaRiego=1;  
  }
  
  
  
}//_______________________________________________________________________________________________________________________________________________________________________________




//__________________________________________________________________________________________________________________________________________________________________________________________________________________
//______________________________FUNCIONES DEL MENU_________________________________________________________________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________________________________________________________________________________________________________


void menu_Principal(){  //_________________________________________________________________________________________________________________________________________Menu Principal 
  int posCursor=0;
  int selecMenu=-1;
  int opcionMenu=0;
  int MaxOpc=3;     //Numero maximo de item en el menu
  loop_menu=true;
  do{
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);        
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);
    imprimenu_Principal(posCursor,opcionMenu);     //Se imprime el menu y el cursor, segun la posicion de este   
   //La estructura if-else posiciona el cursor y define el parametro opcion Mrnu
    if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      posCursor=LCDMueveCursor(posCursor,'r');	// Se resta una posición al cursor LCDMueveCursor(posCursor, numero de opciones del menu, char s o r "operacion de suma o resta de posicion")
      if(opcionMenu==0) opcionMenu=MaxOpc;    //Modificacion del parametro opcionMenu
      else opcionMenu--;
    }
    else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      posCursor=LCDMueveCursor(posCursor,'s');
      if(opcionMenu==MaxOpc) opcionMenu=0; //Modificacion del parametro opcionMenu
      else opcionMenu++;
    }  
    else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      selecMenu = opcionMenu; // Al pulsar a la derecha (para acceder a esa opcion) se actualiza la opción de menú elegida según donde esta el cursor ahora.
    }				
    else if(resultado_boton==4){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      loop_menu=false;
    }
    // Según la opción elegida del menú "opcionMenu" se llama a otro menú o se cierra el menú actual:
    switch( selecMenu ){
      case 0:
        ProgramacionRiego();        
        loop_menu=true;
	break;
      case 1:
        lcd.clear();
        lcd.setCursor(0, 0); lcd.print("Se activa la iluminacion segun configuracion horaria");
        
        digitalWrite(pin_rele_valvula,LOW);
        digitalWrite(51,LOW);        
        digitalWrite(49,LOW);        
        digitalWrite(47,LOW);        
        digitalWrite(45,LOW);        
        
        delay(1000);
        digitalWrite(pin_rele_valvula,HIGH);
        digitalWrite(51,HIGH);        
        digitalWrite(49,HIGH);        
        digitalWrite(47,HIGH);        
        digitalWrite(45,HIGH); 
        loop_menu=true;
	break;
      case 2:
        VisualizacionParametros();
        loop_menu=true;
	break;
      case 3:
        menu_Ajustes();
        loop_menu=true;
        break;
    }  
    selecMenu=-1;  
  }while(loop_menu==true);  
  lcd.clear();
  lcd.setCursor(2, 0); lcd.print("Hasta pronto");
  delay(1000);
  analogWrite(pin_brilloLCD,0);
  lcd.noDisplay(); 
}

void imprimenu_Principal(int posCursor, int opcionMenu){
  lcd.clear();
//Cursor que indica la opción seleccionada:
  lcd.setCursor(0, posCursor); 
  lcd.write(byte(0));
//imprime las opciones del menu segun posicion del cursor
//  if(opcionMenu<=filamaxLCD){ 
    lcd.setCursor(1, 0); lcd.print("Conf. Riego");
    lcd.setCursor(1, 1); lcd.print("Conf. Luz");  
 // }
 // else{
    lcd.setCursor(1,2); lcd.print("Ver Parametros");  
    lcd.setCursor(1,3); lcd.print("Ajustes");  
 // }
  delay(200);
}//________________________________________________________________________________________________________________________________________________________________________



void ProgramacionRiego(){//_______________________________________________________________________________________________________________________________________________Menu Principal >> ProgramacionRiego
   int posCursor=0;
  int selecMenu=-1;
  int opcionMenu=0;
  int MaxOpc=1;     //Numero maximo de item en el menu 0 a n
  loop_menu=true;  
  do{
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);    
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);
    imprime_ProgramacionRiego(posCursor,opcionMenu);     //Se imprime el menu y el cursor, segun la posicion de este   
   //La estructura if-else posiciona el cursor y define el parametro opcion Mrnu
    if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      posCursor=LCDMueveCursor(posCursor, 'r');	// Se resta una posición al cursor LCDMueveCursor(posCursor, numero de opciones del menu, char s o r "operacion de suma o resta de posicion")
      if(opcionMenu==0) opcionMenu=MaxOpc;    //Modificacion del parametro opcionMenu
      else              opcionMenu--;
    }
    else if(resultado_boton==2){	//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras   
      posCursor=LCDMueveCursor(posCursor, 's');
      if(opcionMenu==MaxOpc) opcionMenu=0; //Modificacion del parametro opcionMenu
      else opcionMenu++;
    }  
    else if(resultado_boton==3)//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      selecMenu = opcionMenu;				// Al pulsar a la derecha (para acceder a esa opcion) se actualiza la opción de menú elegida según donde esta el cursor ahora.
    else if(resultado_boton==4){//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      loop_menu=false;
    }
    // Según la opción elegida del menú "opcionMenu" se llama a otro menú o se cierra el menú actual:
    switch( selecMenu ){
      case 0:
        conf_HoraYFecha_riego();
        loop_menu=true;
	break;
      case 1:
        conf_cantidad_agua();
        loop_menu=true;
	break;
    }
    selecMenu=-1;
  }while(loop_menu==true);
}

void imprime_ProgramacionRiego(int posCursor, int opcionMenu){//______________________________________________________________________________________________imprime_ProgramacionRiego
  lcd.clear();
  //Cursor que indica la opción seleccionada:
  lcd.setCursor(0, posCursor);
  lcd.write(byte(0));
  //imprime las opciones del menu
  lcd.setCursor(1, 0); lcd.print("Riego Programado");
  lcd.setCursor(1, 1); lcd.print("Conf. Cantidad Agua");
  delay(200);

}//_________________________________________________________________________________________________________________________________________________________________________________



void conf_HoraYFecha_riego(){//____________________________________________________________________________________________________________________________________Menu Principal >> ProgramacionRiego >> conf_hora_riego 
  loop_menu=true;
  seleccionador=0;
  int hora=0;
  int minuto=0;
  int dias=1;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Advertencia!");
  lcd.setCursor(0,1);
  lcd.print("El riego programado");
  lcd.setCursor(0,2);
  lcd.print("desactiva el riego");  
  lcd.setCursor(0,3);
  lcd.print("detectando humedad");  
  delay(3000); 
  do{
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);    
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Ingrese hora riego:");
    imprime_lahora(hora_riego,minuto_riego,0,0,1);    // imprime_lahora(hora,minuto,segundo,pos_x,pos_y)
    lcd.setCursor(0,2);
    lcd.print("Intervalo de dias:");                            
    lcd.setCursor(0,3);
    lcd.print(dias_riego);                            
    
    
    switch(seleccionador){
      case 0:  //_______________________________________Ajusta hora riego
        parpadeo(0, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          hora_riego=incrementador_numerico(hora,'s',0,23);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras 
          hora_riego=incrementador_numerico(hora,'r',0,23);
        }
        else if(resultado_boton==3){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=1;
        }   
        else if(resultado_boton==4){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          loop_menu=false; 
        }     
        break;
      case 1:  //_______________________________________Ajusta minuto riego
        parpadeo(3, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);      
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          minuto_riego=incrementador_numerico(minuto,'s',0,59);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          minuto_riego=incrementador_numerico(minuto,'r',0,59);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=2;
        }   
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=0; 
        }
       	break;
      case 2:  //_______________________________________Ajusta dias regado
        //parpadeo(0,3,1,cont_parpadeo,3);   //parpadeo(pos_x,pos_y,numero_de_celdas,cont_parpadeo,duracion)
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          dias_riego= incrementador_numerico(dias,'s',1,7);    //incrementador_numerico(variable_a_modificar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          dias_riego= incrementador_numerico(dias,'r',1,7);
        }
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Hora de riego");
          lcd.setCursor(0,1);
          lcd.print("modificada");                    
          imprime_lahora(hora_riego,minuto_riego,0,0,2);    // imprime_lahora(hora,minuto,segundo,pos_x, pos_y)
          lcd.setCursor(0,3);
          lcd.print("cada "); lcd.print(dias_riego); lcd.print(" dias");                
          lcd.setCursor(0,1);
          delay(3000);
          loop_menu=false;
        }
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=1; 
        }
       	break;
    }
  }while(loop_menu==true);
}


void conf_cantidad_agua(){//____________________________________________________________________________________________________________________________________Menu Principal >> ProgramacionRiego >> conf_hora_riego
 
  
}



void VisualizacionParametros(){  //______________________________________________________________________________________________________________________________Menu Principal >> Visualizacion de Parametros
  int posCursor=0;  
  loop_menu=true;
  seleccionador=0;
  int MaxOpc=2;
  do{ 
    //Estructura basica dentro del loop do-while, donde se captan parametros de sensores, tiempo de reposo, pulsacion de un boton, etc. por medio de funciones
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb); //se debe pasar el valor y pin de cada sensor
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);    
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);

    imprime_VisualizacionParametros(posCursor,seleccionador);     //Se imprime el menu y el cursor, segun la posicion de este   

    //A continuacion se modifican las variables definidas al principio de la funcion, dependiendo el boton que se presione
    if(resultado_boton==1){//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      posCursor=LCDMueveCursor(posCursor,'r');	// Se resta una posición al cursor LCDMueveCursor(posCursor, numero de opciones del menu, char s o r "operacion de suma o resta de posicion")
      if(seleccionador==0) seleccionador=MaxOpc;    //Modificacion del parametro seleccionador
      else seleccionador--;
    }
    else if(resultado_boton==2){	//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras    
      posCursor=LCDMueveCursor(posCursor,'s');
      if(seleccionador==MaxOpc) seleccionador=0; //Modificacion del parametro seleccionador
      else seleccionador++;
    }  
    else if(resultado_boton==3){//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    }				
    else if(resultado_boton==4){//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      loop_menu=false;
    }
  }while(loop_menu==true);
}


void imprime_VisualizacionParametros(int posCursor, int seleccion){
  char celcius=byte(1);
  
  String descripcion_parametro[5][2]= {{"Tºcaja:","C"},{"Hum amb:","%"},{"Tºamb:","C"},{"Nivel agua:","cm"} ,{"Hum tierra:","%"}};
  int variables_display [6] = {0 , sensor_T_caja , sensor_H_amb , sensor_T_amb , sensor_prox1 , sensor_hum_tierra};
  
  
  
  Serial.println(posCursor);
  //??modificar pos_y con posCursor, para que no haya que poner muchos if??  
  //imprime la fecha y hora
  if(posCursor==0){
    lcd.clear();    
    lcd.setCursor(0, 0);   
    lcd.print(myRTC.dayofmonth);
    lcd.print("/");             
    lcd.print(myRTC.month);     
    lcd.print("/");             
    lcd.print(myRTC.year);      
    lcd.print(" ");            
    lcd.print(myRTC.hours);     
    lcd.print(":");             
    lcd.print(myRTC.minutes);  
    lcd.setCursor(0, 1); lcd.print("T"); lcd.write(byte(1)); lcd.print("caja:");
    lcd.setCursor(14,1); lcd.print(sensor_T_amb-10.0);
    lcd.setCursor(16,1); lcd.print(" C  "); 
    lcd.setCursor(0, 2); lcd.print("Humedad:");
    lcd.setCursor(14,2); lcd.print(sensor_H_amb); 
    lcd.setCursor(16,2); lcd.print("%"); 
    lcd.setCursor(0, 3); lcd.print("T amb:");
    lcd.setCursor(14,3); lcd.print(sensor_T_amb); 
    lcd.setCursor(16,3); lcd.print(" C");
  }
  
  else if(posCursor==1){
    lcd.clear();  
    lcd.setCursor(0, 0); lcd.print("T"); lcd.write(byte(1)); lcd.print("caja:");
    lcd.setCursor(14,0); lcd.print(sensor_T_amb-10.0);
    lcd.setCursor(16,0); lcd.print(" C  "); 
    lcd.setCursor(0, 1); lcd.print("Humedad:");
    lcd.setCursor(14,1); lcd.print(sensor_H_amb); 
    lcd.setCursor(16,1); lcd.print("%"); 
    lcd.setCursor(0, 2); lcd.print("T"); lcd.write(byte(1)); lcd.print("amb:");
    lcd.setCursor(14,2); lcd.print(sensor_T_amb); 
    lcd.setCursor(16,2); lcd.print(" C");
    lcd.setCursor(0, 3); lcd.print("Nivel Agua:");
    lcd.setCursor(14,3); lcd.print(sensor_prox1); 
    lcd.setCursor(16,3); lcd.print(" cm"); 
  }
  else if(posCursor==2){
    lcd.clear();    
    lcd.setCursor(0, 0); lcd.print("Humedad:");
    lcd.setCursor(14,0); lcd.print(sensor_H_amb); 
    lcd.setCursor(16,0); lcd.print("%"); 
    lcd.setCursor(0, 1); lcd.print("T"); lcd.write(byte(1)); lcd.print("amb:");
    lcd.setCursor(14,1); lcd.print(sensor_T_amb); 
    lcd.setCursor(16,1); lcd.print(" C");
    lcd.setCursor(0, 2); lcd.print("Nivel Agua:");
    lcd.setCursor(14,2); lcd.print(sensor_prox1);
    lcd.setCursor(16,2); lcd.print(" cm");
    lcd.setCursor(0, 3); lcd.print("Hum. Tierra:");
    lcd.setCursor(14,3); lcd.print(sensor_hum_tierra);
    lcd.setCursor(16,3); lcd.print("%");
  }
  delay(200);
} //_______________________________________________________________________________________________________________________________________________________________________________


void menu_Ajustes(){  //__________________________________________________________________________________________________________________________________________Menu Principal >> Menu Ajuste
  int posCursor=0;
  int selecMenu=-1;
  int opcionMenu=0;
  int MaxOpc=1;     //Numero maximo de item en el menu 0 a n
  loop_menu=true;  
  do{
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);    
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);
    imprimenu_Ajustes(posCursor,opcionMenu);     //Se imprime el menu y el cursor, segun la posicion de este   
   //La estructura if-else posiciona el cursor y define el parametro opcion Mrnu
    if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      posCursor=LCDMueveCursor(posCursor, 'r');	// Se resta una posición al cursor LCDMueveCursor(posCursor, numero de opciones del menu, char s o r "operacion de suma o resta de posicion")
      if(opcionMenu==0) opcionMenu=MaxOpc;    //Modificacion del parametro opcionMenu
      else              opcionMenu--;
    }
    else if(resultado_boton==2){	//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras   
      posCursor=LCDMueveCursor(posCursor, 's');
      if(opcionMenu==MaxOpc) opcionMenu=0; //Modificacion del parametro opcionMenu
      else opcionMenu++;
    }  
    else if(resultado_boton==3)//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      selecMenu = opcionMenu;				// Al pulsar a la derecha (para acceder a esa opcion) se actualiza la opción de menú elegida según donde esta el cursor ahora.
    else if(resultado_boton==4){//0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      loop_menu=false;
    }
    // Según la opción elegida del menú "opcionMenu" se llama a otro menú o se cierra el menú actual:
    switch( selecMenu ){
      case 0:
        conf_hora();
        loop_menu=true;
	break;
      case 1:
        conf_fecha();
        loop_menu=true;
	break;
    }
    selecMenu=-1;
  }while(loop_menu==true);
}  

void imprimenu_Ajustes(int posCursor, int opcionMenu){
  lcd.clear();
  //Cursor que indica la opción seleccionada:
  lcd.setCursor(0, posCursor);
  lcd.write(byte(0));
  //imprime las opciones del menu
  lcd.setCursor(1, 0); lcd.print("Conf. hora");
  lcd.setCursor(1, 1); lcd.print("Conf. fecha");
  delay(200);
}//________________________________________________________________________________________________________________________________________________________________________


void conf_hora(){//_______________________________________________________________________________________________________________________________________________Ajustes >> Configuracion Hora
  loop_menu=true;
  seleccionador=0;
  
  //Variables de reloj RTC
  int segundo=myRTC.seconds;
  int minuto=myRTC.minutes;
  int hora=myRTC.hours;
  do{
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);    
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Ingrese la hora:");
    imprime_lahora(hora,minuto,segundo,0,1);    // imprime_lahora(hora,minuto,segundo,pos_x,pos_y)
    switch(seleccionador){
      case 0:  //_______________________________________hora
        parpadeo(0, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          hora=incrementador_numerico(hora,'s',0,23);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras 
          hora=incrementador_numerico(hora,'r',0,23);
        }
        else if(resultado_boton==3){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=1;
        }   
        else if(resultado_boton==4){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          loop_menu=false; 
        }     
        break;
      case 1:  //_______________________________________minuto
        parpadeo(3, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);      
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          minuto=incrementador_numerico(minuto,'s',0,59);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          minuto=incrementador_numerico(minuto,'r',0,59);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=2;
        }   
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=0; 
        }
       	break;
      case 2:  //_______________________________________segundo
        parpadeo(6, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          segundo=incrementador_numerico(segundo,'s',0,59);    //incrementador_numerico(variable_a_modificar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          segundo=incrementador_numerico(segundo,'r',0,59);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          myRTC.setDS1302Time(segundo,minuto,hora,myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year);//myRTC.seconds, myRTC.minutes, hourUpdate, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year
          lcd.clear();
          lcd.setCursor(0,1);
          lcd.print("Hora modificada");                    
          imprime_lahora(hora,minuto,segundo,0,0);    // imprime_lahora(hora,minuto,segundo,pos_y,pos_x)
          delay(2000);
          loop_menu=false;
        }
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=1; 
        }
       	break;
    }
  }while(loop_menu==true);
}


void conf_fecha(){//_________________________________________________________________________________________________________________________________________________Ajustes >> Configuracion de Fecha
  loop_menu=true;
  seleccionador=0;
  int dia=myRTC.dayofmonth;
  int mes=myRTC.month;
  int ano=myRTC.year;
  int dia_semana=myRTC.dayofweek;
  do{
    LecturaSensores();//sensor_HyT_amb, pin_sensor_HyT_amb, sensor_T_amb, pin_sensor_T_amb);
    reposoLCD(cont_reposoLCD, seg, seg_ant,loop_menu);    
    resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    presionarBoton(cont_reposoLCD);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Ingrese la fecha:");
    imprime_lafecha(dia,mes,ano,dia_semana,0,1);    // imprime_lahora(hora,minuto,segundo,pos_x,pos_y)
    switch(seleccionador){
      case 0:  //_______________________________________dia_semana
        parpadeo(0, 1, 3,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          dia_semana=incrementador_numerico(dia_semana,'s',1,7);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          dia_semana=incrementador_numerico(dia_semana,'r',1,7);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=1;
        }   
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          loop_menu=false; 
        }     
        break;
      case 1:  //_______________________________________dia
        parpadeo(4, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,contador,duracion_parpadeo)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          dia=incrementador_numerico(dia,'s',1,31);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          dia=incrementador_numerico(dia,'r',1,31);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=2;
        }   
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=0; 
        }
       	break;
      case 2:  //_______________________________________mes
        parpadeo(7, 1, 2,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,contador,duracion_parpadeo)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          mes=incrementador_numerico(mes,'s',1,12);    //incrementador_numerico(variable_a_incrementar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          mes=incrementador_numerico(mes,'r',1,12);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=3;
        }   
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=1; 
        }
       	break;       
      case 3:  //_______________________________________ano
        parpadeo(10, 1, 4,cont_parpadeo, 2);   //parpadeo(pos_x,pos_y,numero_de_celdas,duracion)
        delay(200);
        if(resultado_boton==1){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          ano=incrementador_numerico(ano,'s',2000,2099);    //incrementador_numerico(variable_a_modificar, suma_o_resta, limite_inferior, limite_superior);
        }
        else if(resultado_boton==2){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          ano=incrementador_numerico(ano,'r',2000,2099);
        }  
        else if(resultado_boton==3){ //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          myRTC.setDS1302Time(myRTC.seconds, myRTC.minutes, myRTC.hours,dia_semana,dia,mes,ano);//myRTC.seconds, myRTC.minutes, myRTC.hours, myRTC.dayofweek, myRTC.dayofmonth, myRTC.month, myRTC.year
          lcd.clear();
          lcd.setCursor(0,1);
          lcd.print("Fecha modificada");                    
          imprime_lafecha(dia,mes,ano,dia_semana,0,0);    // imprime_lahora(hora,minuto,segundo,pos_x,pos_y)          
          delay(2000);
          loop_menu=false;
        }
        else if(resultado_boton==4){  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
          seleccionador=2; 
        }
       	break;
    }
  }while(loop_menu==true);
}//_________________________________________________________________________________________________________________________________________________________________________________


//__________________________________________________________________________________________________________________________________________________________________________________________________________________
//______________________________FUNCIONES ESTRUCTURALES____________________________________________________________________________________________________________________________________________________________________________
//__________________________________________________________________________________________________________________________________________________________________________________________________________________

void imprime_lahora(int hora,int minuto, int segundo,int pos_x,int pos_y){//____________________________________________________________________________________________Impreime la Hora
  lcd.setCursor(pos_x,pos_y);
  if(hora<10){  //imprime la hora
    lcd.print("0");
    lcd.print(hora);
  }else{
    lcd.print(hora);
  }
  lcd.print(":");
  if(minuto<10){  //imprime minuto
    lcd.print("0");
    lcd.print(minuto);
  }else{
    lcd.print(minuto);
  }
  lcd.print(":");
  if(segundo<10){  //imprime minuto
    lcd.print("0");
    lcd.print(segundo);
  }else{
    lcd.print(segundo);
  }
}//_________________________________________________________________________________________________________________________________________________________________________________

void imprime_lafecha(int dia,int mes, int ano, int dia_semana,int pos_x,int pos_y){  //_____________________________________________________________________________Imprime la fecha
  lcd.setCursor(pos_x,pos_y);
  switch(dia_semana){
    case 1:
      lcd.print("dom ");
      break;
    case 2:
      lcd.print("lun ");
      break;
    case 3:
      lcd.print("mar ");
      break;
    case 4:
      lcd.print("mie ");
      break;
    case 5:
      lcd.print("jue ");
      break;
    case 6:
      lcd.print("vie ");
      break;
    case 7:
      lcd.print("sab ");
      break;
  }  
  if(dia<10){  //imprime la hora
    lcd.print("0");
    lcd.print(dia);
  }else{
    lcd.print(dia);
  }
  lcd.print("/");
  if(mes<10){  //imprime minuto
    lcd.print("0");
    lcd.print(mes);
  }else{
    lcd.print(mes);
  }
  lcd.print("/");
  lcd.print(ano);
}//_________________________________________________________________________________________________________________________________________________________________________________


void parpadeo(int pos_x,int pos_y,int celdas,int &cont_parpadeo_2,int duracion){//_______________________________________________________________________________________Parpadeo  
  //parpadeo(int pos_x,int pos_y,numero_de_celdas_parpadean,contador_parpadeo, duracion_del_parpadeo)
  if(cont_parpadeo_2<duracion){
      cont_parpadeo_2++;
  }
  else if(cont_parpadeo_2>=duracion && cont_parpadeo_2<=duracion*2){
    for(int i=0;i<celdas;i++){
      lcd.setCursor(pos_x+i,pos_y);
      lcd.print(" ");
    }
    cont_parpadeo_2++;
  }
  else cont_parpadeo_2=0;
}//_________________________________________________________________________________________________________________________________________________________________________________



int incrementador_numerico(int var, char ope, int minimo, int maximo){//________________________________________________________________________________________________Incrementador Numerico
  //incrementador_numerico(variable_modificada, operacion_[+ o -], valor_minimo_var, valor_maximo_var)
  if(ope =='s'){
    if(var==maximo) var=minimo;
    else var++;
  }
  else if(ope =='r'){
    if(var==minimo) var=maximo;
    else var--;
  }
  return var;
}//_________________________________________________________________________________________________________________________________________________________________________________



void reposoLCD(int &cont_reposoLCD_2, int &seg_2, int &seg_ant_2,boolean &loop_menu_2){ //_______________________________________________________________________________Rposo LCD
  //Funcion que apaga el LCS si pasan N seg. sin presionar un boton
  seg_2=myRTC.seconds;
  if(seg_2!=seg_ant_2){
    cont_reposoLCD_2++;
    seg_ant_2=seg_2;
    if(cont_reposoLCD_2 >60){
      cont_reposoLCD_2=0;
      loop_menu_2=false;
    }
    else loop_menu_2=true; 
  }
}//_________________________________________________________________________________________________________________________________________________________________________________



int LCDMueveCursor(int pos, char ope){//_______________________________________________________________________________________________________________________________LCD Mueve Cursor
//char ope debería de ser el carácter 's' o 'r' para sumar o restar una posición. int pos es la posicion actual del cursor
  if(ope =='s'){
    if(pos==filamaxLCD) pos=0;
    else pos=pos+1;
  }  
  else if(ope =='r'){
    if(pos==0) pos=filamaxLCD;
    else pos=pos-1;
  }


  return pos;
}//____________________________________________________________________________________________________________________________________________________________________________________

void actualizaEncoder(){//____________________________________________________________________________________________________________________________________________Presiona Encoder
  //Deteccion del movimiento del encoder EC11
  int MSB = digitalRead(pin_encoder1); //MSB = most significant bit
  int LSB = digitalRead(pin_encoder2); //LSB = least significant bit
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

//  Serial.print(encoderValue);
//  Serial.print(" - ");
//  Serial.println(encoderValue);

//Codifica el resultado del boton y almacena en char resultadBoton
  encoderNavegador= (encoderValue);
  if(encoderNavegador<last_encoderNavegador){
    cont_encoder=cont_encoder-0.25;
  }
  if(encoderNavegador>last_encoderNavegador){
    cont_encoder=cont_encoder+0.25;
  }
  cont_encoder2=round(cont_encoder);
  if(cont_encoder2>last_cont_encoder2){
    resultado_boton=2;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    Serial.println("abajo");
  }
  if(cont_encoder2<last_cont_encoder2){
    resultado_boton=1;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
    Serial.println("arriba");
  }

  
  last_encoderNavegador=encoderNavegador;
  lastEncoded = encoded; //store this value for next time
  last_cont_encoder2=cont_encoder2;
}//____________________________________________________________________________________________________________________________________________________________________________________


void presionarBoton(int &cont_reposoLCD_2){ //_______________________________________________________________________________________________________________________Presionar Boton
  // PROCEDIMIENTO QUE COMPRUEBA SI HA HABIDO NUEVAS PULSACIONES CON LOS BOTONES:  
  int tolerancia=10;
  int voltaje_select=600;
  int voltaje_atras=535;
  int voltaje_otro=500;
  int data = analogRead(pin_botones_analog);   
  if (data > 450){
    Serial.println(data);
    cont_reposoLCD_2=0; //el contador de la funcion reposo LCD vuelve a 0 ya que se presiono un boton
    if (data > voltaje_atras - tolerancia && data < voltaje_atras + tolerancia) {
      resultado_boton=4;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      Serial.println("atras");}
    else if (data > voltaje_select-tolerancia && data < voltaje_select+tolerancia){
      resultado_boton=3;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      Serial.println("select");}
    else if (data > voltaje_otro-tolerancia && data < voltaje_otro+tolerancia){
      resultado_boton=0;  //0:nulo, 1:arriba, 2:abajo, 3:selecionar, 4:atras
      Serial.println("nulo");}
      
    lcd.setCursor(17,0); 
    lcd.print("   ");
    lcd.setCursor(17,0);
    lcd.print(data);
    delay(200);      
  }
  

}//____________________________________________________________________________________________________________________________________________________________________________________
//_______________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________________


cambio_2
