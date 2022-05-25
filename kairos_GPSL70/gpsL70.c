/**
 * @file gpsL70.c
 * @author Ana María Ruiz Fernández
 * @brief Módulo en C para el sistema GPS-L70 QUECTEL. Este módulo está diseñado
 * para su uso en MicroPython con la tarjeta KAIROS, diseñada por INTESC
 * Electrónica y Embebidos. Importar en Python como:
 *      from gpsL70 import GPSL70
 * Este módulo está basado en la librería de python gpsL70_lib desarrollada por
 * el Dr. Fernando Quiñones de INTESC Electrónica y Embebidos.
 * @version 0.1
 * @date 2022-04-01
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// importar librerías generales
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/objtuple.h"
#include "py/mphal.h"
#include "ports/stm32/spi.h"
#include "ports/stm32/uart.h"

// funciones gpsl70
STATIC mp_obj_t gpsl70_armarcadena(mp_obj_t self_in, const mp_obj_t mns_obj);
STATIC mp_obj_t gpsl70_sendconfig(mp_obj_t self_in, const mp_obj_t mns_obj);
STATIC mp_obj_t gpsl70_checksum(mp_obj_t self_in, const mp_obj_t mns_obj);
STATIC mp_obj_t gpsl70_on(mp_obj_t self_in);
STATIC mp_obj_t gpsl70_off(mp_obj_t self_in);
STATIC mp_obj_t gpsl70_read(mp_obj_t self_in);
STATIC mp_obj_t gpsl70_readraw(mp_obj_t self_in);
STATIC mp_obj_t gpsl70_writeraw(mp_obj_t self_in, mp_obj_t data_obj);

/**
 * @brief Convierte de decimal a hexadecimal
 * 
 * @param cs buffer donde se almacena el hexadecimal 
 * @param n numero decimal a convertir
 */
void toHex(char *cs, int n)
{
    char hexCode[] = "0123456789ABCDEF";
    if (n < 16)
    {
        cs[1] = '0';
        cs[2] = hexCode[(size_t)n];
    }
    else
    {
        int num = n / 16, rem = n % 16;
        cs[1] = hexCode[(size_t)num];
        cs[2] = hexCode[(size_t)rem];
    }
}

/**
 * @brief  Convierte un hexadecimal en decimal
 * 
 * @param hex cadena con el hexadecimal
 * @return int equivalente decimal
 */
int toDec(const char *hex)
{
    char hex1, hex2;
    if (strlen(hex) == 3)
    {
        hex1 = hex[1];
        hex2 = hex[2];
    } else if (strlen(hex) == 2)
    {
        hex1 = '0';
        hex2 = hex[1];
    } else
    {
        return 0;
    }

    // obtener valor decimal del caracter
    int msd = (int)hex1, lsd = (int)hex2;
    // convertir el caracter hexadecimal a decimal
    // cifra más significativa
    if (msd >= 48 && msd < 58)
        msd -= 48;
    else if (msd >= 65 && msd < 71)
        msd -= 55;
    else
        return 0;

    // cifra menos significativa
    if (lsd >= 48 && lsd < 58)
        lsd -= 48;
    else if (lsd >= 65 && lsd < 71)
        lsd -= 55;
    else
        return 0;

    // obtener el equivalente decimal
    return msd*16 + lsd;
}

/**
 * @brief extrae los datos y el checksum de la cadena de datos
 * 
 * @param mens_obj el mensaje leído
 * @return STATIC 
 */
