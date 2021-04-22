/*
    ophyra_tftdisp.c

    Modulo de usuario en C para la utilizacion de la pantalla TFT ST7735 la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Este archivo incluye la definicion de una clase y 19 funciones que necesita la pantalla TFT para funcionar 
    correctamente en el lenguaje MicroPython.

    Para construir el firmware incluyendo este modulo, ejecutar en Cygwin:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_TFTDISP_ENABLED=1 all

        En USER_C_MODULES se especifica el path hacia la localizacion de la carpeta modules, donde se encuentra
        el archivo ophyra_tftdisp.c y micropython.mk

    Escrito por: Jonatan Salinas y Carlos D. Hernández.
    Ultima fecha de modificacion: 22/04/2021.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/mphal.h"          
#include "ports/stm32/spi.h"
//#include "lib/oofatfs/ff.h"
/*
    Definicion de los Comandos
*/

#define CMD_NOP     (0x00)  // No Operacion
#define CMD_SWRESET (0x01)  // Software Reset
#define CMD_RDDID   (0x04)  // Read Display ID
#define CMD_RDDST   (0x09)  // Read Display Status

#define CMD_SLPIN   (0x10)  // Sleep in & Booster Off
#define CMD_SLPOUT  (0x11)  // Sleep out & Booster On
#define CMD_PTLON   (0x12)  // Partial mode On
#define CMD_NORON   (0x13)  // Partial Off (Normal)

#define CMD_INVOFF  (0x20)  // Display inversion Off
#define CMD_INVON   (0x21)  // Display inversion On
#define CMD_DISPOFF (0x28)  // Display Off
#define CMD_DISPON  (0x29)  // Display On
#define CMD_CASET   (0x2A)  // Column Address set
#define CMD_RASET   (0x2B)  // Row Address set
#define CMD_RAMWR   (0x2C)  // Memory Write
#define CMD_RAMRD   (0x2E)  // Memory Read

#define CMD_PTLAR   (0x30)  // Partial Start/End Address set
#define CMD_COLMOD  (0x3A)  // Interface Pixel Format
#define CMD_MADCTL  (0x36)  // Memory Data Acces Control

#define CMD_RDID1   (0xDA)  // Read ID1
#define CMD_RDID2   (0xDB)  // Read ID2
#define CMD_RDID3   (0xDC)  // Read ID3
#define CMD_RDID4   (0xDD)  // Read ID4

/*
    Comandos de Función de Panel
*/

#define CMD_FRMCTR1 (0xB1)  // In normal mode (Full colors)
#define CMD_FRMCTR2 (0xB2)  // In Idle mode (8-colors)
#define CMD_FRMCTR3 (0xB3)  // In partial mode + Full colors
#define CMD_INVCTR  (0xB4)  // Display inversion control

#define CMD_PWCTR1  (0xC0)  // Power Control Settings
#define CMD_PWCTR2  (0xC1)  // Power Control Settings
#define CMD_PWCTR3  (0xC2)  // Power Control Settings
#define CMD_PWCTR4  (0xC3)  // Power Control Settings
#define CMD_PWCTR5  (0xC4)  // Power Control Settings
#define CMD_VMCTR1  (0xC5)  // VCOM Control

#define CMD_GMCTRP1 (0xE0)
#define CMD_GMCTRN1 (0xE1)

/*
    Identificadores de los pines de trabajo
*/

const pin_obj_t *Pin_DC=pin_D6;
const pin_obj_t *Pin_CS=pin_A15;
const pin_obj_t *Pin_RST=pin_D7;
const pin_obj_t *Pin_BL=pin_A7;

/*
    Definicion de la paleta de colores TFT
*/

#define COLOR_BLACK     (0x0000)
#define COLOR_BLUE      (0x001F)
#define COLOR_RED       (0xF800)
#define COLOR_GREEN     (0x07E0)
#define COLOR_CYAN      (0x07FF)
#define COLOR_MAGENTA   (0xF81F)
#define COLOR_YELLOW    (0xFFE0)
#define COLOR_WHITE     (0xFFFF)

