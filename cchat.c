/**
 * @file cchat.c
 * @author Luis Fernandes 10-10239 <lfernandes@ldc.usb.ve>
 * @author Rebeca Machado 10-10406 <rebeca@ldc.usb.ve>
 *
 * Programa del Cliente. Se conecta al servidor a través de un puerto
 * y nombre de dominio (o IP) dado. Su comunicación es realizada a través 
 * de un socket y crea un hilo para poder escuchar las respuestas que 
 * recibe del servidor. El hilo principal se queda esperando las entradas
 * del usuario para escribirlas al servidor.
 * 
 */
 
 
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "errors.h"
#include <getopt.h>
#include <signal.h>
#include "htip.c"

//------------------------------------------------------- Variables globales -//

/**
 * \var puerto
 * \brief Puerto por el que escucha el servidor.
 */
int puerto;

/**
 * \var server
 * \brief Servidor al que se realizará la conexión del cliente.
 */
char *server;

/**
 * \var usuario
 * \brief Nombre del usuario introducido con la opción -n.
 */
char *usuario;

/**
 * \var archivo
 * \brief Nombre del archivo introducido con la opción -a.
 */
char *archivo;

/**
 * \var aflag
 * \brief Bandera de control.
 *
 * Indica si se ha introducido un archivo con la opción -a.
 */
int aflag = 0; 

/**
 * \var sockfd
 * \brief Socket con el cual se establece la counicación con el servidor.
 */
int sockfd;


//------------------------------------------------------------------ Métodos -//

/**
 * salir
 * 
 * @brief Sale del sistema.
 * 
 * Envia al servidor el comando fue y cierra el socket por el cual se está
 * comunicando al servidor.
 */

void salir() {
    printf("\n\n¡Hasta luego!\n");
    write(sockfd, "fue\n", 4);
    close(sockfd);
    exit(0);
}


/**
 * escribir_socket
 * 
 * @brief Función que escribe en el socket lo leído de entrada estándar.
 * 
 * Esta función se encarga de escribir en el socket lo que el archivo de
 * entrada indique, línea por línea. Así mismo, escribe en el socket todo
 * aquello que esté ingresando el usuario por entrada estandar, caracter por
 * caracter.
 */
 
void escribir_socket() {
  int c;
  char outbuffer;
  
  strcat(usuario, "\n");
  write(sockfd, usuario, strlen(usuario));
  
  //en este if se escribe si se ha introducido algún archivo
  if (aflag) {
    FILE *file;
    char *line = NULL;
    size_t len;
    ssize_t read;

    file = fopen(archivo, "r");

    if (file == NULL)
        fatalerror("Error en el archivo de entrada.\n");

    while((read = getline(&line, &len, file)) != -1){
        write(sockfd, line, strlen(line));
    }

    fclose(file);
  }

  while ((c = getchar()) != EOF) {
    /*Se escriben los caracteres en el socket*/
    outbuffer = c;
    if (write(sockfd, &outbuffer, 1) != 1)
      fatalerror("No se pudo escribir al socket\n");
  }
  salir();
}


/**
 * escuchar_socket
 * 
 * @brief Lee e imprime lo recibido del socket de comunición con el servidor.
 * 
 * Rutina que ejecuta el hilo lector.
 */

void *escuchar_socket() {
   
    char inbuffer;
    
    while (1) {
        if (read(sockfd, &inbuffer, 1) == 0)
            fatalerror("No se pudo leer del socket.\n");
        
        if (inbuffer == EOF) {
            printf("\n\n¡Hasta luego!\n");
            close(sockfd);
            exit(0);
        }
        printf("%c", inbuffer);
    }
}


/**
 * check_invocation
 * 
 * @brief Evalúa los parámetros introducidos por la invocación del programa.
 * 
 * @param argc Cantidad de argumentos introducidos.
 * @param *argv Apuntador a cadena de caracteres que contiene los argumentos
 *				introducidos.
 * 
 * Evalúa argumentos y los asocia a variables globales, también evalúa que
 * se estén usando las opciones -n -h y -p que son obligatorios para el uso del
 * programa. De no usarse alguno de los parámetros cierra el programa e indica
 * la correcta invocación del programa.
 */

void check_invocation(int argc, char *argv[]) {
    
    int opt; //variable para obtener el parámetro.
    // Modo de uso correcto en cantidad de parámetros
    int pflag = 0; //variable que indica si se usó el flag -p
    int hflag = 0; //variable que indica si se usó el flag -h
    int nflag = 0; //variable que indica si se usó el flag -n
    opterr = 0; 
    
    while ((opt = getopt (argc, argv, "h:p:a:n:")) != -1) {
        
        switch (opt) {
            case 'p':
                pflag = 1; //indica que se invocó "-p"
                puerto = atoi(optarg);
                if (puerto < 1024 || puerto > 65535) {
                    fprintf(stderr,"Puerto inválido. Debe estar entre 1025 y \
65.535.\n");
                    exit(0);
                }
                break;
            
            case 'h':
                hflag = 1;
                server = optarg;
                break;

            case 'n':
                nflag = 1;
                usuario = optarg;
                break;
        
            case 'a':
                aflag = 1;
                archivo = optarg;
                break;

            case ':':
                fprintf(stderr, "Opción -%c requiere un argumento.\n", optopt);
                exit(1);
                
            case '?':
                fprintf (stderr, "Opción desconocida '-%c'.\n", optopt);
                exit(1);
            }
    }
    if (!(pflag) || !(hflag) || !(nflag)) {
        fprintf(stderr, "Modo de uso: %s -h <host> -p <puerto> -n <nombre> \
[-a <archivo>]\n", argv[0]);
        exit(0);
    }
}

/**
 * ctrlc_handler
 * 
 * @brief Manejador de la señal enviada por presionar Ctrl+C
 * 
 * Llama a la función salir.
 */

void ctrlc_handler() {
    salir();
}

//------------------------------------------------------- Programa principal -//

/**
 * main
 * 
 * @brief Programa principal.
 * 
 * @see Proyecto 1 - Informe.pdf
 */

main(int argc, char *argv[]) {
  
    struct sockaddr_in serveraddr;
    signal(SIGINT, ctrlc_handler);

    /*Para recordar el nombre del programa en caso de errores*/
    programname = argv[0];
    pthread_t hilo_escucha;
    char ip[100];
    check_invocation(argc, argv);
    
    hostname_to_ip(server, ip);

    /* Para obtener la dirección del servidor*/
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(ip);
    serveraddr.sin_port = htons(puerto);

    /* Abrir el socket*/
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        fatalerror("No se pudo abrir el socket.\n");

    /* Conectarse al servidor*/
    if (connect(sockfd, (struct sockaddr *) &serveraddr,
                sizeof(serveraddr)) < 0)
        fatalerror("No se pudo conectar al servidor.\n");

    printf("Hola %s, bienvenido al chat :).\n", usuario);
    printf("El largo máximo de los mensajes es 500 caracteres.\n");
    
	/*Crear el hilo que lee lo escrito por el servidor*/
    if (pthread_create(&hilo_escucha, NULL, escuchar_socket, NULL))
        fatalerror("No se pudo crear un hilo para manejar al cliente.");
    
    /* Invocar la función que se encarga de escribir en el socket*/
    escribir_socket(sockfd);
    pthread_kill(hilo_escucha, 0);
    close(sockfd);

    exit(EXIT_SUCCESS);
}