STATIC mp_obj_t extract_data(const mp_obj_t mens_obj)
{
    mp_check_self(mp_obj_is_str_or_bytes(mens_obj));
    GET_STR_DATA_LEN(mens_obj, str, str_len);
    // obtener la cadena
    char mens[str_len];
    strcpy(mens, (char *)str);
    int chks1 = 0;

    // buffer de datos
    size_t index = 0, data_index = 0, mens_len = strlen(mens);
    char data[mens_len-4];

    // iniciar el ciclo para extraer la información
    for (size_t i = 0; i < mens_len; i++)
    {
        if (mens[i] != '$' && mens[i] != '*')
        {
            data[data_index] = mens[i];
            data_index++;
        }
        else if (mens[i] == '*')
        {
            char temp[4];

            // get the checksum from the message
            while (true)
            {
                temp[index] = mens[i];
                i++;
                index++;

                if ((int)mens[i] < 48 || (int)mens[i] > 70)
                {
                    break;
                }
            }

            temp[index] = 0;

            // cerrar la cadena data
            data[data_index] = '\0';
            
            // convertir a decimal
            chks1 = toDec(temp);
            break;
        } else if (i > 3)
        {
            // si la cadena se conforma de más de un caracter
            if (data_index > 1)
            {
                data[data_index] = '\0';
            } else
            {
                // mandar cadena de error a data
                data[data_index] = '\0';
                strcpy(data,"PMTK001,0,0");
            }
            
            // mandar chks1 a 0
            chks1 = 0;
            break;
        }

    }
    

    // crear el tuple
    // el primer elemento será la cadena de datos, el segundo el checksum
    mp_obj_t tuple[2];
    // obtener la cadena de datos
    tuple[0] = mp_obj_new_str(data,strlen(data));
    // obtener el checksum
    tuple[1] = mp_obj_new_int(chks1);

    return mp_obj_new_tuple(2,tuple);
    // return mp_obj_new_str(data,strlen(data));
}

/**
 * @brief Busca el coma (,) en la cadena y obtiene la cadena de caracteres entre index
 * y la siguiente coma en el mensaje
 *
 * @param mens Cadena de caracteres que contiene el mensaje
 * @param index Indice a partir del cual se busca el siguiente coma (,)
 * @param buffer Cadena de caracteres donde se almacenan los datos
 * @return size_t Indice de la próxima ocurrencia de un coma (,)
 */
size_t split(char* mens, size_t index, char* buffer)
{
    size_t buff_index = 0, next_index = 0;

    for (size_t i = index; i < strlen(mens); i++)
    {
        if (mens[i] == ',')
        {
            // si se encuentra el coma, cerrar el buffer
            buffer[buff_index] = '\0';
            // asignar a next_index el valor del indice siguiente a la coma
            next_index = i+1;
            break;
        } else
        {
            // guardar el caracter en el buffer
            buffer[buff_index] = mens[i];
            buff_index++;
        }
    }

    return next_index;
}

// definición de la estructura de la clase
typedef struct _gpsl70_class_obj
{
    mp_obj_base_t base;
} gpsl70_class_obj_t;

// definición del tipo de clase
const mp_obj_type_t gpsl70_class_type;

// creación del objeto uart
pyb_uart_obj_t uart;


/**
 * @brief Imprime información sobre la clase
 *
 * @return STATIC
 */
STATIC void gpsl70_class_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    mp_print_str(print, "gpsl70()");
}