/*
    SPI1 Conf
*/
#define TIMEOUT_SPI     (5000)
/*
    Font Lib implemented here.
*/
#define WIDTH       (6)
#define HEIGHT      (8)
#define START       (32)
#define END         (127)
const uint8_t Font[]=
{
0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x06, 0x5F, 0x06, 0x00,
0x00, 0x07, 0x03, 0x00, 0x07, 0x03,
0x00, 0x24, 0x7E, 0x24, 0x7E, 0x24,
0x00, 0x24, 0x2B, 0x6A, 0x12, 0x00,
0x00, 0x63, 0x13, 0x08, 0x64, 0x63,
0x00, 0x36, 0x49, 0x56, 0x20, 0x50,
0x00, 0x00, 0x07, 0x03, 0x00, 0x00,
0x00, 0x00, 0x3E, 0x41, 0x00, 0x00,
0x00, 0x00, 0x41, 0x3E, 0x00, 0x00,
0x00, 0x08, 0x3E, 0x1C, 0x3E, 0x08,
0x00, 0x08, 0x08, 0x3E, 0x08, 0x08,
0x00, 0x00, 0xE0, 0x60, 0x00, 0x00,
0x00, 0x08, 0x08, 0x08, 0x08, 0x08,
0x00, 0x00, 0x60, 0x60, 0x00, 0x00,
0x00, 0x20, 0x10, 0x08, 0x04, 0x02,
0x00, 0x3E, 0x51, 0x49, 0x45, 0x3E,
0x00, 0x00, 0x42, 0x7F, 0x40, 0x00,
0x00, 0x62, 0x51, 0x49, 0x49, 0x46,
0x00, 0x22, 0x49, 0x49, 0x49, 0x36,
0x00, 0x18, 0x14, 0x12, 0x7F, 0x10,
0x00, 0x2F, 0x49, 0x49, 0x49, 0x31,
0x00, 0x3C, 0x4A, 0x49, 0x49, 0x30,
0x00, 0x01, 0x71, 0x09, 0x05, 0x03,
0x00, 0x36, 0x49, 0x49, 0x49, 0x36,
0x00, 0x06, 0x49, 0x49, 0x29, 0x1E,
0x00, 0x00, 0x6C, 0x6C, 0x00, 0x00,
0x00, 0x00, 0xEC, 0x6C, 0x00, 0x00,
0x00, 0x08, 0x14, 0x22, 0x41, 0x00,
0x00, 0x24, 0x24, 0x24, 0x24, 0x24,
0x00, 0x00, 0x41, 0x22, 0x14, 0x08,
0x00, 0x02, 0x01, 0x59, 0x09, 0x06,
0x00, 0x3E, 0x41, 0x5D, 0x55, 0x1E,
0x00, 0x7E, 0x11, 0x11, 0x11, 0x7E,
0x00, 0x7F, 0x49, 0x49, 0x49, 0x36,
0x00, 0x3E, 0x41, 0x41, 0x41, 0x22,
0x00, 0x7F, 0x41, 0x41, 0x41, 0x3E,
0x00, 0x7F, 0x49, 0x49, 0x49, 0x41,
0x00, 0x7F, 0x09, 0x09, 0x09, 0x01,
0x00, 0x3E, 0x41, 0x49, 0x49, 0x7A,
0x00, 0x7F, 0x08, 0x08, 0x08, 0x7F,
0x00, 0x00, 0x41, 0x7F, 0x41, 0x00,
0x00, 0x30, 0x40, 0x40, 0x40, 0x3F,
0x00, 0x7F, 0x08, 0x14, 0x22, 0x41,
0x00, 0x7F, 0x40, 0x40, 0x40, 0x40,
0x00, 0x7F, 0x02, 0x04, 0x02, 0x7F,
0x00, 0x7F, 0x02, 0x04, 0x08, 0x7F,
0x00, 0x3E, 0x41, 0x41, 0x41, 0x3E,
0x00, 0x7F, 0x09, 0x09, 0x09, 0x06,
0x00, 0x3E, 0x41, 0x51, 0x21, 0x5E,
0x00, 0x7F, 0x09, 0x09, 0x19, 0x66,
0x00, 0x26, 0x49, 0x49, 0x49, 0x32,
0x00, 0x01, 0x01, 0x7F, 0x01, 0x01,
0x00, 0x3F, 0x40, 0x40, 0x40, 0x3F,
0x00, 0x1F, 0x20, 0x40, 0x20, 0x1F,
0x00, 0x3F, 0x40, 0x3C, 0x40, 0x3F,
0x00, 0x63, 0x14, 0x08, 0x14, 0x63,
0x00, 0x07, 0x08, 0x70, 0x08, 0x07,
0x00, 0x71, 0x49, 0x45, 0x43, 0x00,
0x00, 0x00, 0x7F, 0x41, 0x41, 0x00,
0x00, 0x02, 0x04, 0x08, 0x10, 0x20,
0x00, 0x00, 0x41, 0x41, 0x7F, 0x00,
0x00, 0x04, 0x02, 0x01, 0x02, 0x04,
0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
0x00, 0x00, 0x03, 0x07, 0x00, 0x00,
0x00, 0x20, 0x54, 0x54, 0x54, 0x78,
0x00, 0x7F, 0x44, 0x44, 0x44, 0x38,
0x00, 0x38, 0x44, 0x44, 0x44, 0x28,
0x00, 0x38, 0x44, 0x44, 0x44, 0x7F,
0x00, 0x38, 0x54, 0x54, 0x54, 0x08,
0x00, 0x08, 0x7E, 0x09, 0x09, 0x00,
0x00, 0x18, 0xA4, 0xA4, 0xA4, 0x7C,
0x00, 0x7F, 0x04, 0x04, 0x78, 0x00,
0x00, 0x00, 0x00, 0x7D, 0x40, 0x00,
0x00, 0x40, 0x80, 0x84, 0x7D, 0x00,
0x00, 0x7F, 0x10, 0x28, 0x44, 0x00,
0x00, 0x00, 0x00, 0x7F, 0x40, 0x00,
0x00, 0x7C, 0x04, 0x18, 0x04, 0x78,
0x00, 0x7C, 0x04, 0x04, 0x78, 0x00,
0x00, 0x38, 0x44, 0x44, 0x44, 0x38,
0x00, 0xFC, 0x44, 0x44, 0x44, 0x38,
0x00, 0x38, 0x44, 0x44, 0x44, 0xFC,
0x00, 0x44, 0x78, 0x44, 0x04, 0x08,
0x00, 0x08, 0x54, 0x54, 0x54, 0x20,
0x00, 0x04, 0x3E, 0x44, 0x24, 0x00,
0x00, 0x3C, 0x40, 0x20, 0x7C, 0x00,
0x00, 0x1C, 0x20, 0x40, 0x20, 0x1C,
0x00, 0x3C, 0x60, 0x30, 0x60, 0x3C,
0x00, 0x6C, 0x10, 0x10, 0x6C, 0x00,
0x00, 0x9C, 0xA0, 0x60, 0x3C, 0x00,
0x00, 0x64, 0x54, 0x54, 0x4C, 0x00,
0x00, 0x08, 0x3E, 0x41, 0x41, 0x00,
0x00, 0x00, 0x00, 0x77, 0x00, 0x00,
0x00, 0x00, 0x41, 0x41, 0x3E, 0x08,
0x00, 0x02, 0x01, 0x02, 0x01, 0x00,
0x00, 0x3C, 0x26, 0x23, 0x26, 0x3C
};

