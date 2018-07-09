//INCLUSAO DE BIBLIOTECAS
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <queue.h>

//DEFINIÇÃO DAS PORTAS DO ARDUINO
#define Interrup_Rot 2 //USO PARA INTERRUPÇÃO 
#define Comb 3 //USO PARA LER SENSOR DE COMBUSTIVEL
#define Comb_Led 4//USO PARA ACIONAR LED DE ALERTA DE FALTA DE COMBUSTIVEL
#define Temp_Alarm 5//USO PARA ACIONAR LED DE ALERTA PARA TEMPERATURA ALTA
#define LM35 A3// USO PARA LER SENSOR DE TEMPERATURA

//DEFINIÇÃO DAS CONSTANTES
#define Temp_Max 30 //TEMPERATURA MAXIMA PARA ACIONAR O ALERTA (MANTEVE SE BAIXA PARA SER POSSIVEL TESTE EM BANCADA)
#define QUEUE_ITEM_SIZE 2 //TAMANHO EM BYTES DOS ITENS NA FILA DE MENSAGENS
#define QUEUE_LENGTH    2//QUANTIDADE DE ITENS NA FILA DE MENSAGENS

//DECLARAÇÃO DAS FUNÇOES QUE SERÃO CRIADAS PELO KERNEL
void TaskLer_Temperatura( void *pvParameters );
void TaskLer_Combustivel( void *pvParameters );
void TaskRotacao( void *pvParameters );
void TaskEnviar( void *pvParameters );

//VARIAVEIS GLOBAIS PARA GUARDAR O ENDEREÇO DA FILA DE MENSAGENS
int Fila_Rotacao ;//
int Fila_Combustivel ;
int Fila_Temperatura ;

//VARIAVEL GLOBAL PARA SER USADA NA FUNÇÃO DE TRATAMENTO DA INTERRUPÇÃO
volatile int Pulso_RPM = 0;


unsigned long Tempo_Anterior_Rotacao = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
/*
  unsigned long Tempo_Anterior_Combustivel = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
  unsigned long Tempo_Anterior_Temperatura = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
  unsigned long Tempo_Anterior_Enviar = 0; // ARMAZENA O QUANTO TEMPO JA SE PASSOU
*/

void setup() {
  //INICIA COMUNICAÇÃO SERIAL
  Serial.begin(9600);

  //DECLARA A INTERRUPÇÃO
  attachInterrupt(digitalPinToInterrupt(Interrup_Rot), F_RPM, FALLING); //HABILITA INTERRUPÇÃO NO PINO 2

  //CRIA AS TAREFAS A SEREM EXECUTADAS
  xTaskCreate(
    TaskLer_Temperatura
    ,  (const portCHAR *)"Ler_Temperatura"
    ,  128
    ,  NULL
    ,  2
    ,  NULL );

  xTaskCreate(
    TaskLer_Combustivel
    ,  (const portCHAR *) "Ler_Combustivel"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );

  xTaskCreate(
    TaskRotacao
    ,  (const portCHAR *) "Rotacao"
    ,  128  // Stack size
    ,  NULL
    ,  1 // Priority
    ,  NULL );
  xTaskCreate(
    TaskEnviar
    ,  (const portCHAR *) "Enviar"
    ,  128  // Stack size
    ,  NULL
    ,  3  // Priority
    ,  NULL );

  //CRIAS AS FILAS DE MENSAGENS
  Fila_Rotacao = xQueueCreate(QUEUE_LENGTH, 4);//MUDADO TAMANHO PARA 4 BYTE POIS VARIAVEL É FLOAT
  Fila_Combustivel = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE) ;
  Fila_Temperatura = xQueueCreate(QUEUE_LENGTH, QUEUE_ITEM_SIZE);

  Serial.print("id fila Fila_Rotacao = "); Serial.println(Fila_Rotacao);
  Serial.print("id fila Fila_Combustivel = "); Serial.println(Fila_Combustivel);
  Serial.print("id fila Fila_Temperatura = "); Serial.println(Fila_Temperatura);

}
void loop()
{
  //empyt

}

//FUNÇÃO DE TRATAMENTO DA INTERRUPÇÃO
void F_RPM(void) {
  Pulso_RPM++;  //QUANDO DETECTAR UM PULSO NA PORTA 2 , ACRESCENTA A CONTAGEM DE PULSOS
}

//TASK PARA TEMPERATURA
void TaskLer_Temperatura( void *pvParameters __attribute__((unused)) )
{
  //DECLARA A CONFIGURAÇÃO DE PORTA
  pinMode(Temp_Alarm, OUTPUT);
  pinMode(LM35, INPUT);

  int Temperatura;//VARIAVEL PARA ARMEZANAR A TEMPERATURA

  // unsigned long Tempo_Atual;
  //int Tempo_De_Chamada;

  for (;;) // A Task shall never return or exit.
  {
    //Tempo_Atual = micros();
    // Tempo_De_Chamada = Tempo_Atual - Tempo_Anterior_Temperatura;

    Temperatura = 0;//ZERA REGISTRO ANTERIOR

    //REALIZA 500 LEITURAS  INSTANTANEAS PARA ARMENIZAR VARIAÇÕES DE VALORES
    for (int i = 1; i < 501; i++) {
      Temperatura = Temperatura + analogRead(LM35); //LER E ARMAZENA VALOR
    }
    Temperatura = Temperatura / 500;//TIRA A MEDIA DOS VALORES LIDO
    Temperatura = (Temperatura * 5 * 100 / 1023);//CONVERTE VALOR DE TENSÃO PARA TEMPERATURA

    //ATUAÇÃO DO ALARME
    if (Temperatura >= Temp_Max) {
      digitalWrite(Temp_Alarm, HIGH);
    } else {
      digitalWrite(Temp_Alarm, LOW);
    }

    // Serial.println(Temperatura);
    //vTaskDelay(1);

    //ESCREVE VALOR NA FILA DE MENSAGENS
    xQueueSend(
      Fila_Temperatura,
      &Temperatura,
      10
    );
    //Tempo_Anterior_Temperatura = micros();
  }
}

