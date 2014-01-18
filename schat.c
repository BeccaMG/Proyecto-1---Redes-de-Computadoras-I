/**
 * @file schat.c
 * @author Luis Fernandes 10-10239 <lfernandes@ldc.usb.ve>
 * @author Rebeca Machado 10-10406 <rebeca@ldc.usb.ve>
 *
 * Programa principal del servidor. Escucha peticiones por un puerto y un
 * socket específico que determina el usuario al invocar el programa. Crea un
 * hilo manager que maneja las solicitudes el cliente y crea un hilo por cada
 * conexión que se establece.
 * 
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <signal.h>
#include <pthread.h>

#include "errors.h"
#include "lista.c"

#define QUEUELENGTH 5
#define MAXLENGTH 500
#define MAXLENGTH_USER 25

//------------------------------------------------------- Variables globales -//

/**
 * \var sala_pedida
 * \brief Sala con la que se crea el servidor (por defecto "actual").
 */
char *sala_pedida = "actual";

/**
 * \var salida
 * \brief Un caracter EOF.
 * 
 * Se usa para indicarle al cliente que debe finalizar su ejecución cuando lo
 * lee del socket.
 * 
 */
char *salida;

/**
 * \var puerto
 * \brief Puerto por el que escucha el servidor.
 */
int puerto;

/**
 * \var sockfd
 * \brief Socket por el que el servidor escucha peticiones.
 */
int sockfd;

/**
 * \var ret_value
 * \brief Valor de retorno de los hilos.
 */
int ret_value;

/**
 * \var lista_global_salas
 * \brief Lista de salas del servidor.
 */
lista lista_global_salas;

/**
 * \var cola_global_comandos
 * \brief Cola de los comandos de la cual lee el hilo manager.
 */
lista cola_global_comandos;

/**
 * \var lista_global_hilos_usuarios
 * \brief Lista de hilos y usuarios.
 * 
 * Esta lista contiene todos los usuarios del sistema y el hilo asignado a cada
 * uno, de manera que cada elemento de la lista coincide con un hilo y un
 * cliente del servidor.
 */
lista lista_global_hilos_usuarios;

/**
 * \var tid_manager
 * \brief Thread ID del hilo manager.
 */
pthread_t tid_manager;

/**
 * \var mutex_comandos
 * \brief Semáforo que bloquea la cola de comandos.
 */
pthread_mutex_t mutex_comandos;

/**
 * \var mutex_salas
 * \brief Semáforo que bloquea la lista de salas.
 */
pthread_mutex_t mutex_salas;

//------------------------------------------------ Definición de estructuras -//

/**
 * \struct usuario
 * \brief Struct que representa un usuario del sistema.
 */
typedef struct {
    
    /**
     * \var nombre_usuario
     * \brief Nombre del usuario cliente.
     */
    char *nombre_usuario;
    
    /**
     * \var socket
     * \brief Socket por el que lee y escribe el cliente.
     */
    int socket;
    
    /**
     * \var mutex_socket
     * \brief Semáforo que bloquea el socket por el que se comunica el cliente.
     */
    pthread_mutex_t mutex_socket;
    
    /**
     * \var lista_salas_suscritas
     * \brief Lista de salas a las que está suscrito el usuario.
     */
    lista lista_salas_suscritas;
    
} usuario;


/**
 * \struct sala
 * \brief Struct que representa una sala del sistema.
 */
typedef struct {
    
    /**
     * \var nombre_sala
     * \brief Nombre de la sala.
     */
    char *nombre_sala;
    
    /**
     * \var lista_usuarios_activos
     * \brief Lista de usuarios que están suscritos a la sala.
     */
    lista lista_usuarios_activos;
    
} sala;


/**
 * \struct comando
 * \brief Struct que representa un comando que envía cada usuario conectado.
 * 
 * Estos comandos son manejados por el hilo manager en una cola de comandos.
 */
typedef struct {
    
    /**
     * \var texto
     * \brief Texto del comando.
     */
    char *texto;
    
    /**
     * \var sender
     * \brief Usuario que envía el comando.
     */
    usuario *sender;
    
} comando;


/**
 * \struct hilo_usuario
 * \brief Struct que representa un hilo y un usuario asociado a una conexión.
 */
