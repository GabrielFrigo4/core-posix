# 🗺️ Roadmap & Backlog

> Planejamento estratégico, status operacional e visão de futuro para os utilitários de sistema e elevação de privilégios Core POSIX (`rtdo`/`rtgo`).

---

## 📊 Status do Projeto

| Área                         |   Status   | Cobertura / Estado                                                          |
| :--------------------------- | :--------: | :-------------------------------------------------------------------------- |
| **Utilitário `rtdo`**        | 🟢 Estável | Alternativa leve ao `sudo`/`doas` via autenticação TTY (`crypt`)            |
| **Utilitário `rtgo`**        | 🟢 Estável | Elevação imediata passwordless (< 1ms) restrita ao grupo `wheel`            |
| **Modelo de Segurança**      | 🟢 Estável | Defesa em profundidade de 2 camadas (`4750 root:wheel` + validação runtime) |
| **Higienização de Ambiente** | 🟢 Estável | Expurgo defensivo de `LD_*`, `IFS` e imposição de `PATH` seguro             |
| **Automação & Makefile**     | 🟢 Estável | Targets POSIX silenciosos para compilação, teste e instalação               |

---

## 🎯 Backlog & Próximas Frentes

_Nenhuma tarefa pendente no momento._

---

> [!TIP]
> Para detalhes sobre convenções de código e diretrizes de engenharia, consulte o [PRINCIPLES.md](PRINCIPLES.md).
