#ifndef UART_COM
#define UART_COM

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <string.h>
#include "crc/crc16.h"

#define ESP_ADDR 0x01
#define LAST_MATR	5350 
#define N1_ 0x53
#define N2_ 0x50
#define N1 0x05
#define N2 0x03
#define N3 0x05
#define N4 0x00
#define STD_SIZE 0x07
#define CRC_SIZE 0x02
#define RX_SIZE 0x07

#define DASH_CTRL 0x00
#define TERM_CURVE_CTRL 0x01

#define TTY "/dev/serial0"


typedef unsigned char uart_table_t[9][7];
typedef unsigned int uart_content_size_t[9];

typedef enum __commandIndex_t{
	REQ_INTERNAL_TEMP=0, //Solicita temp interna
	REQ_REF_TEMP,				// Solicita temp de referencia
	REQ_USER_COMS,			// Lê comandos do usuário
	SEND_CTRL_SIG,			// Envia sinal de controle (int 4 bytes)
	SEND_REF_SIG,				// Envia sinal de referencia (float 4 bytes)
	SEND_SIST_STATE,		// Envia estado do sistema
	SEND_REF_CTRL_MODE, // Modo do controle de temperatura por referencia
	SEND_OPER_STATE,		// Envia estado de funcionamento
	SEND_AMB_TEMP				// Envia a temperatura ambiente


}commandIndex_t;

typedef union __rx_data_t{
	unsigned char data[RX_SIZE];
	struct {
		unsigned char id_device;
		unsigned char code;
		unsigned char sub_code;
		unsigned char content[4];
	};
}rx_data_t;

extern int uart0_filestream;
extern unsigned char write_made;


void init_uart();
void write_uart(commandIndex_t indexCom, void* content);
rx_data_t read_uart();


#endif
