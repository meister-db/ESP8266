#include <stdio.h>
#include <string.h>
#include <delay.h>
#define BUFFSIZE 32
/*
    2018 ESP 헤더 헤레본

    인터넷 말씀이 ESP와 달라 
    코드와로 서로 맞지 아니 ㅎㆍㄹ셰
    이러한 연유로 어린 학생이
    짜고 싶은 코드가 있어도
    마침내 MDP를 끝내지 못한 학생이 많으니라.
    내 이를 위하여 여엿비 너겨 새로 200줄을 만드노니
    학생마다 해여 쉽게 익혀 날로 쑤메 
    편안케 하고져 할 따름이니라.   
       
    7기 -조승준, 김민혁-
*/

/* rx 인터럽트 사용시 붙혀넣기
interrupt [USART0_RXC] void usart0_rxc_isr(void)
{
    #asm("cli") //인터럽트 비활성화
    if(waitIPD(1000)){
        lcdprint(rx_buffer);      
    }
    else{
        lcdprint("IPD timeout");
    } 
    #asm("sei") //인터럽트 재활성화
}
*/

unsigned char rx_buffer[BUFFSIZE];

void tx_char(char data){              //data 문자 전송
    while(!(UCSR0A&(1<<UDRE0))){};
    UDR0 = data; 
}

void tx_string(char *str){            // 문자열 전송
   while(*str!='\0'){
          tx_char(*str);
          str++; 
          delay_ms(10);
   }
}

//1ms 이내에 검출하지 못하면 쓰레기값 출력
char rx_char(){
    if(!(UCSR0A&(1<<RXC0))) delay_ms(1);
    if((UCSR0A&(1<<RXC0))) return UDR0 & 0xff;
    else return 0;     
}

// 버퍼를 초기화함
void buffer_clear(){               
    int i=0;
    for(i=0;i<BUFFSIZE;i++){
        rx_buffer[i]=0;
    }
}

//while 문을 이용해서 str 까지 rx_buffer에 저장한다. timeOUT 또한 있다. timeout이내 일시 1반환
int rx_string(char *END, int timeOUT) {
	int i = 0;
	unsigned char byte = 0;
	char *HOME = END;       
    
    buffer_clear();         
    
	while (1) {
		byte = rx_char(); 
        if(byte == 0){  
			timeOUT--; 
			if (timeOUT <= 0) {  
                tx_string("TimeOut \r\n");
                buffer_clear();               
                return 0;  
			}
            continue;
        }
		rx_buffer[i] = byte;

		if (byte == *END) {
			END++;
			if (*END == '\0') {
				return 1;
			}
		}
		else {
			END = HOME;
		}
		i++;
	}
}

//while 문을 이용해서 str 을 rx 에서 검출한다 timeOUT 내에 검출하지못하면 0을 반환한다.
int waitString(char *str, int timeOUT) {
	char *HOME = str;
	char byte;

	while (1) {
		byte = rx_char();
		if (byte == *str) {
			str++;
			if (*str == '\0') {
				return 1;
			}
		}
		else {
			str = HOME;
			timeOUT--;
			if (timeOUT <= 0) {
				return 0;
			}
		}
	}
}

//OK기다리고 결과값 반환 1=OK 0=None
int waitOK() {
    int result;
    result = waitString("OK",1000);
    return result;
}

//USART0 rate : 9600 , 
void USART_init(){
    UCSR0A=(0<<RXC0) | (0<<TXC0) | (0<<UDRE0) | (0<<FE0) | (0<<DOR0) | (0<<UPE0) | (0<<U2X0) | (0<<MPCM0);
    UCSR0B=(0<<RXCIE0) | (0<<TXCIE0) | (0<<UDRIE0) | (1<<RXEN0) | (1<<TXEN0) | (0<<UCSZ02) | (0<<RXB80) | (0<<TXB80);
    UCSR0C=(0<<UMSEL0) | (0<<UPM01) | (0<<UPM00) | (0<<USBS0) | (1<<UCSZ01) | (1<<UCSZ00) | (0<<UCPOL0);
    UBRR0H=0x00;
    UBRR0L=0x67;             
}

//명령어를 전송함
void AT_Command(char *command) {
    //strcat(command,"\r\n");   이거쓰면 명령어 1/2로 들어감 *정상아님*
    tx_string(command);
}

// +검출후 rx를 버퍼에 저장함 OK까지, 버퍼크기 현재 32
int waitIPD(int Timeout){
    if(waitString("+",Timeout)){
        rx_string("OK",Timeout); 
        return 1;
    }else{
        return 0;
    }
}

//클라이언트일때의 AT명령어 집합
void TCP_Client() {
    
    AT_Command("AT+CWMODE=3 \r\n"); 
    waitOK();
    
    AT_Command("AT+CWJAP=\"PRO2018\",\"*MDP2018*\" \r\n"); 
    while(!waitOK());
    
    AT_Command("AT+CIPMUX=1 \r\n");
    waitOK(); 
    
    AT_Command("AT+CIPSTART=1,\"TCP\",\"192.168.0.30\",3030 \r\n");
    waitOK();   
    
}

//서버일때의 AT 명령어 집합
void TCP_Server(){
    AT_Command("AT+CWMODE=3 \r\n"); 
    waitOK();
    
    AT_Command("AT+CWJAP=\"B\",\"\" \r\n"); 
    while(!waitOK());
    
    
    AT_Command("AT+CIPMUX=1 \r\n");
    waitOK();             
    
    AT_Command("AT+CIPSERVER=1,1004\r\n");
    waitOK();
    
}
//데이터(문자열) 전송 알아서 길이도 구해줌
int TCP_Send(char *str){
    int len = 0; 
    char buffer[64]; 
    
    len = strlen(str);  
    
    sprintf(buffer,"AT+CIPSEND=1,%d\r\n",len);
    AT_Command(buffer);
    delay_ms(10);      
    
    AT_Command(str);
    AT_Command("\r\n");
    waitOK();
}