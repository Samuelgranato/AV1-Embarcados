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

//bot�o 1 do oled para simula��o do sensor magnetico
#define BUT1_PIO           PIOD
#define BUT1_PIO_ID        ID_PIOD
#define BUT1_PIO_IDX       28u
#define BUT1_PIO_IDX_MASK  (1u << BUT1_PIO_IDX)

//botao 2 do oled para reinicio
#define BUT2_PIO      PIOC
#define BUT2_PIO_ID   ID_PIOC
#define BUT2_IDX  31
#define BUT2_PIO_IDX_MASK (1 << BUT2_IDX)

float bike_radius = 0.325;
float PI = 3.141;

volatile Bool f_rtt_alarme = false;
volatile Bool pause_flag = false;


int velocidade =0;
int velocidade_maxima = 0 ;
int count_velocidade_att = 0;

int idle_count = 0;
int idle_lastPulse = 0;

int distancia = 0;
int pulsos = 0;
int delta_pulsos = 0;
char string_distancia[32];
char string_velocidade[32];
char string_velocidade_maxima[32];

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
	
	idle_lastPulse = pulsos;
}

void pause_callback(void)
{
	pause_flag = !pause_flag;
	
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

void print_time(){
	sprintf(string_segundos,"%d",segundos);
	sprintf(string_minutos,"%d",minutos);
	sprintf(string_horas,"%d",horas);
				
				
				
	font_draw_text(&arial_72, string_horas,40, 340, 2);
	font_draw_text(&arial_72, string_minutos,130, 340, 2);
	font_draw_text(&arial_72, string_segundos,220, 340, 2);
}

void increment_time(){
	segundos +=1;
	if(segundos == 60){
		segundos = 0;
		ili9488_draw_filled_rectangle(220, 340, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
		
		minutos +=1;
		
		if(minutos == 60){
			minutos = 0;
			ili9488_draw_filled_rectangle(220, 340, ILI9488_LCD_WIDTH-1, ILI9488_LCD_HEIGHT-1);
			
			horas+=1;
		}
	}
}

void print_dist_vel(){
	sprintf(string_velocidade,"%d",velocidade);
	sprintf(string_velocidade_maxima,"%d",velocidade_maxima);
	sprintf(string_distancia,"%d",distancia);
	
	
	
	ili9488_draw_filled_rectangle(50, 200,  ILI9488_LCD_WIDTH-1, 270);
	font_draw_text(&arial_72, string_velocidade,50, 190, 2);
	
	font_draw_text(&calibri_36, "Max:", 160, 230, 1);
	font_draw_text(&calibri_36, string_velocidade_maxima,250, 230, 2);
	
	font_draw_text(&arial_72, string_distancia,50, 60, 2);
}

void idle_mode(){
	
}

void RTT_Handler(void)
{
	if(pulsos == idle_lastPulse){
		idle_count +=1;
		if(idle_count == 20){
			idle_mode();
		}
	}else{
		idle_count = 0;
	}
	uint32_t ul_status;
	
		/* Get RTT status */
		ul_status = rtt_get_status(RTT);
	
		/* IRQ due to Time has changed */
		if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {  }
	
		/* IRQ due to Alarm */
		if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
			pin_toggle(LED_PIO, LED_IDX_MASK);    // BLINK Led
			f_rtt_alarme = true;                  // flag RTT alarme
			
			
			//contador para atualizar a velocidade e distancia a cada 4 segundos
			if(count_velocidade_att == 0){
				print_dist_vel();
				
			}
			
			if(!pause_flag){
				count_velocidade_att +=1;
			
				if(count_velocidade_att ==4){
					count_velocidade_att = 0;
				}		
			
				increment_time();
			}
			rtc_set_date_alarm(RTC, 1, MOUNT, 1, DAY);
			rtc_set_time_alarm(RTC, 1, HOUR, 1, MINUTE, 1, SECOND+1);
			
			print_time();
		}

}



/************************************************************************/
/* funcoes                                                              */
/************************************************************************/




void io_init(void){
	//botao1 oled
	pmc_enable_periph_clk(BUT1_PIO_ID);
	
	pio_set_debounce_filter(BUT1_PIO,BUT1_PIO_IDX_MASK,20);
	pio_configure(BUT1_PIO, PIO_INPUT, BUT1_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_enable_interrupt(BUT1_PIO, BUT1_PIO_IDX_MASK);
	
	NVIC_EnableIRQ(BUT1_PIO_ID);
	NVIC_SetPriority(BUT1_PIO_ID, 4); // Prioridade 4
	
	pio_handler_set(BUT1_PIO,
	BUT1_PIO_ID,
	BUT1_PIO_IDX_MASK,
	PIO_IT_FALL_EDGE,
	but_callback);
	
	//botao2 oled
	pmc_enable_periph_clk(BUT2_PIO_ID);
		
	pio_set_debounce_filter(BUT2_PIO,BUT2_PIO_IDX_MASK,20);
	pio_configure(BUT2_PIO, PIO_INPUT, BUT2_PIO_IDX_MASK, PIO_PULLUP | PIO_DEBOUNCE);
	pio_enable_interrupt(BUT2_PIO, BUT2_PIO_IDX_MASK);
		
	NVIC_EnableIRQ(BUT2_PIO_ID);
	NVIC_SetPriority(BUT2_PIO_ID, 4); // Prioridade 4
		
	pio_handler_set(BUT2_PIO,
	BUT2_PIO_ID,
	BUT2_PIO_IDX_MASK,
	PIO_IT_FALL_EDGE,
	pause_callback);
	
	
	
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
	font_draw_text(&calibri_36, "Velocidade (km/h)", 20, 150, 1);
	font_draw_text(&calibri_36, "Tempo ttl (h m s)", 20, 300, 1);
	
	
	
	
	
	
	// Desliga watchdog
	WDT->WDT_MR = WDT_MR_WDDIS;
	
	sysclk_init();
	io_init();
	
	// Inicializa RTT com IRQ no alarme.
	f_rtt_alarme = true;
	
	// super loop
	// aplicacoes embarcadas n�o devem sair do while(1).
	
	while(1) {
			if (f_rtt_alarme){
      
		  /*
		   * O clock base do RTT � 32678Hz
		   * Para gerar outra base de tempo � necess�rio
		   * usar o PLL pre scale, que divide o clock base.
		   *
		   * Nesse exemplo, estamos operando com um clock base
		   * de pllPreScale = 32768/32768/2 = 2Hz
		   *
		   * Quanto maior a frequ�ncia maior a resolu��o, por�m
		   * menor o tempo m�ximo que conseguimos contar.
		   *
		   * Podemos configurar uma IRQ para acontecer quando 
		   * o contador do RTT atingir um determinado valor
		   * aqui usamos o irqRTTvalue para isso.
		   * 
		   * Nesse exemplo o irqRTTvalue = 8, causando uma
		   * interrup��o a cada 2 segundos (lembre que usamos o 
		   * pllPreScale, cada incremento do RTT leva 500ms (2Hz).
		   */
		  uint16_t pllPreScale = (int) (((float) 32768) / 2.0);
		  uint32_t irqRTTvalue  = 2;
      
		  // reinicia RTT para gerar um novo IRQ
		  RTT_init(pllPreScale, irqRTTvalue);
		  

			  distancia = 2*PI*bike_radius*pulsos;
		  
		  
		  
			  if(count_velocidade_att == 0){
				velocidade = (2*PI*delta_pulsos/4) * bike_radius;
			
				//para km/h
				velocidade = velocidade * 3.6;
				
				if(velocidade > velocidade_maxima){
					velocidade_maxima = velocidade;
				}
				delta_pulsos = 0;
			  }
		  
		  
		  
      
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