//TASK PARA COMBUSTIVEL
void TaskLer_Combustivel( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  //VARIAVEIS PARA COMPARAR A VARIAÇÃO DO NIVEL INTERNO
  int Registrador_Comb = 0;
  int Comparador_Comb = 0;
  //unsigned long Tempo_Atual;
  //int Tempo_De_Chamada;

  //CONFIGURAÇÃO DE PORTA
  pinMode(Comb, INPUT);
  for (;;)
  {
    /*
      while (1) { //SIMULAÇÃO DE TRAVAMENTO
      //empty
      }*/
    // Tempo_Atual = micros();
    // Tempo_De_Chamada = Tempo_Atual - Tempo_Anterior_Combustivel;

    //REGISTRADOR DE REFERENCIA , SEMPRE CONTABILIZA
    Comparador_Comb++;

    //REGISTRA APENAS QUANDO NÃO HOUVE COMBUSTIVEL = NIVEL ALTO NA ENTRADA DIGITAL
    if (digitalRead(Comb) == HIGH) {
      Registrador_Comb++;
    }

    //SE ATINGIU NUMERO DE  REGISTROS TOTAL PARA COMPARAÇÃO
    if (Comparador_Comb >= 50) {

      //SE O LIQUIDO ESTEVE EM NIVEL BAIXO NO TANQUE = NIVEL ALTO NA PORTA , MAIS DE 50% ENTÃO ACIONA O ALARME
      if (((Comparador_Comb - Registrador_Comb) < 25)) {
        digitalWrite(Comb_Led, HIGH);
      } else {
        digitalWrite(Comb_Led, LOW);
      }

      //ENVIA DADOS PARA A FILA DE MENSAGENS
      xQueueSend(
        Fila_Combustivel,
        &Registrador_Comb,
        10
      );

      //ZERA VARIAVEIS DE REGISTRO
      Comparador_Comb = 0;
      Registrador_Comb = 0;
    }

     vTaskDelay(1);


    //Tempo_Anterior_Combustivel = micros();
  }
}

//TASK PARA LER A ROTAÇÃO
void TaskRotacao( void *pvParameters __attribute__((unused)) )  // This is a Task.
{

  //VARIAVEIS LOCAIS
  float Rotacao = 0;//ARMAZENA A ROTAÇÃO
  unsigned long Tempo_Atual;//USADOS PARA ARMAZENAR OS TEMPOS
  int Tempo_De_Chamada;
  for (;;)
  {
    Tempo_Atual = millis();
    noInterrupts(); //DESABILITA A INTERRUPÇÃO
    Tempo_De_Chamada = ( Tempo_Atual - Tempo_Anterior_Rotacao);//DETERMINA O TEMPO QUE SE FICOU CONTANDO OS PULSOS

    if (Tempo_De_Chamada > 0) {//REDUNDANCIA DE INFORMAÇÃO
      Rotacao = (1000 / Tempo_De_Chamada);//PEGA A PORCENTAGEM DE TEMPO PARA 1S COMO 100%
      Rotacao = Rotacao * float(Pulso_RPM) * 60   ;//MULTIPLICA PELA QUANTIDADE DE PULSOS PARA SABER PULSOS POR SEGUNDO E DEPOIS POR 60 PARA PULSOS POR MINUTO,O MOTOR É 1/1 , LOGO, UM PULSO UMA VOLTA
      Pulso_RPM = 0;//ZERA O REGITRADOR DE PULSOS

      //Serial.println(Rotacao);
    }

    //ENVIA DADOS PARA FILA DE MENSAGENS
    xQueueSend(
      Fila_Rotacao,
      &Rotacao,
      10
    );

    //DÁ UM TEMPO MINIMO PARA MELHORAR A LEITURA DA ROTAÇÃO
    vTaskDelay(15);
    Tempo_Anterior_Rotacao = Tempo_Atual;//SALVA O ULTIMO TEMPO LIDO
    interrupts(); //HABILITA INTERRUPÇÃO NOVAMENTE

  }
}

//TASK PARA ENVIO DE DADOS VIA SERIAL
void TaskEnviar( void *pvParameters __attribute__((unused)) )  // This is a Task.
{
  //VARIAVEIS LOCAIS PARA ARMAZENAR VALORES RECEBIDOS DA FILA DE MENSAGENS
  int Tempo_Temp;
  int Tempo_Comb;
  float Tempo_Rotacao;
  // unsigned long Tempo_Atual;
  //int Tempo_De_Chamada_Enviar;

  for (;;)
  {
    // Tempo_Atual = micros();
    //Tempo_De_Chamada_Enviar = Tempo_Atual - Tempo_Anterior_Enviar;

    //LER DADOS DA FILA DE MENSAGENS , UMA FILA PRA CADA TAREFA
    xQueueReceive(Fila_Temperatura, &Tempo_Temp, 20);
    xQueueReceive(Fila_Combustivel, &Tempo_Comb, 20);
    xQueueReceive(Fila_Rotacao, &Tempo_Rotacao, 20);

    // Serial.print(Tempo_De_Chamada_Enviar);Serial.print(" ; ");
    //IMPRIMI DADOS LIDOS
    Serial.print(Tempo_Temp); Serial.print(" ; "); Serial.print(Tempo_Comb); Serial.print(" ; "); Serial.println(Tempo_Rotacao );
    //vTaskDelay(1);
    //Tempo_Anterior_Enviar = micros();
  }

}