/**
 * @brief Creador para la clase GPS_HIDE
 *
 * @param type
 * @param n_args Debe ser 0
 * @param n_kw
 * @param args
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_class_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    printf("\n############## CREATING CLASS ##############\n");
    // revisar que no se tengan argumentos
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // crear el objeto de clase
    gpsl70_class_obj_t *self = m_new_obj(gpsl70_class_obj_t);
    self->base.type = &gpsl70_class_type;

    // asignar tipo al objeto uart
    uart.base.type = &pyb_uart_type;
    // definir el ID al puerto 2
    uart.uart_id = PYB_UART_2;
    // hacer referencia al objeto existente
    MP_STATE_PORT(pyb_uart_obj_all)[uart.uart_id - 1] = &uart;

    // almacenar attach_to_repl ya que uart_init lo desactiva
    bool attach_to_repl = uart.attached_to_repl;

    // inicializar el uart
    uint32_t baudr[1] = { 9600 };
    if(!uart_init(&uart,baudr[0],UART_WORDLENGTH_8B,UART_PARITY_NONE,UART_STOPBITS_1,UART_HWCONTROL_NONE))
    {
        // error si no se inicializa. este error surge si el puerto 2 no existe
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("UART(%d) doesn't exist"), uart.uart_id);
    }
    printf("baudrate: %ld\n",uart_get_baudrate(&uart));

    // restaurar attach_to_repl para que el UART siga funcionando incluso si se duplica la terminal
    uart_attach_to_repl(&uart, attach_to_repl);

    // definir timeout
    uart.timeout = 1000;
    
    // definir timeout_char. asegurarse de que sea al menos un caracter
    // el valor mínimo es 2-3 ms, se utiliza 3 ms para evitar errores
    // uart.timeout_char = 3; 
    uart.timeout_char = 1;
    uint32_t min_timeout_char = 13000 / baudr[0] + 2;
    if (uart.timeout_char < min_timeout_char) {
        uart.timeout_char = min_timeout_char;
    }
    printf("%ld\n",min_timeout_char);
    // configurar el buffer de lectura
    m_del(byte, uart.read_buf, uart.read_buf_len << uart.char_width);

    // configurar y encender pin stbyGPS
    mp_hal_pin_output(pin_C1);
    mp_hal_pin_write(pin_C1, 1);

    // delay de 1000 ms
    HAL_Delay(1000);

    printf("\n############## CLASS CREATED ##############\n");
    return MP_OBJ_FROM_PTR(self);
}

/**
 * @brief Inicializar el GPS
 *
 * @param self_in
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_init(mp_obj_t self_in)
{
    printf("\n******GPSL70 INIT*********\n");
    // crear el tuple
    mp_obj_t tuple[2];

    // configurando el Packet Type: 314 PMTK_API_SET_NMEA_OUTPUT
    char mns[] = "PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,1,0"; //49 bytes
    mp_obj_t mns_obj = mp_obj_new_str(mns,strlen(mns));
    // enviar el mensaje de configuración
    tuple[0] = gpsl70_sendconfig(self_in, mns_obj);
    // printf("\nConfig 314: %s\n",a);

    // Confiurando la tasa de muestreo:
    // Packet Type: 300 PMTK_API_SET_FIX_CTL
    mns[0] = 0;
    strcpy(mns,"PMTK300,1000,0,0,0,0");
    mns_obj = mp_obj_new_str(mns,strlen(mns));
    //funcion que envia el mensaje de configuracion
    tuple[1] = gpsl70_sendconfig(self_in, mns_obj);
    // printf("\nConfig 300: %s\n",b);

    printf("\n******GPSL70 INIT DONE*********\n");
    return mp_obj_new_tuple(2,tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(gpsl70_init_obj, gpsl70_init);

/**
 * @brief Da el formato correcto a la cadena de configuración
 *
 * @param mns
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_armarcadena(mp_obj_t self_in, const mp_obj_t mns_obj)
{
    // obtener la longitud de la cadena
    mp_check_self(mp_obj_is_str_or_bytes(mns_obj));
    GET_STR_DATA_LEN(mns_obj, str, str_len);
    // obtener la cadena
    char mns[str_len];
    strcpy(mns, (char *)str);

    // crear la cadena completa
    char mnso[strlen(mns) + 8];
    mnso[0] = '$';

    // concatenar el mensaje
    size_t i;
    for (i = 0; i < strlen(mns); i++)
    {
        mnso[i+1] = mns[i];
    }

    mnso[i+1] = '\0';

    // obtener el checksum
    int chks = mp_obj_get_int(gpsl70_checksum(self_in, mns_obj));
    char checksum[6] = "*";
    // convertir a hexadecimal
    toHex(checksum, chks);
    strcat(checksum, "\r\n");

    // añadir el checksum a la cadena
    strcat(mnso, checksum);

    return mp_obj_new_str(mnso, strlen(mnso));
}
MP_DEFINE_CONST_FUN_OBJ_2(armarcadena_obj, gpsl70_armarcadena);

/**
 * @brief Envía el mensaje de configuración
 *
 * @param mns
 * @return STATIC*
 */
