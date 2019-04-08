/*
 * main.c
 *
 * Created: 05/03/2019 18:00:58
 *  Author: eduardo
 */ 

#define YEAR        2018
#define MOUNT       3
#define DAY         19
#define WEEK        12
#define HOUR        15
#define MINUTE      45
#define SECOND      0

#include <asf.h>
#include "tfont.h"
#include "sourcecodepro_28.h"
#include "calibri_36.h"
#include "arial_72.h"
#include "math.h"

#define LED_PIO       PIOC
#define LED_PIO_ID    ID_PIOC
#define LED_IDX       8u
#define LED_IDX_MASK  (1u << LED_IDX)

//botão 1 do oled
#define BUT1_PIO           PIOD
#define BUT1_PIO_ID        ID_PIOD
#define BUT1_PIO_IDX       28u
#define BUT1_PIO_IDX_MASK  (1u << BUT1_PIO_IDX)


float bike_radius = 0.325;
float PI = 3.141;

volatile Bool f_rtt_alarme = false;

int velocidade;

int distancia;
int pulsos = 0;
int delta_pulsos = 0;
char string_distancia[32];
char string_velocidade[32];

int segundos = 0;
int	minutos = 0;
int horas = 0;

char string_segundos[32];
char string_minutos[32];
char string_horas[32];



/************************************************************************/
/* prototypes                                                           */
/************************************************************************/
void pin_toggle(Pio *pio, uint32_t mask);
void io_init(void);
void RTC_init(void);
static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses);


struct ili9488_opt_t g_ili9488_display_opt;

/************************************************************************/
/* interrupcoes                                                         */
/************************************************************************/

void but_callback(void)
{
	pulsos +=1;
	delta_pulsos +=1;
}

void font_draw_text(tFont *font, const char *text, int x, int y, int spacing) {
	char *p = text;
	while(*p != NULL) {
		char letter = *p;
		int letter_offset = letter - font->start_char;
		if(letter <= font->end_char) {
			tChar *current_char = font->chars + letter_offset;
			ili9488_draw_pixmap(x, y, current_char->image->width, current_char->image->height, current_char->image->data);
			x += current_char->image->width + spacing;
		}
		p++;
	}
}

void increment_time(){
	segundos +=1;
	if(segundos == 60){
		segundos = 0;
		minutos +=1;
		
		if(minutos == 60){
			minutos = 0;
			horas+=1;
		}
	}
}

void RTT_Handler(void)
{
	uint32_t ul_status;

	/* Get RTT status */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		pin_toggle(LED_PIO, LED_IDX_MASK);    // BLINK Led
		f_rtt_alarme = true;                  // flag RTT alarme
		
		sprintf(string_distancia,"%d",distancia);
		sprintf(string_velocidade,"%d",velocidade);
		
		//font_draw_text(&arial_72, "    ",50, 60, 2);
		//font_draw_text(&arial_72, "    ",50, 190, 2);
		
		font_draw_text(&arial_72, string_distancia,50, 60, 2);
		font_draw_text(&arial_72, string_velocidade,50, 190, 2);
	}
}



/************************************************************************/
/* funcoes                                                              */
/************************************************************************/




void io_init(void){
	//botao oled
	pmc_enable_periph_clk(BUT1_PIO_ID);
	pio_set_debounce_filter(BUT1_PIO,BUT1_PIO_IDX_MASK,20);
	
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	
	 pio_handler_set(BUT1_PIO,
	 BUT1_PIO_ID,
	 BUT1_PIO_IDX_MASK,
	 PIO_IT_FALL_EDGE,
	 but_callback);

	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4); // Prioridade 4
	
	
	
	
	/* led */
	pmc_enable_periph_clk(LED_PIO_ID);
	pio_configure(LED_PIO, PIO_OUTPUT_0, LED_IDX_MASK, PIO_DEFAULT);
}

static float get_time_rtt(){
	uint ul_previous_time = rtt_read_timer_value(RTT);
}

