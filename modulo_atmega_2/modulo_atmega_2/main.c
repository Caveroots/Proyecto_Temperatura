/*
 * modulo_atmega_2.c
 *
 * Created: 06/06/2019 12:25:12 p.m.
 * Author : Victor.Cavero
 */ 

 #define F_CPU 8000000UL
 #define BAUD 9600
 #define BUAD_RATE_CALC ((F_CPU/16/BAUD) - 1)
 #include <avr/io.h>
 #include <util/delay.h>
 #include <stdlib.h>
 #include <string.h>
 uint8_t l_temp,h_temp;
 /*UART*/
 void uart_init(){
	 UBRR0H = (BUAD_RATE_CALC >> 8);
	 UBRR0L = BUAD_RATE_CALC;
	 UCSR0B = (1 << TXEN0)| (1 << TXCIE0) | (1 << RXEN0) | (1 << RXCIE0);
	 UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
 }
 void uart_send(char* msg){
	 for (int i =0; i< strlen(msg);i++){
		 while (( UCSR0A & (1<<UDRE0))  == 0){};
		 UDR0 = msg[i];
	 }
 }
 /*Wi-Fi*/
 void wifi_init(){
	 
	 uart_send("AT\r\n"); _delay_ms(5000);
	 uart_send("AT+CWMODE=3\r\n"); _delay_ms(1000);
	 uart_send("AT+CWJAP=\"HUAWEIP9\",\"amaya2020\"\r\n"); _delay_ms(5000);
	 
 }
 void wifi_send(float t, int s){
	 char m[5];
	 dtostrf(t,4,2,m);
	 uart_send("AT+CIPSTART=\"TCP\",\"184.106.153.149\",80\r\n"); _delay_ms(5000);
	 uart_send("AT+CIPSEND=49\r\n"); _delay_ms(5000);
	 uart_send("GET /update?key=LUXB9A34S3728XXY&");
	 if (s==1){
		 uart_send("field1=");
	 };
	 if (s==2){
		 uart_send("field2=");
	 };
	 if (s==3){
		 uart_send("field3=");
	 };
	 if (s==4){
		 uart_send("field4=");
	 };
	 uart_send(m);
	 uart_send("\r\n");
	 _delay_ms(16000);
	 uart_send("AT+CIPCLOSE\r\n");
	 _delay_ms(5000);
 } 
 /*FUNCIONES PARA NRF24L01 */
 void csn(uint8_t estado)
 {
	 PORTB = (estado<<PORTB2);
 }
 void SPI_MasterInit(void)
 {
	 /* Set MOSI and SCK output, all others input */
	 DDRB = (1<<DDB3)|(1<<DDB5)|(1<<DDB2)|(1<<DDB1);
	 /* Enable SPI, Master, set clock rate fck/16 */
	 SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
	 //Disable CSN/
	 PORTB= (1<<PORTB2);
	 //Disabel CE/
	 PORTB= (0<<PORTB1);
 }
 uint8_t SPI_MasterTransmit(uint8_t msg_spi)
 {
	 /* Start transmission */
	 SPDR = msg_spi;
	 /* Wait for transmission complete */
	 while(!(SPSR & (1<<SPIF)));
	 return SPDR;
 }
 uint8_t RF_Read(uint8_t reg)
 {
	 csn(0);
	 SPI_MasterTransmit(0x00 | (0x1F & reg ));
	 uint8_t resultado = SPI_MasterTransmit(0xff);
	 csn(1);
	 return resultado;
 }
 uint8_t RF_Write(uint8_t reg, uint8_t valor)
 {
	 uint8_t status;
	 csn(0);
	 status = SPI_MasterTransmit(0x20 | (0x1F & reg));
	 SPI_MasterTransmit(valor);
	 csn(1);
	 return status;
 }
 void ce(uint8_t estado)
 {
	 PORTB = (estado<<PORTB1);
 }
 void standby_mode()
 {
	 _delay_ms(200);
	 RF_Write(0x01,0x00); //disable acknowledge
	 RF_Write(0x02,0x20); //enable data pipes
	 RF_Write(0x00,0x02); //enable PWR_UP bit
	 RF_Write(0x11,0x01); //bytes of data pipe
	 RF_Write(0x12,0x01); //bytes of data pipe
	 RF_Write(0x13,0x01); //bytes of data pipe
	 RF_Write(0x14,0x01); //bytes of data pipe
	 RF_Write(0x15,0x01); //bytes of data pipe
	 RF_Write(0x16,0x02); //bytes of data pipe
	 RF_Write(0x00,0x02);
	 _delay_ms(2);
 }

 void set_pipe(uint8_t pipe)
 {
	 RF_Write(0x0F,pipe);
	 
 }

 uint8_t flush_rx(void)
 {
	 return SPI_MasterTransmit(0xE2);
 }
 uint8_t flush_tx(void)
 {
	 return SPI_MasterTransmit(0xE1);
 }
 uint8_t rx_mode()
 {
	 RF_Write(0x00,0x03);
	 ce(1);
	 _delay_ms(100);
	 uint8_t ava = RF_Read(0x07);
	 if((ava & 0x40) == 0x40)
	 {
		 csn(0);
		 SPI_MasterTransmit(0x61);
		 l_temp = SPI_MasterTransmit(0xff);
		 h_temp = SPI_MasterTransmit(0xff);
		 csn(1);
		 ce(0);
		 RF_Write(0x07,0x40);
		 return 0xff;
	 }
	 else{
		 ce(0);
		 return 0x00;
	 }
	 
	 
 }

 float int_to_float(uint8_t ent, uint8_t dec){
	 float flo = ent+ 0.01*dec;
	 return(flo);
 }
 int main(void)
 {
	 	 uint8_t ava;
	 uart_init();
	 SPI_MasterInit();
	 standby_mode();

	 while (1)
	 {
		set_pipe(0xC6);
		 ava = rx_mode();
		 while (ava == 0)
		 {
			 ava = rx_mode(h_temp,l_temp);
		 }
		 float dato1 = int_to_float(h_temp, l_temp);
		 set_pipe(0xAA);
		 ava = rx_mode();
		 while (ava == 0)
		 		 {
			 		 ava = rx_mode(h_temp,l_temp);
		 		 }
		 float dato2 = int_to_float(h_temp, l_temp);

		 wifi_send(dato1,1);
		 wifi_send(dato2,2);
		 _delay_ms(10000);
		  }
 }

