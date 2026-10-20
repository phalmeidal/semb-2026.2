#include "heap.h"

void heap_inicializar(Heap *heap)
{
    heap->tamanho = 0;
}

bool heap_esta_vazia(const Heap *heap)
{
    return heap->tamanho == 0;
}

/** Insere elemento na Heap
 * @param heap ponteiro para heap válida
 * @param valor valor a ser inserido na heap
 * @return retorna `true` caso operação tenha sido concluída com éxito.
 * 
 * Começa da ultima posição, checa se o pai é maior ou igual a valor, caso não: desce
 * ele e repete o processo com a posição dele.
 * No fim, insere valor na posição vaga.
 */
bool heap_inserir(Heap *heap, int32_t valor)
{
    if (heap->tamanho == HEAP_CAPACIDADE) {
        return false;
    }

    size_t indice = heap->tamanho++;
    while (indice > 0) {
        size_t pai = (indice - 1u) / 2u;
        if (heap->dados[pai] >= valor) {
            break;
        }
        heap->dados[indice] = heap->dados[pai];
        indice = pai;
    }
    heap->dados[indice] = valor;
    return true;
}

/** Remove elemento da Heap
 * @param heap ponteiro para heap válida
 * @param maior ponteiro para variável externa que guarde o valor removido da heap
 * @return retorna `true` caso operação tenha sido concluída com éxito.
 * 
 * Guarda o valor a ser removido na variável `maior`.
 * Começando da posição inicial da heap, escolhe o maior filho para ocupar o
 * lugar do pai removido e repete o processo para a posição vaga deixada pelo
 * filho movido enquanto o filho for maior que o ultimo elemento, e caso não
 * seja, o ultimo elemento toma o lugar vago do pai.
 */
bool heap_remover_maximo(Heap *heap, int32_t *maior)
{
    if (heap_esta_vazia(heap)) {
        return false;
    }

    *maior = heap->dados[0];
    size_t restantes = --heap->tamanho;
    if (restantes == 0) {
        return true;
    }

    int32_t ultimo = heap->dados[restantes];
    size_t pai = 0;

    /* As capacidades do T1 (8000) e T2 (512) evitam overflow em 2*pai+1. */
    while (2u * pai + 1u < restantes) {
        size_t filho = 2u * pai + 1u;
        if (filho + 1u < restantes &&
            heap->dados[filho + 1u] > heap->dados[filho]) {
            ++filho;
        }
        if (ultimo >= heap->dados[filho]) {
            break;
        }
        heap->dados[pai] = heap->dados[filho];
        pai = filho;
    }

    heap->dados[pai] = ultimo;
    return true;
}
