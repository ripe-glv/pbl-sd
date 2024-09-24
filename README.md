<h1 align="center"> Tetris </h1>
<h3 align="center"> Desenvolvimento do Jogo Tetris para o kit de desenvolvimento DE1-SoC para a disciplina de Sistemas Digitais </h3>
<h3 align="center"> Equipe: Daniel Lucas Rios da Silva, Filipe Carvalho Matos Galvão, Luan Barbosa dos Santos Costa
Docente: Ângelo Amâncio Duarte </h3>
<hr>

<div align="justify"> 
<h2> Resumo </h2>
O projeto em questão visa desenvolver um jogo semelhante ao tetris utilizando a linguagem de programação C, e a placa FPGA DE1-SoC para interação com o usuário, sendo que é utilizado o acelerômetro da placa para obter os movimentos desejados pelo usuário para as peças, assim como os botões da placa para permitir que o jogador interaja com o menu, pause o jogo, e gire as peças. Além disso, é utilizada sua saída VGA para apresentar a interface gráfica do jogo no monitor com a resolução de 320x240.
</div>
<h2>Funcionamento do Jogo</h2>
    <p>O <strong>Tetris</strong> é um jogo em que o jogador organiza peças de formatos variados que caem de forma contínua na tela. O objetivo é movimentar e rotacionar essas peças para formar linhas horizontais completas, que são removidas, concedendo pontos ao jogador. O jogo termina quando as peças se acumulam e alcançam o topo da área de jogo.</p>

<h2>Interface e Controles</h2>
<p>Neste projeto, foram integrados vários periféricos da DE1-SoC para a interação com o jogo, conforme detalhado abaixo:</p>

<h3>1. Movimento das Peças - Acelerômetro</h3>
<p>O <strong>acelerômetro</strong> da DE1-SoC é utilizado para capturar os movimentos das peças no eixo horizontal (esquerda e direita) asssim como no eixo vertical, para acelerar a queda da peça. Ao inclinar a placa levemente para a esquerda, direita, ou para baixo, as peças se movem de forma correspondente dentro da área de jogo. Este método de controle adiciona uma interatividade física ao jogo, tornando a experiência mais dinâmica e intuitiva.</p>

<h3>2. Controle de Rotação e Interações com o Menu - Botões</h3>
<p><strong>Botões</strong> da placa são utilizados para outras funcionalidades, como:</p>
<ul>
<li><strong>Rotação das peças</strong>: Pressionar um botão específico permite girar a peça atual, facilitando seu encaixe no tabuleiro.</li>
<li><strong>Interação com o menu</strong>: Um botão é reservado para navegar pelas opções de menu, como iniciar o jogo ou ajustar configurações.</li>
<li><strong>Pausa</strong>: Outro botão é dedicado à função de pausar e retomar o jogo conforme necessário.</li>
</ul>
<h3>3. Exibição Gráfica - Saída VGA</h3>
<p>A interface gráfica do jogo é exibida através da <strong>saída VGA</strong>, permitindo que o jogo seja visualizado em um monitor externo. O monitor utiliza uma resolução de <strong>320x240</strong>, o que oferece uma experiência visual simples e eficiente para o Tetris. A área de jogo é clara e visível, com cores distintas entre as peças que estão caindo, e as que já estão posicionadas, e um layout de fácil compreensão.</p>
<h2>Dinâmica do Jogo</h2>
<ol>
<li><strong>Inicialização</strong>: Ao iniciar o jogo, o menu principal será exibido, apresentando o nome "Tetris", as opções de iniciar e sair, assim como as três melhores pontuações.</li>
<li><strong>Jogabilidade</strong>: Durante o jogo, o jogador controla a posição das peças inclinando a placa, e pode rotacioná-las ou pausar o jogo utilizando os botões.</li>
<li><strong>Condições de Vitória/Derrota</strong>: O jogo continua indefinidamente enquanto o jogador for capaz de formar linhas completas sem que as peças atinjam o topo da tela. </li>
<li><strong>Pontuação</strong>: Se a pontuação do usuário for superior a alguma das três melhores, ela fica armazenada no ranking enquanto jogo estiver em execução. </li>
</ol>
