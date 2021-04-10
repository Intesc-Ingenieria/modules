/*
    ophyra_eeprom.c

    Modulo de usuario en C para la utilizacion de la memoria EEPROM M24C32 en la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Este modulo en C contiene las funciones necesarias para escribir y leer cierta cantidad de bytes de la
    memoria EEPROM.

    Escrito por: Carlos D. Hernández y Jonatan Salinas.
    Ultima fecha de modificacion: 10/04/2021.

*/
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"       
#include "i2c.h"
#include "py/objstr.h"
#include <string.h>
#include <math.h>

#define M24C32_OPHYRA_ADDRESS         (80)          //ID o direccion de este esclavo para ser identificado en el puerto I2C
#define I2C_TIMEOUT_MS                (50)          //Timeout para I2C
#define PAGE_SIZE                     (32)          //Tamanio de pagina de la EEPROM modelo M24C32

typedef struct _eeprom_class_obj_t{
    mp_obj_base_t base;
} eeprom_class_obj_t;

const mp_obj_type_t eeprom_class_type;

/*
    Funcion print. Se invoca cuando el usuario escribe algo como:
    miEeprom = MC24C32()
    print(miEeprom)
*/
STATIC void eeprom_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "eeprom_class()");
}

/*
    make_new: Constructor de la clase. Esta funcion se invoca cuando el usuario de Micropython escribe algo similar a:
        miEeprom = MC24C32()
*/
STATIC mp_obj_t eeprom_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    eeprom_class_obj_t *self = m_new_obj(eeprom_class_obj_t);
    self->base.type = &eeprom_class_type;

    //Se inicia el puerto 1 de I2C.
    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    return MP_OBJ_FROM_PTR(self);
}

/*
    Funcion de escritura hacia la memoria EEPROM. Se invoca cuando el usuario de Micropython escribe algo como:
        miEeprom.write(0x6EA3, arregloBytes)
    En Micropython se deben especificar dos parametros:
        1.- La direccion de memoria a partir de donde se van a comenzar a escribir los datos.
            b11-b5 indican la pagina en donde se van a comenzar a escribir los datos.
            b4-b0 indican el offset de la pagina a partir del cual se van a comenzar a escribir los datos.
            
        2.- El arreglo de bytes (bytearray) que se va a escribir en memoria.
*/
STATIC mp_obj_t eeprom_write(mp_obj_t self_in, mp_obj_t eeaddr, mp_obj_t data_bytes_obj) {

    uint16_t addr = (uint16_t)mp_obj_get_int(eeaddr);

    mp_check_self(mp_obj_is_str_or_bytes(data_bytes_obj));
    GET_STR_DATA_LEN(data_bytes_obj, str, str_len);         //Este macro toma el bytearray pasado como parametro y
                                                            //genera el pointer "str", que es un pointer a caracter;
                                                            //y str_len, de tipo size_t que es la longitud del array.
    
    char mi_copia[str_len];                                 //Se hace una copia del string/array pasado como parametro.
    strcpy(mi_copia, (char *)str);

    uint16_t offset = addr&0x1F;                            //Calculo del offset de la pagina donde se comenzara a escribir
    uint16_t pag_inicio = (addr&0x0FE0)>>5;                 //De que pagina se comenzara a escribir?
    uint16_t pag_final = (uint16_t)floor(pag_inicio + (((int)(str_len) + offset)/PAGE_SIZE));   //En que pagina me detendre?
    uint16_t num_pags_a_escribir = (pag_final-pag_inicio) + 1;          //En cuantas paginas voy a escribir?

    uint16_t direccion_de_memoria;
    uint16_t bytes_arr_temp = 0;                            //variable que guardara el tamanio del array que se va a mandar
    uint16_t num_bytes_que_faltan = (uint16_t)(str_len);    //por cada pagina que se escriba

    int cont = 0;
    
    for(int pag_actual=0; pag_actual<num_pags_a_escribir; pag_actual++){
        if(num_pags_a_escribir==1){                             //Si solo escribire en una pagina
            bytes_arr_temp = num_bytes_que_faltan; 
        }
        else{                                                   //Si escribire en varias paginas
            if(pag_actual == 0){                                //Si estoy en la primera de ellas
                bytes_arr_temp = 32-offset;
            }
            else if(pag_actual == (num_pags_a_escribir-1)){     //Si estoy en la ultima pagina a escribir
                bytes_arr_temp = num_bytes_que_faltan;
            }
            else{
                bytes_arr_temp = 32;
            }
        }

        direccion_de_memoria = (pag_inicio<<5)|offset;          //Calculo de los 16 bits de la direccion de memoria a escribir.

        uint8_t datos_a_escribir[2+bytes_arr_temp];    
        datos_a_escribir[0] = (uint8_t)(direccion_de_memoria>>8);           //MSB de la direccion de memoria
        datos_a_escribir[1] = (uint8_t)(direccion_de_memoria&0xFF);         //LSB de la direccion de memoria

        for(int i=0; i<bytes_arr_temp; i++){                    //Se ponen en el array "datos_a_escribir" la cantidad de datos
            datos_a_escribir[i+2] = mi_copia[cont];             //a mandar en este envio
            cont++;
        }

        //Se escriben esos datos mediante I2C
        i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, datos_a_escribir, (2+bytes_arr_temp), true);

        num_bytes_que_faltan = num_bytes_que_faltan - bytes_arr_temp;       //Ahora, cuantos bytes faltan por escribir?
        pag_inicio++;                                                       //Paso a la siguiente pagina
        offset = 0;                                                         //Como estoy en una nueva pag, offset es 0.
        mp_hal_delay_us(6000);                                              //delay para permitir a la memoria escribir los datos

    }

    return mp_obj_new_int(0);
}

