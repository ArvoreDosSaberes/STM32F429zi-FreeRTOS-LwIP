import React from 'react';
import { Link } from 'react-router-dom';
import '../styles/notFound.css';

const NotFoundPage: React.FC = () => {
  return (
    <div className="page-root-not-found">
      <div className="container">
        <div className="box">
          <div className="error-code">404</div>
          <div className="error-icon">&#x1F50D;</div>
          <h1>Pagina Nao Encontrada - React UI</h1>
          <p>O recurso que voce esta procurando nao existe neste servidor.</p>
          <Link to="/" className="btn-home">
            &#x1F3E0; Voltar para Inicio
          </Link>
        </div>
      </div>
    </div>
  );
};

export default NotFoundPage;
