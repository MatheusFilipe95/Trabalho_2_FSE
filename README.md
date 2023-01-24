#### Projeto 2 correspondente a matéria de Fundamentos de Sistemas Embarcados.

## Aluno
|Matrícula | Aluno |
| -- | -- |
| 140155350  |  Matheus Filipe Faria Alves de Andrade |


## Compilação

Para compilar, apenas digite 

```
make clean
make
```

para limpar os objetos antigos e criar novos.

## Execução


A execução é bem simples, basta iniciar o executável:

```
./forno
```

Assim que executado, o próprio programa te orienta para o que fazer.

De antemão, é possível executar com 3 valores distintos:

```
./forno 1
./forno 2
./forno 3
```

Em que 1 indica que será inicializado o forno no modo Debug, 2 que será controlado inicialmente pelo dashboard e 3 indica que será inicializado pela curva de reflow.


Uma vez executado o programa, é possível alterar pelo Dashboard se o modo de operação será manual pelo próprio dashboard ou se será por meio de um arquivo csv pré definido. É válido dizer que, inicializado no modo debug, é possível ir para os dois modos padrões, mas não é possível retornar ao modo debug, sendo necessário reinicializar o programa.




