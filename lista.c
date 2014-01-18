/**
 * @file lista.c
 * @author Luis Fernandes 10-10239 <lfernandes@ldc.usb.ve>
 * @author Rebeca Machado 10-10406 <rebeca@ldc.usb.ve>
 * 
 * Funciones para el manejo de listas.
 */

#include <stdio.h>
#include <stdlib.h>


/**
 * \struct nodo
 * \brief Struct que representa un elemento de una lista.
 */

typedef struct nodo {
    
    /**
     * @var elemento
     * @brief Elemento del nodo.
     */
    void *elemento;
    
    /**
     * @var sig
     * @brief Siguiente nodo en la lista.
     */
    struct nodo *sig;
    
} nodo;


/**
 * \struct usuario
 * \brief Struct que representa una lista de nodos.
 */

typedef struct {
    
    /**
     * @var cabeza
     * @brief Primer elemento de la lista.
     */
    nodo *cabeza;
    
} lista;


/**
 * lista_vacia
 * 
 * @brief Verifica si una lista es vacía.
 * @param l Lista a verificar.
 * @return Un entero igual a 0 si está vacía. Mayor a 0 si no lo está.
 *
 */

int lista_vacia(lista l) {
    return (l.cabeza == NULL);
}


/**
 * crear_lista
 * 
 * @brief Inicializa una lista.
 * @param l Lista a inicializar.
 *
 */

void crear_lista(lista *l) {
    l->cabeza = NULL;
}


/**
 * agregar_principio
 * 
 * @brief Agrega un elemento al principio de la lista.
 * @param l Lista a la que se agrega el elemento.
 * @param elem Elemento a agregar.
 *
 */

void agregar_principio(lista *l, void *elem) {
    nodo *nuevo = malloc(sizeof(nodo));
    nuevo->elemento = elem;
    nuevo->sig = NULL;
    if (!lista_vacia(*l)) {
        nuevo->sig = l->cabeza;
    }
    l->cabeza = nuevo;
}


/**
 * agregar_final
 * 
 * @brief Agrega un elemento al final de la lista.
 * @param l Lista a la que se agrega el elemento.
 * @param elem Elemento a agregar.
 *
 */

void agregar_final(lista *l, void *elem) {
    nodo *nuevo = malloc(sizeof(nodo));
    nuevo->elemento = elem;
    nuevo->sig = NULL;
    if (lista_vacia(*l)) {
        l->cabeza = nuevo;
    } else {
        nodo *aux = l->cabeza;
        while (aux->sig != NULL) {
            aux = aux->sig;
        }
        aux->sig = nuevo;
    }
}

/**
 * encontrar_elemento
 * 
 * @brief Encuentra un elemento de la lista y lo devuelve.
 * 
 * @param l Lista en la que se busca el elemento.
 * @param elem Elemento a buscar.
 * @param compare Función de comparación entre elementos de la lista.
 *
 * La función recorre la lista comparando elemento por elemento con el
 * parámetro pasado y la función compare, que debe manejar un elemento de la
 * lista y un elem.
 */

void *encontrar_elemento(lista l, void *elem, int (*compare)(void *,void*)) {
    
    if (!lista_vacia(l)) {
        nodo *aux = l.cabeza;
        
        while (aux != NULL) {
            if (compare(elem, aux->elemento))
                return aux->elemento;
            aux = aux->sig;
        }
    }
    return NULL;
}


/**
 * eliminar_elemento
 * 
 * @brief Encuentra un elemento de la lista y lo elimina.
 * 
 * @param l Lista en la que se busca y elimina el elemento.
 * @param elem Elemento a eliminar.
 * @param compare Función de comparación entre elementos de la lista.
 * @param destruir Indica si se libera memoria o no.
 *
 * Se elimina un elemento de la lista recorriendo los elementos con la función
 * compare y se extrae de la misma si se encuentra. En caso de que destruir sea
 * 1, se libera la memoria. Si es 0, simplemente se elimina de la lista.
 */

void eliminar_elemento(lista *l, void *elem, int (*compare)(void *,void*), 
                       int destruir) {
    
    if (!lista_vacia(*l)) {
        
        nodo *aux = l->cabeza;
        nodo *aux2 = aux;
        
        if (compare(elem, aux->elemento)) {
            l->cabeza = aux->sig;
            if (destruir)
                free(aux);
            return;
        }
        
        aux=aux->sig;
        
        while (aux != NULL) {
            if (compare(elem, aux->elemento))
                break;
            aux2 = aux;
            aux=aux->sig;
        }
        
        if (aux != NULL) { // Sí encontró la lista
            aux2->sig = aux->sig;
            if (destruir)
                free(aux->elemento);
                free(aux);
            return;
        }
    }
}


/**
 * existe_elemento
 * 
 * @brief Verifica si la lista contiene un elemento.
 * 
 * @param l Lista en la que se busca el elemento.
 * @param elem Elemento a buscar.
 * @param compare Función de comparación entre elementos de la lista.
 * @return 1 si encuentra el elemento, 0 en caso contrario.
 * 
 * Se recorre la lista con la función compare y se verifica, elemento por
 * elemento, si es el que se buscar.
 */

int existe_elemento(lista l, void *elem, int (*compare)(void *,void*)) {
    nodo *aux = l.cabeza;
    while (aux != NULL) {
        if (compare(elem, aux->elemento))
            return 1;
        aux = aux->sig;
    }
    return 0;
}


/**
 * extraer_primero
 * 
 * @brief Extrae el primero elemento de una lista, simulando una cola.
 * @param l Lista cuyo primero elemento se extrae.
 * @return Primer elemento de l.
 *
 */

void *extraer_primero(lista *l) {
    if (lista_vacia(*l)) 
        return NULL;
    nodo *aux;
    aux = l->cabeza;
    l->cabeza = aux->sig;
    return aux->elemento;
}


/**
 * longitud
 * 
 * @brief Devuelve el tamaño de una lista (cantidad de elementos).
 * @param l Lista cuyo tamaño será devuelto.
 * @return Entero que representa el tamaño de la lista.
 *
 */

int longitud(lista l) {
    nodo *aux;
    int total = 0;
    aux = l.cabeza;
    while (aux != NULL) {
        total++;
        aux = aux->sig;
    }
    return total;
}


/**
 * destruir_lista
 * 
 * @brief Destruye y libera el espacio de una lista.
 * @param l Lista a destruir.
 *
 */

void destruir_lista(lista *l) {
    nodo *aux, *aux2;
    aux = l->cabeza;
    while (aux != NULL) {
        aux2 = aux->sig;
        free(aux->elemento);
        free(aux);
        aux = aux2;
    }
    l->cabeza = NULL;
}