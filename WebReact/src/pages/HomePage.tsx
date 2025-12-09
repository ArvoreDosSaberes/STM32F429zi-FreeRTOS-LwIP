import React from 'react';
import STLogo from '../components/STLogo';
import Butterfly from '../components/Butterfly';
import '../styles/home.css';

const HomePage: React.FC = () => {
  return (
    <div className="page-root-home">
      <div className="container">
        <div className="header">
          <div className="logo">
            <STLogo />
          </div>
          <h1>STM32F429ZI Template com React</h1>
          <h2>FreeRTOS + LwIP HTTP Server + React UI</h2>
          <span className="subtitle">Arvore dos Saberes</span>
          <div className="tech-badges">
            <span className="badge">STM32F429ZI</span>
            <span className="badge">FreeRTOS v11.2</span>
            <span className="badge">LwIP v2.2</span>
            <span className="badge">ARM Cortex-M4</span>
          </div>
          <Butterfly />
        </div>

        <div className="content">
          <h2>Sobre o Projeto</h2>
          <p>
            Este projeto tem como objetivo <strong>ensinar e facilitar o desenvolvimento</strong>
            {' '}
            com microcontroladores da familia STM32, especificamente o STM32F429ZI.
          </p>
          <p>
            Oferecemos a comunidade <strong>templates prontos para uso</strong>, permitindo
            {' '}
            que desenvolvedores deem os primeiros passos com o STM32 de forma rapida e
            {' '}
            eficiente, sem a necessidade de configurar tudo do zero.
          </p>

          <div className="features">
            <div className="feature-card">
              <h3>&#x2699; FreeRTOS</h3>
              <p>
                Sistema operacional de tempo real para gerenciamento de tarefas e recursos.
              </p>
            </div>
            <div className="feature-card">
              <h3>&#x1F310; LwIP Stack</h3>
              <p>Pilha TCP/IP leve e eficiente para conectividade Ethernet.</p>
            </div>
            <div className="feature-card">
              <h3>&#x1F4E1; Servidor HTTP</h3>
              <p>Servidor web integrado para monitoramento e configuracao.</p>
            </div>
            <div className="feature-card">
              <h3>&#x1F4DA; Codigo Documentado</h3>
              <p>Comentarios detalhados e tutoriais para facilitar o aprendizado.</p>
            </div>
            <div className="feature-card">
              <h3>&#x269B; React UI</h3>
              <p>
                Interface web moderna desenvolvida em React para interagir com o servidor
                HTTP embarcado e visualizar recursos do template STM32.
              </p>
            </div>
          </div>
        </div>

        <div className="buttons">
          <a
            href="https://github.com/ArvoreDosSaberes/STM32F429zi-FreeRTOS-LwIP"
            className="btn btn-github"
            target="_blank"
            rel="noreferrer"
          >
            &#x1F4BB; GitHub do Projeto
          </a>
          <a
            href="https://mcu.tec.br"
            className="btn btn-site"
            target="_blank"
            rel="noreferrer"
          >
            &#x1F310; MCU.tec.br
          </a>
          <a
            href="https://youtube.com/@mcu_fpga"
            className="btn btn-youtube"
            target="_blank"
            rel="noreferrer"
          >
            &#x25B6; Canal YouTube
          </a>
        </div>

        <div className="footer">
          <p>
            Desenvolvido com &#x2764; por
            {' '}
            <a href="https://github.com/carlosdelfino">Carlos Delfino</a>
          </p>
          <p>STM32F429ZI | Cortex-M4 @ 168MHz | Template v1.0.0</p>
        </div>
      </div>
    </div>
  );
};

export default HomePage;
