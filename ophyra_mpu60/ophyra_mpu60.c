/*
    ophyra_mpu60.c

    Modulo de usuario en C para la utilizacion del sensor MPU6050 de la tarjeta Ophyra, fabricada por
    Intesc Electronica y Embebidos.

    Este archivo incluye las funciones que leen de los registros del sensor los valores de aceleracion en el
    eje X, Y y Z; y los valores del giroscopio en X, Y y Z; y el valor de la temperatura en grados Celcius.
    Se utilizan las funciones i2c_writeto e i2c_read from para la comunicacion con el sensor mediante I2C.

    Para construir el firmware incluyendo este modulo, ejecutar en Cygwin (desde la carpeta stm32 de micropython):
        make BOARD=OPHYRA USER_C_MODULES=../../../modules CFLAGS_EXTRA=-DMODULE_OPHYRA_MPU60_ENABLED=1 all

        En USER_C_MODULES se especifica el path hacia la localizacion de la carpeta modules, donde se encuentra
        el archivo ophyra_mpu60.c y micropython.mk

    Escrito por: Jonatan Salinas y Carlos Daniel.
    Ultima fecha de modificacion: 21/03/2021.

*/

#include "py/runtime.h"
#include "py/obj.h"
#include "ports/stm32/mphalport.h"        
#include "i2c.h"

#define MPU6050_OPHYRA_ADDRESS      (104)
#define I2C_TIMEOUT_MS              (50)
        //Definicion de los registros necesarios del sensor:
#define MPU60_WHO_AM_I_REG          (117)
#define MPU60_SMPLRT_DIV_REG        (25)
#define POWER_MANAG_REG             (107)
#define GYR_CONFIG_REG              (27)
#define ACCEL_CONFIG_REG            (28)
#define ACCEL_REG_X                 (59)
#define ACCEL_REG_Y                 (61)
#define ACCEL_REG_Z                 (63)
#define TEMP_REG                    (65)
#define GYR_REG_X                   (67)
#define GYR_REG_Y                   (69)
#define GYR_REG_Z                   (71)

typedef struct _mpu60_class_obj_t{
    mp_obj_base_t base;
    float g;
    float sen;     
} mpu60_class_obj_t;

const mp_obj_type_t mpu60_class_type;

STATIC mpu60_class_obj_t mi_mpu60_obj;
/*
    Esta funcion imprime la informacion que la estructura mpu60_class_obj_t contiene en cierto momento.
    Se invoca al escribir en micropython "print(obj)", ejemplo:
        SAG = MPU6050()
        print(SAG)
*/
STATIC void mpu60_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind){
    (void)kind;
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_print_str(print, "OPHYRA_MPU6050_SENSOR_OBJ\n");
    mp_print_str(print, "(g: ");
    mp_obj_print_helper(print, mp_obj_new_float(self->g), PRINT_REPR);  
    mp_print_str(print, " sen: ");
    mp_obj_print_helper(print, mp_obj_new_float(self->sen), PRINT_REPR);  
    mp_print_str(print, ")");         

}

/*
    Funcion que inicia el puerto 1 para la comunicacion I2C con el sensor.
    Se lee el valor del registro WHO_AM_I para verificar la presencia del sensor. Si no se verifica
    que esta disponible, sucede un error.
*/
STATIC void mpu60_start(void){

    i2c_init(I2C1, MICROPY_HW_I2C1_SCL, MICROPY_HW_I2C1_SDA, 400000, I2C_TIMEOUT_MS);

    uint8_t data[2] = { MPU60_WHO_AM_I_REG };
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data, 1, false);
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, data, 1, true);
    if (data[0] != 0x68) {
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("MPU6050 not found.\n"));
    }

}

/*
    Funcion que se invoca al crear un nuevo objeto en el codigo de Micropython de tipo MPU6050(), ejemplo:
        SAG = MPU6050()
*/
STATIC mp_obj_t mpu60_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    //Verificacion del numero de argumentos.
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    mi_mpu60_obj.base.type = &mpu60_class_type;
   
    mpu60_start();

    return MP_OBJ_FROM_PTR(&mi_mpu60_obj);
}
/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        SAG.init(8,1000)
    Sirve para ajustar el rango de funcionamiento del acelerometro y del giroscopio.