/*
    Definicion de la estructura de datos dispuesta para la pantalla TFT
*/
typedef struct _tftdisp_class_obj_t{
    mp_obj_base_t base;
    const spi_t *spi;
    bool power_on;
    bool inverted;
    bool backlight_on;
    uint8_t margin_row;
    uint8_t margin_col;
    uint8_t width;
    uint8_t height;
} tftdisp_class_obj_t;

const mp_obj_type_t tftdisp_class_type;

//Funcion print
STATIC void tftdisp_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "tftdisp_class()");
}

/*
    make_new: Constructor de la clase. Esta funcion se invoca cuando el usuario de Micropython escribe:
        ST7735()
*/
STATIC mp_obj_t tftdisp_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    tftdisp_class_obj_t *self = m_new_obj(tftdisp_class_obj_t);
    self->base.type = &tftdisp_class_type;

    //definicion de los pines de trabajo para la TFT  
    mp_hal_pin_config(Pin_DC, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN,0);
    mp_hal_pin_config(Pin_CS, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN,0);
    mp_hal_pin_config(Pin_RST, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN,0);
    mp_hal_pin_config(Pin_BL, MP_HAL_PIN_MODE_OUTPUT, MP_HAL_PIN_PULL_DOWN,0);
    
    //definición de los estados logicos 
    self->power_on=true;
    self->inverted=false;
    self->backlight_on=true;
    //inicialiacion de las columnas y filas del display TFT
    self->margin_row=0;
    self->margin_col=0;
    self->spi=&spi_obj[0];
    // Configuraciones de la comunicacion SPI
    //spi_set_params(&spi_obj[0], PRESCALE, BAUDRATE, POLARITY, PHASE, BITS, FIRSTBIT);
    SPI_InitTypeDef *init = &self->spi->spi->Init;
    init->Mode = SPI_MODE_MASTER;
    init->BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    init->CLKPolarity = SPI_POLARITY_HIGH;
    init->CLKPhase = SPI_PHASE_2EDGE;
    init->Direction = SPI_DIRECTION_2LINES;
    init->DataSize = SPI_DATASIZE_8BIT;
    init->NSS = SPI_NSS_SOFT;
    init->FirstBit = SPI_FIRSTBIT_MSB;
    init->TIMode = SPI_TIMODE_DISABLED;
    init->CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    init->CRCPolynomial = 0;
    spi_init(self->spi,false);

    return MP_OBJ_FROM_PTR(self);
}

//  Funciones Aqui