typedef struct {
    
    /**
     * \var hilo
     * \brief Thread ID del hilo que maneja una conexión.
     */
    pthread_t hilo;
    
    /**
     * \var cliente
     * \brief Usuario que representa el cliente que crea una conexión.
     */
    usuario *cliente;
    
} hilo_usuario;


/**
 * \struct param_hc
 * \brief Envuelve un hilo_usuario para pasarlo como parámetro a un hilo.
 */
typedef struct {
    
    /**
     * \var hilo_cliente
     * \brief Parámetro que se pasa a un hilo asociado a un cliente.
     * 
     * El hilo maneja esta conexión con un nuevo cliente, utilizando los
     * atributos del hilo_usuario pasado como parámetro mediante esta
     * estructura.
     */
    hilo_usuario *hilo_cliente;
    
} param_hc;


//------------------------------------------------------------------ Métodos -//

/**
 * hilos_iguales
 * 
 * @brief Compara dos estructuras hilo_usuario.
 * 
 * @param h1 Nombre del usuario.
 * @param h2 Estructura hilo_usuario a comparar.
 * @return 1 si son iguales y 0 si son diferentes.
 * 
 * Compara el nombre del cliente de una estructura hilo_usuario con un nombre de
 * usuario pasado como parámetro.
 */

int hilos_iguales(void *h1, void *h2) {
    char *hilo1 = (char *) h1;
    hilo_usuario *hilo2 = (hilo_usuario *) h2;
    return (!strcmp(hilo1,hilo2->cliente->nombre_usuario));
}


/**
 * salas_iguales
 * 
 * @brief Compara dos salas.
 * 
 * @param s1 Nombre de la sala.
 * @param s2 Sala a comparar.
 * @return 1 si son iguales y 0 si son diferentes.
 * 
 * Compara el nombre de una sala con el nombre pasado como parámetro.
 */

int salas_iguales(void *s1, void *s2) {
    char *sala1 = (char *) s1;
    sala *sala2 = (sala *) s2;
    return (!strcmp(sala1,sala2->nombre_sala));
}


/**
 * usuarios_iguales
 * 
 * @brief Compara dos usuarios.
 * 
 * @param u1 Nombre del usuario.
 * @param u2 Usuario a comparar.
 * @return 1 si son iguales y 0 si son diferentes.
 * 
 * Compara el nombre del usuario u2 con el nombre pasado como parámetro.
 */

int usuarios_iguales(void *u1, void *u2) {
    char *usuario1 = (char *) u1;
    usuario *usuario2 = (usuario *) u2;
    return (!strcmp(usuario1,usuario2->nombre_usuario));
}


/**
 * crear_sala
 * 
 * @brief Crea una nueva sala y la agrega a la lista global de salas.
 * 
 * @param sala_agregar Nombre de la sala nueva.
 * @param user Usuario que solicita crear la sala.
 * 
 * Crea una nueva sala mediante un malloc, le asigna el nombre pasado como
 * parámetro y la agrega al inicio de la lista de salas para mayor eficiencia.
 * Al agregar a la lista se bloquea el semáforo correspondiente en caso de que
 * otro procedimiento esté accediendo a la misma lista. Si ocurre algún error o
 * la sala ya existe, el procedimiento le escribe directamente al socket del
 * usuario que ejecutó el comando.
 */

void crear_sala(char *sala_agregar, usuario *user) {
    
    pthread_mutex_lock(&mutex_salas);

    if (existe_elemento(lista_global_salas, sala_agregar, salas_iguales)) {
        
        pthread_mutex_lock(&user->mutex_socket);
        write(user->socket, "\nLa sala ya existe.\n\n", 21);
        pthread_mutex_unlock(&user->mutex_socket);
        
        pthread_mutex_unlock(&mutex_salas);
        return;
    }
    
    sala *nueva_sala = malloc(sizeof(sala));
    
    if (nueva_sala == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        pthread_mutex_unlock(&mutex_salas);
        return;
    }
    
    nueva_sala->nombre_sala = sala_agregar;
    crear_lista(&nueva_sala->lista_usuarios_activos);
                    
    agregar_principio(&lista_global_salas, nueva_sala);
    
    pthread_mutex_unlock(&mutex_salas);
}


