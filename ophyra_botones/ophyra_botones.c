/*
    ophyra_botones.c

    Modulo de usuario en C para la utilizacion de los cuatro botones la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Este archivo incluye la definicion de una clase con 4 funciones, que retornan el estado del pin
    en donde esta el boton especificado.

    Para construir el firmware incluyendo este modulo, ejecutar en Cygwin:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_BOTONES_ENABLED=1 all

        En USER_C_MODULES se especifica el path hacia la localizacion de la carpeta modules, donde se encuentra
        el archivo ophyra_botones.c y micropython.mk

    Escrito por: Jonatan Salinas.
    Ultima fecha de modificacion: 04/03/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"          //Para el uso de la funcion mp_hal_pin_config

typedef struct _buttons_class_obj_t{
    mp_obj_base_t base;
} buttons_class_obj_t;

const mp_obj_type_t buttons_class_type;

//Funcion print
STATIC void buttons_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "buttons_class()");
}

/*
    make_new: Constructor de la clase. Esta funcion se invoca cuando el usuario de Micropython escribe:
        sw()
*/
STATIC mp_obj_t buttons_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    buttons_class_obj_t *self = m_new_obj(buttons_class_obj_t);
    self->base.type = &buttons_class_type;

    //Configuracion de los 4 pines donde estan los botones en la Ophyra, como INPUT y PULL_UP
    mp_hal_pin_config(pin_C2, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);
    mp_hal_pin_config(pin_D5, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);
    mp_hal_pin_config(pin_D4, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);
    mp_hal_pin_config(pin_D3, MP_HAL_PIN_MODE_INPUT, MP_HAL_PIN_PULL_UP, 0);

    return MP_OBJ_FROM_PTR(self);
}

/*
    Estas 4 funciones retornan el estado actual de un pin especifico, para saber si el boton ha sido presionado
    o no. Retorna 0 si el boton esta presionado, y 1 si no lo esta.
*/
STATIC mp_obj_t button0_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_C2));
}

STATIC mp_obj_t button1_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_D5));
}

STATIC mp_obj_t button2_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_D4));
}

STATIC mp_obj_t button3_pressed(mp_obj_t self_in) {
    return mp_obj_new_int(mp_hal_pin_read(pin_D3));
};

//Se asocian las funciones arriba escritas con su correspondiente objeto de funcion para Micropython.
MP_DEFINE_CONST_FUN_OBJ_1(button0_pressed_obj, button0_pressed);
MP_DEFINE_CONST_FUN_OBJ_1(button1_pressed_obj, button1_pressed);
MP_DEFINE_CONST_FUN_OBJ_1(button2_pressed_obj, button2_pressed);
MP_DEFINE_CONST_FUN_OBJ_1(button3_pressed_obj, button3_pressed);

/*
    Se asocia el objeto de funcion de Micropython con cierto string, que sera el que se utilice en la
    programacion en Micropython. Ej: Si se escribe:
        sw().sw1()
    Internamente se llama al objeto de funcion button0_pressed_obj, que esta asociado con la funcion
    button0_pressed, que retorna el valor al leer el pin donde se encuentra el boton 1.
*/
STATIC const mp_rom_map_elem_t buttons_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_sw1), MP_ROM_PTR(&button0_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_sw2), MP_ROM_PTR(&button1_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_sw3), MP_ROM_PTR(&button2_pressed_obj) },
    { MP_ROM_QSTR(MP_QSTR_sw4), MP_ROM_PTR(&button3_pressed_obj) },
    //Nombre de la func. que se va a invocar en Python     //Pointer al objeto de la func. que se va a invocar.
};
                                
STATIC MP_DEFINE_CONST_DICT(buttons_class_locals_dict, buttons_class_locals_dict_table);

const mp_obj_type_t buttons_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_botones,
    .print = buttons_class_print,
    .make_new = buttons_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&buttons_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_botones_globals_table[] = {
                                                    //Nombre del archivo (User C module)
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_botones) },
            //Nombre de la clase        //Nombre del "tipo" asociado.
    { MP_ROM_QSTR(MP_QSTR_sw), MP_ROM_PTR(&buttons_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_botones_globals, ophyra_botones_globals_table);

// Define module object.
const mp_obj_module_t ophyra_botones_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_botones_globals,
};

// Registro del modulo para hacerlo disponible para Python.
//                      nombreArchivo, nombreArchivo_user_cmodule, MODULE_IDENTIFICADOR_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_botones, ophyra_botones_user_cmodule, MODULE_OPHYRA_BOTONES_ENABLED);

