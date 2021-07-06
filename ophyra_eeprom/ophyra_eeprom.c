/*
    ophyra_eeprom.c

    C usermod to use the EEPROM M24C32 memory on the Ophyra board, manufactured by
    Intesc Electronica y Embebidos, located in Puebla, Pue. Mexico.

    This C usermod contains the necessary functions to write and read certain amount of bytes to/from the
    EEPROM memory.

    Written by: Carlos D. Hernández y Jonatan Salinas.
    Last modification: 10/04/2021.

*/
#include "py/runtime.h"
#include "py/obj.h"
#include "py/mphal.h"       
#include "i2c.h"
#include "py/objstr.h"
#include <string.h>
#include <math.h>

#define M24C32_OPHYRA_ADDRESS         (80)          //ID or the slave direction to be identified in the IC2 port
#define I2C_TIMEOUT_MS                (50)          //Timeout for I2C
#define PAGE_SIZE                     (32)          //Page size of the M24C32 (32 bytes)

typedef struct _eeprom_class_obj_t{
    mp_obj_base_t base;
} eeprom_class_obj_t;

const mp_obj_type_t eeprom_class_type;

/*
    Print function. It is invoked when the Micropython user writes something like this:
        miEeprom = MC24C32()
        print(miEeprom)
*/
STATIC void eeprom_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mp_print_str(print, "eeprom_class()");
}

/*
    make_new: Class constructor. This function is invoked when the Micropython user writes:
        miEeprom = MC24C32()
*/
STATIC mp_obj_t eeprom_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    eeprom_class_obj_t *self = m_new_obj(eeprom_class_obj_t);
    self->base.type = &eeprom_class_type;

    //The I2C port 1 is initialized 
    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    return MP_OBJ_FROM_PTR(self);
}

/*
    Write function to the EEPROM. It is invoked when the MicroPython user writes something like this:
        miEeprom.write(0x6EA3, arregloBytes)
    In MicroPython, two parameters must be specified:
        1.- The memory address from where the data will begin to be written.
            b11-b5 indicate the page in which the data will begin to be written.
            b4-b0 indicate the offset of the page from where the data will begin to be written.
        
        2.- The array of bytes (bytearray) that is going to be written in the memory.
*/
STATIC mp_obj_t eeprom_write(mp_obj_t self_in, mp_obj_t eeaddr, mp_obj_t data_bytes_obj) {

    uint16_t addr = (uint16_t)mp_obj_get_int(eeaddr);

    mp_check_self(mp_obj_is_str_or_bytes(data_bytes_obj));
    GET_STR_DATA_LEN(data_bytes_obj, str, str_len);         //This macro takes the passed bytearray from the parameters and
                                                            //generates the "str" pointer, which is a pointer to character;
                                                            //and str_len, of type size_t which is the array length.
    
    char mi_copia[str_len];                                 //A copy of the string/array is made.
    strcpy(mi_copia, (char *)str);

    uint16_t offset = addr&0x1F;                            //The offset of the page is calculated, from where the data will begin to be written
    uint16_t pag_inicio = (addr&0x0FE0)>>5;                 //From which page the data starts being written on?
    uint16_t pag_final = (uint16_t)floor(pag_inicio + (((int)(str_len) + offset)/PAGE_SIZE));   //In which page we stop?
    uint16_t num_pags_a_escribir = (pag_final-pag_inicio) + 1;          //In how many pages we are going to write data?

    uint16_t direccion_de_memoria;
    uint16_t bytes_arr_temp = 0;                            //Variable that stores the length of the array that is going to be sended (depending the page)
    uint16_t num_bytes_que_faltan = (uint16_t)(str_len);

    int cont = 0;
    
    for(int pag_actual=0; pag_actual<num_pags_a_escribir; pag_actual++){
        if(num_pags_a_escribir==1){                             //If we only write in one page
            bytes_arr_temp = num_bytes_que_faltan; 
        }
        else{                                                   //If I write in various pages
            if(pag_actual == 0){                                //If I am in the first one of them
                bytes_arr_temp = 32-offset;
            }
            else if(pag_actual == (num_pags_a_escribir-1)){     //If I am in the last page of them
                bytes_arr_temp = num_bytes_que_faltan;
            }
            else{
                bytes_arr_temp = 32;
            }
        }

        direccion_de_memoria = (pag_inicio<<5)|offset;          //We calculate the 16 bits of the memory address, from where the data will start to be written

        uint8_t datos_a_escribir[2+bytes_arr_temp];    
        datos_a_escribir[0] = (uint8_t)(direccion_de_memoria>>8);           //MSB of the memory address
        datos_a_escribir[1] = (uint8_t)(direccion_de_memoria&0xFF);         //LSB of the memory address

        for(int i=0; i<bytes_arr_temp; i++){                    //The data to be written in this iteration is put inside the "datos_a_escribir" array
            datos_a_escribir[i+2] = mi_copia[cont];
            cont++;
        }

        //The data is sended and written using I2C
        i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, datos_a_escribir, (2+bytes_arr_temp), true);

        num_bytes_que_faltan = num_bytes_que_faltan - bytes_arr_temp;       //Now, how many bytes are left to write?
        pag_inicio++;                                                       //Go to the next page
        offset = 0;                                                         //As we are now in a new page, the offset is 0.
        mp_hal_delay_us(6000);                                              //delay to allow the memory to write the data

    }

    return mp_obj_new_int(0);
}