*/
STATIC mp_obj_t init_function(mp_obj_t self_in, mp_obj_t range_accel_config_obj, mp_obj_t range_gyr_config_obj) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int miRangoAccel = mp_obj_get_int(range_accel_config_obj);
    int miRangoGyr = mp_obj_get_int(range_gyr_config_obj);

    int env_accel_config;
    int env_gyr_config;

    if(miRangoAccel == 2){
        env_accel_config = 0;
        self->g = 16384;
    }
    else if(miRangoAccel == 4){
        env_accel_config = 8;
        self->g = 8192;        
    }
    else if(miRangoAccel == 8){
        env_accel_config = 16;
        self->g = 4096;        
    }    
    else if(miRangoAccel == 16){
        env_accel_config = 24;
        self->g = 2048;        
    }
    else{
        mp_raise_ValueError(MP_ERROR_TEXT("Ingresaste un valor de rango equivocado para el acelerometro."));
    }    

    if(miRangoGyr == 250){
        env_gyr_config = 0;
        self->sen = 131;          
    }
    else if(miRangoGyr == 500){
        env_gyr_config = 8;
        self->sen = 65.5;          
    }
    else if(miRangoGyr == 1000){
        env_gyr_config = 16;
        self->sen = 32.8;          
    }
    else if(miRangoGyr == 2000){
        env_gyr_config = 24;
        self->sen = 16.4;         
    }
    else{
        mp_raise_ValueError(MP_ERROR_TEXT("Ingresaste un valor de rango equivocado para el giroscopio."));
    }

    //Despierto el sensor
    uint8_t data0[2] = {POWER_MANAG_REG, 0};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data0, 2, true);
    //Configuracion de la tasa de salida de datos (Data output rate or Sample Rate)
    uint8_t data1[2] = {MPU60_SMPLRT_DIV_REG, (uint8_t)(7)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data1, 2, true);
    //Configuracion del rango del acelerometro
    uint8_t data2[2] = {ACCEL_CONFIG_REG, (uint8_t)(env_accel_config)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data2, 2, true);
    //Configuracion del rango del giroscopio
    uint8_t data3[2] = {GYR_CONFIG_REG, (uint8_t)(env_gyr_config)};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data3, 2, true);

    return mp_obj_new_float(1);
}
/*
    Funcion que lee los dos registros correspondientes a cierto eje del acelerometro o del giroscopio.
    Se retorna el valor de aceleracion (en gravedades o G) o el valor del giroscopio (en °/seg) en ese eje.
*/
STATIC mp_obj_t read_axis(int axis, float g_o_sin){
    uint8_t myAxis[1] = {(uint8_t)axis};
    uint8_t lectura_bytes[2];
                                       
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, myAxis, 1, false);       
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, lectura_bytes, 2, true);

    int16_t miValorRes = (int16_t)(lectura_bytes[0] << 8 | lectura_bytes[1]);

    if(miValorRes > 32767){
        miValorRes = (65536 - miValorRes)*-1;
    }

    float resultado = (float)(miValorRes/(float)g_o_sin);

    return mp_obj_new_float(resultado);
}

/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        x=SAG.accX()
    Retorna el valor de aceleracion del sensor en el eje X.
*/                                
STATIC mp_obj_t get_accelerationX(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_X, self->g);
}

/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        y=SAG.accY()
    Retorna el valor de aceleracion del sensor en el eje Y.
*/   
STATIC mp_obj_t get_accelerationY(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_Y, self->g);
}

/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        z=SAG.accZ()
    Retorna el valor de aceleracion del sensor en el eje Z.
*/ 
STATIC mp_obj_t get_accelerationZ(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(ACCEL_REG_Z, self->g);
}

/*
    Funcion que lee los registros de temperatura del sensor. Se calcula el valor de temperatura en grados
    Celcius y se retorna ese valor. Se invoca al escribir en Micropython una linea como esta:
        tmp=SAG.temp()
*/ 
STATIC mp_obj_t get_temperature(mp_obj_t self_in) {
    uint8_t registro_temp[1] = {(uint8_t)TEMP_REG};
    uint8_t lectura_temperatura[2];

    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, registro_temp, 1, false);    
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, lectura_temperatura, 2, true);

    int16_t miTempLeida = (int16_t)(lectura_temperatura[0] << 8 | lectura_temperatura[1]);

    float miTemperatura_calculada = (float)(miTempLeida/(float)340 + (float)36.53);

    return mp_obj_new_float(miTemperatura_calculada);
}

/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        gx=SAG.gyrX()
    Retorna el valor del giroscopio del sensor en el eje X.
*/
STATIC mp_obj_t get_gyroscopeX(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_X, self->sen);
}

/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        gy=SAG.gyrY()
    Retorna el valor del giroscopio del sensor en el eje Y.
*/
STATIC mp_obj_t get_gyroscopeY(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_Y, self->sen);
}

/*
    Funcion que se invoca al escribir en el codigo de Micropython una linea como esta:
        gz=SAG.gyrZ()
    Retorna el valor del giroscopio del sensor en el eje Z.
*/
STATIC mp_obj_t get_gyroscopeZ(mp_obj_t self_in) {
    mpu60_class_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return read_axis(GYR_REG_Z, self->sen);
}

