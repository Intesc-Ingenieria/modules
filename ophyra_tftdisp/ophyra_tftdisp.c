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
    Ultima fecha de modificacion: 05/04/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"          
#include "ports/stm32/spi.h"
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
#define PRESCALE        (2)
#define BAUDRATE        (8000000)
#define POLARITY        (1)
#define PHASE           (0)
#define BITS            (8)
#define FIRSTBIT        (0x00000000U)

#define TIMEOUT_SPI     (5000)
//#define SPI1            (1)


/*
    Font Lib implemented here.
*/
#define WIDTH       (6)
#define HEIGHT      (8)
#define START       (32)
#define END         (127)
static const uint8_t Font[]=
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
    bool power_on;
    bool inverted;
    bool blacklight_on;
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
    self->blacklight_on=true;
    //inicialiacion de las columnas y filas del display TFT
    self->margin_row=0;
    self->margin_col=0;

    // Configuraciones de la comunicacion SPI
    spi_set_params(&spi_obj[1], PRESCALE, BAUDRATE, POLARITY, PHASE, BITS, FIRSTBIT);
    spi_init(&spi_obj[1],false);

    return MP_OBJ_FROM_PTR(self);
}

//  Funciones Aqui

/*
    write_cmd() nos sirve para comunicarnos con la pantalla TFT a traves de comandos preestablecidos, los
    cuales sirven para configurar la tft previo a su funcionamiento.
    Ejemplo de transmision de datos en Python:
                self.write_cmd(CMD_RASET)
*/
STATIC void write_cmd(int cmd)
{
    mp_hal_pin_low(Pin_DC);
    mp_hal_pin_low(Pin_CS);
    //definimos un espacio de tamaño de 1 byte
    spi_transfer(&spi_obj[1],1, (const uint8_t *)cmd, NULL, TIMEOUT_SPI);
    mp_hal_pin_high(Pin_CS);
}

/*
    write_data() nos permite mandar arreglos de memoria definidos del tipo bytearray solo de manera interna para el control
    de datos que conforman ciertas funciones como show_image().
    Ejemplo de transmision de datos en Python:
                self.write_data(bytearray([0x00, y0 + self.margin_row, 0x00, y1 + self.margin_row]))
*/
STATIC void write_data( uint8_t *data, uint8_t len)
{
    mp_hal_pin_high(Pin_DC);
    mp_hal_pin_low(Pin_CS);
    //Medimos el tamaño del array con sizeof() para saber el tamaño en bytes
    spi_transfer(&spi_obj[1],len, data, NULL, TIMEOUT_SPI);
    mp_hal_pin_high(Pin_CS);
}

/*
    st7735() es la funcion que inicializa la pantalla TFT es el equivalente a:
        ST7735().init()
    En este caso habra un cambio el cual la funcion se invocara de la siguiente manera:
        ST7735().init(True)

*/
STATIC mp_obj_t st7735_init(mp_obj_t self_in, mp_obj_t orient)
{
    tftdisp_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    //mp_obj_is_bool(orient);
    //primer hard reset 
    //reset() function here
    write_cmd(CMD_SWRESET);
    mp_hal_delay_ms(150);
    write_cmd(CMD_SLPOUT);
    mp_hal_delay_ms(255);

    //Optimizacion de la transmision de datos y delays
    write_cmd(CMD_FRMCTR1);
    //convertirlo en array dinamico?
    uint8_t data_set3[]={0x01, 0x2C, 0x2C};
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

    if(orient==mp_const_none || orient==mp_const_true)
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

//Se asocian las funciones arriba escritas con su correspondiente objeto de funcion para Micropython.
MP_DEFINE_CONST_FUN_OBJ_2(st7735_init_obj, st7735_init);


/*
    Se asocia el objeto de funcion de Micropython con cierto string, que sera el que se utilice en la
    programacion en Micropython. Ej: Si se escribe:
        ST7735().backlight(1)
    Internamente se llama al objeto de funcion backlight_state_obj, que esta asociado con la funcion
    backlight_state, que cambia el estado de la luz de fondo de la pantalla TFT.
*/
STATIC const mp_rom_map_elem_t tftdisp_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&st7735_init_obj) },
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

