# Aplicar as alterações ao GitHub

As alterações foram preparadas em uma cópia local baseada no commit `24f86bb` da branch `main` de `newtmagalhaes/semb-2026.2`. Para manter a adaptação em outra conta, use um fork desse repositório. Se a integração responder `403: Resource not accessible by integration` ao tentar criar o fork, crie-o pela interface do GitHub; isso é uma limitação da autorização da integração, não dos arquivos do projeto.

Na conta `phalmeidal`, a página de criação é https://github.com/newtmagalhaes/semb-2026.2/fork. Depois de criar `phalmeidal/semb-2026.2`, a publicação do commit depende de a integração ter acesso de escrita ao novo repositório.

O pacote contém os arquivos completos e `ALTERACOES.patch`. Em um clone limpo do repositório original, crie uma branch e aplique o patch:

```sh
git switch -c t2-stm32f030
git apply --check /caminho/ALTERACOES.patch
git apply /caminho/ALTERACOES.patch
make test
make firmware test-limite
git add .
git commit -m "Adapta max-heap para NUCLEO-F030R8 com validacao PC e UART"
git push -u origin t2-stm32f030
```

O push usa a autenticação e as permissões do integrante no próprio computador. Se a branch principal tiver mudado, revise o resultado de `git apply --check` antes de continuar. Também é possível copiar os arquivos do pacote para uma branch nova; preserve os arquivos pessoais existentes e revise o diff.

Os binários ARM pré-compilados estão em `build/` no pacote, com SHA-256 no relatório de memória. Eles são ignorados pelo Git e podem ser regenerados. As evidências físicas devem ser produzidas pela equipe e adicionadas depois dos testes na placa. Para o sistema de entregas da disciplina, o arquivo de código é apenas `entrega/t2.c`.
