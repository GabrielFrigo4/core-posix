# 📜 Princípios de Engenharia & Filosofia — Core POSIX

> _"Rule of Simplicity: Design for simplicity; add complexity only where you must."_<br>
> — Eric S. Raymond, _The Art of UNIX Programming_ (2003)

O repositório **Core POSIX** reúne implementações minimalistas, auditáveis e de alta segurança de ferramentas de sistema em linguagem C. Concebido sob a mais pura filosofia Unix, ele fornece alternativas seguras e enxutas para elevação controlada de privilégios (`rtdo` e `rtgo`), rejeitando complexidades monolíticas em favor do menor privilégio e da conformidade estrita ao padrão POSIX.

> [!IMPORTANT]
> **A Regra de Ouro do Agente de IA:** Ao entrar em qualquer diretório de repositório, o agente DEVE SEMPRE ler os arquivos `AGENTS.md`, `PRINCIPLES.md` e `.agents/` daquele repositório antes de realizar qualquer alteração.

---

## 🏛️ Os 18 Princípios de Design (17 Princípios UNIX + Soberania do Usuário)

### 1. Regra da Modularidade (_Rule of Modularity_)

> _Escreva partes simples conectadas por interfaces limpas._

- Cada utilitário é estritamente isolado em seu próprio subdiretório (`rtdo/` para autenticação interativa via `/dev/tty` e `rtgo/` para execução imediata sem senha).

### 2. Regra da Clareza (_Rule of Clarity_)

> _Clareza é melhor que esperteza._

- Código em C99 autoexplicativo, sem construções obscuras de ponteiros ou macros aninhadas. Toda função tem um único nível de abstração.

### 3. Regra da Composição (_Rule of Composition_)

> _Projete programas para serem conectados a outros programas._

- Os utilitários executam comandos via `execvp`, passando descritores de arquivo e argumentos sem intermediários.

### 4. Regra da Separação (_Rule of Separation_)

> _Separe a política do mecanismo; separe o motor da interface._

- O mecanismo de troca de credenciais (`setuid`, `setgid`, `initgroups`) é isolado das rotinas de parsing de linha de comando.

### 5. Regra da Simplicidade (_Rule of Simplicity_)

> _Projete para a simplicidade; adicione complexidade apenas onde estritamente necessário._

- Binários compilados com poucas centenas de linhas, auditáveis visualmente em minutos por qualquer engenheiro de segurança.

### 6. Regra da Parcimônia (_Rule of Parsimony_)

> _Escreva um programa grande apenas quando estiver claro por demonstração que nada mais resolverá._

- Rejeitamos configurações complexas em arquivos de regras ou parsers pesados. A autorização é baseada no grupo canônico `wheel`.

### 7. Regra da Transparência (_Rule of Transparency_)

> _Projete para a visibilidade para tornar inspeção e depuração fáceis._

- Mensagens de erro explícitas e diagnósticos claros emitidos exclusivamente em `stderr`.

### 8. Regra da Robustez (_Rule of Robustness_)

> _A robustez é filha da transparência e da simplicidade._

- **Higiene de Memória & Segredos:** Senhas e buffers intermediários são apagados imediatamente com `explicit_bzero`.
- **Sanitização de Ambiente:** Limpeza mandatória de variáveis de ambiente perigosas (`LD_PRELOAD`, `LD_LIBRARY_PATH`, `IFS`) antes de `execvp`.
- **Restrição a `wheel` (4750):** Binários SUID protegidos por permissão `chmod 4750 root:wheel` e verificação em runtime C.

### 9. Regra da Representação (_Rule of Representation_)

> _Dobre o conhecimento em dados para que a lógica do programa possa ser estúpida e robusta._

- Buffers estáticos com checagem rígida de limites (`sizeof`) em vez de alocação dinâmica vulnerável a vazamentos.

### 10. Regra do Menor Espanto (_Rule of Least Surprise_)

> _No design de interfaces, sempre faça a coisa menos surpreendente._

- Semântica de invocação idêntica a executores clássicos Unix: `rtdo command [args...]`.

### 11. Regra do Silêncio (_Rule of Silence_)

> _Quando um programa não tem nada surpreendente a dizer, ele não deve dizer nada._

- Em caso de sucesso na autorização, o utilitário entrega o controle imediatamente ao comando filho sem emitir nenhum caractere na tela.

### 12. Regra do Reparo (_Rule of Repair_)

> _Quando você precisar falhar, falhe ruidosamente e o mais rápido possível._

- Qualquer inconsistência em UID, grupos, termios ou falha na leitura do TTY aborta a execução instantaneamente com `exit(EXIT_FAILURE)`.

### 13. Regra da Economia (_Rule of Economy_)

> _O tempo do programador é caro; economize-o em preferência ao tempo da máquina._

- Execução imediata com zero overhead de parsing de políticas complexas em disco.

### 14. Regra da Geração (_Rule of Generation_)

> _Evite codificação manual; escreva programas para escrever programas quando puder._

- Makefiles modulares e regras de compilação automáticas com verificação de sanitizers.

### 15. Regra da Otimização (_Rule of Optimization_)

> _Prototipe antes de polir. Faça funcionar antes de otimizar._

- Foco primordial na solidez criptográfica e de permissões; otimizações de compilação com `-O2` ativadas por padrão.

### 16. Regra da Diversidade (_Rule of Diversity_)

> _Desconfie de todas as afirmações de "uma única maneira verdadeira"._

- Portabilidade plena entre distribuições Linux e FreeBSD (`pwd->pw_passwd` vs `getspnam`, detecção dinâmica de `-lcrypt`).

### 17. Regra da Extensibilidade (_Rule of Extensibility_)

> _Projete para o futuro, porque ele chegará antes do que você imagina._

- Arquitetura de código C modular que permite plugar novos métodos de autenticação sem refatorar o núcleo SUID.

### 18. Regra da Soberania do Usuário (_Rule of User Sovereignty_)

> _Honre a escolha explícita e deliberada do usuário antes de impor padrões genéricos._

- O administrador da máquina escolhe explicitamente os membros do grupo `wheel` autorizados a elevar privilégios.

---

## 🧼 Princípios de Clean Code para C & Makefiles

1. **Conformidade C99 & POSIX:** Compilação com zero warnings:
   `cc -Wall -Wextra -Werror -pedantic -std=c99`
2. **Makefiles POSIX Silenciosos:**
    ```makefile
    .POSIX:
    .SILENT:

    MAKEFLAGS += --no-print-directory -s
    ```
3. **Identificadores e Mensagens em Inglês:** Nomes de variáveis, funções, macros e mensagens de terminal estritamente em inglês.
4. **Arquitetura de Comentários (A Tríade Sem Vazamento):**
    - Header Banner: 64 hífens (`# ----------------------------------------------------------------`).
    - Seções Estruturais: 32 caracteres (`### ================================` e `### --------------------------------`). Título $\le$ 32 caracteres.
    - Zero Comentários Narrativos no código C.
