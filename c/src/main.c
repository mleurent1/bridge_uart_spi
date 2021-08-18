#include <stdint.h>
#include <avr/io.h>
//#include <avr/cpufunc.h> // _NOP()
#include <avr/interrupt.h>
#include <avr/sleep.h>

#define BUFLEN 8

volatile uint16_t tick;
volatile uint8_t uart_buf[BUFLEN], spi_buf[BUFLEN];
volatile uint8_t uart_rx_idx, uart_tx_idx, spi_idx;

ISR(TIMER0_COMPA_vect) {
	tick++;
}
/*
ISR(TIMER1_COMPA_vect) {
	PINB = (1<<PINB5);
}
*/
ISR(USART_RX_vect) {
	uart_buf[uart_rx_idx++] = UDR0;
}
ISR(USART_UDRE_vect) {
	if (uart_tx_idx < uart_buf[0]) {
		//UDR0 = uart_buf[++uart_tx_idx]; // UART loop-back
		UDR0 = spi_buf[uart_tx_idx++];
	} else {
		UCSR0B &= ~(1<<UDRIE0); // Disable Tx data register empty interrupt
	}
}
ISR(SPI_STC_vect) {
	spi_buf[spi_idx++] = SPDR;
	if (spi_idx < uart_buf[0]) {
		SPDR = uart_buf[spi_idx+1];
	}
}

void wait_ms(uint16_t t)
{
	uint16_t next_tick = tick + t;
	while (tick != next_tick) {
		sleep_mode();
	}
}

int main(void)
{
	tick = 0;
	uart_rx_idx = 0;
	spi_idx = 0;
	uart_buf[0] = 1; // Set nbytes to 1 to prevent early burst complete events

	// Timer 0, used for wait_ms
	OCR0A = 250 - 1; // Set output compare A register to match every 1ms
	TCCR0A = (2<<WGM00); // CTC (clear timer on compare match) mode
	//TCCR0B =  (3<<CS00); // clock/64 => 250kHz
	TIMSK0 = (1<<OCIE0A); // compare A match interrupt enable

	// Timer 1, used for LED blink
	/*
	OCR1A = 62500 - 1; // Set output compare A register to match every 250ms
	TCCR1B = (1<<WGM12) | (3<<CS10); // CTC (clear timer on compare match) mode + clock/64 => 250kHz
	TIMSK1 = (1<<OCIE1A); // compare A match interrupt enable
	DDRB = (1<<DDB5); // set PB5 as output (connected to LED)
	*/

	// UART
	UBRR0 = 104; // 16MHz/(16*9600) = 104
	UCSR0B = (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0); // Rx and Tx enabled + Rx tranfer complete interrupt enabled
	UCSR0C = (3<<UCSZ00); // 8bit, 1stop, no parity

	// SPI
	DDRB = (1<<DDB2) | (1<<DDB3) | (1<<DDB5); // SPI MOSI and SCK + unused SPI SS that must be configured as output
	DDRD = (1<<DDD6); // SPI SS
	PORTD = (1<<PORTD6);
	SPCR = (1<<SPIE) | (1<<SPE) | (1<<MSTR) | (1<<SPR0); // Enable SPI master MSB first and interrupt + clock/16 => 1MHz

	// enable global interrupt
	sei(); 
	
	while(1) {

		// SPI burst complete
		if (spi_idx == uart_buf[0]) {
			spi_idx = 0;
			PORTD |= (1<<PORTD6); // SPI SS = 1
			// Send SPI Rx data back via UART
			uart_tx_idx = 0;
			UCSR0B |= (1<<UDRIE0); // Enable Tx data register empty interrupt
		}

		// UART Rx burst complete
		if (uart_rx_idx == (uart_buf[0]+1)) {
			uart_rx_idx = 0;
			// UART loop-back
			//uart_tx_idx = 0;
			//UCSR0B |= (1<<UDRIE0); // Enable Tx data register empty interrupt
			// Start SPI transactoion
			spi_idx = 0;
			PORTD &= ~(1<<PORTD6); // SPI SS = 0
			SPDR = uart_buf[spi_idx+1];
		}

		sleep_mode();
	}

	return(0);
}