/*
    write_cmd() Intern function | Nos sirve para comunicarnos con la pantalla TFT a traves de comandos preestablecidos, los
    cuales sirven para configurar la tft previo a su funcionamiento.
    Ejemplo de transmision de datos en Python:
                self.write_cmd(CMD_RASET)
*/
STATIC void write_cmd(int cmd)
{
    mp_hal_pin_low(Pin_DC);
    mp_hal_pin_low(Pin_CS);
    //definimos un espacio de tamaño de 1 byte
    uint8_t aux[1]={(uint8_t)cmd};
    spi_transfer(&spi_obj[0],1, aux, NULL, TIMEOUT_SPI);

    mp_hal_pin_high(Pin_CS);
}

/*
    write_data() Intern function | Nos permite mandar arreglos de memoria definidos del tipo bytearray solo de manera interna para el control
    de datos que conforman ciertas funciones como show_image().
    Ejemplo de transmision de datos en Python:
                self.write_data(bytearray([0x00, y0 + self.margin_row, 0x00, y1 + self.margin_row]))
*/
STATIC void write_data( uint8_t *data, uint8_t len)
{
    mp_hal_pin_high(Pin_DC);
    mp_hal_pin_low(Pin_CS);
    //Medimos el tamaño del array con sizeof() para saber el tamaño en bytes
    spi_transfer(&spi_obj[0],len, data, NULL, TIMEOUT_SPI);

    mp_hal_pin_high(Pin_CS);
}
/*
    set_window() intern function | Define configuraciones para las filas y columnas en el display de la pantalla de tal manera que
    cuando se ubique un pixel o caracter? este se preconfigure.
    
    Disclaimer.
        Se necesita hacer una conversion a los objetos con los que trabajes en las otras funciones que se definan
        dentro de uPython.
*/
STATIC void set_window(mp_obj_t self_in, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
    //Set window frame boundaries.
    //Any pixels written to the display will start from this area. 

    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    // set row XSTART/XEND
    write_cmd(CMD_RASET);
    uint8_t bytes_send[]={0x00, y0 + self->margin_row, 0x00, y1 + self->margin_row};
    write_data(bytes_send, sizeof(bytes_send));

    // set column XSTART/XEND
    write_cmd(CMD_CASET);
    uint8_t bytes_send1[]={0x00, x0 + self->margin_col, 0x00, x1 + self->margin_col};
    write_data(bytes_send1, sizeof(bytes_send1));

    // write addresses to RAM
    write_cmd(CMD_RAMWR);
}

/*
    reset() Hard reset the display.
*/
STATIC void reset(void)
{
    mp_hal_pin_low(Pin_DC);
    mp_hal_pin_high(Pin_RST);
    mp_hal_delay_ms(500);
    mp_hal_pin_low(Pin_RST);
    mp_hal_delay_ms(500);
    mp_hal_pin_high(Pin_RST);
    mp_hal_delay_ms(500);
}

/*
    write_pixels() intern function | Sirve para dibujar un pixel en el display
    de tal forma que todas las funciones necesitan a esta funcion para dibujar
    en pantalla los pixeles deseados rebiendo en si el tamaño de pixeles a dibujar
    y el color.
*/

STATIC void write_pixels(uint16_t count, uint16_t color)
{
        //Write pixels to the display.
        //count - total number of pixels
        //color - 16-bit RGB value
    uint8_t data_transfer[2]={(uint8_t)(color>>8), (uint8_t)(color&0xFF)};
    mp_hal_pin_high(Pin_DC);
    mp_hal_pin_low(Pin_CS);
    for(uint16_t i=0; i<count;i++)
    {   
        //Mandamos el color del tamaño de 2 bytes si el color es de 16 bits
        //siempre se mandan 2 bytes a esta funcion
        spi_transfer(&spi_obj[0],2,data_transfer, NULL, TIMEOUT_SPI);
    }
    mp_hal_pin_high(Pin_CS);
}
/*
    hline() | Intern Function. Esta funcion es de uso interno para la creacion de
    lineas horizontales en el display TFT.
    Acepta parametros primitivos al ser de uso interno.
*/
STATIC mp_obj_t hline(mp_obj_t self_in, uint8_t x, uint8_t y, uint8_t w, uint16_t color)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(x>=self->width || y>=self->height)
    {
        return mp_const_none;
    }
    set_window(self, x, y, x+w-1, y);
    write_pixels(x+w-1, color);
    return mp_const_none;
}

/*
    vline() | Intern function. Esta funcion es de uso interno para la
    creacion de lineas verticales en el display TFT, de este manera se
    envian parametros primitivos al ser de uso interno
*/
STATIC mp_obj_t vline(mp_obj_t self_in, uint8_t x, uint8_t y, uint8_t h, uint16_t color)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(x>=self->width || y>=self->height)
    {
        return mp_const_none;
    }
    if((y+h-1)>=self->height)
    {
        h=self->height-y;
    }
    set_window(self, x, y, x, y+h-1);
    write_pixels(y+h-1, color);
    return mp_const_none;
}

