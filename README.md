## PROJECT SPANE

Lembra daquela sensação nostálgica de abrir um emulador para jogar os clássicos da sua infância? Agora, imagine canalizar toda essa energia e foco para o aprendizado. Um emulador de jogos educativos é a fusão perfeita entre o formato envolvente dos videogames e a absorção de conhecimento prático.

A proposta não é criar apenas "joguinhos de matemática sem graça", mas sim resgatar o espírito dos grandes clássicos, onde cada fase superada representa uma lição aprendida.

ㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤ<img width="768" height="513" alt="image" src="https://github.com/user-attachments/assets/d8a8e2b5-809b-4ede-a733-b7d5d8709ca8" />


## Screencast
**Vídeo apresentando a navegação na SPANE Engine e a seleção de um de seus jogos, o Logic Snake.**
https://www.youtube.com/watch?v=yJwiTBg43fY

## Principais Ferramentas ⚙️:

- **draw.io**: utilizado para o diagrama.
- [Notion](https://app.notion.com/p/373c226bd9dc80b5bf2cd36dda22cdff?v=373c226bd9dc8099821d000cc6935f3c): ferramenta utilizada para o backlog e controle de tarefas.
- [Figma](https://www.figma.com/design/vX3Ak107LvhA7SHgESZi5J/SPANE?node-id=0-1&t=TX1BJ89Nwpzmp9ew-1): usado para simular o prototipo e a interface do programa
- **Discord**: usado para reuniões e encontros 


**Tutorial de instalação do Spane**:https://app.notion.com/p/Preparando-o-ambiente-363c226bd9dc80ea84c0db7329d90313?source=copy_link

🔧 ```bash install.sh```

🌎 ```spane```


## Tela do quadro Kanban
<img width="1438" height="715" alt="Captura de tela 2026-06-22 000203" src="https://github.com/user-attachments/assets/bb51a60b-9b2d-45e9-9bf9-0fa67ac162ff" />

<img width="1457" height="712" alt="Captura de tela 2026-06-22 000255" src="https://github.com/user-attachments/assets/0fbc65b0-3404-4d7c-95e5-8fef2f07ed0c" />



## Bug Tracker
**Registro e acompanhamento de bugs, melhorias e novas funcionalidades do projeto Spane.**

<img width="1312" height="392" alt="Captura de tela 2026-06-21 161319" src="https://github.com/user-attachments/assets/c42afad6-18fc-4cff-a544-d8ed7dde36c2" />

**Mouse desaparecendo (Corrigido)**

O cursor do mouse sumia aleatoriamente durante o uso, especialmente ao alternar entre a área do jogo e a barra lateral. O problema estava no rastreamento de hover dos botões do diálogo de erro, que sobrescrevia a visibilidade do cursor padrão do X11. A solução foi limitar o rastreamento de hover apenas para quando o mouse está sobre elementos interativos, restaurando o cursor padrão do sistema nas demais áreas.

**Modo tela cheia adaptável (Corrigido)**

**Antes**:

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/9e934fc0-c78a-469a-bb81-d0d1b0196d2d" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/09ad6ebb-a56f-4c33-a902-c2a15ebb78cf" />

**Durante**:
A engine por um momento estava carregando direto na tela cheia, como dá para observar na imagem abaixo, mas o bug ainda persistia:
https://ibb.co/gZGktSVh


**Depois**:
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/0b9841e4-db4a-4e09-b177-fb4bbd91f5e6" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/9eecbdc4-bf50-4ff6-8d96-beb491a841d4" />
A janela agora se ajusta automaticamente à resolução do navegador no modo web e ao tamanho do monitor no modo X11. Anteriormente, o SPANE Engine usava um tamanho fixo de 1000 por 700 pixels que não se adaptava a telas maiores ou menores. A solução implementada detecta a resolução disponível no ambiente e redimensiona a área de renderização proporcionalmente, mantendo a qualidade visual em qualquer tamanho de tela.



## 🤝 Relato de experiência em programação em pares
"Durante o desenvolvimento do projeto, utilizamos a programação em pares para implementar funcionalidades e corrigir problemas. Enquanto um integrante escrevia o código, o outro acompanhava, revisava e sugeria melhorias em tempo real. Essa prática contribuiu para reduzir erros, aumentar a qualidade do código e promover a troca de conhecimentos entre a equipe. Além disso, a colaboração constante facilitou a tomada de decisões e tornou o processo de desenvolvimento mais eficiente."
<img width="1284" height="699" alt="Captura de tela 2026-06-16 233257" src="https://github.com/user-attachments/assets/f45b99c8-bd8f-43a8-bd09-616516135eb1" />

## 👩‍💻 Equipe:
- João Lucas Mendes Dos Santos: testes gerais, Notion, back log.
- Augusto Freitas: figma, back end, Notion.
- Daniel Luiz Massud: back end, front end, testes gerais, arquitetura.
- Fernando Antônio: parte criativa, suporte geral.
- Gustavo Cassemiro: back log, arquitetura.
- Glauberson: suporte geral, comunicação.
- Caio Catão: Notion, figma, front end, back end.