/**
 * eliminar_sala
 * 
 * @brief Elimina una sala de la lista global de salas.
 * 
 * @param sala_eliminar Nombre de la sala a eliminar.
 * @param user Usuario que solicita eliminar la sala.
 * 
 * Recorre la lista de salas en busca de la que se solicita eliminar. 
 * Al eliminar de la lista se bloquea el semáforo correspondiente en caso de que
 * otro procedimiento esté accediendo a la misma lista. Si ocurre algún error o
 * la sala no existe, el procedimiento le escribe directamente al socket del
 * usuario que ejecutó el comando.
 */

void eliminar_sala(char *sala_eliminar, usuario *user) {
    
    pthread_mutex_lock(&mutex_salas);
    
    sala *s = (sala *) encontrar_elemento(lista_global_salas, sala_eliminar,
                                          salas_iguales);
    usuario *user_sala;
    nodo *usuarios_sala;
    
    if (s != NULL) {
        usuarios_sala = s->lista_usuarios_activos.cabeza;
        
        while (usuarios_sala != NULL) {
            user_sala = (usuario *) usuarios_sala->elemento;
            eliminar_elemento(&user_sala->lista_salas_suscritas, sala_eliminar,
                              salas_iguales, 0);
            usuarios_sala = usuarios_sala->sig;
        }
        
        eliminar_elemento(&lista_global_salas, sala_eliminar, salas_iguales, 1);
        
    } else {
        pthread_mutex_lock(&user->mutex_socket);
        write(user->socket, "\nLa sala no existe.\n\n", 21);
        pthread_mutex_unlock(&user->mutex_socket);
    }
    
    pthread_mutex_unlock(&mutex_salas);
}


/**
 * suscribir_usuario
 * 
 * @brief Suscribe un usuario a una sala.
 * 
 * @param sala_suscribir Nombre de la sala a suscribir.
 * @param user Usuario que solicita suscribirse a la sala.
 * 
 * Recorre la lista de salas en busca de la que se solicita. Una vez
 * encontrada, se agrega al usuario al inicio de la lista de usuarios activos de
 * esa sala y se agrega la sala al inicio de las salas suscritas del usuario.
 * Al suscribirse a la sala, se bloquea el semáforo correspondiente a la lista
 * global de salas en caso de que otro procedimiento esté accediendo a la misma
 * lista. Si ocurre algún error o la sala no existe, el procedimiento le
 * escribe directamente al socket del usuario que ejecutó el comando.
 */

void suscribir_usuario(char *sala_suscribir, usuario *user) {
    
    pthread_mutex_lock(&mutex_salas);

    if (!lista_vacia(lista_global_salas)) {
        sala *actual = (sala *) encontrar_elemento(lista_global_salas,
                                sala_suscribir, salas_iguales);
                
        if (actual != NULL) {
            if (!existe_elemento(user->lista_salas_suscritas,
                                 actual->nombre_sala, salas_iguales)) {
                
                agregar_principio(&user->lista_salas_suscritas, actual);
                agregar_principio(&actual->lista_usuarios_activos, user);
            
            } else {
                pthread_mutex_lock(&user->mutex_socket);
                write(user->socket, "\nYa estás suscrito.\n\n", 22);
                pthread_mutex_unlock(&user->mutex_socket);
            }
            pthread_mutex_unlock(&mutex_salas);
            return;
        }
    }
    
    pthread_mutex_lock(&user->mutex_socket);
    write(user->socket, "\nLa sala no existe.\n\n", 21);
    pthread_mutex_unlock(&user->mutex_socket);
    
    pthread_mutex_unlock(&mutex_salas);
}



/**
 * desuscribir_usuario
 * 
 * @brief De-suscribe al usuario de todas las salas.
 * 
 * @param user Usuario que solicita de-suscribirse a la sala.
 * 
 * Elimina al usuario de todas las salas a las que está suscrito, recorriendo
 * la lista del usuario y eliminándolo sala por sala. Después crea una sala
 * vacía en la lista de salas suscritas del usuario.
 * 
 * Al de-suscribirse de las salas, se bloquea el semáforo correspondiente a la
 * lista global de salas en caso de que otro procedimiento esté accediendo a
 * la misma lista.
 */