STATIC mp_obj_t gpsl70_sendconfig(mp_obj_t self_in, const mp_obj_t mns_obj)
{
    printf("\n************* SENDCONFIG ***********\n");
    // limpiar la terminal
    // char temp;
    // mp_obj_t lol;
    for (size_t i = 0; i < 4; i++)
    {
        printf("\n Loop %d \n", i);
        // read line
        gpsl70_readraw(self_in);
    }

    printf("\n---- armar cadena ----\n");

    // armar la cadena de configuración
    mp_obj_t mnso_obj = gpsl70_armarcadena(self_in, mns_obj);
    mp_check_self(mp_obj_is_str_or_bytes(mnso_obj));
    GET_STR_DATA_LEN(mnso_obj, str, str_len);
    // obtener la cadena
    char mnso[str_len];
    strcpy(mnso, (char *)str);
    printf("message: %s",mnso);

    size_t bytes_sent = 0, intentos = 3;
    int errcode = 0, num = 0;
    char resp = ' ';
    mp_obj_t data_obj = mp_obj_new_str("PMTK001,0,0",strlen("PMTK001,0,0"));
    mp_obj_t mens_obj;

    while ((resp != '3') && (num < intentos))
    {
        printf("num: %d\n", num);

        while (errcode != 0 || bytes_sent != strlen(mnso))
        {
            if (uart_tx_wait(&uart, uart.timeout))
            {
                bytes_sent = uart_tx_data(&uart, &mnso, strlen(mnso) >> uart.char_width, &errcode);
                printf("bytes_sent: %d, menslen: %d, err: %d\n", bytes_sent, strlen(mnso), errcode);
                if (errcode == 0)
                {
                    errcode = 1;
                    break;
                }

            }
        }

        HAL_Delay(100);

        size_t chks1 = 0, chks2 = 1;

        while (chks1 != chks2)
        {
            // leer una linea del puerto
            mens_obj = gpsl70_readraw(self_in);
            // si se entrega un None, asignar la cadena de error
            if (mens_obj == mp_const_none)
            {
                mens_obj = mp_obj_new_str("$PMTK001,0,0*FF",strlen("$PMTK001,0,0*FF"));
            }

            // extraer los datos del mensaje
            data_obj = extract_data(mens_obj);

            mp_obj_iter_buf_t iter_buf;
            mp_obj_t item, iterable = mp_getiter(data_obj, &iter_buf);
            while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) 
            {
                // si el objeto es un int, obtener el valor del checksum
                if (MP_OBJ_IS_SMALL_INT(item))
                    chks1 = mp_obj_get_int(item);
                // si el objeto es un string, obtener el objeto string
                else if (mp_obj_is_str(item))
                    data_obj = item;
            }
            
            // obtener el checksum de los datos
            chks2 = mp_obj_get_int(gpsl70_checksum(self_in,data_obj));
        }
        
        printf("----------------\n");

        // convertir data_obj a una cadena de char
        mp_check_self(mp_obj_is_str_or_bytes(data_obj));
        GET_STR_DATA_LEN(data_obj, str, str_len);
        char data[str_len];
        strcpy(data, (char *)str);
        printf("data: %s\n", data);

        // extraer "PMTK001" de la cadena
        char *PMTK = strstr(data, "PMTK001,");
        if (PMTK != NULL)
        {
            // printf("PMTK001: %s\n", PMTK);
            // extraer el elemento entre la primer y segunda coma
            resp = PMTK[12];
            printf("resp found: %c\n",resp);
        }

        num++;
    }

    printf("\n************* SENDCONFIG DONE ***********\n");

    // si resp es 3, enviar el mensaje leído
    if (resp == '3')
        return mens_obj;
    else
        return mp_obj_new_str("No config",strlen("No config"));

}

MP_DEFINE_CONST_FUN_OBJ_2(sendconfig_obj, gpsl70_sendconfig);

/**
 * @brief Calcula el checksum de acuerdo al mensaje
 *
 * @param mns
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_checksum(mp_obj_t self_in, const mp_obj_t mns_obj)
{
    // obtener el mensaje
    mp_check_self(mp_obj_is_str_or_bytes(mns_obj));
    GET_STR_DATA_LEN(mns_obj, str, str_len);
    char mns[str_len];
    strcpy(mns, (char *)str);
    
    // extraer el valor decimal del primer caracter
    int chks = (int)mns[0];
    for (size_t i = 1; i < strlen(mns); i++)
    {
        chks ^= (int)mns[i];
    }

    return mp_obj_new_int(chks);
}
MP_DEFINE_CONST_FUN_OBJ_2(checksum_obj, gpsl70_checksum);

/**
 * @brief Enciende el pin para activar el sensor GPS
 *
 * @param self_in
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_on(mp_obj_t self_in)
{
    mp_hal_pin_write(pin_C1, 1);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(on_obj, gpsl70_on);

/**
 * @brief Apaga el pin para desactivar el sensor GPS
 *
 * @param self_in
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_off(mp_obj_t self_in)
{
    mp_hal_pin_write(pin_C1, 0);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(off_obj, gpsl70_off);




/**
 * @brief Función de lectura para datos de interes: Latitud, longitud, altura,
 * velocidad, UTC
 *
 * @param self_in
 * @return STATIC
 */
