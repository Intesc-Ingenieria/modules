/*
    ophyra_botones.c

    Modulo de usuario en C para la utilizacion de la memoria eeprom en la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Este archivo incluye la definicion de una clase con 4 funciones, que retornan el estado del pin
    en donde esta el boton especificado.

    Para construir el firmware incluyendo este modulo, ejecutar en Cygwin:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_MODULE_OPHYRA_EEPROM_ENABLED=1 all

        En USER_C_MODULES se especifica el path hacia la localizacion de la carpeta modules, donde se encuentra
        el archivo ophyra_eeprom.c y micropython.mk

    Escrito por: Carlos D. Hernández.
    Ultima fecha de modificacion: 09/03/2021.

*/
#include <stdio.h>
#include <string.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"        
#include "i2c.h"

#define M24C32_OPHYRA_ADDRESS         (80)
#define I2C_TIMEOUT_MS                (50)

typedef struct _eeprom_class_obj_t{
    mp_obj_base_t base;
    //char data[10];
} eeprom_class_obj_t;

const mp_obj_type_t eeprom_class_type;

//Funcion print
STATIC void eeprom_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "eeprom_class()");
}

/*
    make_new: Constructor de la clase. Esta funcion se invoca cuando el usuario de Micropython escribe:
        MC24C32()
*/
STATIC mp_obj_t eeprom_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    eeprom_class_obj_t *self = m_new_obj(eeprom_class_obj_t);
    self->base.type = &eeprom_class_type;

    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    return MP_OBJ_FROM_PTR(self);
}

/*
    Estas 2 funciones retornan la escritura y lectura de la memoria eeprom. 
*/
STATIC mp_obj_t eeprom_write(mp_obj_t data_str) {
    //char data[]={'0'+shr, '0'+andr, '\0'};
    //strcat(data, value);
    //aqui va el i2c
    int response;

    response=i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, data_str, strlen(data_str), true);
    if(response==0)
    {
        printf("No se puede escribir en  la eeprom\n");
    }
    printf("Si se puede escribir\n");
    //mp_hal_delay_ms(500);
    return mp_obj_new_int(response); 
    //return mp_obj_new_int(1);

}

STATIC mp_obj_t eeprom_read() {
    
    char *data;
    //i2c debe de ir aqui
    i2c_readfrom(I2C1,M24C32_OPHYRA_ADDRESS,data,32,true);
    //mp_hal_delay_ms(500);
    printf(data);
    return mp_obj_new_str(data);
};


//Se asocian las funciones arriba escritas con su correspondiente objeto de funcion para Micropython.
MP_DEFINE_CONST_FUN_OBJ_1(eeprom_write_obj, eeprom_write);
MP_DEFINE_CONST_FUN_OBJ_0(eeprom_read_obj, eeprom_read);
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