void desuscribir_usuario(usuario *user) {
    
    pthread_mutex_lock(&mutex_salas);
    
    nodo *aux;
    sala *s;

    aux = user->lista_salas_suscritas.cabeza;
    
    while (aux != NULL) {
        s = (sala *) aux->elemento;
        eliminar_elemento(&s->lista_usuarios_activos, user->nombre_usuario,
                          usuarios_iguales, 0);
        aux = aux -> sig;
    }
    
    crear_lista(&user->lista_salas_suscritas);
    
    pthread_mutex_unlock(&mutex_salas);
}


/**
 * eliminar_usuario
 * 
 * @brief Elimina un usuario del sistema.
 * 
 * @param user Usuario que se va a eliminar.
 * 
 * La función primero desuscribe al usuario de todas las salas y después lo
 * elimina de la lista global de hilos y usuarios, liberando la memoria.
 */

void eliminar_usuario(usuario *user) {
    desuscribir_usuario(user);
    eliminar_elemento(&lista_global_hilos_usuarios, user->nombre_usuario,
                      hilos_iguales, 1);
}


/**
 * imprimir_lista_salas
 * 
 * @brief Imprime una lista de salas.
 * 
 * @param l Lista a imprimir.
 * @param user Usuario que desea imprimir una lista de salas.
 * @param sistema Variable de control que indica si son las salas del sistema.
 * 
 * La función recorre la lista pasada como parámetro imprimiendo cada uno de
 * sus elementos (salas). En caso de que la variable sistema sea 1, imprime el
 * encabezado de la lista de salas del sistema. En caso de que sea 0, imprime la
 * lista de salas a las que está suscrito un usuario. Al recorrer la lista se
 * bloquea el semáforo de la lista global de salas en caso de que otro
 * procedimiento la desee modificar mientras se lee de ella.
 * 
 * La lista de salas (vacía o no) se envía directamente al socket del usuario
 * que ejecuta el comando.
 */

void imprimir_lista_salas(lista l, usuario *user, int sistema) {
    
    pthread_mutex_lock(&mutex_salas);
    pthread_mutex_lock(&user->mutex_socket);
    
    if (sistema) {
        write(user->socket, "\nLISTA DE SALAS DEL SISTEMA\n====================\
======\n",56);
    } else {
        write(user->socket, "\nLISTA DE SALAS SUSCRITAS\n======================\
==\n",51);
    }

    nodo *aux = l.cabeza;
    
    while (aux != NULL) {
        sala *actual = (sala *) aux->elemento;
        
        write(user->socket, "\"", 1);
        write(user->socket, actual->nombre_sala, strlen(actual->nombre_sala));
        write(user->socket, "\"\n", 2);
        
        aux = aux->sig;
    }
    
    write(user->socket, "\n", 1);
    
    pthread_mutex_unlock(&user->mutex_socket);
    pthread_mutex_unlock(&mutex_salas);
}


/**
 * listar_usuarios
 * 
 * @brief Imprime la lista de usuarios del sistema.
 * 
 * @param user Usuario que desea imprimir una lista de salas.
 * 
 * La función recorre la lista de usuarios del sistema e imprime cada elemento
 * de la misma directamente en el socket del usuario que ejecuta el comando.
 */

void listar_usuarios(usuario *user) {
    
    nodo *nodo_aux = NULL;
    hilo_usuario *hc_aux = NULL;
    usuario *usuario_aux = NULL;
    
    char *user_name = malloc(MAXLENGTH_USER);
    
    if (user_name == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        return;
    }
    
    pthread_mutex_lock(&user->mutex_socket);
    write(user->socket, "\nLISTA DE USUARIOS DEL SISTEMA\n=====================\
========\n",61);
    
    if(!(lista_vacia(lista_global_hilos_usuarios))){
        nodo_aux = lista_global_hilos_usuarios.cabeza;
    }

    while(nodo_aux != NULL){
        hc_aux = nodo_aux->elemento;
        usuario_aux = hc_aux->cliente;
        strcpy(user_name, usuario_aux->nombre_usuario);
        strcat(user_name, "\n");
        
        write(user->socket, user_name, strlen(user_name));
        
        nodo_aux = nodo_aux->sig;
    }
    
    write(user->socket, "\n", 1);
    free(user_name);
    pthread_mutex_unlock(&user->mutex_socket);
}


