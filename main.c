#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "PID/pid.h"
#include "uart_com.h"
#include "crc/crc16.h"
#include "csv_pull.h"

#include <unistd.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <pthread.h>
#include <time.h>

int vent_gpio=24;
int res_gpio=23;

typedef struct {
  float reference;
	float internal_temp;
	float external_temp;
	int user_com;
  double ctrl_signal;
	double Kp, Ki, Kd;
	unsigned char ctrl_mode;
	unsigned char debug_mode;
	unsigned char poweron;
	unsigned char init;
}data_furn;

typedef enum __usr_comm_t{
	POWERON = 0xA1,
	SHUTDOWN,
	INIT_HEAT,
	CANCEL_PROCESS,
	MENU
}usr_comm_t;

data_furn data_ctrl={.reference=30.0,
	.external_temp=0.0,
	.debug_mode=0x00,
	.ctrl_signal=0.0,
	.ctrl_mode=0x00,
	.poweron=0x00,
	.init=0x00,
	.Kp=30.0,
	.Ki=0.2,
	.Kd=400.0,
};

pthread_t TR_request;
//pthread_t uart_polling;
pthread_t csv_polling;

FILE *fpt;


void pid_update_command(data_furn *ctrl){
	printf("\nInsira agora os valores para as constantes do PID:\n\n");
	printf("\nP (proporcional): ");	
	scanf("%lf", &ctrl->Kp);
	printf("\nI (integrativo): ");	
	scanf("%lf", &ctrl->Ki);
	printf("\nD (derivativo): ");	
	scanf("%lf", &ctrl->Kd);

	printf("\nValores lidos para as constantes PID:\n\n");
	printf("P = %lf\n", ctrl->Kp);
	printf("I = %lf\n", ctrl->Ki);
	printf("D = %lf\n", ctrl->Kd);
	pid_configura_constantes(ctrl->Kp, ctrl->Ki, ctrl->Kd);
}

void shutdown_furnance(int sig){
  // TODO: Desligar todos os periféricos e terminar
  // os processos
  printf("\nForno desligando :)\n");
	fclose(fpt);
	pthread_cancel(TR_request); //Fechando as threads
	//pthread_cancel(uart_polling); //Fechando as threads
	close(uart0_filestream);	//Fechando o UART
  exit(0);
}

void *update_TR_terminal(void *param){
	data_furn* ctrl = (data_furn*) param;
	int aux;
	while(1){
		printf("\nDigite: 1 - para atualizar a Temperatura de Referência (TR);\n");
		printf("2 - para atualizar as constantes PID;\n");
		printf("3 - para ver o status das variaveis;\n");
		scanf("%d", &aux);
		switch(aux){
			case 1:
				if(ctrl->debug_mode==0x01){
					printf("\nA temperatura de referência eh %f, qual a nova temperatura?", ctrl->reference);
					scanf("%f", &ctrl->reference);
				}
				else if(ctrl->ctrl_mode==0x01){
					printf("\nA temperatura de ref. eh selecionada pela curva!\n");
				}
				else if(ctrl->ctrl_mode==0x00){
					printf("\nA temperatura é selecionada pelo dashboard!\n");
				}
				break;
			case 2:
				pid_update_command(ctrl);
				break;
			case 3:
				printf("Sinal de controle: %lf \n", ctrl->ctrl_signal);
				printf("Referencia: %lf \n", ctrl->reference);
				printf("Temp Int: %lf \n", ctrl->internal_temp);

				break;
		}	


	}
}

void uart_polling(void *param){

	data_furn* ctrl = (data_furn*) param;
	rx_data_t rx_uart;
	float *aux_float;
	int *aux_int;

	//while(1){
		usleep(80000);	
		rx_uart = read_uart();

		switch(rx_uart.sub_code){
			case 0xC1:
				aux_float= (float*)&rx_uart.content[0];
				ctrl->internal_temp = *aux_float;
				//printf("\ninternal_temp = %f\n", ctrl->internal_temp);
				break;

			case 0xC2:
				aux_float = (float*)&rx_uart.content[0];
				ctrl->reference = *aux_float;
				//printf("\nreference = %f\n", ctrl->reference);
				break;

			case 0xC3:
				aux_int = (int*)&rx_uart.content[0];
				ctrl->user_com = *aux_int;
				//printf("\nuser_com = %d\n", ctrl->user_com);
			break;	
		//}
		write_made=0x00;
		memset(&rx_uart.data[0], 0, RX_SIZE);
	}
	usleep(10000);
}

