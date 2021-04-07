/*
    ophyra_eeprom.c

    Modulo de usuario en C para la utilizacion de la memoria eeprom en la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Para construir el firmware incluyendo este modulo, ejecutar en Cygwin:
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_EEPROM_ENABLED=1 all****

        En USER_C_MODULES se especifica el path hacia la localizacion de la carpeta modules, donde se encuentra
        el archivo ophyra_eeprom.c y micropython.mk

    Escrito por: Carlos D. Hernández y Jonatan Salinas.
    Ultima fecha de modificacion: 02/04/2021.

*/
#include <stdio.h>
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"
//#include "ports/stm32/mphalport.h"        
#include "i2c.h"
#include "py/objstr.h"
#include <string.h>
#include <stdio.h>
#include <math.h>


#define M24C32_OPHYRA_ADDRESS         (80)
#define I2C_TIMEOUT_MS                (50)
#define PAGE_SIZE                     (32)

typedef struct _eeprom_class_obj_t{
    mp_obj_base_t base;
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

    printf("Ando en make new\n");

    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    return MP_OBJ_FROM_PTR(self);
}

/*
    Estas 2 funciones retornan la escritura y lectura de la memoria eeprom. 
*/
STATIC mp_obj_t eeprom_write(mp_obj_t self_in, mp_obj_t eeaddr, mp_obj_t data_bytes_obj) {
    printf("**************************\n");
    printf("Se supone que voy a escribir\n");

    uint16_t addr = (uint16_t)mp_obj_get_int(eeaddr);
    printf("En la direccion: %d\n", addr);

    mp_check_self(mp_obj_is_str_or_bytes(data_bytes_obj));
    GET_STR_DATA_LEN(data_bytes_obj, str, str_len);

    printf("mi string length: %d\n", (int)str_len);

    char mi_copia[str_len];
    strcpy(mi_copia, (char *)str);

    printf("Mi copia vale: %s\n", mi_copia);
    int tamanioCopia = sizeof(mi_copia)/sizeof(mi_copia[0]);
    printf("Tamanio de mi copia es: %d\n", tamanioCopia);


    ///Aqui empece a modificar
    uint16_t offset = addr&0x1F;
    printf(" Offset es: %d\n", offset);
    uint16_t pag_inicio = (addr&0x0FE0)>>5;
    printf(" pag_inicio es: %d\n", pag_inicio);

    uint16_t pag_final = (uint16_t)floor(pag_inicio + (((int)(str_len) + offset)/PAGE_SIZE));
    printf(" pag_final es: %d\n", pag_final);

    uint16_t num_pags_a_escribir = (pag_final-pag_inicio) + 1;
    printf(" num_pags_a_escribir es: %d\n", num_pags_a_escribir);

    uint16_t direccion_de_memoria;
    uint16_t bytes_arr_temp = 0;
    uint16_t num_bytes_que_faltan = (uint16_t)(str_len);
    printf(" num_bytes_que_faltan es: %d\n", num_bytes_que_faltan);
    int cont = 0;
    printf(" Voy a entrar al for\n");
    
    for(int pag_actual=0; pag_actual<num_pags_a_escribir; pag_actual++){
        printf("   Estoy en pagina: %d\n", pag_inicio);
        if(num_pags_a_escribir==1){         //solo 1 pagina o primera pagina
            bytes_arr_temp = num_bytes_que_faltan; 
        }
        else{
            if(pag_actual == 0){
                bytes_arr_temp = 32-offset;
            }
            else if(pag_actual == (num_pags_a_escribir-1)){
                bytes_arr_temp = num_bytes_que_faltan;
            }
            else{
                bytes_arr_temp = 32;
            }
        }

        printf("   bytes_arr_temp: %d\n", bytes_arr_temp);
        direccion_de_memoria = (pag_inicio<<5)|offset;
        printf("   direccion_de_memoria: %d\n", direccion_de_memoria);

        uint8_t datos_a_escribir[2+bytes_arr_temp];    
        datos_a_escribir[0] = (uint8_t)(direccion_de_memoria>>8);
        datos_a_escribir[1] = (uint8_t)(direccion_de_memoria&0xFF);

        for(int i=0; i<bytes_arr_temp; i++){
            datos_a_escribir[i+2] = mi_copia[cont];
            cont++;
        }

        for(int y=0; y<(2+bytes_arr_temp); y++){
            printf("Byte %d vale: %d\n", y, datos_a_escribir[y]);
        }

        i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, datos_a_escribir, (2+bytes_arr_temp), true);

        num_bytes_que_faltan = num_bytes_que_faltan - bytes_arr_temp;
        printf("   Ahora faltan: %d bytes.\n", num_bytes_que_faltan);

        pag_inicio++;
        offset = 0;
        mp_hal_delay_us(6000);


        //Agregar un delay para permitir que los datos se escriban.
    }


    /*uint8_t datos_a_escribir[2+str_len];    
    datos_a_escribir[0] = (uint8_t)(addr>>8);
    datos_a_escribir[1] = (uint8_t)(addr&0xFF);

    for(int i=0; i<str_len; i++){
        datos_a_escribir[i+2] = mi_copia[i];
    }

    for(int y=0; y<(2+str_len); y++){
        printf("Byte %d vale: %d\n", y, datos_a_escribir[y]);
    }

    i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, datos_a_escribir, (2+str_len), true);
    */

    printf("Se supone que ya acabe de escribir.\n");
    return mp_obj_new_int(0);
}