/**
 * enviar_mensaje
 * 
 * @brief Envía un mensaje de parte de un usuario.
 * 
 * @param user Usuario que desea imprimir una lista de salas.
 * @param mens Mensaje a enviar.
 * 
 * La función recibe un usuario y un mensaje a enviar. Recorre la lista de
 * salas suscritas del usuario y por cada sala suscrita, envía el mensaje al
 * socket de cada usuario suscrito a esa sala, incluyéndolo a él mismo.
 */

void enviar_mensaje(usuario *user, char *mens){

    nodo *nodo_sala_usuario;
    nodo *aux_nodo;
    
    if(!(lista_vacia(lista_global_salas))){
        nodo_sala_usuario = user->lista_salas_suscritas.cabeza;
    }
        
    char *complemento_mensaje = malloc(strlen(mens)+200);
    
    if (complemento_mensaje == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        return;
    }
    
    sala *aux_sala_usuario;//auxiliar para moverse por las salas del user
    usuario *user_act;
    nodo *user_nod;
    mens = mens + 4;

    // por cada sala en el usuario
    while(nodo_sala_usuario != NULL){
        
        if(!(lista_vacia(user->lista_salas_suscritas))) {
            aux_nodo = lista_global_salas.cabeza;
        }
        
        if(nodo_sala_usuario->elemento != NULL) {
            aux_sala_usuario = nodo_sala_usuario->elemento;
            
            if(!(lista_vacia(aux_sala_usuario->lista_usuarios_activos))) {
                user_nod = aux_sala_usuario->lista_usuarios_activos.cabeza;
                
                // por cada usuario en la sala
                while(user_nod != NULL){
                    user_act = user_nod->elemento;

                    strcpy(complemento_mensaje, "\n>> ");
                    strcat(complemento_mensaje, user->nombre_usuario);
                    strcat(complemento_mensaje, "@");
                    strcat(complemento_mensaje, aux_sala_usuario->nombre_sala);
                    strcat(complemento_mensaje, ": ");
                    strcat(complemento_mensaje, mens);
                    strcat(complemento_mensaje, "\n");
                    
                    pthread_mutex_lock(&user_act->mutex_socket);
                    write(user_act->socket, complemento_mensaje, 
                          strlen(complemento_mensaje));
                    pthread_mutex_unlock(&user_act->mutex_socket);

                    user_nod = user_nod->sig;
                }
            }
         }

        nodo_sala_usuario = nodo_sala_usuario->sig;
    }
    free(complemento_mensaje);
}


//------------------------------------------------------------- Hilo manager -//

/**
 * rutina_hilo_manager
 * 
 * @brief Función que ejecuta el hilo manager del servidor.
 * 
 * @see Proyecto 1 - Informe.pdf
 */

void *rutina_hilo_manager() {
    
    comando *com;
    char aux[4];
        
    while (1) {
        pthread_mutex_lock(&mutex_comandos);
        if (!lista_vacia(cola_global_comandos)) {
            
            com = (comando *) extraer_primero(&cola_global_comandos);
            pthread_mutex_unlock(&mutex_comandos);
           
            strncpy(aux, com->texto, 3);
            
            aux[3] = '\0';
            com->texto = com->texto+4;
            
            if (aux != NULL) {
                if (!strcmp(aux,"cre")) {
                    if (strcmp(com->texto,"")) // Revisa que la sala no es vacía
                        crear_sala(com->texto, com->sender);
                } else if (!strcmp(aux,"eli")) {
                    if (existe_elemento(lista_global_salas,com->texto,
                        salas_iguales))
                        eliminar_sala(com->texto, com->sender);
                } else if (!strcmp(aux,"sus")) {
                    suscribir_usuario(com->texto, com->sender);
                } else if (!strcmp(aux,"sal")) {
                    imprimir_lista_salas(lista_global_salas, com->sender, 1);
                } else if (!strcmp(aux,"des")) {
                    desuscribir_usuario(com->sender);
                } else if (!strcmp(aux,"mis")) {
                    imprimir_lista_salas(com->sender->lista_salas_suscritas,
                                         com->sender, 0);
                }
            }
            free(com);
        }
        pthread_mutex_unlock(&mutex_comandos);
    }
}


//------------------------------------------------------------- Hilo cliente -//

/**
 * rutina_hilo_cliente
 * 
 * @brief Función que ejecuta el hilo cliente de cada conexión.
 * @param args Argumentos que se le pasan a la función (estructura param_hc).
 * 
 * @see Proyecto 1 - Informe.pdf
 */