/*
    Funcion de lectura de la memoria EEPROM. Se invoca cuando el usuario de Micropython escribe algo como:
        PalR = miEeprom.read(0x6EA3, len(arregloBytes))
    En Micropython se deben especificar dos parametros:
        1.- La direccion de memoria a partir de donde se van a comenzar a leer los datos.
            b11-b5 indican la pagina en donde se van a comenzar a leer los datos.
            b4-b0 indican el offset de la pagina a partir del cual se van a comenzar a leer los datos.
            
        2.- La cantidad de bytes que se va a leer de la memoria.

    La funcion devuelve los bytes leidos, en forma de arreglo de bytes (bytearray).
*/
STATIC mp_obj_t eeprom_read(mp_obj_t self_in, mp_obj_t eeaddr, mp_obj_t bytes_a_leer) {

    uint16_t addr = (uint16_t)mp_obj_get_int(eeaddr);
    int bytes_que_leere = mp_obj_get_int(bytes_a_leer);

    uint8_t datos_leidos[bytes_que_leere];                      //Array donde se colocaran los datos leidos.
    int pos = 0;

    uint16_t offset = addr&0x1F;                                //Esta funcion es muy similar a la de escritura.
    uint16_t pag_inicio = (addr&0x0FE0)>>5;
    uint16_t pag_final = (uint16_t)floor(pag_inicio + ((bytes_que_leere + offset)/PAGE_SIZE));
    uint16_t num_pags_a_leer = (pag_final-pag_inicio) + 1;

    uint16_t direccion_de_memoria;
    uint16_t bytes_arr_temp = 0;
    uint16_t num_bytes_que_faltan = (uint16_t)(bytes_que_leere);
    
    for(int pag_actual=0; pag_actual<num_pags_a_leer; pag_actual++){
        if(num_pags_a_leer==1){       
            bytes_arr_temp = num_bytes_que_faltan; 
        }
        else{
            if(pag_actual == 0){
                bytes_arr_temp = 32-offset;
            }
            else if(pag_actual == (num_pags_a_leer-1)){
                bytes_arr_temp = num_bytes_que_faltan;
            }
            else{
                bytes_arr_temp = 32;
            }
        }

        direccion_de_memoria = (pag_inicio<<5)|offset;

        uint8_t direccion_a_leer[2];
        direccion_a_leer[0] = (uint8_t)(direccion_de_memoria>>8);       //MSB de la direccion a leer.
        direccion_a_leer[1] = (uint8_t)(direccion_de_memoria&0xFF);     //LSB de la direccion a leer.

        uint8_t arreglo_temporal[bytes_arr_temp];

        //Solo si bytes_arr_temp es diferente de 0, entonces lees.
        if(bytes_arr_temp != 0){
            i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, direccion_a_leer, 2, false);
            i2c_readfrom(I2C1,M24C32_OPHYRA_ADDRESS, arreglo_temporal, bytes_arr_temp, true);
        }

        for(int y=0; y<bytes_arr_temp; y++){                //Lo que lei, lo pongo en el array que sera devuelto
            datos_leidos[pos] = arreglo_temporal[y];        //al usuario al final de la funcion.
            pos++;
        }

        num_bytes_que_faltan = num_bytes_que_faltan - bytes_arr_temp;       //Cuantos bytes faltan por leer?
        pag_inicio++;
        offset = 0;       
    }

    return mp_obj_new_bytearray((size_t)bytes_que_leere, datos_leidos);     //Se devuelve el bytearray de los datos leidos.
};


//Se asocian las funciones arriba escritas con su correspondiente objeto de funcion para Micropython.
MP_DEFINE_CONST_FUN_OBJ_3(eeprom_write_obj, eeprom_write);
MP_DEFINE_CONST_FUN_OBJ_3(eeprom_read_obj, eeprom_read);

/*
    Se asocia el objeto de funcion de Micropython con cierto string, que sera el que se utilice en la
    programacion en Micropython. Ej: Si se escribe:
        MC224().read()
*/
STATIC const mp_rom_map_elem_t eeprom_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&eeprom_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&eeprom_write_obj) },
    //Nombre de la func. que se va a invocar en Python     //Pointer al objeto de la func. que se va a invocar.
};
                                
STATIC MP_DEFINE_CONST_DICT(eeprom_class_locals_dict, eeprom_class_locals_dict_table);

const mp_obj_type_t eeprom_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_eeprom,
    .print = eeprom_class_print,
    .make_new = eeprom_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&eeprom_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_eeprom_globals_table[] = {
                                                    //Nombre del archivo (User C module)
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_eeprom) },
            //Nombre de la clase        //Nombre del "tipo" asociado.
    { MP_ROM_QSTR(MP_QSTR_M24C32), MP_ROM_PTR(&eeprom_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_eeprom_globals, ophyra_eeprom_globals_table);

// Define module object.
const mp_obj_module_t ophyra_eeprom_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_eeprom_globals,
};

// Registro del modulo para hacerlo disponible para Python.
//                      nombreArchivo, nombreArchivo_user_cmodule, MODULE_IDENTIFICADOR_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_eeprom, ophyra_eeprom_user_cmodule, MODULE_OPHYRA_EEPROM_ENABLED);

