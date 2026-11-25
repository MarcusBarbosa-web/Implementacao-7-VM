# Implementação 7: Memória Virtual

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
- quando uma página sai da memória, a entrada dela na TLB é invalidada, e a posição liberada é a primeira a ser usada na próxima falta da TLB

Quando a página não está na memória (page fault), ela é lida do arquivo `BACKING_STORE.bin`.

## Arquivos

| Arquivo | Descrição |
|---|---|
| `vm.c` | código-fonte |
| `Makefile` | compila e limpa |
| `evidencias.log` | registro dos testes |
| `mvpb.pdf` | relatório |

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

O `BACKING_STORE.bin` precisa estar na pasta de onde o programa é executado.

## Formato da saída

O programa mostra na tela uma linha por endereço, e grava o `correct.txt` com essas mesmas linhas e mais 5 linhas de estatísticas no final. O `correct.txt` é sobrescrito a cada execução.

Linha de cada endereço:

```
Virtual address: 16916 TLB: 0 Physical address: 20 Value: 0
```

Linhas finais, só no `correct.txt`:

```
Number of Translated Addresses = 1000
Page Faults = 539
Page Fault Rate = 0.539
TLB Hits = 54
TLB Hit Rate = 0.054
```

## Tratamento de erros

O programa termina com mensagem no `stderr` e código de saída 1 quando:

- o número de argumentos não é 3 (mostra o modo de uso)
- o algoritmo de página ou da TLB não é `fifo` nem `lru`
- o arquivo de endereços não existe ou não pode ser aberto
- o `BACKING_STORE.bin` não pode ser aberto
- o arquivo de endereços está vazio, tem uma linha muito longa ou uma linha fora do formato (texto, negativo, acima de 65535, decimal, hexadecimal ou mais de um número por linha)

Linhas em branco, espaços ao redor do número e `\r\n` são aceitos. O arquivo de endereços é validado inteiro antes de começar a gravar, então um erro de formato não altera o `correct.txt` anterior. Se o arquivo de entrada for o próprio `correct.txt`, o programa dá erro de formato na linha 1 e não o apaga.

## Como testar

Guarde uma cópia do `correct.txt` do professor como `correct_prof.txt` antes de rodar, porque o programa sobrescreve o `correct.txt`.

```
./vm addresses.txt lru fifo > tela.txt
diff correct.txt correct_prof.txt && echo ARQUIVO_OK
head -n -5 correct_prof.txt | diff - tela.txt && echo TELA_OK
./vm correct.txt lru fifo ; echo "exit=$?"
```

O esperado é `ARQUIVO_OK`, `TELA_OK` e, no último comando, `Erro: formato invalido na linha 1` com `exit=1`. Os testes completos estão no `evidencias.log`.

## Limitações

- Criar 16 threads a cada consulta deixa o programa mais lento com arquivos de entrada muito grandes.
- O nome `BACKING_STORE.bin` é fixo e o arquivo é procurado na pasta atual.
- O `correct.txt` é gravado direto. Se a leitura do `BACKING_STORE.bin` falhar no meio da execução, ele pode ficar incompleto.
