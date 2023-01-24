#include "uart_com.h"
#include <stdlib.h>


static uart_table_t response_table ={
	{0x01,0x23,0xC1,N1, N2, N3, N4},
	{0x01,0x23,0xC2,N1, N2, N3, N4},
	{0x01,0x23,0xC3,N1, N2, N3, N4},
	{0x01,0x16,0xD1,N1, N2, N3, N4},
	{0x01,0x16,0xD2,N1, N2, N3, N4},
	{0x01,0x16,0xD3,N1, N2, N3, N4},
	{0x01,0x16,0xD4,N1, N2, N3, N4},
	{0x01,0x16,0xD5,N1, N2, N3, N4},
	{0x01,0x16,0xD6,N1, N2, N3, N4}
	};

static uart_content_size_t size_table = {
	0x00, 0x00, 0x00, 0x04, 0x04, 0x01, 0x01, 0x01, 0x04
};

unsigned char tx_buffer[20];
unsigned char rx_buffer[20];
unsigned char write_made;
int uart0_filestream = -1;

void init_uart(){

    uart0_filestream = open(TTY, O_RDWR | O_NOCTTY | O_NDELAY);      //Open in non blocking read/write mode
		if(uart0_filestream == -1){
			printf("Erro! O UART não foi corretamente inicializado.\n");
			exit(1);
		}
		else{
			printf("UART Corretamente inicializada\n;");
		}
		struct termios options;
		tcgetattr(uart0_filestream, &options);
		options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
		options.c_iflag = IGNPAR;
		options.c_oflag = 0;
		options.c_lflag = 0;
		tcflush(uart0_filestream, TCIFLUSH);
		tcsetattr(uart0_filestream, TCSANOW, &options);
}

static int crc_check(short int crc_16, unsigned char *crc_in){
	unsigned char crc_8[2];
	
	crc_8[0] = (unsigned char)(crc_16 & 0x00FF);
	crc_8[1] = (unsigned char)((crc_16 & 0xFF00)>>8);

	if(crc_8[0] == crc_in[0]){
		if(crc_8[1] != crc_in[1]){
			//printf("\nErro no CRC!\n");
			//printf("CRC lido: %X %X\n", crc_in[0], crc_in[1]);
			//printf("CRC calculado: %X %X\n", crc_8[0], crc_8[1]);
			//printf("CRC calculado: %X\n", crc_16);
			return 1;
		}
	}
	else{
			//printf("\nErro no CRC!\n");
			//printf("CRC lido: %X %X\n", crc_in[0], crc_in[1]);
			//printf("CRC calculado: %X %X\n", crc_8[0], crc_8[1]);
			//printf("CRC calculado: %X\n", crc_16);
			return 1;
	}
	return 0;

}

rx_data_t read_uart(){		
	short int crc_16;
	size_t rx_lenght=0;
	rx_data_t rx_uart;
	rx_lenght = read(uart0_filestream, (void*)rx_buffer, 20);
	if(rx_lenght < 0)
		printf("Erro na leitura do UART.\n");
	else if(rx_lenght > 0){
		while(rx_buffer[2]==0xD1 || rx_buffer[2]==0xD2){
			for(size_t j=0; j<3; j++){	
				for(size_t i=0; i<20;i++)
					rx_buffer[i] = rx_buffer[i+1];
			rx_lenght-=3;
			}
		}
		crc_16= calcula_CRC(&rx_buffer[0], RX_SIZE);
		if(crc_check(crc_16, &rx_buffer[RX_SIZE]) == 0){
			memcpy(&rx_uart.data[0], rx_buffer, RX_SIZE);
			return rx_uart;
		}
		else{
			//printf("bytes recebidos: ");
			//for(size_t i=0; i<rx_lenght;i++)
			//	printf("%X ", rx_buffer[i]);
			//printf("\n");
			memset(&rx_uart.data[0], 0, RX_SIZE);
			return rx_uart;
		}
	}
}


void write_uart(commandIndex_t indexCom, void *content){
		int error;
		short int crc_16;
		unsigned char crc_8[2];
		unsigned char *payload = (unsigned char*)content;
		memcpy(&tx_buffer[0], &response_table[indexCom][0], STD_SIZE);

		if(size_table[indexCom] > 0x00){
			memcpy(&tx_buffer[7], payload, size_table[indexCom]);
		}

		crc_16 = calcula_CRC(&tx_buffer[0], STD_SIZE+size_table[indexCom]);
		crc_8[0] = (unsigned char)(crc_16 & 0X00FF);
		crc_8[1] = (unsigned char)((crc_16>>8) & 0X00FF);

	
		memcpy(&tx_buffer[STD_SIZE+size_table[indexCom]], &crc_8[0], 2);
		
		error = write(uart0_filestream, &tx_buffer[0], STD_SIZE+size_table[indexCom]+2);

		//printf("Dados transmitidos:");
		//for(size_t i=0; i <STD_SIZE+size_table[indexCom]+2; i++)
		//	printf(" %#04X", tx_buffer[i]);
		//printf("\n");

		if(error < 0)
			printf("Erro na transmissão!\n\n");
		usleep(10000);

}
