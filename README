# Implementação 6: Memória Virtual

Infraestrutura de Software
Aluno: Marcus Vinícius Pereira Barbosa
e-mail: mvpb@cesar.school

## O que o programa faz

Simula a tradução de endereços lógicos (16 bits) em endereços físicos usando paginação:

- páginas e frames de 256 bytes
- memória física com 128 frames (preenchidos de 0 a 127)
- TLB com 16 entradas
- substituição de páginas e da TLB com FIFO ou LRU
- busca na TLB com uma thread por entrada

Quando a página não está na memória (page fault), ela é lida do arquivo `BACKING_STORE.bin`.

## Arquivos

| Arquivo | Descrição |
|---|---|
| `vm.c` | código-fonte |
| `Makefile` | compila e limpa |
| `evidencias.log` | registro dos testes |

Arquivos do professor, não incluídos no repositório: `addresses.txt`, `BACKING_STORE.bin` e o `correct.txt` de referência.

## Como compilar

```
make          # gera o executável vm
make clean    # remove o executável
```

## Como executar

```
./vm <arquivo_de_enderecos> <fifo|lru> <fifo|lru>
```

- 1º argumento: arquivo com um endereço lógico (0 a 65535) por linha
- 2º argumento: algoritmo de substituição de página
- 3º argumento: algoritmo de substituição da TLB

Exemplo (página com LRU, TLB com FIFO):

```
./vm addresses.txt lru fifo
```

O `BACKING_STORE.bin` precisa estar na pasta de onde o programa é executado. Se tudo der certo, o programa não imprime nada no terminal e grava o resultado em `correct.txt`.

## Limitações

- Criar 16 threads a cada consulta deixa o programa mais lento com arquivos de entrada muito grandes.
- O nome `BACKING_STORE.bin` é fixo e o arquivo é procurado na pasta atual.
