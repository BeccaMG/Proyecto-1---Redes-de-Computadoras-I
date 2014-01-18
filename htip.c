/**
 * @file htip.c
 * @author Luis Fernandes 10-10239 <lfernandes@ldc.usb.ve>
 * @author Rebeca Machado 10-10406 <rebeca@ldc.usb.ve>
 * 
 * Traducción de nombres de dominio a direcciones IP.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

/**
 * hostname_to_ip
 * 
 * @brief Traduce un nombre de dominio a una IP.
 * @param hostname Nombre de dominio.
 * @param ip IP resultante de la transformación.
 * 
 * Consulta el servidor de nombres de la máquina donde se ejecuta el programa
 * para resolver el hostname y devolver la IP correspondiente.
 * 
 * @see http://www.binarytides.com/hostname-to-ip-address-c-sockets-linux/
 */

int hostname_to_ip(char * hostname , char* ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
        
    if ( (he = gethostbyname( hostname ) ) == NULL) 
    {
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;
    
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return 0;
    }
    
    return 1;
}