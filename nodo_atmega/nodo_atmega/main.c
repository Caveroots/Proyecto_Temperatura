/*
 * nodo_atmega.c
 *
 * Created: 05/06/2019 07:01:11 p.m.
 * Author : Victor.Cavero
 */ 

#define FOSC 8000000 // Clock Speed
#define BAUD 9600
#define MYUBRR FOSC/8/BAUD-1
#define F_CPU 8000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <string.h>
#include <avr/interrupt.h>
#include <stdint.h>
int rpta1;
void USART_Init( unsigned int ubrr)
{
	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0A = (1<<U2X0);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void USART_Transmit(unsigned char data )
{
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = data;
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
	RF_Write(0x02,0x3F); //enable data pipes
	RF_Write(0x00,0x02); //enable PWR_UP bit
	csn(0);
	SPI_MasterTransmit(0x30);
	SPI_MasterTransmit(0xC6); //LSBYTE 0xAA
	SPI_MasterTransmit(0xC2);
	SPI_MasterTransmit(0xC2);
	SPI_MasterTransmit(0xC2);
	SPI_MasterTransmit(0xC2);
	csn(1);
	_delay_ms(2);
}
void tx_mode(uint8_t msg1, uint8_t msg2)
{
	RF_Write(0x00,0x02);
	csn(0);
	SPI_MasterTransmit(0xB0);
	SPI_MasterTransmit(msg1);
	SPI_MasterTransmit(msg2);
	csn(1);
	ce(1);
	_delay_us(50);
	ce(0);
	_delay_us(150);
}
/* One-Wire */
uint8_t	onewire_reset(void)
{
	uint8_t retval, retries;
	PORTD &= ~(1<<PD7); // low
	DDRD &= ~(1<<PD7);  // input
	retries = 128;
	while (!(PIND & (1<<PD7))) { // read
		if (--retries == 0) {
			return (2);
		}
		_delay_us(1);
	}
	DDRD |=(1<<PD7);// output
	_delay_us(480);
	DDRD &= ~(1<<PD7); // input
	_delay_us(66);
	retval = PIND & (1<<PD7);// read
	_delay_us(414);
	return retval;
}
static uint8_t onewire_bit(uint8_t value)
{
	uint8_t sreg;
	sreg = SREG;
	cli();
	DDRD |=(1<<PD7); // output
	_delay_us(1);
	if (value) {
		DDRD &= ~(1<<PD7); // input
	}
	_delay_us(14);
	value = !((PIND &(1<<PD7)) == 0); // read
	_delay_us(45);
	DDRD &= ~(1<<PD7); // input
	SREG = sreg;

	return value;
}
uint8_t onewire_write(uint8_t value)
{
	uint8_t i, r;
	for (i = 0; i < 8; ++i) {
		r = onewire_bit(value & 0x01);
		value >>= 1;
		if (r) {
			value |= 0x80;
		}
	}
	return value;
}
uint8_t onewire_read(void)
{
	return onewire_write(0xFF);
}

/* Sensor */
uint16_t sensor_read(void){
	uint16_t  t;
	onewire_reset();
	onewire_write(0xCC); // skip rom
	onewire_write(0x44); // convert Temperature
	onewire_reset();
	onewire_write(0xCC); // skip rom
	onewire_write(0xBE); // read sensor
	t = onewire_read();
	t |= (uint16_t)onewire_read() << 8;
	return ((t >> 4) * 100 + ((t << 12) / 6553) * 10);
}

void enviar_data(uint16_t t){
	uint8_t n=t/100;
	uint8_t d = t-n;
	if(d>199){
		n = n+ (d*0.01);
		n=n&0xFF;
		d=d-200;
	};
	if  (d>99){
		n = n+ (d*0.01);
		n=n&0xFF;
		d= d-100;
	};

	tx_mode(d,n);
	USART_Transmit(n);
	USART_Transmit(d);
}
int main(void)
{
	USART_Init(MYUBRR);
	SPI_MasterInit();
	standby_mode();
	uint16_t t;
	uint16_t test_t;
	test_t = sensor_read();
	_delay_ms(1500);
	while (1)
	{
		t= sensor_read();
		enviar_data(t);
		_delay_ms(2000);
	}
}	

