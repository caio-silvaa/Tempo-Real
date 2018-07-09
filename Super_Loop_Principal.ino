//INCLUI BIBLIOTECAS
#include "U8glib.h"

//DEFINE PORTAS DO ARDUINO UNO
#define Interrup_Rot 2
#define Comb 3
#define Comb_Led 4
#define Temp_Alarm 5
#define LM35 A3

//DEFINE CONSTANTE PARA ALARME DE TEMPERATURA
#define Temp_Max 30

//VARIAVEIS GLOBAIS
float Temperatura = 0;
float Rotacao = 0;
volatile int Pulso_RPM = 0;
int Comparador_Comb = 0;
int Registrador_Comb = 0;

//TEMPO USADO PARA CALCULAR ROTAÇÃO
unsigned long Tempo_Atual = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
unsigned long Tempo_Anterior = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU

/*
  unsigned long Tempo_Atual_Analise = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
  unsigned long Tempo_Anterior_Temperatura = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
  unsigned long Tempo_Anterior_Combustivel = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
  unsigned long Tempo_Anterior_Rotacao = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
  unsigned long Tempo_Anterior_Display = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU

  int Tempo_Temperatura = 0;
  int Tempo_Combustivel = 0;
  int Tempo_Rotacao = 0;
  int Tempo_Display = 0;

*/

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK); //CRIA DISPLAY U8G

//DECLARA FUNÇÃO DE DESENHO NO DISPLAY
void draw()
{
  //Comandos graficos para o display devem ser colocados aqui
  //Seleciona a fonte de texto
  u8g.setFont(u8g_font_8x13B);
  //Linha superior - temperatura
  u8g.drawStr( 5, 15, "BCC722 - VQV");


  //IMPRIME A TEMPERATURA
  u8g.setFont(u8g_font_fub20);
  u8g.setPrintPos (3 , 61);
  u8g.print(Temperatura);
  u8g.setFont(u8g_font_fub20);
  u8g.drawCircle(95, 45, 3);
  u8g.drawStr( 100, 61, "C");

  //IMPRIME A ROTAÇÃO
  u8g.setFont(u8g_font_fub20);
  u8g.drawStr( 68, 37, "RPM");
  u8g.setFont(u8g_font_fub20);
  if (Rotacao < 10) {
    u8g.setPrintPos (50, 37);
  } else if ((Rotacao < 100 ) && (Rotacao > 9)) {
    u8g.setPrintPos (35 , 37);
  } else if ((Rotacao < 1000 ) && (Rotacao > 99)) {
    u8g.setPrintPos (20 , 37);
  } else {
    u8g.setPrintPos (5 , 37);
  }
  u8g.print(Rotacao, 0);

  //DESENHA RETANGULOS
  u8g.drawRFrame(0, 39, 128, 23, 2);
  u8g.drawRFrame(0, 16, 128, 23, 2);

}

void setup(void)
{
  //INICIA COMUNICAÇÃO SERIAL
  Serial.begin(9600);

  //CONFIGURAÇÃO DE I/O
  pinMode(Interrup_Rot, INPUT_PULLUP);
  pinMode(Comb, INPUT);
  pinMode(Comb_Led, OUTPUT);
  pinMode(Temp_Alarm, OUTPUT);
  pinMode(LM35, INPUT);

  //DECLARA INTERRUPÇÃO
  attachInterrupt(digitalPinToInterrupt(Interrup_Rot), F_RPM, FALLING); //HABILITA INTERRUPÇÃO NO PINO 20

  //DISPLAY
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255, 255, 255);
  }
}

void loop(void)
{
  Ler_Temperatura();//CHAMA FUNÇÃO TEMPERATURA
  Alarme_Temperatura();//CHAMA FUNÇÃO DE ALARME DE TEMPERATURA
  Ler_Combustivel();//CHAMA FUNÇÃO DE COMBUSTIVEL
  Alarme_Combustivel();//CHAMA FUNÇÃO DE ALARME DE COMBUSTIVEL
  Ler_Rotacao();//CHAMA FUNÇÃO PARA LER ROTAÇÃO
  Display();//CHAMA FUNÇÃO PARA IMRIMIR NO DISPLAY
  delay(250);//DELAY PARA AJUSTE NA LEITURA DA ROTAÇÃO
}

void F_RPM(void) { //FUNÇÃO DE TRATAMENTO DA INTERRUPÇÃO
  Pulso_RPM++;  //QUANDO DETECTAR UM PULSO NA PORTA 2 , ACRESCENTA A CONTAGEM DE PULSOS
}