static void RTT_init(uint16_t pllPreScale, uint32_t IrqNPulses)
{
	uint32_t ul_previous_time;

	/* Configure RTT for a 1 second tick interrupt */
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	ul_previous_time = rtt_read_timer_value(RTT);
	while (ul_previous_time == rtt_read_timer_value(RTT));
	
	rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);

	/* Enable RTT interrupt */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 0);
	NVIC_EnableIRQ(RTT_IRQn);
	rtt_enable_interrupt(RTT, RTT_MR_ALMIEN);
}

void configure_lcd(void){
	/* Initialize display parameter */
	g_ili9488_display_opt.ul_width = ILI9488_LCD_WIDTH;
	g_ili9488_display_opt.ul_height = ILI9488_LCD_HEIGHT;
	g_ili9488_display_opt.foreground_color = COLOR_CONVERT(COLOR_WHITE);
	g_ili9488_display_opt.background_color = COLOR_CONVERT(COLOR_WHITE);

	/* Initialize LCD */
	ili9488_init(&g_ili9488_display_opt);
	ili9488_draw_filled_rectangle(0, 0, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
	
}




void pin_toggle(Pio *pio, uint32_t mask){
	
	
	
	
	if(pio_get_output_data_status(pio, mask)){
		pio_clear(pio, mask);	
		}else{
		pio_set(pio,mask);
	}
}


int main(void) {
	board_init();
	sysclk_init();	
	configure_lcd();
	
	delay_init();
	
	
	
	rtc_set_date_alarm(RTC, 1, MOUNT, 1, DAY);
	rtc_set_time_alarm(RTC, 1, HOUR, 1, MINUTE, 1, SECOND+1);
	
	
	font_draw_text(&calibri_36, "Distancia (m)", 20, 20, 1);
	font_draw_text(&calibri_36, "Velocidade (m/s)", 20, 150, 1);
	font_draw_text(&calibri_36, "Tempo total", 20, 300, 1);
	
	
	font_draw_text(&calibri_36, ":", 110, 355, 1);
	font_draw_text(&calibri_36, ":", 220, 355, 1);
	
	
	
	
	
	// Desliga watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	sysclk_init();
	io_init();
	
	// Inicializa RTT com IRQ no alarme.
	f_rtt_alarme = true;
	
	// super loop
	// aplicacoes embarcadas não devem sair do while(1).
	
	while(1) {
			if (f_rtt_alarme){
      
		  /*
		   * O clock base do RTT é 32678Hz
		   * Para gerar outra base de tempo é necessário
		   * usar o PLL pre scale, que divide o clock base.
		   *
		   * Nesse exemplo, estamos operando com um clock base
		   * de pllPreScale = 32768/32768/2 = 2Hz
		   *
		   * Quanto maior a frequência maior a resolução, porém
		   * menor o tempo máximo que conseguimos contar.
		   *
		   * Podemos configurar uma IRQ para acontecer quando 
		   * o contador do RTT atingir um determinado valor
		   * aqui usamos o irqRTTvalue para isso.
		   * 
		   * Nesse exemplo o irqRTTvalue = 8, causando uma
		   * interrupção a cada 2 segundos (lembre que usamos o 
		   * pllPreScale, cada incremento do RTT leva 500ms (2Hz).
		   */
		  uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
		  uint32_t irqRTTvalue  = 2;
      
		  // reinicia RTT para gerar um novo IRQ
		  RTT_init(pllPreScale, irqRTTvalue);
		  
		  distancia = 2*PI*bike_radius*pulsos;
		  
		  velocidade = 2*PI*delta_pulsos/4;
		  delta_pulsos = 0;
		  
		  
      
		 /*
		  * caso queira ler o valor atual do RTT, basta usar a funcao
		  *   rtt_read_timer_value()
		  */
      
		  /*
		   * CLEAR FLAG
		   */
		  f_rtt_alarme = false;
		}
		
		 pmc_sleep(SAM_PM_SMODE_SLEEP_WFI);
		
	}
}