void *rutina_hilo_cliente(void *args) {
    
    param_hc *parametro = (param_hc *) args; //socket de la comunicación
    
    // Inicializo el usuario
    usuario *user = parametro->hilo_cliente->cliente;
    user->nombre_usuario = malloc(MAXLENGTH_USER);
    crear_lista(&user->lista_salas_suscritas);
    
    if (user->nombre_usuario == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        ret_value = 1;
        pthread_exit(&ret_value);
    }
    
    char *nombre_aux = malloc(MAXLENGTH_USER);
    
    if (nombre_aux == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        ret_value = 1;
        pthread_exit(&ret_value);
    }
    
    char *mens_pide_nombre = "Ese nombre de usuario ya existe, por favor \
ingrese otro: \n";

    char c;
    int status;
    int i = 0;

    do {
        i = 0;

        while (read(user->socket, &c, 1) == 1) {
            
            if (c=='\n') {
                *(nombre_aux + i) = '\0';
                break;
            } else {
                *(nombre_aux + i) = c;
                i++;
            }
        }
        
        if(existe_elemento(lista_global_hilos_usuarios, nombre_aux, 
                            hilos_iguales)){
            write(user->socket, mens_pide_nombre, strlen(mens_pide_nombre));
        }

    } while(existe_elemento(lista_global_hilos_usuarios, nombre_aux, 
                            hilos_iguales));

    strcpy(user->nombre_usuario, nombre_aux);
    free(nombre_aux);
    
    // Aquí se suscribe al usuario a la sala default
    comando *com_inicial = malloc(sizeof(comando));
    
    if (com_inicial == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        ret_value = 1;
        pthread_exit(&ret_value);
    }
    
    com_inicial->sender = user;
    com_inicial->texto = malloc(MAXLENGTH);
    
    strcpy(com_inicial->texto,"sus ");
    strcat(com_inicial->texto, sala_pedida);

    pthread_mutex_lock(&mutex_comandos);
    agregar_final(&cola_global_comandos, com_inicial);
    pthread_mutex_unlock(&mutex_comandos);
    
    // A partir de aquí se lee del socket permanentemente
    char *mensaje = malloc(MAXLENGTH);
    
    if (mensaje == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        ret_value = 1;
        pthread_exit(&ret_value);
    }
    
    comando *com_cliente;
    int j;
    
    while (1) {
        
        i = 0;
        j = 0;
        
        while ((status = read(user->socket, &c, 1)) == 1) {
            
            
            if (j >= MAXLENGTH) {
                
                mensaje = realloc(mensaje, sizeof(j*2));
                j = 0;
                
                if (mensaje == NULL) {
                    fprintf(stderr, "No se puede reasignar memoria.\n");
                    pthread_exit(&ret_value);
                }
            }
            
            if (c == '\n') {
                *(mensaje+i) = '\0';
                break;
            } else {
                *(mensaje+i) = c;
                i++;
                j++;
            }
        }
        
        if (status != 1) {
            ret_value = 1;
            pthread_exit(&ret_value);
            
        } else {
            
            if (strncmp(mensaje, "\0", 1)) {
                
                if (strlen(mensaje) >= 4) {
                    if (!strncmp(mensaje, "men ", 4)) {
                        enviar_mensaje(user, mensaje);
                    } else if (!strncmp(mensaje, "sus ", 4) || 
                               !strncmp(mensaje, "cre ", 4) ||
                               !strncmp(mensaje, "eli ", 4)) {
                        
                        com_cliente = malloc(sizeof(comando));
                    
                        if (com_cliente == NULL) {
                            fprintf(stderr, "No se puede asignar memoria.\n");
                            pthread_exit(&ret_value);
                        }
                    
                        com_cliente->sender = user;
                        com_cliente->texto = malloc(sizeof(strlen(mensaje)));
                        
                        if (com_cliente->texto == NULL) {
                            fprintf(stderr, "No se puede asignar memoria.\n");
                            pthread_exit(&ret_value);
                        }
                        
                        strcpy(com_cliente->texto, mensaje);
                        
                        pthread_mutex_lock(&mutex_comandos);
                        agregar_final(&cola_global_comandos, com_cliente);
                        pthread_mutex_unlock(&mutex_comandos);
                        
                    } else {
                        pthread_mutex_lock(&user->mutex_socket);
                        write(user->socket, "Comando no reconocido\n", 22);
                        pthread_mutex_unlock(&user->mutex_socket);
                    }
                    
                } else if (strlen(mensaje) == 3) {
                    if (!strncmp(mensaje, "sal", 3) || 
                        !strncmp(mensaje, "mis", 3) ||
                        !strncmp(mensaje, "des", 3)) {
                       
                        com_cliente = malloc(sizeof(comando));
                    
                        if (com_cliente == NULL) {
                            fprintf(stderr, "No se puede asignar memoria.\n");
                            pthread_exit(&ret_value);
                        }
                        
                        com_cliente->sender = user;
                        com_cliente->texto = malloc(sizeof(strlen(mensaje)));
                        
                        if (com_cliente->texto == NULL) {
                            fprintf(stderr, "No se puede asignar memoria.\n");
                            pthread_exit(&ret_value);
                        }
                        
                        strcpy(com_cliente->texto, mensaje);
                        
                        pthread_mutex_lock(&mutex_comandos);
                        agregar_final(&cola_global_comandos, com_cliente);
                        pthread_mutex_unlock(&mutex_comandos);
                        
                    } else if (!strncmp(mensaje, "usu", 3)){
                        listar_usuarios(user);

                    } else if (!strncmp(mensaje, "fue", 3)) {
                        eliminar_usuario(user);
                        pthread_mutex_lock(&user->mutex_socket);
                        write(user->socket, salida, 1);
                        pthread_mutex_unlock(&user->mutex_socket);
                        ret_value = 0;
                        pthread_exit(&ret_value);

                    } else {
                        pthread_mutex_lock(&user->mutex_socket);
                        write(user->socket, "Comando no reconocido\n", 22);
                        pthread_mutex_unlock(&user->mutex_socket);
                    }
                    
                } else {
                    pthread_mutex_lock(&user->mutex_socket);
                    write(user->socket, "Comando no reconocido\n", 22);
                    pthread_mutex_unlock(&user->mutex_socket);
                }
            }
        }
    }
    free(mensaje);
}