STATIC mp_obj_t gpsl70_read(mp_obj_t self_in)
{
    printf("\n########## READ ##########\n");
    // inicializar el contador de mensajes
    int a = 0;
    // crear mensaje de NoValid
    char novalid[] = "NoValid";

    char GPRMC[13][11], GPGGA[15][11];
    size_t index = 0, GPRMC_size = 13, GPGGA_size = 15;
    mp_obj_t data_obj;

    // ciclar para la lectura de 2 mensajes
    while (a < 2)
    {
        int chks1 = 0, chks2 = 1;
        // ciclar mientras los checksums no coincidan
        while (chks1 != chks2)
        {
            index = 0;
            // leer la linea
            mp_obj_t mens_obj = gpsl70_readraw(self_in);
            // si se trata de un None, asignar el mensaje de error
            if (mens_obj == mp_const_none)
            {
                mens_obj = mp_obj_new_str("$GPRMC,0,0*FF", strlen("$GPRMC,0,0*FF"));
            }

            // extraer los datos del mensaje
            data_obj = extract_data(mens_obj);
            mp_obj_iter_buf_t iter_buf;
            mp_obj_t item, iterable = mp_getiter(data_obj, &iter_buf);
            while ((item = mp_iternext(iterable)) != MP_OBJ_STOP_ITERATION) 
            {
                // si el objeto es un int, obtener el valor del checksum
                if (MP_OBJ_IS_SMALL_INT(item))
                    chks1 = mp_obj_get_int(item);
                // si el objeto es un string, obtener el objeto string
                else if (mp_obj_is_str(item))
                    data_obj = item;
            }

            // obtener el checksum de los datos
            chks2 = mp_obj_get_int(gpsl70_checksum(self_in,data_obj));
        }

        // convertir data_obj a una cadena de char
        mp_check_self(mp_obj_is_str_or_bytes(data_obj));
        GET_STR_DATA_LEN(data_obj, str, str_len);
        char data[str_len];
        strcpy(data, (char *)str);

        // buscar un coma en la cadena
        if(strchr(data,',') != NULL)
        {
            // extraer el identificador del mensaje
            char temp[11];
            index = split(data,0,temp);
            size_t temp_index = index;
            // revisar si el mensaje es de tipo GPRMC y es el primer intento
            if (strcmp(temp,"GPRMC") == 0 && a < 1)
            {
                // incrementar el contador de mensajes
                a++;
                // almcenar el identificador
                strcpy(GPRMC[0],temp);
                // almacenar el resto de los datos
                for (size_t i = 1; i < 13; i++)
                {
                    index = split(data,index,GPRMC[i]);
                    // si se agregó una cadena vacía, reducir el tamaño de
                    // GPRMC_size
                    if (temp_index == index-1)
                    {
                        GPRMC_size--;
                    }
                    // almacenar el nuevo indice
                    temp_index = index;
                }
            // revisar si el mensaje es de tipo GPGGA
            } else if (strcmp(temp,"GPGGA") == 0)
            {
                // incrementar el contador de mensajes
                a++;

                // almcenar el identificador
                strcpy(GPGGA[0],temp);
                // almacenar el resto de los datos
                for (size_t i = 1; i < 15; i++)
                {
                    index = split(data,index,GPGGA[i]);
                    // si se agregó una cadena vacía, reducir el tamaño de
                    // GPGGA_size
                    if (temp_index == index-1)
                    {
                        GPGGA_size--;
                    }
                    // almacenar el nuevo indice
                    temp_index = index;
                }
            }
        }
    }
    // revisar la cantidad de datos obtenidos
    if (GPRMC_size > 8 && GPGGA_size > 13)
    {
        if (GPRMC[2][0] == 'A')
        {
            // declarar el objeto de micropython
            mp_obj_t data_obj[7];
            // agregar los datos
            data_obj[0] = mp_obj_new_str(GPRMC[3],strlen(GPRMC[3]));
            data_obj[1] = mp_obj_new_str(GPRMC[4],strlen(GPRMC[4]));
            data_obj[2] = mp_obj_new_str(GPRMC[5],strlen(GPRMC[5]));
            data_obj[3] = mp_obj_new_str(GPRMC[6],strlen(GPRMC[6]));
            data_obj[4] = mp_obj_new_str(GPGGA[9],strlen(GPGGA[9]));
            data_obj[5] = mp_obj_new_str(GPRMC[7],strlen(GPRMC[7]));
            data_obj[6] = mp_obj_new_str(GPRMC[1],strlen(GPRMC[1]));
            // regresar el objeto como una lista
            return mp_obj_new_list(7,data_obj);
        } else
        // en caso contrario regresar el mensaje "NoValid"
        {
            return mp_obj_new_str(novalid,strlen(novalid));
        }
    } else
    // en caso contrario regresar el mensaje "NoValid"
    {
        return mp_obj_new_str(novalid,strlen(novalid));
    }
}