/*
    pixel0(x, y, color) | Intern Function nos sirve para dibujar un solo pixel individual en la pantalla TFT
    de tal forma que funciones de uso publico en python puedan mandar datos primitivos, 
    para acelerar el proceso de trazado en el display tft.
    
*/
STATIC mp_obj_t pixel0(mp_obj_t self_in, uint8_t x, uint8_t y, uint16_t color )
{
    //Draw a single pixel0 on the display with given color.
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    set_window(self, x, y, x+1, y+1);
    write_pixels(1,color);
    return mp_const_none;
}
/*
    rect_int | Intern Function is the same with functio rect only receive primitive
    params.
*/
STATIC mp_obj_t rect_int(mp_obj_t self_in, uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
    //Draw a rectangle with specified coordinates/size and fill with color.
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(x>=self->width || y>=self->height)
    {
        return mp_const_none;
    }
    if((x+w-1) >= self->width)
    {
        w=self->width-x;
    }
    if((y+h-1)>=self->height)
    {
        h=self->height-y;
    }
    set_window(self, x, y, x+w-1, y+h-1);
    write_pixels((w*h), color);
    return mp_const_none;
}

/*
    st7735() es la funcion que inicializa la pantalla TFT es el equivalente a:
        ST7735().init()
    En este caso habra un cambio el cual la funcion se invocara de la siguiente manera:
        ST7735().init(True)

*/
STATIC mp_obj_t st7735_init(size_t n_args, const mp_obj_t *args)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    //mp_obj_is_bool(orient);
    //primer hard reset 

    reset(); //function here

    write_cmd(CMD_SWRESET);

    mp_hal_delay_ms(150);
    write_cmd(CMD_SLPOUT);

    mp_hal_delay_ms(255);

    //Optimizacion de la transmision de datos y delays
    write_cmd(CMD_FRMCTR1);

    //convertirlo en array dinamico?
    uint8_t data_set3[]={0x01, 0x2C, 0x2D};
    write_data(data_set3, sizeof(data_set3));

    write_cmd(CMD_FRMCTR2);

    uint8_t data_set6[]={0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D};
    write_data(data_set6, sizeof(data_set6));

    mp_hal_delay_ms(10);

    write_cmd(CMD_INVCTR);

    uint8_t data_set1[]={0x07};
    write_data(data_set1, sizeof(data_set1));

    
    write_cmd(CMD_PWCTR1);

    uint8_t dataset[]={0xA2, 0x02, 0x84};
    write_data(dataset, sizeof(dataset));

    write_cmd(CMD_PWCTR2);

    uint8_t dataset1[]={0xC5};
    write_data(dataset1, sizeof(dataset1));

    write_cmd(CMD_PWCTR3);

    uint8_t dataset2[]={0x8A, 0x00};
    write_data(dataset2, sizeof(dataset2));

    write_cmd(CMD_PWCTR4);

    uint8_t dataset3[]={0x8A, 0x2A};
    write_data(dataset3, sizeof(dataset3));

    write_cmd(CMD_PWCTR5);

    uint8_t dataset4[]={0x8A, 0xEE};
    write_data(dataset4, sizeof(dataset4));

    write_cmd(CMD_VMCTR1);

    uint8_t data_vmc[]={0x0E};
    write_data(data_vmc, sizeof(data_vmc));

    write_cmd(CMD_INVOFF);

    write_cmd(CMD_MADCTL);

    if(args[1]==0)
    {
        uint8_t data_orient[]={0xA0};
        write_data(data_orient, sizeof(data_orient));

        self->width=160;
        self->height=128;
    }
    else
    {
        uint8_t datas[]={0x00};
        write_data(datas, sizeof(datas));

        self->width=128;
        self->height=160;

    }
    write_cmd(CMD_COLMOD);

    uint8_t dataset0[]={0x05};
    write_data(dataset0, sizeof(dataset0));


    write_cmd(CMD_CASET);

    uint8_t dataset5[]={0x00, 0x01, 0x00, 127};
    write_data(dataset5, sizeof(dataset5));


    write_cmd(CMD_RASET);

    uint8_t dataset6[]={0x00, 0x01, 0x00, 159};
    write_data(dataset6, sizeof(dataset6));

    
    write_cmd(CMD_GMCTRP1);

    uint8_t dataset7[]={0x02, 0x1c, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2d, 0x29, 0x25, 0x2b, 0x39, 0x00, 0x01, 0x03, 0x10};
    write_data(dataset7, sizeof(dataset7));


    write_cmd(CMD_GMCTRN1);

    uint8_t dataset8[]={0x03, 0x1d, 0x07, 0x06, 0x2e, 0x2c, 0x29, 0x2d, 0x2e, 0x2e, 0x37, 0x3f, 0x00, 0x00, 0x02, 0x10};
    write_data(dataset8, sizeof(dataset8));


    write_cmd(CMD_NORON);

    mp_hal_delay_ms(10);

    write_cmd(CMD_DISPON);

    mp_hal_delay_ms(100);
    
    return mp_const_none;
}