/**
 * rutina_hilo_manager
 * 
 * @brief Verifica que la invocación al programa sea correcta.
 * @param argc Cantidad de argumentos del programa principal.
 * @param argv Arreglo de argumentos del programa principal.
 * 
 * Modo de invocación: schat -p <puerto> [-s <sala>]
 */

void check_invocation(int argc, char *argv[]) {
    
    int opt;
    int pflag = 0; //variable que indica si se usó el flag -p
    opterr = 0; 
    
    while ((opt = getopt (argc, argv, "p:s:")) != -1) {
        
        switch (opt) {
            case 'p':
                pflag = 1; //indica que se invocó "-p"
                puerto = atoi(optarg);
                if (puerto < 1024 || puerto > 65535) {
                    fprintf(stderr,"Puerto inválido. Debe estar entre 1025 y \
65.535.\n");
                    exit(1);
                }
                break;
            
            case 's':
                sala_pedida = optarg;
                break;
        
            case ':':
                fprintf(stderr, "Opción -%c requiere un argumento.\n", optopt);
                exit(1);
                
            case '?':
                fprintf (stderr, "Opción desconocida '-%c'.\n", optopt);
                exit(1);
        }

    }
    
    if (!pflag) {
        fprintf (stderr,"Modo de uso: %s -p <puerto> [-s <sala>]\n",
                argv[0]);
        exit(1);
    }
}


/**
 * ctrlc_handler
 * 
 * @brief Manejador de la señal enviada por presionar Ctrl+C
 * 
 * Indica en el servidor que este ha sido terminado y libera la memoria
 * necesaria. Además, envía la señal de finalización (caracter salida) a cada
 * cliente en el sistema.
 */