STATIC mp_obj_t eeprom_read(mp_obj_t self_in, mp_obj_t eeaddr, mp_obj_t bytes_a_leer) {
    printf("**************************\n");
    printf("Se supone que voy a leer\n");

    uint16_t addr = (uint16_t)mp_obj_get_int(eeaddr);
    printf("Leere de la direccion: %d\n", addr);

    int bytes_que_leere = mp_obj_get_int(bytes_a_leer);
    printf("Leere %d bytes\n", bytes_que_leere);

    uint8_t datos_leidos[bytes_que_leere];
    int pos = 0;

    ///Aqui empece a modificar
    uint16_t offset = addr&0x1F;
    printf(" Offset es: %d\n", offset);
    uint16_t pag_inicio = (addr&0x0FE0)>>5;
    printf(" pag_inicio es: %d\n", pag_inicio);

    uint16_t pag_final = (uint16_t)floor(pag_inicio + ((bytes_que_leere + offset)/PAGE_SIZE));
    printf(" pag_final es: %d\n", pag_final);

    uint16_t num_pags_a_leer = (pag_final-pag_inicio) + 1;
    printf(" num_pags_a_leer es: %d\n", num_pags_a_leer);

    uint16_t direccion_de_memoria;
    uint16_t bytes_arr_temp = 0;
    uint16_t num_bytes_que_faltan = (uint16_t)(bytes_que_leere);
    printf(" num_bytes_que_faltan es: %d\n", num_bytes_que_faltan);
    printf(" Voy a entrar al for\n");
    
    for(int pag_actual=0; pag_actual<num_pags_a_leer; pag_actual++){
        printf("   Estoy en pagina: %d\n", pag_inicio);
        if(num_pags_a_leer==1){         //solo 1 pagina o primera pagina
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

        printf("   bytes_arr_temp: %d\n", bytes_arr_temp);
        direccion_de_memoria = (pag_inicio<<5)|offset;
        printf("   direccion_de_memoria: %d\n", direccion_de_memoria);

        uint8_t direccion_a_leer[2];
        direccion_a_leer[0] = (uint8_t)(direccion_de_memoria>>8);
        direccion_a_leer[1] = (uint8_t)(direccion_de_memoria&0xFF);

        printf("direccion_a_leer[0]: %d\n", (int)direccion_a_leer[0]);
        printf("direccion_a_leer[1]: %d\n", (int)direccion_a_leer[1]);

        uint8_t arreglo_temporal[bytes_arr_temp];

        //SI bytes_arr_temp ES DIFERENTE DE CERO!!
        if(bytes_arr_temp != 0){
            i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, direccion_a_leer, 2, false);
            i2c_readfrom(I2C1,M24C32_OPHYRA_ADDRESS, arreglo_temporal, bytes_arr_temp, true);
        }

        printf("Se supone que ya lei algo: \n");
        for(int i=0; i<bytes_arr_temp; i++){
            printf("Byte %d vale: %d\n", i, arreglo_temporal[i]);
        }

        for(int y=0; y<bytes_arr_temp; y++){
            datos_leidos[pos] = arreglo_temporal[y];
            pos++;
        }

        num_bytes_que_faltan = num_bytes_que_faltan - bytes_arr_temp;
        printf("   Ahora faltan: %d bytes.\n", num_bytes_que_faltan);

        pag_inicio++;
        offset = 0;       
    }

    /*
    uint8_t direccion_a_leer[2];
    direccion_a_leer[0] = (uint8_t)(addr>>8);
    direccion_a_leer[1] = (uint8_t)(addr&0xFF);

    printf("direccion_a_leer[0]: %d\n", (int)direccion_a_leer[0]);
    printf("direccion_a_leer[1]: %d\n", (int)direccion_a_leer[1]);

    i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, direccion_a_leer, 2, false);
    i2c_readfrom(I2C1,M24C32_OPHYRA_ADDRESS, datos_leidos, bytes_que_leere, true);

    printf("Se supone que ya lei algo: \n");
    for(int i=0; i<bytes_que_leere; i++){
        printf("Byte %d vale: %d\n", i, datos_leidos[i]);
    }
    */


    printf("Se supone que ya acabe de leer: \n");
    return mp_obj_new_bytearray((size_t)bytes_que_leere, datos_leidos); 
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