/*
    power() esta funcion sirve para encender la pantalla u obtener el estado de la pantalla?
*/
STATIC mp_obj_t power(mp_obj_t self_in, mp_obj_t state)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(state==mp_const_none)
    {
        return mp_obj_new_bool(self->power_on?1:0);
    }
    if(state==mp_const_true)
    {
        write_cmd(CMD_DISPON);
        self->power_on=true;
    }
    if (state==mp_const_false)
    {
        write_cmd(CMD_DISPOFF);
        self->power_on=false;
    }
    return mp_const_none;
    
}

/*
    inverted() sirve para obtener una inversion de colores en la TFT a traves de comandos
*/
STATIC mp_obj_t inverted(mp_obj_t self_in, mp_obj_t state)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if(state==0)
    {
        return mp_obj_new_bool(self->inverted?1:0);
    }
    if(state==mp_const_true || state==mp_obj_new_int(1))
    {
        write_cmd(CMD_INVON);
        self->inverted=true;
    }
    else
    {
        write_cmd(CMD_INVOFF);
        self->inverted=false;
    }
    
    return mp_const_none;
}

/*
    backlight() Esta funcion permite encender la luz de la pantalla TFT.
*/
STATIC mp_obj_t backlight(mp_obj_t self_in, mp_obj_t state)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if(state==mp_const_none)
    {
        return mp_obj_new_bool(self->backlight_on?1:0);
    }
    if(state==mp_obj_new_int(1) || state==mp_const_true)
    {
        mp_hal_pin_high(Pin_BL);
        self->backlight_on=true;
    }
    else
    {
        mp_hal_pin_low(Pin_BL);
        self->backlight_on=false;
    }
    return mp_const_none;
}
/*
    pixel(x, y, color) nos sirve para dibujar un solo pixel individual en la pantalla TFT
    Ejemplo en uPython:
        tft.pixel(10,20,tft.rgbcolor(255,25,0))
*/
STATIC mp_obj_t pixel(size_t n_args, const mp_obj_t *args)
{
    //Draw a single pixel on the display with given color.
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint8_t x_int8=(uint8_t)mp_obj_get_int(args[1]);
    uint8_t y_int8=(uint8_t)mp_obj_get_int(args[2]);
    uint16_t color_int16=mp_obj_get_int(args[3]);
    set_window(self, x_int8, y_int8, x_int8+1, y_int8+1);
    write_pixels(1,color_int16);
    return mp_const_none;
}
/*
    rgbcolor() funcion para establecer colores en la pantalla tft dado los
    parametros Red, Green, Blue.
*/
STATIC mp_obj_t rgbcolor(size_t n_args, const mp_obj_t *args)
{
    //Pack 24-bit RGB into 16-bit value.
    uint8_t red, green, blue;
    red=mp_obj_get_int(args[1]);
    green=mp_obj_get_int(args[2]);
    blue=mp_obj_get_int(args[3]);
    return mp_obj_new_int(((red & 0xF8) << 8) | ((green & 0xFC) << 3 ) | (blue >> 3));
}

/*
    rect() Esta funcion nos sirve para la creacion de un
    cuadrilatero en el que a traves de las coordenadas x, y
    , la altura h, la anchura w y el color se implementa en 
    esta función.
    Ejemplo de uso en uPython:
        tft.rect(10,20,50,60,tft.rgbcolor(23,0,254))
*/

STATIC mp_obj_t rect(size_t n_args, const mp_obj_t *args)
{
    //Draw a rectangle with specified coordinates/size and fill with color.
    tftdisp_class_obj_t *self = args[0];
    uint8_t x=mp_obj_get_int(args[1]);
    uint8_t y=mp_obj_get_int(args[2]);
    uint8_t w=mp_obj_get_int(args[3]);
    uint8_t h=mp_obj_get_int(args[4]);
    uint16_t color=mp_obj_get_int(args[5]);
    if(x>=self->width || y>=self->height)
    {
        return mp_const_none;
    }
    if((x+w-1) >= self->width)
    {
        w=self->width-x;
    }
    if((y+h-1)>=self->height)
    {
        h=self->height-y;
    }
    set_window(self, x, y, x+w-1, y+h-1);
    write_pixels((w*h), color);
    return mp_const_none;
}