MP_DEFINE_CONST_FUN_OBJ_1(read_obj, gpsl70_read);

/**
 * @brief Lee la linea del puerto.
 *
 * @param self_in
 * @return STATIC La línea leída o None al timeout.
 */
STATIC mp_obj_t gpsl70_readraw(mp_obj_t self_in)
{
    // crear el buffer
    char mens[85];
    size_t index = 0;
    // leer los datos
    while(true)
    {
        if (uart_rx_wait(&uart, uart.timeout))
        {
            mens[index] = uart_rx_char(&uart);
            // verificar si se tiene un fin de linea
            if (mens[index] == '\n' && mens[index - 1] == '\r')
            {
                mens[index+1] = '\0';
                break;
            }
            // dejar de leer si no se detecta un caracter después de timeout_char ms 
            else if (index >= 82)
            {
                mens[index+1] = '\0';
                break;
            }

            // incrementar el contador
            index++;
        } else
        {
            return mp_const_none;
        }
    }

    // regresar un objeto String
    return mp_obj_new_str(mens,strlen(mens));
}

MP_DEFINE_CONST_FUN_OBJ_1(readraw_obj, gpsl70_readraw);

/**
 * @brief Enviar una cadena de datos.
 *
 * @param self_in
 * @param data_obj Mensaje en formato string
 * @return STATIC Regresa el número de caracteres enviados o None al timeout
 */
STATIC mp_obj_t gpsl70_writeraw(mp_obj_t self_in, mp_obj_t data_obj)
{
    // obtener la cadena
    mp_check_self(mp_obj_is_str_or_bytes(data_obj));
    GET_STR_DATA_LEN(data_obj, str, str_len);
    char data[str_len];
    strcpy(data, (char *)str);

    size_t bytes_sent = 0;
    int errcode = 1;

    // esperar a que el registro TX esté vacío
    // si no está disponible, regresar none
    if (!uart_tx_wait(&uart, uart.timeout))
        return mp_const_none;

    // enviar los datos. bytes_sent almacena el número de bytes enviado
    bytes_sent = uart_tx_data(&uart, &data, strlen(data) >> uart.char_width, &errcode);
    
    return mp_obj_new_int(bytes_sent);
}

MP_DEFINE_CONST_FUN_OBJ_2(writeraw_obj, gpsl70_writeraw);


STATIC const mp_rom_map_elem_t gpsl70_class_locals_dict_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&gpsl70_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_armarcadena), MP_ROM_PTR(&armarcadena_obj)},
    {MP_ROM_QSTR(MP_QSTR_sendconfig), MP_ROM_PTR(&sendconfig_obj)},
    {MP_ROM_QSTR(MP_QSTR_checksum), MP_ROM_PTR(&checksum_obj)},
    {MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&on_obj)},
    {MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&off_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&read_obj)},
    {MP_ROM_QSTR(MP_QSTR_readraw), MP_ROM_PTR(&readraw_obj)},
    {MP_ROM_QSTR(MP_QSTR_writeraw), MP_ROM_PTR(&writeraw_obj)},
};

STATIC MP_DEFINE_CONST_DICT(gpsl70_class_locals_dict, gpsl70_class_locals_dict_table);

const mp_obj_type_t gpsl70_class_type = {
    {&mp_type_type},
    .name = MP_QSTR_gpsL70,
    .print = gpsl70_class_print,
    .make_new = gpsl70_class_make_new,
    .locals_dict = (mp_obj_dict_t *)&gpsl70_class_locals_dict,
};

STATIC const mp_rom_map_elem_t gpsl70_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR__name__), MP_ROM_QSTR(MP_QSTR_gpsL70)},
    {MP_ROM_QSTR(MP_QSTR_GPSL70), MP_ROM_PTR(&gpsl70_class_type)},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_gpsl70_globals, gpsl70_globals_table);

const mp_obj_module_t gpsL70_user_cmodule = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&mp_module_gpsl70_globals,
};

MP_REGISTER_MODULE(MP_QSTR_gpsL70, gpsL70_user_cmodule, MODULE_GPSL70_ENABLED);