//FUNÇÃO PARA LER TEMPERATURA
void Ler_Temperatura() {

  // Tempo_Atual_Analise = millis();
  // Tempo_Temperatura = Tempo_Atual_Analise - Tempo_Anterior_Temperatura;

  Temperatura = 0;//ZERA A VARIAVEL GLOBAL

  //REALIZA 500 LEITURA PARA ARMENIZAR A VARIAÇÃO
  for (int i = 1; i < 501; i++) {
    Temperatura = Temperatura + analogRead(LM35);//LER E SOMA VALOR

  }
  Temperatura = Temperatura / 500;//TIRA MEDIA
  Temperatura = (Temperatura * 5 / 1023) / 0.01;//CONVERTE TENSAO PARA TEMPERATURA
}

//FUNÇÃO PARA ALARME DE TEMPERATURA
void Alarme_Temperatura() {

  //VERIFICA SE É MAIOR QUE A TEMPERATURA PERMITIDA
  if (Temperatura > Temp_Max) {
    digitalWrite(Temp_Alarm, HIGH);
  } else {
    digitalWrite(Temp_Alarm, LOW);
  }

  //  Tempo_Anterior_Temperatura = millis();
}

//FUNÇÃO PARA FAZER AS LEITURAS DO SENSOR DE COMBUSTIVEL
void Ler_Combustivel() {
  //Tempo_Atual_Analise = millis();
  //Tempo_Combustivel = Tempo_Atual_Analise - Tempo_Anterior_Combustivel;

  //CONTADOR INCREMENTAL
  Comparador_Comb++;//
  if (digitalRead(Comb) == HIGH) {

    Registrador_Comb++;//REGISTRA SE NIVEL NO TANQUE FOI BAIXO = NIVEL ALTO NA PORTA DIGITAL
  }
}

//FUNÇÃO PARA ALARME DE COMBUSTIVEL
void Alarme_Combustivel() {
  //Serial.print("Comparador_Comb: ");Serial.println(Comparador_Comb);
  //Serial.print("Registrador_Comb: ");Serial.println(Registrador_Comb);

  //SE COONTADOR CHEGAR A 100
  if (Comparador_Comb > 100) {

    //SE LEITURA DE NIVEL BAIXO NO TANQUE FOR MAIOR QUE 30% ENTAO SINALIZA
    if (((Comparador_Comb - Registrador_Comb) < 30)) {
      digitalWrite(Comb_Led, HIGH);
    } else {
      digitalWrite(Comb_Led, LOW);
    }

    //ZERA VARIAVEIS
    Comparador_Comb = 0;
    Registrador_Comb = 0;
  }
  //Tempo_Anterior_Combustivel = millis();
}

//FUNÇÃO PARA IMPRIMIR DADOS
void Display() {
  //Tempo_Atual_Analise = millis();
  //Tempo_Display = Tempo_Atual_Analise - Tempo_Anterior_Display;

  //RODA PRIMEIRA PAGINA DO OLED
  u8g.firstPage();
  do
  {
    draw();//CHAMA FUNÇÃO DE DESENHO
  } while ( u8g.nextPage() );//FICA ATUALIZANDO A PAGINA

  //Serial.print(Tempo_Display);Serial.print(" ; ");Serial.print(Tempo_Temperatura);Serial.print(" ; ");Serial.print(Tempo_Combustivel);Serial.print(" ; ");Serial.println(Tempo_Rotacao);
  delay(50);//DELAY PARA CARREGAR PAGINA
  //Tempo_Anterior_Display = millis();
}

//FUNÇÃO PARA ROTAÇÃO
void Ler_Rotacao() {

  // Tempo_Atual_Analise = millis();
  // Tempo_Rotacao = Tempo_Atual_Analise - Tempo_Anterior_Rotacao;
  Tempo_Atual = millis();//REGISTRA TEMPO
  noInterrupts(); //DESABILITA A INTERRUPÇÃO
  float Tempo_Decorrido = Tempo_Atual - Tempo_Anterior; //ARMAZENA TEMPO QUE PASSOU CONTANDO PULSOS

  if (Tempo_Decorrido > 0) {//REDUNTANCIA
    Rotacao = (1000 / Tempo_Decorrido);//CALCULA PORCENTAGEM ONDE 1S É 100%
    Rotacao = Rotacao * float(Pulso_RPM) * 60   ; //CONVERTE PARA PULSOS POR MINUTO ONDE 1 PULSO É UMA VOLTA
    Pulso_RPM = 0;//ZERA VARIAVEL DA FUNÇÃO DE INTERRUPÇÃO
    Tempo_Anterior = Tempo_Atual;//ATUALIZA TEMPO QUE JA PASSOU

  }
  //Tempo_Anterior_Rotacao = millis();
  interrupts(); //HABILITA INTERRUPÇÃO NOVAMENTE
}