/*
    line() Esta funcion crea el trazado de lineas en el display
    a través del algoritmo de Bresenham's
*/
STATIC mp_obj_t line(size_t n_args, const mp_obj_t *args)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    //transformar los objetos
    uint8_t x0=mp_obj_get_int(args[1]);
    uint8_t y0=mp_obj_get_int(args[2]);
    uint8_t x1=mp_obj_get_int(args[3]);
    uint8_t y1=mp_obj_get_int(args[4]);
    uint16_t color=mp_obj_get_int(args[5]);
    uint8_t start, end;
    //comienzo de la funcion
    if(x0==x1)
    {
        if (y1<y0)
        {
            start=x1;
            end=y1;
        }
        else
        {
            start=x0;
            end=y0;
        }
        vline(self, start, end, abs(y1-y0)+1, color);
    }
    else if(y0==y1)
    {
        if (x1<x0)
        {
            start=x1;
            end=y1;
        }
        else
        {
            start=x0;
            end=y0;
        }
        hline(self, start, end, abs(x1-x0)+1, color);
    }
    else
    {   
        //Comienzo del algoritmo de Bresenham's
        int dx = abs(x1-x0), inx = x0<x1 ? 1 : -1;
        int dy = abs(y1-y0), iny = y0<y1 ? 1 : -1; 
        int err = (dx>dy ? dx : -dy)/2, e2;
 
        while(1){
            pixel0(self, x0, y0, color);
            if (x0==x1 && y0==y1) break;
            e2 = err;
            if(e2 >-dx) 
            { 
                err -= dy; 
                x0 += inx; 
            }
            if(e2 < dy) 
            { 
                err += dx;
                y0 += iny; }
            }
    }
    return mp_const_none;
    
}

/*
    char() | Intern Function esta funcion pone un solo caracter en la pantalla
    tft, esta funcion es dependencia de la funcion text()
*/
STATIC mp_obj_t charfunc(mp_obj_t self_in, uint8_t x, uint8_t y, char ch, uint16_t color, uint8_t sizex, uint8_t sizey)
{
    //Draw a character at a given position using the user font.
    //Font is a data dictionary, can be scaled with sizex and sizey.
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //Font is define not necesary put parameter in this function
    uint8_t startchar=START;
    uint8_t endchar=END;
    uint16_t ci=(uint16_t)ch;

    if(!sizex && !sizey)
    {
        //this is by defect in the function uPython
        sizex=1;
        sizey=1;
    }

    if(startchar<=ci && ci<=endchar)
    {
        ci=(ci-startchar)*WIDTH;

        // this the equivalent to ch = font['data'][ci:ci + width]
        uint8_t ch[6];
        for(uint8_t i=0;i<WIDTH;i++)
        {
            ch[i]=Font[ci];
            ci++;
        }
        //end to equivalent
        
        //no font scaling
        uint8_t px=x;

        if(sizex<=1 && sizey<=1)
        {
            for(uint8_t k=0; k<WIDTH;k++)
            {
                uint8_t py=y;

                char temp = ch[k];
                for(uint8_t i=0; i<HEIGHT;i++)
                {
                    if(temp&0x01)
                    {
                        pixel0(self, px, py, color);
                    }
                    py+=1;
                    temp>>=1;
                }
                px+=1;
            }
        }
        else //scale to given sizes
        {
            for(uint8_t c=0;c<WIDTH;c++)
            {
                uint8_t py=y;

                for(uint8_t i=0; i<HEIGHT;i++)
                {
                    if(ch[c]&0x01)
                    {
                        rect_int(self, px, py, sizex, sizey, color);
                    }
                    
                    py+=sizey;
                    c>>=1;
                }
                px+=sizex;
            }
        }
    }
        // character not found in this font
        return mp_const_none;
} 

/*
    text()
*/
STATIC mp_obj_t text(size_t n_args, const mp_obj_t *args)
{
    //Draw text at a given position using the user font.
    //Font can be scaled with the size parameter.
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    uint8_t x = mp_obj_get_int(args[1]);
    uint8_t y = mp_obj_get_int(args[2]);

    mp_check_self(mp_obj_is_str_or_bytes(args[3]));
    GET_STR_DATA_LEN(args[3], str, str_len);
    char string[str_len];
    strcpy(string, (char *)str);

    uint16_t color = mp_obj_get_int(args[4]);

    uint8_t width=WIDTH+1;

    uint8_t px=x;

    for(uint8_t i=0;i<str_len;i++)
    {
        charfunc(self, px, y, string[i], color, 1, 1);
        px+=width;
        // wrap the text to the next line if it reaches the end
        if(px+width>self->width)
        {
            y+=HEIGHT+1;
            px=x;
        }
    }
    return mp_const_none;

}
/*

*/
STATIC mp_obj_t clear(mp_obj_t self_in, mp_obj_t color)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint16_t color16b=mp_obj_get_int(color);
    rect_int(self, 0, 0, self->width, self->height, color16b);
    return mp_const_none;
}