/*
    Read function. It is invoked when the MicroPython user writes something like this:
        PalR = miEeprom.read(0x6EA3, len(arregloBytes))
    In MicroPython, two parameters must be specified:
        1.- The memory address from where the data will begin to be read.
            b11-b5 indicate the page in which the data will begin to be read.
            b4-b0 indicate the offset of the page from where the data will begin to be read.
        
        2.- The amount of bytes to be read from the EEPROM.

    The function returns the read bytes, in the form of an array of bytes (bytearray).     
*/
STATIC mp_obj_t eeprom_read(mp_obj_t self_in, mp_obj_t eeaddr, mp_obj_t bytes_a_leer) {

    uint16_t addr = (uint16_t)mp_obj_get_int(eeaddr);
    int bytes_que_leere = mp_obj_get_int(bytes_a_leer);

    uint8_t datos_leidos[bytes_que_leere];                      //Array in which the read bytes will be put.
    int pos = 0;

    uint16_t offset = addr&0x1F;                                //This function is very similar to the one that writes data to the EEPROM.
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
        direccion_a_leer[0] = (uint8_t)(direccion_de_memoria>>8);       //MSB of the memory address to be read.
        direccion_a_leer[1] = (uint8_t)(direccion_de_memoria&0xFF);     //LSB of the memory address to be read.

        uint8_t arreglo_temporal[bytes_arr_temp];

        //Only if bytes_arr_temp is different from 0, then you read.
        if(bytes_arr_temp != 0){
            i2c_writeto(I2C1, M24C32_OPHYRA_ADDRESS, direccion_a_leer, 2, false);
            i2c_readfrom(I2C1,M24C32_OPHYRA_ADDRESS, arreglo_temporal, bytes_arr_temp, true);
        }

        for(int y=0; y<bytes_arr_temp; y++){                //The read bytes are put in the array that is going
            datos_leidos[pos] = arreglo_temporal[y];        //to be returned to the user at the end of the function.
            pos++;
        }

        num_bytes_que_faltan = num_bytes_que_faltan - bytes_arr_temp;       //How many bytes are left to read?
        pag_inicio++;
        offset = 0;       
    }

    return mp_obj_new_bytearray((size_t)bytes_que_leere, datos_leidos);     //We return the bytearray of the read data.
};


//We associate the functions above with their corresponding Micropython function object.
MP_DEFINE_CONST_FUN_OBJ_3(eeprom_write_obj, eeprom_write);
MP_DEFINE_CONST_FUN_OBJ_3(eeprom_read_obj, eeprom_read);

/*
    Here, we associate the "function object" of Micropython with a specific string. This string is the one
    that will be used in the Micropython programming.   
*/
STATIC const mp_rom_map_elem_t eeprom_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&eeprom_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&eeprom_write_obj) },
    //Name of the Micropython function     //Associated function object
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
                                                    //Name of this C usermod file
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_eeprom) },
            //Name of the class        //Name of the associated "type"
    { MP_ROM_QSTR(MP_QSTR_M24C32), MP_ROM_PTR(&eeprom_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_eeprom_globals, ophyra_eeprom_globals_table);

// Define module object.
const mp_obj_module_t ophyra_eeprom_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_eeprom_globals,
};

// We need to register the module to make it available for Python.
//                          nameOfTheFile, nameOfTheFile_user_cmodule, MODULE_IDENTIFIER_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_eeprom, ophyra_eeprom_user_cmodule, MODULE_OPHYRA_EEPROM_ENABLED);