void *csv_polling_func(void *param){

	data_furn* ctrl = (data_furn*) param;
	fpt = fopen("furn_data.csv","w+");

	//Inserindo os cabeçalhos
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);	
	fprintf(fpt, "Data-Hora, temp. interna, temp. externa, temp. ref., ");
	fprintf(fpt, "Resist./Vent. (%%)\n");
	size_t count=0;	
	size_t i=0;

	while(1){
		if((ctrl->ctrl_mode==0x01) && (data_ctrl.debug_mode ==0x00)){
			ctrl->reference = csv_ref[i];
			if(count>=60){
				i++;
				count=0;
				if(i>=9)
					i=0;
			}
			printf("reference= %lf\n", ctrl->reference);
		}
		else{
			count=0;
			i=0;
		}

    //fprintf(fpt, "%s, ", asctime(localtime(&t)));
		t = time(NULL);
		tm = *localtime(&t);
		fprintf(fpt, "%d/%d/%d-%d:%d:%d, ", tm.tm_mday, tm.tm_mon+1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
		fprintf(fpt, "%f, ", ctrl->internal_temp);
		fprintf(fpt, "%f, ", ctrl->external_temp);
		fprintf(fpt, "%f, ", ctrl->reference);
		fprintf(fpt, "%f %%\n ", ctrl->ctrl_signal);
		sleep(1);
		count++;
	}

}

void update_pwm(int pwm_val){
	//printf("PWMVAL = %d\n", pwm_val);

  if(pwm_val < 0){
    softPwmWrite(res_gpio, 0);
    if(pwm_val > -40){
      softPwmWrite(vent_gpio, 40);
    }
    else
      softPwmWrite(vent_gpio, abs(pwm_val));
  } 
  else{
    softPwmWrite(res_gpio, pwm_val);
    softPwmWrite(vent_gpio, 0);
  }
 
}

void temp_control(data_furn *ctrl){
	int ctrl_signal_int;
	float ref_data_f;
	if(ctrl->poweron == 0x01){
		if(ctrl->ctrl_mode==0x00){
			write_uart(REQ_REF_TEMP, NULL); //Requisita a temperatura de referência
		}
		else{
			ref_data_f=ctrl->reference;
			write_uart(SEND_REF_SIG, &ref_data_f);
		}
		
		uart_polling(&data_ctrl);
		write_uart(REQ_INTERNAL_TEMP, NULL); //Requisita a temperatura interna
		uart_polling(&data_ctrl);
		//Calculo PID
		ctrl->ctrl_signal=pid_controle((double)(ctrl->internal_temp));
		pid_atualiza_referencia(ctrl->reference);
		//Atualiza o pwm
		
		
		if(ctrl->init == 0x01){
			ctrl_signal_int = (int)ctrl->ctrl_signal;
		}
		else{
			ctrl_signal_int = 0;
		}
		update_pwm(ctrl_signal_int);

		//Envia sinal de controle
		write_uart(SEND_CTRL_SIG, &ctrl_signal_int);
		uart_polling(&data_ctrl);
	}
	else{
		ctrl_signal_int = 0;
		update_pwm(ctrl_signal_int);
		write_uart(SEND_CTRL_SIG, &ctrl_signal_int);
		uart_polling(&data_ctrl);
		ref_data_f=0.0;
		write_uart(SEND_REF_SIG, &ref_data_f);
		uart_polling(&data_ctrl);
	}
}

void init_gpio(int gpio_res, int gpio_vent){
  if(wiringPiSetupGpio()==-1)
		exit(1);

  pinMode(gpio_res, OUTPUT);
  pinMode(gpio_vent, OUTPUT);
  if(softPwmCreate(gpio_res, 1, 100)!=0)
		printf("erro na criacao do SOFTPWM\n");
  if(softPwmCreate(gpio_vent, 40, 100)!=0)
		printf("erro na criacao do SOFTPWM\n");

}