/*
  show_image()   
*/
STATIC mp_obj_t show_image(size_t n_args, const mp_obj_t *args)
{   
    //tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    /*
        Se supone que yo recibo el puntero pero de alguna manera que
        es lo que tengo que hacer con el ?
        ya esta almacenado, pero en la funcion no tiene ningun uso especifico
    */
    /*
    mp_check_self(mp_obj_is_str_or_bytes(args[1]));
    GET_STR_DATA_LEN(args[1], str, str_len);  
    FIL fil;
    FATFS fatfs;
    UINT br;
    //traspaso el obj str a char en C
    char name[str_len];
    strcpy(name, (char *)str);
    FRESULT file;
    file=f_mount(&fatfs);
    if(file!=FR_OK)
    {
        printf("mount error\n");
        return mp_const_none;
    }
    file=f_open(&fatfs, &fil, name, FA_OPEN_EXISTING|FA_READ);
    if(file!=FR_OK)
    {
        printf("No such file\n");
        return mp_const_none;
    }
    char buff[(uint)f_size(&fil)];
    file=f_read(&fil,buff,f_size(&fil),&br);
    */
    return mp_const_none;

    
}
//Se asocian las funciones arriba escritas con su correspondiente objeto de funcion para Micropython.
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(st7735_init_obj, 1, 2, st7735_init);
MP_DEFINE_CONST_FUN_OBJ_2(inverted_obj, inverted);
MP_DEFINE_CONST_FUN_OBJ_2(power_obj, power);
MP_DEFINE_CONST_FUN_OBJ_2(backlight_obj, backlight);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rgbcolor_obj,4 , 4, rgbcolor);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pixel_obj, 4, 4, pixel);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rect_obj, 6, 6, rect);
MP_DEFINE_CONST_FUN_OBJ_VAR(line_obj, 6, line);
MP_DEFINE_CONST_FUN_OBJ_VAR(text_obj, 5, text);
MP_DEFINE_CONST_FUN_OBJ_2(clear_obj, clear);
MP_DEFINE_CONST_FUN_OBJ_VAR(show_image_obj, 4, show_image);
/*
    Se asocia el objeto de funcion de Micropython con cierto string, que sera el que se utilice en la
    programacion en Micropython. Ej: Si se escribe:
        ST7735().backlight(1)
    Internamente se llama al objeto de funcion backlight_state_obj, que esta asociado con la funcion
    backlight_state, que cambia el estado de la luz de fondo de la pantalla TFT.
*/
STATIC const mp_rom_map_elem_t tftdisp_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&st7735_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_inverted), MP_ROM_PTR(&inverted_obj) },
    { MP_ROM_QSTR(MP_QSTR_power), MP_ROM_PTR(&power_obj) },
    { MP_ROM_QSTR(MP_QSTR_backlight), MP_ROM_PTR(&backlight_obj) },
    { MP_ROM_QSTR(MP_QSTR_rgbcolor), MP_ROM_PTR(&rgbcolor_obj) },
    { MP_ROM_QSTR(MP_QSTR_pixel), MP_ROM_PTR(&pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_rect), MP_ROM_PTR(&rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_line), MP_ROM_PTR(&line_obj) },
    { MP_ROM_QSTR(MP_QSTR_text), MP_ROM_PTR(&text_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear), MP_ROM_PTR(&clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_show_image), MP_ROM_PTR(&show_image_obj) },
    //Nombre de la func. que se va a invocar en Python     //Pointer al objeto de la func. que se va a invocar.
};
                                
STATIC MP_DEFINE_CONST_DICT(tftdisp_class_locals_dict, tftdisp_class_locals_dict_table);

const mp_obj_type_t tftdisp_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_tftdisp,
    .print = tftdisp_class_print,
    .make_new = tftdisp_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&tftdisp_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_tftdisp_globals_table[] = {
                                                    //Nombre del archivo (User C module)
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_tftdisp) },
            //Nombre de la clase        //Nombre del "tipo" asociado.
    { MP_ROM_QSTR(MP_QSTR_ST7735), MP_ROM_PTR(&tftdisp_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_tftdisp_globals, ophyra_tftdisp_globals_table);

// Define module object.
const mp_obj_module_t ophyra_tftdisp_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_tftdisp_globals,
};

// Registro del modulo para hacerlo disponible para Python.
//                      nombreArchivo, nombreArchivo_user_cmodule, MODULE_IDENTIFICADOR_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_tftdisp, ophyra_tftdisp_user_cmodule, MODULE_OPHYRA_TFTDISP_ENABLED);