void ctrlc_handler(){
    
    pthread_kill(tid_manager, 0);
    
    hilo_usuario *hc;
    sala *s;
    
    while (!lista_vacia(lista_global_hilos_usuarios)) {
        hc = (hilo_usuario *) extraer_primero(&lista_global_hilos_usuarios);
        pthread_mutex_lock(&hc->cliente->mutex_socket);
        write(hc->cliente->socket, salida, 1);
        pthread_mutex_unlock(&hc->cliente->mutex_socket);
        
        free(hc->cliente->nombre_usuario);
        free(hc->cliente);
        free(hc);
    }
    
    while (!lista_vacia(lista_global_salas)) {
        s = (sala *) extraer_primero(&lista_global_salas);
        free(s);
    }
    
    free(salida);
    close(sockfd);
    exit(0);
}


//------------------------------------------------------- Programa principal -//


/**
 * main
 * 
 * @brief Programa principal.
 * 
 * @see Proyecto 1 - Informe.pdf
 */

int main(int argc, char *argv[]) {
    
    // Rutinas iniciales
    check_invocation(argc,argv);
    signal(SIGINT, ctrlc_handler);
    printf("Esperando conexiones por el puerto = %d...\n", puerto);
    
    // Crear el tid manager y la lista global de salas
    crear_lista(&lista_global_salas);
    crear_lista(&cola_global_comandos);
    crear_lista(&lista_global_hilos_usuarios);
    salida = malloc(1);
    
    if (salida == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        exit(1);
    }
    
    *salida = EOF;
    
    if (pthread_create(&tid_manager, NULL, rutina_hilo_manager, NULL))
        fatalerror("No se pudo crear el hilo manager.\n");
    
    comando *com_inicial = malloc(sizeof(comando));
    
    if (com_inicial == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        exit(1);
    }
                        
    com_inicial->sender = NULL;
    com_inicial->texto = malloc(MAXLENGTH);
    
    if (com_inicial->texto == NULL) {
        fprintf(stderr, "No se puede asignar memoria.\n");
        exit(1);
    }
                        
    strcpy(com_inicial->texto,"cre ");
    strcat(com_inicial->texto, sala_pedida);

    pthread_mutex_lock(&mutex_comandos);
    agregar_final(&cola_global_comandos, com_inicial);
    pthread_mutex_unlock(&mutex_comandos);
    
    int newsockfd;
    struct sockaddr_in clientaddr, serveraddr;
    int clientaddrlength;
    pthread_t id_hc; //identificador del hilo cliente.
    
    /* Remember the program name for error messages. */
    programname = argv[0];
    /* Open a TCP socket. */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0)
        fatalerror("No se puede abrir el socket.\n");
    
    /* Bind the address to the socket. */
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(puerto);
    
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) != 0)
        fatalerror("No se pudo asociar al socket.\n");
    
    if (listen(sockfd, QUEUELENGTH) < 0)
        fatalerror("No se puede escuchar por el socket.\n");

    while (1) {
        /* Wait for a connection. */
        clientaddrlength = sizeof(clientaddr);
        newsockfd = accept(sockfd, (struct sockaddr *) &clientaddr,
                           &clientaddrlength);

        if (newsockfd < 0) {
            fprintf(stderr, "Error al aceptar la conexión\n");
            continue;
        }
        
        // Asigno la memoria para lo que se le pasa al hilo
        usuario *usuario_nuevo = malloc(sizeof(usuario));
        
        if (usuario_nuevo == NULL) {
            fprintf(stderr, "No se puede asignar memoria.\n");
            continue;
        }
    
        hilo_usuario *hilo_nuevo_cliente = malloc(sizeof(hilo_usuario));
        
        if (hilo_nuevo_cliente == NULL) {
            fprintf(stderr, "No se puede asignar memoria.\n");
            continue;
        }
        
        param_hc *parametro = malloc(sizeof(param_hc));
        
        if (parametro == NULL) {
            fprintf(stderr, "No se puede asignar memoria.\n");
            continue;
        }
        
        // Inserta el socket al usuario
        usuario_nuevo->socket = newsockfd;
        
        // Inserta los parámetros del hilo
        hilo_nuevo_cliente->cliente = usuario_nuevo;
        parametro->hilo_cliente = hilo_nuevo_cliente;
        
        agregar_principio(&lista_global_hilos_usuarios, hilo_nuevo_cliente);
        
        if (pthread_create(&hilo_nuevo_cliente->hilo, NULL, rutina_hilo_cliente,
                            parametro)) {
            fprintf(stderr, "No se pudo crear un hilo para manejar al \
cliente.");
            continue;
        }
    }
    
    free(salida);
    return 0;
}