void init_state(data_furn *ctrl){
	char aux;
	printf("Deseja utilizar os valores padrao para o controlador PID?(y/n)");
	scanf("%c", &aux);
	if(aux == 'n'){
		pid_update_command(ctrl);
	}
	else{
		printf("\n Valores padrão utilizados para o PID:\n\n");	
		printf("P = %lf\n", ctrl->Kp);
		printf("I = %lf\n", ctrl->Ki);
		printf("D = %lf\n", ctrl->Kd);
		pid_configura_constantes(ctrl->Kp, ctrl->Ki, ctrl->Kd);
	}
	write_uart(SEND_SIST_STATE, &data_ctrl.poweron);
	uart_polling(&data_ctrl);
	write_uart(SEND_OPER_STATE, &data_ctrl.init);
	uart_polling(&data_ctrl);
	write_uart(SEND_REF_CTRL_MODE, &data_ctrl.ctrl_mode);
	uart_polling(&data_ctrl);
	csv_reference();
}

int main(int argc, char **argv){

  signal(SIGINT, shutdown_furnance);
  init_gpio(res_gpio, vent_gpio);

  if(argc == 1){
    printf("Por favor insira o método de controle da temperatura:\n");
    printf("1 - para controle via terminal;\n");
    printf("2 - para controle via dashboard;\n");
    printf("3 - para controle via arquivo csv.\n");
    
    return 0;

  }
  if(argc >= 2){
		init_uart();
		if(argv[1][0]=='1'){
			printf("Controle via terminal (debug) escolhido.\n");
			init_state(&data_ctrl);
			data_ctrl.ctrl_mode = TERM_CURVE_CTRL;
			data_ctrl.debug_mode = 0x01;

		}			
		else if(argv[1][0]=='2'){
			printf("Controle via dashboard escolhido.\n");
			init_state(&data_ctrl);
			data_ctrl.ctrl_mode = DASH_CTRL;
			//Indica no dashboard que ele será o controlador de TR
		}
		else if(argv[1][0]=='3'){
			printf("Controle via arquivo CSV.\n");
			init_state(&data_ctrl);
			data_ctrl.ctrl_mode = TERM_CURVE_CTRL;
			csv_reference();
		}
		else{
			printf("Nenhuma das opções eh válida. Tente novamente.\n");
			return 0;
		}

  }
  
//	if(pthread_create(&uart_polling, NULL, &uart_polling_func, &data_ctrl) != 0){
//			printf("Erro na criação da thread UART. Verifique o programa!\n");
//			exit(1);
//		}
	if(pthread_create(&TR_request, NULL, &update_TR_terminal, &data_ctrl) != 0){
			printf("Erro na criação da thread. Verifique o programa!\n");
			exit(1);
		}
	if(pthread_create(&csv_polling, NULL, &csv_polling_func, &data_ctrl) != 0){
			printf("Erro na criação da thread write CSV. Verifique o programa!\n");
			exit(1);
		}


  while(1){
		switch(data_ctrl.user_com){
			case(0xA1):
				data_ctrl.poweron = 0x01;
				write_uart(SEND_SIST_STATE, &data_ctrl.poweron);
				break;
			case(0xA2):
				data_ctrl.poweron = 0x00;
				write_uart(SEND_SIST_STATE, &data_ctrl.poweron);
				break;
			case(0xA3):

				data_ctrl.init = 0x01;
				write_uart(SEND_OPER_STATE, &data_ctrl.init);
				break;
			case(0xA4):
				data_ctrl.init = 0x00;
				write_uart(SEND_OPER_STATE, &data_ctrl.init);
				break;
			case(0xA5):
				data_ctrl.debug_mode = 0x00;
				if(data_ctrl.ctrl_mode == 0x00){
					data_ctrl.ctrl_mode=0x01;
					printf("Controle via arquivo CSV.\n");
					
				}
				else{
					data_ctrl.ctrl_mode=0x00;
					printf("Controle via dashboard escolhido.\n");
				}

				write_uart(SEND_REF_CTRL_MODE, &data_ctrl.ctrl_mode);
				break;

			}
		data_ctrl.user_com = 0x00;
		uart_polling(&data_ctrl);
		write_uart(REQ_USER_COMS, NULL); //Requisita os commandos de usuário
		uart_polling(&data_ctrl);
		temp_control(&data_ctrl);
		usleep(500000);
  }
  return 0;
}