/*
    Funcion que permite escribir un byte de informacion a un registro especifico del sensor.
*/
STATIC mp_obj_t write_function(mp_obj_t self_in, mp_obj_t num_obj, mp_obj_t address_obj) {
    int numero_a_escribir = mp_obj_get_int(num_obj);
    int direccion_a_escribir = mp_obj_get_int(address_obj);

    uint8_t data[2] = {(uint8_t)direccion_a_escribir, (uint8_t)numero_a_escribir};
    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, data, 2, true);

    return mp_obj_new_int(0);
}

/*
    Funcion que permite leer un byte de informacion de un registro especifico del sensor.
    Retorna el valor leido del registro.
*/
STATIC mp_obj_t read_function(mp_obj_t self_in, mp_obj_t address_obj) {
    int direccion_a_leer = mp_obj_get_int(address_obj);

    uint8_t registro_a_leer[1] = {(uint8_t)direccion_a_leer};

    i2c_writeto(I2C1, MPU6050_OPHYRA_ADDRESS, registro_a_leer, 1, false);    
    i2c_readfrom(I2C1, MPU6050_OPHYRA_ADDRESS, registro_a_leer, 1, true);    

    return mp_obj_new_int(registro_a_leer[0]);
};

MP_DEFINE_CONST_FUN_OBJ_3(init_function_obj, init_function);
MP_DEFINE_CONST_FUN_OBJ_1(get_accelerationX_obj, get_accelerationX);
MP_DEFINE_CONST_FUN_OBJ_1(get_accelerationY_obj, get_accelerationY);
MP_DEFINE_CONST_FUN_OBJ_1(get_accelerationZ_obj, get_accelerationZ);
MP_DEFINE_CONST_FUN_OBJ_1(get_temperature_obj, get_temperature);
MP_DEFINE_CONST_FUN_OBJ_1(get_gyroscopeX_obj, get_gyroscopeX);
MP_DEFINE_CONST_FUN_OBJ_1(get_gyroscopeY_obj, get_gyroscopeY);
MP_DEFINE_CONST_FUN_OBJ_1(get_gyroscopeZ_obj, get_gyroscopeZ);
MP_DEFINE_CONST_FUN_OBJ_3(write_function_obj, write_function);
MP_DEFINE_CONST_FUN_OBJ_2(read_function_obj, read_function);

STATIC const mp_rom_map_elem_t mpu60_class_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&init_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_accX), MP_ROM_PTR(&get_accelerationX_obj) },
    { MP_ROM_QSTR(MP_QSTR_accY), MP_ROM_PTR(&get_accelerationY_obj) },
    { MP_ROM_QSTR(MP_QSTR_accZ), MP_ROM_PTR(&get_accelerationZ_obj) },
    { MP_ROM_QSTR(MP_QSTR_temp), MP_ROM_PTR(&get_temperature_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyrX), MP_ROM_PTR(&get_gyroscopeX_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyrY), MP_ROM_PTR(&get_gyroscopeY_obj) },
    { MP_ROM_QSTR(MP_QSTR_gyrZ), MP_ROM_PTR(&get_gyroscopeZ_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&write_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_function_obj) },
};
                                
STATIC MP_DEFINE_CONST_DICT(mpu60_class_locals_dict, mpu60_class_locals_dict_table);

const mp_obj_type_t mpu60_class_type = {
    { &mp_type_type },
    .name = MP_QSTR_ophyra_mpu60,
    .print = mpu60_class_print,
    .make_new = mpu60_class_make_new,
    .locals_dict = (mp_obj_dict_t*)&mpu60_class_locals_dict,
};

STATIC const mp_rom_map_elem_t ophyra_mpu60_globals_table[] = {
                                                    //Nombre del archivo (User C module)
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ophyra_mpu60) },
            //Nombre de la clase        //Nombre del "tipo" asociado.
    { MP_ROM_QSTR(MP_QSTR_MPU6050), MP_ROM_PTR(&mpu60_class_type) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_ophyra_mpu60_globals, ophyra_mpu60_globals_table);

// Define module object.
const mp_obj_module_t ophyra_mpu60_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_ophyra_mpu60_globals,
};

// Registro del modulo para hacerlo disponible para Python.
//                      nombreArchivo, nombreArchivo_user_cmodule, MODULE_IDENTIFICADOR_ENABLED
MP_REGISTER_MODULE(MP_QSTR_ophyra_mpu60, ophyra_mpu60_user_cmodule, MODULE_OPHYRA_MPU60_ENABLED);

